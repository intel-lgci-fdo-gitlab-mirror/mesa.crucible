#!/usr/bin/env python3
# Copyright 2026 Intel Corporation
# SPDX-License-Identifier: MIT

"""Import a Vulkan-Headers release and print a commit message to stdout.

Examples:
    python3 misc/import_vulkan.py           # latest numbered release
    python3 misc/import_vulkan.py v1.4.365  # a specific release

Replaces include/vulkan and include/vk_video with the complete upstream trees,
including removal of obsolete files. Also imports registry/vk.xml into
include/vulkan.
"""

from __future__ import annotations

import argparse
import io
import json
import re
import shutil
import sys
import tarfile
import tempfile
import urllib.error
import urllib.request
from collections.abc import Mapping
from pathlib import Path, PurePosixPath
from textwrap import dedent
from typing import BinaryIO, TypeAlias

REPOSITORY = 'KhronosGroup/Vulkan-Headers'
RELEASE_TAG = re.compile(r'v(\d+)\.(\d+)\.(\d+)')
REQUEST_TIMEOUT = 60
TREES = ('vulkan', 'vk_video')
HEADER_PREFIXES = tuple(f'include/{tree}/' for tree in TREES)
REQUIRED_FILES = (
    PurePosixPath('vulkan/vulkan.h'),
    PurePosixPath('vulkan/vulkan_core.h'),
    PurePosixPath('vulkan/vk.xml'),
    PurePosixPath('vk_video/vulkan_video_codecs_common.h'),
)
COMMIT_MESSAGE = dedent('''\
    include: Update Vulkan headers to {tag}

    Import the complete Vulkan and video headers, plus the registry,
    from Vulkan-Headers {tag} using misc/import_vulkan.py.

    https://github.com/{repository}/tree/{tag}
    ''')

HeaderFiles: TypeAlias = dict[PurePosixPath, bytes]


def tag_version(tag: str) -> tuple[int, int, int]:
    match = RELEASE_TAG.fullmatch(tag)
    if match is None:
        raise ValueError(f'invalid Vulkan-Headers release tag: {tag!r}')
    major, minor, patch = match.groups()
    return int(major), int(minor), int(patch)


def latest_tag() -> str:
    # Match numbered header releases, not the separate Vulkan SDK tags.
    request = urllib.request.Request(
        f'https://api.github.com/repos/{REPOSITORY}/git/matching-refs/tags/v',
        headers={'Accept': 'application/vnd.github+json',
                 'User-Agent': 'crucible-import-vulkan'},
    )
    tags: list[str] = []
    prefix = 'refs/tags/'
    with urllib.request.urlopen(request, timeout=REQUEST_TIMEOUT) as response:
        references = json.load(response)
    for ref in references:
        name = ref['ref']
        if name.startswith(prefix):
            tag = name[len(prefix):]
            if RELEASE_TAG.fullmatch(tag):
                tags.append(tag)
    if not tags:
        raise ValueError('no numbered Vulkan-Headers release tags found')
    return max(tags, key=tag_version)


def validate_headers(files: Mapping[PurePosixPath, bytes], tag: str) -> None:
    for required in REQUIRED_FILES:
        if required not in files:
            raise ValueError(f'archive is missing {required}')
    major, minor, patch = tag[1:].split('.')
    header = files[PurePosixPath('vulkan/vulkan_core.h')]
    version = f'#define VK_HEADER_VERSION {patch}\n'.encode()
    complete = ('#define VK_HEADER_VERSION_COMPLETE '
                f'VK_MAKE_API_VERSION(0, {major}, {minor}, VK_HEADER_VERSION)').encode()
    registry_version = f'#define <name>VK_HEADER_VERSION</name> {patch}</type>'.encode()
    if (version not in header or complete not in header
            or registry_version not in files[PurePosixPath('vulkan/vk.xml')]):
        raise ValueError(f'headers and registry do not match {tag}')


def read_headers(archive: BinaryIO, tag: str) -> HeaderFiles:
    prefix = f'Vulkan-Headers-{tag[1:]}/'
    files: HeaderFiles = {}
    with tarfile.open(fileobj=archive, mode='r:gz') as source:
        for member in source:
            if member.isdir() and member.name.rstrip('/') == prefix.rstrip('/'):
                continue
            if not member.name.startswith(prefix):
                raise ValueError(f'unexpected archive path: {member.name}')
            name = member.name[len(prefix):]
            if not (name.startswith(HEADER_PREFIXES) or name == 'registry/vk.xml'):
                continue
            if member.isdir():
                continue
            path = PurePosixPath(name)
            if not member.isfile() or '..' in path.parts:
                raise ValueError(f'unsafe archive entry: {member.name}')
            target = (PurePosixPath('vulkan/vk.xml') if name == 'registry/vk.xml'
                      else path.relative_to('include'))
            if target in files:
                raise ValueError(f'duplicate archive entry: {member.name}')
            contents = source.extractfile(member)
            assert contents is not None
            with contents:
                files[target] = contents.read()
    validate_headers(files, tag)
    return files


def load_headers(tag: str, archive: Path | None = None) -> HeaderFiles:
    if archive is not None:
        with archive.open('rb') as source:
            return read_headers(source, tag)
    url = f'https://codeload.github.com/{REPOSITORY}/tar.gz/refs/tags/{tag}'
    with urllib.request.urlopen(url, timeout=REQUEST_TIMEOUT) as response:
        return read_headers(io.BytesIO(response.read()), tag)


def install_headers(files: Mapping[PurePosixPath, bytes], include_dir: Path) -> None:
    include_dir.mkdir(parents=True, exist_ok=True)
    for tree in TREES:
        if (include_dir / tree).is_symlink():
            raise ValueError(f'refusing to replace symlink: {include_dir / tree}')
    # Stage both trees before replacing the vendored directories.
    with tempfile.TemporaryDirectory(dir=include_dir) as temporary:
        staging = Path(temporary)
        for path, contents in files.items():
            dest = staging / path
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(contents)
        for tree in TREES:
            dest = include_dir / tree
            if dest.exists():
                shutil.rmtree(dest)
            shutil.move(staging / tree, dest)


def commit_message(tag: str) -> str:
    return COMMIT_MESSAGE.format(tag=tag, repository=REPOSITORY)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('tag', nargs='?',
                        help='Vulkan-Headers release tag (default: latest numbered release)')
    parser.add_argument('--archive', type=Path,
                        help='use a previously downloaded release .tar.gz instead of the network')
    parser.add_argument('--include-dir', type=Path,
                        default=Path(__file__).resolve().parents[1] / 'include',
                        help="destination include directory (default: this repository's include)")
    args = parser.parse_args()
    if args.archive and args.tag is None:
        parser.error('--archive requires an explicit release tag')
    if args.tag is not None and not RELEASE_TAG.fullmatch(args.tag):
        parser.error('expected a release tag of the form v1.4.365')
    try:
        tag = args.tag if args.tag is not None else latest_tag()
        files = load_headers(tag, args.archive)
        install_headers(files, args.include_dir)
    except (OSError, ValueError, EOFError, tarfile.TarError, urllib.error.URLError) as error:
        print(f'{parser.prog}: {error}', file=sys.stderr)
        sys.exit(1)
    print(commit_message(tag), end='')


if __name__ == '__main__':
    main()

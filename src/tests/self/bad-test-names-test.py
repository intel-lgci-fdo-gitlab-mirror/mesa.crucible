#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright 2015-2026 Intel Corporation

import argparse
import re
import subprocess
import sys
from typing import NoReturn


def die(message: str) -> NoReturn:
    print(f'bad-test-names: error: {message}', file=sys.stderr)
    sys.exit(1)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('crucible', help='path to the crucible binary')
    args = parser.parse_args()

    try:
        result = subprocess.run(
            [args.crucible, 'ls-tests'],
            capture_output=True,
        )
    except OSError as error:
        die(f'crucible ls-tests failed: {error}')

    if result.returncode != 0:
        die(f'crucible ls-tests failed:\n'
            f'  stdout: {result.stdout.decode(errors="replace")}\n'
            f'  stderr: {result.stderr.decode(errors="replace")}')

    names = result.stdout.split(b'\n')
    if names[-1] == b'':
        names.pop()
    bad_names = [
        name for name in names
        if re.fullmatch(rb'[_a-zA-Z][_a-zA-Z0-9.-]*', name) is None
    ]
    if bad_names:
        print('bad-test-names: error: found bad test names:', file=sys.stderr)
        for name in bad_names:
            print(name.decode(errors='replace'))
        sys.exit(1)


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
# SPDX-License-Identifier: MIT

import argparse
import subprocess
import sys
from typing import NoReturn


def die(message: str, *, code: int = 1) -> NoReturn:
    print(f'concurrent-output: error: {message}', file=sys.stderr)
    sys.exit(code)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('crucible', help='path to the crucible binary')
    args = parser.parse_args()

    try:
        result = subprocess.run(
            [
                args.crucible, 'run',
                '-j8',
		'self.concurrent-output.*'
	    ],
            capture_output=True,
            check=False,
        )
    except OSError as error:
        die(f'crucible run failed: {error}')

    if result.returncode != 0:
        if b'failed to gather vulkan info' in result.stdout:
            die('Skipping test: Could not load or run Vulkan driver', code=77)
        die(f'crucible run failed:\n'
            f'  stdout: {result.stdout.decode()}\n'
            f'  stderr: {result.stderr.decode()}')

    if b'aabb' in result.stdout:
        die('interleaved output in stdout')

    if b'aabb' in result.stderr:
        die('interleaved output in stderr')


if __name__ == '__main__':
    main()

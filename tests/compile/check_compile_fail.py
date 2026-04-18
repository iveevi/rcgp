#!/usr/bin/env python3

import subprocess
import sys
import re
import difflib
from pathlib import Path

def strip_paths(compiler_output: str) -> str:
    """Strip directory components from file paths, keeping just the filename."""
    lines = []
    for line in compiler_output.splitlines():
        # "In file included from path/to/file.hpp:1:" -> "In file included from file.hpp:1:"
        # "path/to/file.hpp:122:4: error: ..." -> "file.hpp:122:4: error: ..."
        stripped = re.sub(r'(\s*(?:In file included from\s+)?)\S*/', r'\1', line)
        lines.append(stripped)
    return "\n".join(lines) + "\n"

def main():
    if len(sys.argv) < 4:
        print(f"usage: {sys.argv[0]} <compiler> <cxxflags> <source.cpp>", file=sys.stderr)
        sys.exit(1)

    compiler = sys.argv[1]
    cxxflags = sys.argv[2:-1]
    source = Path(sys.argv[-1])
    expected_path = source.with_suffix(".stderr")

    if not expected_path.exists():
        print(f"\033[1;91mfailed:\033[0m {source} missing expected output {expected_path}")
        sys.exit(1)

    result = subprocess.run(
        [compiler, *cxxflags, "-c", str(source), "-o", "/dev/null"],
        capture_output=True,
        text=True,
    )

    actual = strip_paths(result.stderr)
    expected = expected_path.read_text()

    if actual == expected:
        print(f"\033[1;92mpassed:\033[0m {source}")
        sys.exit(0)
    else:
        print(f"\033[1;91mfailed:\033[0m {source} error output differs from expected")
        diff = difflib.unified_diff(
            expected.splitlines(keepends=True),
            actual.splitlines(keepends=True),
            fromfile=str(expected_path),
            tofile="actual",
        )
        for line in diff:
            if line.startswith("+++") or line.startswith("---"):
                sys.stderr.write(f"\033[1m{line}\033[0m")
            elif line.startswith("@@"):
                sys.stderr.write(f"\033[36m{line}\033[0m")
            elif line.startswith("+"):
                sys.stderr.write(f"\033[32m{line}\033[0m")
            elif line.startswith("-"):
                sys.stderr.write(f"\033[31m{line}\033[0m")
            else:
                sys.stderr.write(line)
        sys.exit(1)

if __name__ == "__main__":
    main()

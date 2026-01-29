#!/usr/bin/env python3
import argparse
import difflib
import os
import re
import shlex
import subprocess
import sys
from pathlib import Path


ANSI_RE = re.compile(r'\x1b\[[0-9;]*[mK]')


def strip_ansi(text: str) -> str:
    return ANSI_RE.sub('', text)


def read_cmake_cache_value(cache_path: Path, key: str) -> str | None:
    if not cache_path.exists():
        return None
    with cache_path.open('r', encoding='utf-8') as handle:
        prefix = f'{key}:'
        for line in handle:
            if line.startswith(prefix):
                return line.split('=', 1)[1].strip()
    return None


def detect_compiler_id(compiler: str) -> str:
    try:
        proc = subprocess.run(
            [compiler, '--version'],
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
    except FileNotFoundError:
        return 'unknown'
    output = proc.stdout.lower()
    if 'clang' in output:
        return 'clang'
    if 'gcc' in output or 'g++' in output:
        return 'gcc'
    return 'unknown'


def run_command(cmd: list[str], cwd: Path, capture: bool) -> tuple[int, str]:
    if capture:
        proc = subprocess.run(
            cmd,
            cwd=cwd,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        return proc.returncode, proc.stdout
    returncode = subprocess.call(cmd, cwd=cwd)
    return returncode, ''


def normalize_output(text: str, root: Path) -> str:
    stripped = strip_ansi(text)
    root_str = str(root)
    return stripped.replace(root_str + '/', '').replace(root_str, '.')


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('--build-dir', default=None)
    args = parser.parse_args()

    root = Path(__file__).resolve().parent.parent
    build_dir = Path(args.build_dir) if args.build_dir else root / 'build'
    cache_path = build_dir / 'CMakeCache.txt'

    if not cache_path.exists():
        print(f'Build dir not configured: {build_dir}', file=sys.stderr)
        print('Run CMake configure before invoking this script.', file=sys.stderr)
        return 1

    print('Building test target...', flush=True)
    status, _ = run_command(['cmake', '--build', str(build_dir), '--target', 'test'], root, False)
    if status != 0:
        return status

    test_bin = build_dir / 'test'
    if not test_bin.exists():
        print(f'Test binary not found: {test_bin}', file=sys.stderr)
        return 1

    print('Running runtime tests...', flush=True)
    status, _ = run_command([str(test_bin)], root, False)
    if status != 0:
        return status

    print('Running compile tests...', flush=True)

    cxx = os.environ.get('CXX') or read_cmake_cache_value(cache_path, 'CMAKE_CXX_COMPILER') or 'c++'
    std_value = read_cmake_cache_value(cache_path, 'CMAKE_CXX_STANDARD') or '26'
    cxx_flags = read_cmake_cache_value(cache_path, 'CMAKE_CXX_FLAGS') or ''

    compiler_id = detect_compiler_id(cxx)
    diag_flags: list[str] = []
    if compiler_id == 'clang':
        diag_flags.append('-fcolor-diagnostics')
    elif compiler_id == 'gcc':
        diag_flags.append('-fdiagnostics-color=always')

    extra_flags = shlex.split(cxx_flags)

    expected_dir = root / 'source' / 'test' / 'compile' / 'expected'
    actual_dir = build_dir / 'test-outputs'
    actual_dir.mkdir(parents=True, exist_ok=True)

    compile_dir = root / 'source' / 'test' / 'compile'
    sources = sorted(compile_dir.glob('*.cpp'))
    if not sources:
        print('No compile sources found.')
        return 0

    failed = False

    for src in sources:
        base = src.stem
        expected = expected_dir / f'{base}.txt'
        raw_path = actual_dir / f'{base}.raw.txt'
        actual_path = actual_dir / f'{base}.txt'

        cmd = [
            cxx,
            f'-std=c++{std_value}',
            '-I',
            str(root / 'include'),
            '-fsyntax-only',
            *diag_flags,
            *extra_flags,
            str(src),
        ]

        status, output = run_command(cmd, root, True)
        raw_path.write_text(output, encoding='utf-8')

        normalized = normalize_output(output, root)
        actual_path.write_text(normalized, encoding='utf-8')

        display = output.replace(str(root) + '/', '').replace(str(root), '.')
        print(f'Compile output ({base}):', flush=True)
        print(display, end='' if display.endswith('\n') else '\n', flush=True)
        print()

        if status == 0:
            print(f'Expected compilation to fail but succeeded: {src}', file=sys.stderr)
            failed = True
            continue

        if not expected.exists():
            print(f'Missing expected output: {expected}', file=sys.stderr)
            print(f'Actual output written to: {actual_path}', file=sys.stderr)
            failed = True
            continue

        expected_text = expected.read_text(encoding='utf-8')
        if expected_text != normalized:
            print(f'Compile output mismatch for: {base}', file=sys.stderr)
            diff = difflib.unified_diff(
                expected_text.splitlines(),
                normalized.splitlines(),
                fromfile=str(expected),
                tofile=str(actual_path),
                lineterm='',
            )
            for line in diff:
                print(line, file=sys.stderr)
            failed = True

    if failed:
        print('Compile tests failed.', file=sys.stderr)
        return 1

    print('All tests passed.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

#!/usr/bin/env python3
"""Generate a Raceboat encoding plugin from this Base64-backed starter."""

import argparse
import math
import re
import shutil
import sys
from pathlib import Path

TEMPLATE = Path(__file__).resolve().parent
SKIP_DIRS = {"build", "kit", ".git", "__pycache__"}
SKIP_FILES = {"new_encoding.py", "README.md"}
TEXT_SUFFIXES = {".h", ".cpp", ".txt", ".json", ".py", ".sh", ".cmake", ".md", ""}
MAX_INT32 = 2**31 - 1


def positive_int(value: str) -> int:
    try:
        parsed = int(value)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("must be an integer") from exc
    if parsed <= 0 or parsed > MAX_INT32:
        raise argparse.ArgumentTypeError(f"must be between 1 and {MAX_INT32}")
    return parsed


def positive_seconds(value: str) -> float:
    try:
        parsed = float(value)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("must be a number of seconds") from exc
    if not math.isfinite(parsed) or parsed <= 0:
        raise argparse.ArgumentTypeError("must be a finite positive number of seconds")
    return parsed


def mime_type(value: str) -> str:
    if not re.fullmatch(r"[A-Za-z0-9.+-]+/[A-Za-z0-9.+-]+", value):
        raise argparse.ArgumentTypeError("must be a concrete MIME type, e.g. text/plain")
    return value


def substitutions(name: str, content_type: str, max_bytes: int, encoding_time: float):
    lower = name[0].lower() + name[1:]
    return [
        ("PluginStubEncoding", f"Plugin{name}Encoding"),
        ("encodingStub", f"encoding{name}"),
        ("encoding-stub", f"{lower}-encoding"),
        ('"stub"', f'"{lower}"'),
        ('"text/plain"', f'"{content_type}"'),
        ("constexpr int32_t kMaxBytes = 1'000'000;",
         f"constexpr int32_t kMaxBytes = {max_bytes};"),
        ("constexpr double kEncodingTimeSeconds = 0.25;",
         f"constexpr double kEncodingTimeSeconds = {encoding_time:.17g};"),
    ]


def rename(value: str, pairs) -> str:
    for old, new in pairs:
        value = value.replace(old, new)
    return value


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--name", required=True, help="CamelCase encoding name, e.g. Base64")
    parser.add_argument("--mime-type", required=True, type=mime_type,
                        help="concrete MIME type selected by the design")
    parser.add_argument("--max-bytes", required=True, type=positive_int,
                        help="verified maximum input payload size")
    parser.add_argument("--encoding-time-seconds", required=True, type=positive_seconds,
                        help="scheduler pessimistic encode time in seconds")
    parser.add_argument("--out", required=True, type=Path,
                        help="new plugin directory; it must not exist")
    args = parser.parse_args()

    if not re.fullmatch(r"[A-Z][A-Za-z0-9]*", args.name):
        sys.exit("--name must be CamelCase, e.g. Base64")
    if args.out.exists():
        sys.exit(f"{args.out} already exists")

    pairs = substitutions(args.name, args.mime_type, args.max_bytes,
                          args.encoding_time_seconds)
    for source in sorted(TEMPLATE.rglob("*")):
        relative = source.relative_to(TEMPLATE)
        if (set(relative.parts) & SKIP_DIRS or not source.is_file()
                or source.name in SKIP_FILES
                or source.suffix not in TEXT_SUFFIXES
                and source.name not in {".gitignore"}):
            continue
        destination = args.out / Path(*[rename(part, pairs) for part in relative.parts])
        destination.parent.mkdir(parents=True, exist_ok=True)
        try:
            destination.write_text(rename(source.read_text(), pairs))
        except UnicodeDecodeError:
            shutil.copy(source, destination)
        shutil.copymode(source, destination)

    lower = args.name[0].lower() + args.name[1:]
    (args.out / "README.md").write_text(
        f"# {args.name} Encoding\n\nGenerated from `plugins/encoding-stub`. The source starts "
        "with the Base64 reference codec. Replace it for any non-Base64 format and verify the "
        "MIME type, capacity, timing, and format-specific round trips before integration.\n\n"
        "Build with `./build_artifacts_in_docker_image.sh --image-tag main` and run "
        "`./run_unit_tests.sh --image-tag main`.\n"
    )
    print(f"created {args.out} (encoding id: {lower})")
    print("next: verify the generated metadata against the design, implement the codec, and run tests")
    return 0


if __name__ == "__main__":
    sys.exit(main())
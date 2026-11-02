#!/usr/bin/env bash
set -euo pipefail

UNIT_TESTS=OFF
BUILD_VERSION="${BUILD_VERSION:-2.4.2-r1}"
while [[ $# -gt 0 ]]; do
    case "$1" in
        --unit-tests) UNIT_TESTS=ON; shift ;;
        --build-version) [[ $# -ge 2 ]] || { echo "$1 requires a value" >&2; exit 1; }; BUILD_VERSION="$2"; shift 2 ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
done

case "$(uname -m)" in
    x86_64) PRESET=LINUX_x86_64 ;;
    aarch64|arm64) PRESET=LINUX_arm64-v8a ;;
    *) echo "unsupported build host: $(uname -m)" >&2; exit 2 ;;
esac

cmake --preset "${PRESET}" -Wno-dev \
    -DBUILD_VERSION="${BUILD_VERSION}" \
    -DBUILD_UNIT_TESTS="${UNIT_TESTS}" \
    -DRACEBOAT_SOURCE_DIR="${RACEBOAT_SOURCE_DIR:-}"
cmake --build --preset "${PRESET}" --parallel
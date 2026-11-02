#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TAG="${RACEBOAT_IMAGE_TAG:-main}"
RACEBOAT_DIR="${RACEBOAT_SOURCE_DIR:-}"
if [[ -n "${RACEBOAT_DIR}" ]]; then
    RACEBOAT_DIR="$(cd "${RACEBOAT_DIR}" && pwd)"
else
    for candidate in "${SCRIPT_DIR}/../raceboat" "${SCRIPT_DIR}/../../raceboat"; do
        if [[ -f "${candidate}/test/common/MockEncodingSdk.h" ]]; then
            RACEBOAT_DIR="$(cd "${candidate}" && pwd)"
            break
        fi
    done
fi
if [[ -z "${RACEBOAT_DIR}" ]]; then
    echo "could not locate raceboat/test/common/MockEncodingSdk.h" >&2
    exit 1
fi
while [[ $# -gt 0 ]]; do
    case "$1" in
        -t|--image-tag) [[ $# -ge 2 ]] || { echo "$1 requires a value" >&2; exit 1; }; TAG="$2"; shift 2 ;;
        --image-tag=*) TAG="${1#*=}"; shift ;;
        *) break ;;
    esac
done

BUILDER="ghcr.io/tst-race/raceboat/raceboat-plugin-builder:${TAG}"
RUNTIME="ghcr.io/tst-race/raceboat/raceboat-runtime:${TAG}"
docker run --rm --user "$(id -u):$(id -g)" -e HOME=/tmp \
    -e RACEBOAT_SOURCE_DIR=/raceboat \
    -v "${SCRIPT_DIR}":/code -v "${RACEBOAT_DIR}":/raceboat:ro -w /code "${BUILDER}" \
    ./build_artifacts.sh --unit-tests

ARCH_DIR="$(find "${SCRIPT_DIR}/build" -maxdepth 1 -type d -name 'LINUX_*' -printf '%f\n' -quit)"
if [[ -z "${ARCH_DIR}" ]]; then
    echo "no Linux build directory found under ${SCRIPT_DIR}/build" >&2
    exit 1
fi
docker run --rm -v "${SCRIPT_DIR}":/code -w /code "${RUNTIME}" \
    "/code/build/${ARCH_DIR}/test/source/encodingStubTests" "$@"
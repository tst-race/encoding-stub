#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TAG="${RACEBOAT_IMAGE_TAG:-main}"
while [[ $# -gt 0 ]]; do
    case "$1" in
        -t|--image-tag) [[ $# -ge 2 ]] || { echo "$1 requires a value" >&2; exit 1; }; TAG="$2"; shift 2 ;;
        --image-tag=*) TAG="${1#*=}"; shift ;;
        *) break ;;
    esac
done

BUILDER="ghcr.io/tst-race/raceboat/raceboat-plugin-builder:${TAG}"
docker run --rm --user "$(id -u):$(id -g)" -e HOME=/tmp \
    -v "${SCRIPT_DIR}":/code -w /code "${BUILDER}" \
    ./build_artifacts.sh "$@"
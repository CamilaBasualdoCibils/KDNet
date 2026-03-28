#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"

BAKE_FILE="$ROOT_DIR/dev/docker/Dev-bake.json"
DOCKERFILE_PATH="$ROOT_DIR/dev/docker/Dev.DockerFile"
BUILDER="${BUILDER:-atlasnet-builder}"

PRELAUNCH_CMD_TEMPLATE="${1:-{EXE}}"

if [[ ! -f "$BAKE_FILE" ]]; then
  echo "Missing bake file: $BAKE_FILE" >&2
  exit 1
fi

if [[ ! -f "$DOCKERFILE_PATH" ]]; then
  echo "Missing Dockerfile: $DOCKERFILE_PATH" >&2
  exit 1
fi

if ! docker buildx inspect "$BUILDER" >/dev/null 2>&1; then
  docker buildx create --name "$BUILDER" --use
else
  docker buildx use "$BUILDER" >/dev/null
fi

docker buildx inspect --bootstrap >/dev/null

docker buildx bake \
  --file "$BAKE_FILE" \
  --set "*.dockerfile=$DOCKERFILE_PATH" \
  --set "*.args.PRELAUNCH_CMD=$PRELAUNCH_CMD_TEMPLATE" \
  --set "*.output=type=docker" 
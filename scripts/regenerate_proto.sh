#!/usr/bin/env bash
# Regenerate src/protobuf/topsbot_web.pb.{h,cc} and sync web/protos/
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

PROTOC="${PROTOC:-protoc}"
if ! command -v "${PROTOC}" >/dev/null 2>&1; then
  if [ -x /tmp/protoc312/bin/protoc ]; then
    PROTOC=/tmp/protoc312/bin/protoc
  else
    PROTOC="/home/peter/miniforge3/envs/lerobot/lib/python3.12/site-packages/torch/bin/protoc"
  fi
fi

"${PROTOC}" --proto_path="${ROOT}/proto" \
  --cpp_out="${ROOT}/src/protobuf" \
  "${ROOT}/proto/topsbot_web.proto"

mkdir -p "${ROOT}/web/protos"
cp "${ROOT}/proto/topsbot_web.proto" "${ROOT}/web/protos/"
echo "Regenerated ${ROOT}/src/protobuf/topsbot_web.pb.* and web/protos/topsbot_web.proto"

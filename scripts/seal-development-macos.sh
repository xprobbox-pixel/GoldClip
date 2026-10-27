#!/bin/bash
# Ad-hoc development seals only. No Developer ID, timestamp service or release credentials.
set -euo pipefail
if [[ $# -ne 1 || ! -d "$1/Contents/MacOS" ]]; then
  echo 'Usage: seal-development-macos.sh /path/to/native.bundle' >&2
  exit 2
fi
codesign --force --sign - --timestamp=none "$1"
codesign --verify --all-architectures --deep --strict "$1"

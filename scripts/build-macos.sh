#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
set --
if [[ -n "${JUCE_SOURCE_DIR:-}" ]]; then set -- "$@" "-DJUCE_SOURCE_DIR=$JUCE_SOURCE_DIR"; fi
cmake --preset mac-universal "$@"
cmake --build --preset mac-universal --parallel "${AURUM_JOBS:-4}"
ctest --preset mac-universal --verbose
mkdir -p dist/validation
build/mac-universal/AurumHostTests_artefacts/Release/AurumHostTests --render "$PWD/dist/validation"
for variant in Master Track; do
 for format in VST3 AU Standalone; do
  src="build/mac-universal/Aurum${variant}_artefacts/Release/$format"
  mkdir -p "dist/$variant/$format"
  for bundle in "$src/"*; do
   lipo "$bundle/Contents/MacOS/AURUM CLIP $variant" -verify_arch arm64 x86_64
   ditto "$bundle" "dist/$variant/$format/$(basename "$bundle")"
  done
 done
done
python3 scripts/manifest.py dist

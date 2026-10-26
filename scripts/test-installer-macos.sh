#!/bin/bash
set -euo pipefail
[[ "${GITHUB_ACTIONS:-}" == true ]] || { echo 'This installation/uninstallation test runs only on ephemeral CI runners';exit 1; }
package=$(find dist/installers -maxdepth 1 -name '*.pkg' -print -quit)
sudo installer -pkg "$package" -target /
for variant in Master Track; do
 for format in VST3 AU Standalone; do
  case "$format" in VST3) location='/Library/Audio/Plug-Ins/VST3';extension=vst3;; AU) location='/Library/Audio/Plug-Ins/Components';extension=component;; Standalone) location='/Applications';extension=app;; esac
  bundle="$location/AURUM CLIP $variant.$extension"
  test -d "$bundle"
  cmp "$bundle/Contents/MacOS/AURUM CLIP $variant" "dist/$variant/$format/AURUM CLIP $variant.$extension/Contents/MacOS/AURUM CLIP $variant"
  codesign --verify --all-architectures --deep --strict "$bundle"
  sudo rm -rf "$bundle"
 done
done
prefix=$(sed -n 's/set(AURUM_BUNDLE_PREFIX \(.*\))/\1/p' cmake/Identity.cmake | tr -d '"')
for variant in Master Track;do for format in VST3 AU Standalone;do sudo pkgutil --forget "$prefix.$variant.$format";done;done
echo 'Installer installation, payload checksums and removal passed.'

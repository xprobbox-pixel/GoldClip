#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
stage="${1:-dist}"
version=$(sed -n 's/set(AURUM_VERSION \(.*\))/\1/p' cmake/Identity.cmake)
prefix=$(sed -n 's/set(AURUM_BUNDLE_PREFIX \(.*\))/\1/p' cmake/Identity.cmake | tr -d '"')
mkdir -p "$stage/installers"
work=$(mktemp -d "${TMPDIR:-/tmp}/aurum-package.XXXXXX")
trap 'rm -rf "$work"' EXIT
xml="$work/Distribution.xml"
cat > "$xml" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2"><title>AURUM CLIP $version</title><options customize="always" require-scripts="false" hostArchitectures="arm64,x86_64"/><domains enable_localSystem="true"/><choices-outline>
XML
for variant in Master Track; do
 echo "<line choice=\"$variant\">" >> "$xml"
 for format in VST3 AU Standalone; do echo "<line choice=\"$variant-$format\"/>" >> "$xml"; done
 echo '</line>' >> "$xml"
done
echo '</choices-outline>' >> "$xml"
for variant in Master Track; do
 echo "<choice id=\"$variant\" title=\"$variant edition\"/>" >> "$xml"
 for format in VST3 AU Standalone; do
  case "$format" in VST3) location='/Library/Audio/Plug-Ins/VST3';; AU) location='/Library/Audio/Plug-Ins/Components';; Standalone) location='/Applications';; esac
  root="$work/root-$variant-$format"; mkdir -p "$root"
  for bundle in "$stage/$variant/$format/"*; do
   destination="$root/$(basename "$bundle")";ditto "$bundle" "$destination"
   lipo "$destination/Contents/MacOS/AURUM CLIP $variant" -verify_arch arm64 x86_64
   if [[ -n "${AURUM_APP_SIGN_ID:-}" ]]; then
    codesign --force --options runtime --timestamp --sign "$AURUM_APP_SIGN_ID" "$destination"
    ditto "$destination" "$bundle"
   fi
   codesign --verify --all-architectures --deep --strict "$destination"
  done
  id="$prefix.$variant.$format";package="$variant-$format.pkg"
  info="$work/components-$variant-$format.plist"
  pkgbuild --analyze --root "$root" "$info"
  python3 - "$info" <<'PYINFO'
import plistlib,sys
path=sys.argv[1]
with open(path,'rb') as stream: components=plistlib.load(stream)
for component in components:
 component['BundleIsRelocatable']=False
 component['BundleIsVersionChecked']=False
 component['BundleOverwriteAction']='upgrade'
with open(path,'wb') as stream: plistlib.dump(components,stream)
PYINFO
  pkgbuild --root "$root" --component-plist "$info" --install-location "$location" --identifier "$id" --version "$version" "$work/$package"
  echo "<choice id=\"$variant-$format\" title=\"$variant $format\" start_selected=\"true\"><pkg-ref id=\"$id\"/></choice>" >> "$xml"
  echo "<pkg-ref id=\"$id\" version=\"$version\">$package</pkg-ref>" >> "$xml"
 done
done
echo '</installer-gui-script>' >> "$xml"
output="$stage/installers/AURUM-CLIP-$version-macOS-Universal.pkg"
if [[ -n "${AURUM_INSTALLER_SIGN_ID:-}" ]]; then
 productbuild --distribution "$xml" --package-path "$work" --sign "$AURUM_INSTALLER_SIGN_ID" "$output"
 pkgutil --check-signature "$output"
else
 productbuild --distribution "$xml" --package-path "$work" "$output"
fi
if [[ -n "${AURUM_NOTARY_PROFILE:-}" ]]; then
 if [[ -z "${AURUM_APP_SIGN_ID:-}" || -z "${AURUM_INSTALLER_SIGN_ID:-}" ]]; then echo 'Notarisation requires both signing identities' >&2;exit 1;fi
 xcrun notarytool submit "$output" --keychain-profile "$AURUM_NOTARY_PROFILE" --wait
 xcrun stapler staple "$output";xcrun stapler validate "$output"
 spctl --assess --type install --verbose=4 "$output"
fi
pkgutil --expand-full "$output" "$work/expanded"
cp "$xml" "$stage/installers/Distribution.xml"
python3 scripts/manifest.py "$stage"
echo "Created $output; consult manifest for signing status."

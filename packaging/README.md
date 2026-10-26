# Native installers and optional signing

Run the native build script first. Then:

- macOS: `bash scripts/package-macos.sh` creates `dist/installers/AURUM-CLIP-1.0.0-macOS-Universal.pkg` with six selectable components (Master/Track AU, VST3, standalone). Payloads go to `/Library/Audio/Plug-Ins/Components`, `/Library/Audio/Plug-Ins/VST3`, `/Applications`.
- Windows: install Inno Setup 6, then `./scripts/package-windows.ps1`. The EXE offers Master/Track VST3 and standalone components, standard system VST3 paths, Program Files apps, Start menu entries and uninstall support. MSVC runtime is statically linked; no separately downloaded runtime installer is bundled.

Versions, company and bundle prefix come from `cmake/Identity.cmake`. `Aurum Development` and `com.aurumclip.dev` remain development identities. Keep codes stable for saved sessions. Native CI builds/tests both installers and performs install/payload-hash/uninstall checks **only on its ephemeral runners**. The installer test scripts refuse to run locally. Normal packaging does not install anything.

Unsigned development packages are built when signing credentials are absent. They must not be represented as publisher-signed or notarised. OS security may refuse them. No security-policy bypass is automated.

## Credentialled release workflow

`.github/workflows/signed-release.yml` is a separate, manually dispatched workflow using the GitHub `release` environment. It becomes dispatchable after the workflow is available on the default branch. It requires appropriate certificates and fails rather than silently downgrading a requested signed build.

macOS secrets: `APPLE_CERTIFICATES_P12_BASE64` containing Developer ID Application and Installer certificates, `APPLE_CERTIFICATE_PASSWORD`, `APPLE_APPLICATION_IDENTITY`, `APPLE_INSTALLER_IDENTITY`, `APPLE_ID`, `APPLE_APP_PASSWORD`, `APPLE_TEAM_ID`. A temporary keychain is removed after the job. The packaging script uses `AURUM_APP_SIGN_ID`, `AURUM_INSTALLER_SIGN_ID` and `AURUM_NOTARY_PROFILE`. It signs bundles, signs the product package, submits/waits for notarisation, staples and verifies the package, and assesses it with spctl. Signing identities can instead refer to existing local keychain certificates. None have been executed or verified without credentials.

Windows secrets: `WINDOWS_PFX_BASE64`, `WINDOWS_PFX_PASSWORD`. The script uses `AURUM_SIGN_PFX` and `AURUM_SIGN_PASSWORD`; signtool signs/timestamps executable payloads and the installer and verifies Authenticode signatures. Temporary certificate files are removed. Credentials are never committed.

## Uninstall

Windows: use Settings → Apps → AURUM CLIP or its Start menu uninstaller. User presets/preferences are preserved.

macOS: close audio hosts, then remove only the installed AURUM CLIP Master/Track `.component`, `.vst3` and `.app` bundles from the locations above. Receipts may be removed with `pkgutil --forget` using the exact six identifiers from the package's Distribution XML. User presets/preferences remain in the user's application-data `AurumClip/{Master,Track}` directories. Remove those separately only when intentionally discarding settings.

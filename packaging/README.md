# Installer boundary

Phase 1 produces native plugin/application bundles, not installers. No installer or signing steps are implied by an ordinary CI pass.

A later release phase will add selectable Inno Setup components on Windows and Apple pkgbuild/productbuild components on macOS. VST3, AU and application payloads will use standard locations. Developer ID, Authenticode and notarisation credentials must be securely supplied to a separate release workflow. This directory deliberately contains no nonfunctional installer definitions.

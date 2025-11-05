RTPlotter packaging notes
=========================

This document shows how to create distributable packages for macOS (.dmg) and Linux (AppImage).

Prerequisites
-------------
- Qt6 for your target platform (macOS: macdeployqt; Linux: linuxdeployqt AppImage)
- CMake, a C++ toolchain and required image conversion tools (rsvg-convert or inkscape)
- On macOS: iconutil (system tool) and optionally codesign / notarytool for notarization

Scripts
-------
Two helper scripts are provided in `scripts/`:
- `scripts/package-mac.sh <path-to-macdeployqt>` : builds Release, creates an .icns from `resources/icons/logo.svg` (if rsvg-convert or inkscape are available), and runs macdeployqt to produce a `.app` and `.dmg`.
- `scripts/package-linux-appimage.sh <path-to-linuxdeployqt.AppImage>` : builds Release, prepares an AppDir, converts `resources/icons/logo.svg` to 256x256 PNG (if possible), and runs linuxdeployqt to create an AppImage.

macOS quick steps
-----------------
1. Ensure you have Qt installed and know the path to `macdeployqt` (e.g. `$QT_DIR/6.x/clang_64/bin/macdeployqt`).
2. Run:

```bash
./scripts/package-mac.sh /path/to/macdeployqt
```

This will produce `RTPlotter.app` and a `.dmg` in the build directory if macdeployqt succeeds.

Linux quick steps (AppImage)
---------------------------
1. Download the latest `linuxdeployqt` AppImage from its GitHub releases and make it executable.
2. Run:

```bash
./scripts/package-linux-appimage.sh /path/to/linuxdeployqt.AppImage
```

This will create an AppImage in the build directory.

Notes and caveats
-----------------
- The scripts try to convert SVG to PNG/ICNS using `rsvg-convert` or `inkscape`. If you don't have those, provide pre-generated icons (ICNS/PNG) in the expected paths.
- For proper macOS distribution you may want to codesign and notarize the DMG (requires Apple Developer account and extra steps). See macOS docs for `codesign` and `notarytool` / `altool`.
- `CMakeLists.txt` was updated to set `MACOSX_BUNDLE` and `MACOSX_BUNDLE_ICON_FILE` when building on macOS.

If you want, I can:
- Add an automated CPack configuration to build .dmg/.deb packages as part of CMake.
- Generate the .icns for you here if `rsvg-convert` is available on this machine.
- Create a small GitHub Actions workflow to produce mac and linux artifacts automatically.

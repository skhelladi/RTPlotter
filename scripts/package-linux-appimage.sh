#!/usr/bin/env bash
set -euo pipefail

# package-linux-appimage.sh
# Usage: ./scripts/package-linux-appimage.sh [path-to-linuxdeployqt.AppImage]
# If no path given, set LINUXDEPLOYQT env var to the AppImage path.
# NOTE: This script is for Linux only. On macOS, use ./scripts/package-mac.sh instead.

# Check if we're on macOS and fail early with helpful message
if [[ "$OSTYPE" == "darwin"* ]]; then
  echo "Error: This script is for Linux only (AppImage packaging)." >&2
  echo "You are on macOS. Use ./scripts/package-mac.sh instead to create a .dmg:" >&2
  echo "  ./scripts/package-mac.sh /path/to/macdeployqt" >&2
  exit 1
fi

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
APPDIR="$BUILD_DIR/AppDir"
DESKTOP_FILE="$BUILD_DIR/RTPlotter.desktop"
ICON_PNG="$BUILD_DIR/rtplotter-256.png"
SVG_ICON="$ROOT_DIR/resources/icons/logo.svg"

LINUXDEPLOYQT="${1:-${LINUXDEPLOYQT:-}}"
if [[ -z "$LINUXDEPLOYQT" ]]; then
  echo "linuxdeployqt AppImage path not provided. Pass it as first arg or set LINUXDEPLOYQT env var." >&2
  exit 1
fi

# Tools check
if ! command -v cmake >/dev/null 2>&1; then
  echo "cmake not found" >&2; exit 1
fi

# Build release
mkdir -p "$BUILD_DIR"
pushd "$BUILD_DIR" >/dev/null
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -- -j

# Prepare AppDir
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"

# Copy binary: prefer plain executable, but if build produced a macOS .app bundle use its inner executable
if [ -f RTPlotter ]; then
  cp RTPlotter "$APPDIR/usr/bin/rtplotter"
  chmod +x "$APPDIR/usr/bin/rtplotter"
elif [ -f RTPlotter.app/Contents/MacOS/RTPlotter ]; then
  echo "Found macOS bundle; using inner executable RTPlotter.app/Contents/MacOS/RTPlotter as binary for AppDir"
  cp RTPlotter.app/Contents/MacOS/RTPlotter "$APPDIR/usr/bin/rtplotter"
  chmod +x "$APPDIR/usr/bin/rtplotter"
else
  echo "Error: build/RTPlotter not found and no RTPlotter.app/Contents/MacOS/RTPlotter found. Build may have failed." >&2
  exit 1
fi

# Create .desktop
cat > "$DESKTOP_FILE" <<EOF
[Desktop Entry]
Name=RTPlotter
Exec=rtplotter
Icon=rtplotter
Type=Application
Categories=Science;Education;
EOF
cp "$DESKTOP_FILE" "$APPDIR/usr/share/applications/"

# Convert SVG icon to PNG (256x256)
if command -v rsvg-convert >/dev/null 2>&1; then
  rsvg-convert -w 256 -h 256 "$SVG_ICON" -o "$ICON_PNG"
elif command -v inkscape >/dev/null 2>&1; then
  inkscape "$SVG_ICON" --export-type=png --export-filename="$ICON_PNG" -w 256 -h 256
else
  echo "Warning: no rsvg-convert or inkscape found; please provide a 256x256 PNG icon at $ICON_PNG" >&2
fi
cp "$ICON_PNG" "$APPDIR/usr/share/icons/hicolor/256x256/apps/rtplotter.png"

# Sanity check: ensure the desktop file exists inside AppDir (linuxdeployqt expects it)
if [[ ! -f "$APPDIR/usr/share/applications/RTPlotter.desktop" ]]; then
  echo "Error: expected desktop file not found at $APPDIR/usr/share/applications/RTPlotter.desktop" >&2
  echo "Listing $APPDIR/usr/share/applications:" >&2
  ls -la "$APPDIR/usr/share/applications" || true
  exit 1
fi

# Run linuxdeployqt to create AppImage
"$LINUXDEPLOYQT" "$APPDIR/usr/share/applications/RTPlotter.desktop" -appimage

popd >/dev/null

echo "AppImage packaging complete. Check $BUILD_DIR for the generated AppImage." 

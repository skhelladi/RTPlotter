#!/usr/bin/env bash
set -euo pipefail

# package-mac.sh
# Usage: ./scripts/package-mac.sh [path-to-qt]/bin/macdeployqt
# If no macdeployqt provided, please set MACDEPLOYQT env var or pass path as first arg.

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
RES_DIR="$ROOT_DIR/resources/icons"
ICNS_NAME="RTPlotter.icns"
ICON_SVG="$RES_DIR/logo.svg"

MACDEPLOYQT="${1:-${MACDEPLOYQT:-}}"
if [[ -z "$MACDEPLOYQT" ]]; then
  echo "macdeployqt path not provided. Pass it as first arg or set MACDEPLOYQT env var." >&2
  exit 1
fi

# Ensure required tools
if ! command -v iconutil >/dev/null 2>&1; then
  echo "iconutil not found: required to create .icns. It's available on macOS." >&2
  exit 1
fi

# Prefer rsvg-convert (librsvg) or inkscape for SVG->PNG
SVG2PNG=""
if command -v rsvg-convert >/dev/null 2>&1; then
  SVG2PNG=rsvg-convert
elif command -v inkscape >/dev/null 2>&1; then
  SVG2PNG=inkscape
else
  echo "Warning: no rsvg-convert or inkscape found; you'll need one to convert SVG to PNG for iconset." >&2
fi

mkdir -p "$BUILD_DIR"
pushd "$BUILD_DIR" >/dev/null

# Create Release build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -- -j

# Create .icns from SVG (if svg converter available)
ICONSET_DIR="$BUILD_DIR/AppIcon.iconset"
rm -rf "$ICONSET_DIR"
mkdir -p "$ICONSET_DIR"

if [[ -n "$SVG2PNG" ]]; then
  declare -a SIZES=(16 32 64 128 256 512 1024)
  for s in "${SIZES[@]}"; do
    out="$ICONSET_DIR/icon_${s}x${s}.png"
    if [[ "$SVG2PNG" == "rsvg-convert" ]]; then
      rsvg-convert -w $s -h $s "$ICON_SVG" -o "$out"
    else
      inkscape "$ICON_SVG" --export-type=png --export-filename="$out" -w $s -h $s
    fi
  done
  # add the @2x variants where appropriate
  cp "$ICONSET_DIR/icon_16x16.png" "$ICONSET_DIR/icon_16x16@2x.png" || true
  cp "$ICONSET_DIR/icon_32x32.png" "$ICONSET_DIR/icon_32x32@2x.png" || true
  cp "$ICONSET_DIR/icon_128x128.png" "$ICONSET_DIR/icon_128x128@2x.png" || true
  cp "$ICONSET_DIR/icon_256x256.png" "$ICONSET_DIR/icon_256x256@2x.png" || true
  cp "$ICONSET_DIR/icon_512x512.png" "$ICONSET_DIR/icon_512x512@2x.png" || true

  iconutil -c icns "$ICONSET_DIR" -o "$BUILD_DIR/$ICNS_NAME"
  echo "Created $BUILD_DIR/$ICNS_NAME"
  # copy into resources so CMake install/bundle can use it if desired
  mkdir -p "$RES_DIR"
  cp "$BUILD_DIR/$ICNS_NAME" "$RES_DIR/"
else
  echo "Skipping .icns creation (no SVG->PNG tool). If you have an existing .icns, place it at resources/icons/$ICNS_NAME" >&2
fi

# Build should have created RTPlotter.app if CMake had MACOSX_BUNDLE
APP_BUNDLE="$BUILD_DIR/RTPlotter.app"
if [[ ! -d "$APP_BUNDLE" ]]; then
  echo "No .app bundle found at $APP_BUNDLE." >&2
  echo "Note: On non-mac builds CMake may produce a plain RTPlotter executable at build/RTPlotter instead of an .app bundle." >&2
  echo "If you intended to build a macOS .app, ensure you are on macOS and that CMakeLists sets MACOSX_BUNDLE (it does when building on macOS)." >&2
fi

# Run macdeployqt to bundle Qt frameworks
"$MACDEPLOYQT" "$APP_BUNDLE"

# Fix code signing issue: macdeployqt may leave a broken/incomplete signature
# Remove it so the app can run (or sign properly if codesign identity provided)
# If a codesign identity was provided, ensure it exists in the keychain before attempting to sign.
if [[ -n "${CODESIGN_IDENTITY:-}" ]]; then
  echo "CODESIGN_IDENTITY is set to: $CODESIGN_IDENTITY"
  echo "Checking for the identity in the login keychain..."
  if ! security find-identity -p codesigning -v | grep -F -- "$CODESIGN_IDENTITY" >/dev/null 2>&1; then
    echo "ERROR: Signing identity '$CODESIGN_IDENTITY' not found in keychains." >&2
    echo "Available code signing identities:" >&2
    security find-identity -p codesigning -v >&2 || true
    echo "" >&2
    echo "If you have a .p12 export of your Developer ID certificate, import it into the login keychain with:" >&2
    echo "  security import /path/to/cert.p12 -k ~/Library/Keychains/login.keychain-db -P \"p12-password\" -T /usr/bin/codesign" >&2
    echo "Make sure the private key is present and accessible to codesign. Then re-run this script." >&2
    echo "Alternatively, unset CODESIGN_IDENTITY to remove the signature and continue for local testing." >&2
    popd >/dev/null || true
    exit 2
  fi
fi

if [[ -z "${CODESIGN_IDENTITY:-}" ]]; then
  # No signing identity provided: remove broken signature for testing/development
  echo "Removing broken code signature (signing identity not provided; use for development/testing only)..."
  codesign --remove-signature "$APP_BUNDLE" 2>/dev/null || true
else
  # Resign with proper identity
  echo "Re-signing with identity: $CODESIGN_IDENTITY"
  codesign --deep --force --options runtime --sign "$CODESIGN_IDENTITY" "$APP_BUNDLE"
fi

# Create a proper DMG with Applications symlink (better user experience)
# First, create a temporary DMG staging area
STAGE_DIR="$BUILD_DIR/dmg_stage"
rm -rf "$STAGE_DIR"
mkdir -p "$STAGE_DIR"

# Copy the app bundle
cp -r "$APP_BUNDLE" "$STAGE_DIR/"

# Create symlink to /Applications for easy drag-and-drop install
ln -s /Applications "$STAGE_DIR/Applications"

# Create the DMG with hdiutil (better than macdeployqt's automatic DMG)
DMG_PATH="$BUILD_DIR/RTPlotter.dmg"
rm -f "$DMG_PATH"
hdiutil create -volname "RTPlotter" -srcfolder "$STAGE_DIR" -ov -format UDZO "$DMG_PATH" 2>&1 | grep -v "Copying" | grep -v "Created" || true

echo "✓ macdeployqt finished."
echo "✓ Code signature fixed (for dev/test; use CODESIGN_IDENTITY to sign properly)."
echo "✓ Custom DMG created: $DMG_PATH"
echo "  Users can double-click this DMG and drag RTPlotter.app to Applications."
popd >/dev/null

# Optional: codesign and notarize (semi-automatic)
# To codesign: set CODESIGN_IDENTITY (name of your Developer ID Application identity)
# To notarize: set NOTARIZE=true and provide NOTARY_USERNAME and NOTARY_KEY (path to private key) or configure xcrun notarytool profile
if [[ -n "${CODESIGN_IDENTITY:-}" ]]; then
  echo "Codesigning using identity: $CODESIGN_IDENTITY"
  echo "Signing the app bundle..."
  codesign --deep --force --options runtime --sign "$CODESIGN_IDENTITY" "$APP_BUNDLE"
  echo "Codesign finished. You may verify with: codesign --verify --deep --strict --verbose=2 '$APP_BUNDLE'"
fi

if [[ "${NOTARIZE:-false}" == "true" ]]; then
  echo "Notarization requested"
  if command -v xcrun >/dev/null 2>&1; then
    if [[ -n "${NOTARY_USERNAME:-}" && -n "${NOTARY_KEY:-}" ]]; then
      echo "Submitting to Apple notary service via notarytool..."
      # Example using notarytool with key in a keychain-profile or Apple-supplied key
      # NOTARY_USERNAME - Apple ID e.g. you@apple.com
      # NOTARY_KEY - path to the private key (or set up a keychain profile)
      xcrun notarytool submit "$BUILD_DIR"/RTPlotter.dmg --key "$NOTARY_KEY" --key-id "${NOTARY_KEY_ID:-}" --issuer "$NOTARY_ISSUER_ID" --wait
      echo "Stapling the notarization ticket..."
      xcrun stapler staple "$BUILD_DIR"/RTPlotter.dmg
      echo "Notarization & stapling complete (check output for errors)."
    else
      echo "NOTARIZE=true but NOTARY_USERNAME/NOTARY_KEY/NOTARY_ISSUER_ID not set. Skipping notarize." >&2
    fi
  else
    echo "xcrun not found; cannot notarize from this machine." >&2
  fi
fi

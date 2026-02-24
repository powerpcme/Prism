#!/bin/bash
#
# build-mac-vst3.sh — Build Prism VST3, Standalone, and AU for macOS
#
# Usage: ./build-mac-vst3.sh
#
# Output: mac-artifacts/{Prism.vst3,Prism.component,Prism.app}
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
OUTPUT_DIR="$SCRIPT_DIR/mac-artifacts"
BUILD_TYPE="Release"
MACOS_DEPLOYMENT_TARGET="${MACOS_DEPLOYMENT_TARGET:-14.0}"

echo "============================================"
echo "  Prism — macOS Build (VST3 + AU + Standalone)"
echo "============================================"
echo ""

# ── Check for cmake ──────────────────────────────────────────────────────────
if ! command -v cmake &>/dev/null; then
    echo "ERROR: cmake is not installed."
    echo "       Install it with:  brew install cmake"
    exit 1
fi

# ── Clean stale cache ────────────────────────────────────────────────────────
if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
    CACHED_SRC=$(grep -m1 'CMAKE_HOME_DIRECTORY' "$BUILD_DIR/CMakeCache.txt" 2>/dev/null | cut -d= -f2 || true)
    if [ -n "$CACHED_SRC" ] && [ "$CACHED_SRC" != "$SCRIPT_DIR" ]; then
        echo "Stale build cache detected (was: $CACHED_SRC)"
        echo "Cleaning build directory..."
        rm -rf "$BUILD_DIR"
    fi
fi

# ── Configure ────────────────────────────────────────────────────────────────
echo "[1/4] Configuring CMake..."
cmake -B "$BUILD_DIR" \
      -G "Unix Makefiles" \
      -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
      -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
      -DCMAKE_OSX_DEPLOYMENT_TARGET="$MACOS_DEPLOYMENT_TARGET" \
      -Wno-dev \
      "$SCRIPT_DIR"
echo ""

# ── Build targets ────────────────────────────────────────────────────────────
echo "[2/4] Building Prism VST3, Standalone, and AU..."
cmake --build "$BUILD_DIR" \
      --config "$BUILD_TYPE" \
      --target Prism_VST3 Prism_Standalone Prism_AU \
      -j"$(sysctl -n hw.ncpu)"
echo ""

# ── Copy to artifacts folder ────────────────────────────────────────────────
echo "[3/4] Copying artifacts to $OUTPUT_DIR ..."
ARTEFACTS_DIR="$BUILD_DIR/Prism_artefacts/$BUILD_TYPE"
VST3_PATH="$ARTEFACTS_DIR/VST3/Prism.vst3"
AU_PATH="$ARTEFACTS_DIR/AU/Prism.component"
STANDALONE_PATH="$ARTEFACTS_DIR/Standalone/Prism.app"

if [ ! -d "$VST3_PATH" ]; then
    echo "ERROR: VST3 output not found at $VST3_PATH"
    exit 1
fi
if [ ! -d "$AU_PATH" ]; then
    echo "ERROR: AU output not found at $AU_PATH"
    exit 1
fi
if [ ! -d "$STANDALONE_PATH" ]; then
    echo "ERROR: Standalone output not found at $STANDALONE_PATH"
    exit 1
fi

rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"
cp -R "$VST3_PATH" "$OUTPUT_DIR/"
cp -R "$AU_PATH" "$OUTPUT_DIR/"
cp -R "$STANDALONE_PATH" "$OUTPUT_DIR/"
echo ""

# ── Verify ───────────────────────────────────────────────────────────────────
echo "[4/4] Verifying build..."
VST3_BINARY="$OUTPUT_DIR/Prism.vst3/Contents/MacOS/Prism"
AU_BINARY="$OUTPUT_DIR/Prism.component/Contents/MacOS/Prism"
APP_BINARY="$OUTPUT_DIR/Prism.app/Contents/MacOS/Prism"

if [ -f "$VST3_BINARY" ] && [ -f "$AU_BINARY" ] && [ -f "$APP_BINARY" ]; then
    echo "  VST3:       $(du -sh "$OUTPUT_DIR/Prism.vst3" | cut -f1)"
    echo "  AU:         $(du -sh "$OUTPUT_DIR/Prism.component" | cut -f1)"
    echo "  Standalone: $(du -sh "$OUTPUT_DIR/Prism.app" | cut -f1)"
    echo ""
    echo "============================================"
    echo "  BUILD SUCCESSFUL"
    echo "  Output: $OUTPUT_DIR"
    echo "============================================"
else
    echo "ERROR: One or more output binaries are missing."
    exit 1
fi

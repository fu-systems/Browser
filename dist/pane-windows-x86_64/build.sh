#!/bin/bash
# Pane Browser — Windows Cross-Compile Script (MinGW-w64)
#
# Prerequisites:
#   sudo apt install gcc-mingw-w64-x86-64 mingw-w64-tools
#
# Produces: pane.exe, pane_cli.exe

set -e

echo ""
echo " Pane Browser v0.1.0 — Windows Cross-Compile (MinGW)"
echo " ===================================================="
echo ""

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

if ! command -v x86_64-w64-mingw32-gcc &>/dev/null; then
    echo "ERROR: MinGW not found."
    echo "Install: sudo apt install gcc-mingw-w64-x86-64"
    exit 1
fi

cd "$SRC_ROOT"

mkdir -p build-mingw
cd build-mingw

echo "Configuring..."
cmake "$SRC_ROOT" \
    -DCMAKE_TOOLCHAIN_FILE="$SRC_ROOT/cmake/win32-toolchain.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="-O2"

echo ""
echo "Building..."
make -j"$(nproc)"

echo ""
echo " Build successful!"
echo " Binaries: build-mingw/pane.exe, build-mingw/pane_cli.exe"
echo ""

# Copy to dist
cp -v pane.exe "$SCRIPT_DIR/" 2>/dev/null || true
cp -v pane_cli.exe "$SCRIPT_DIR/" 2>/dev/null || true

echo "Done."

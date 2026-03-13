#!/bin/bash
# Pane Browser — Linux Installer
# Installs the Pane browser and CLI tools to /usr/local/bin

set -e

INSTALL_DIR="/usr/local/bin"

echo "Pane Browser v0.1.0 — Installer"
echo "================================"
echo ""

# Check dependencies
echo "Checking dependencies..."
MISSING=""
for lib in libgtk-3.so.0 libcairo.so.2 libfreetype.so.6 libfontconfig.so.1 libssl.so.3; do
    if ! ldconfig -p 2>/dev/null | grep -q "$lib"; then
        MISSING="$MISSING $lib"
    fi
done

if [ -n "$MISSING" ]; then
    echo ""
    echo "Missing libraries:$MISSING"
    echo ""
    echo "Install them with:"
    echo "  Ubuntu/Debian: sudo apt install libgtk-3-0 libcairo2 libfreetype6 libfontconfig1 libssl3"
    echo "  Fedora:        sudo dnf install gtk3 cairo freetype fontconfig openssl"
    echo "  Arch:          sudo pacman -S gtk3 cairo freetype2 fontconfig openssl"
    echo ""
    read -p "Continue anyway? [y/N] " -r
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Install
echo ""
if [ -w "$INSTALL_DIR" ]; then
    cp -v pane "$INSTALL_DIR/pane"
    cp -v pane_cli "$INSTALL_DIR/pane-cli"
    cp -v pane_render_test "$INSTALL_DIR/pane-render-test"
    chmod +x "$INSTALL_DIR/pane" "$INSTALL_DIR/pane-cli" "$INSTALL_DIR/pane-render-test"
else
    echo "Installing to $INSTALL_DIR (requires sudo)..."
    sudo cp -v pane "$INSTALL_DIR/pane"
    sudo cp -v pane_cli "$INSTALL_DIR/pane-cli"
    sudo cp -v pane_render_test "$INSTALL_DIR/pane-render-test"
    sudo chmod +x "$INSTALL_DIR/pane" "$INSTALL_DIR/pane-cli" "$INSTALL_DIR/pane-render-test"
fi

echo ""
echo "Installed:"
echo "  pane             — GUI browser"
echo "  pane-cli         — CLI rendering demo"
echo "  pane-render-test — Offscreen PNG renderer"
echo ""
echo "Run 'pane' to launch the browser."
echo "Run 'pane-cli <file.html>' to render in terminal."
echo "Run 'pane-render-test output.png' to render to PNG."

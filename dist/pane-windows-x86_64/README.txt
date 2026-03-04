Pane Browser v0.1.0 — Windows x86_64
======================================

A web browser built from scratch in C17 with a pairwise context
transition rendering engine. Uses native Win32 GDI for rendering.

BUILD FROM SOURCE
-----------------
Option A: Visual Studio (recommended)

  1. Install Visual Studio 2022 with "Desktop development with C++"
  2. Open "x64 Native Tools Command Prompt for VS 2022"
  3. Run: build.bat
  4. Binaries appear in build-win\Release\

  For FreeType font support (optional, better text rendering):
  - Install vcpkg: https://vcpkg.io
  - Run: vcpkg install freetype:x64-windows
  - Set VCPKG_ROOT environment variable before running build.bat

Option B: MinGW Cross-Compile (from Linux)

  1. Install: sudo apt install gcc-mingw-w64-x86-64
  2. Run: bash build.sh
  3. Produces pane.exe and pane_cli.exe

FILES
-----
  pane.exe     — GUI Browser (native Win32 window)
  pane_cli.exe — Terminal CLI demo
  build.bat    — MSVC build script
  build.sh     — MinGW cross-compile script

USAGE
-----
  Double-click pane.exe to launch the browser.

  Features:
  - Address bar: type file paths or HTML
  - Navigation: Back (<), Forward (>), Reload (R), Home (H)
  - Scrolling: mouse wheel, arrow keys, Page Up/Down, Space
  - Keyboard: F5 reload, Enter in URL bar to navigate

  CLI mode:
  pane_cli.exe mypage.html

NOTE
----
This is a source-build package. Pre-compiled binaries require
building on a Windows machine with Visual Studio or MinGW.
The rendering engine (HTML parser, CSS cascade, layout) is
pure C17 with no external dependencies.

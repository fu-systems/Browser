Pane Browser v0.1.0 — Linux x86_64
====================================

A web browser built from scratch in C17 with a pairwise context
transition rendering engine.

FILES
-----
  pane             — GTK3 graphical browser
  pane_cli         — Terminal CLI demo (layout tree + display list)
  pane_render_test — Offscreen renderer (outputs PNG)
  install.sh       — Installer script

QUICK START
-----------
  # Run directly (no install needed):
  ./pane

  # Or install system-wide:
  sudo ./install.sh

  # CLI mode:
  ./pane_cli mypage.html

  # Render to PNG:
  ./pane_render_test output.png

DEPENDENCIES
------------
  GTK3, Cairo, FreeType2, Fontconfig (all standard on most desktops)

  Ubuntu/Debian:
    sudo apt install libgtk-3-0 libcairo2 libfreetype6 libfontconfig1

  Fedora:
    sudo dnf install gtk3 cairo freetype fontconfig

  Arch Linux:
    sudo pacman -S gtk3 cairo freetype2 fontconfig

BROWSER FEATURES
----------------
  - Tab bar: Ctrl+T (new), Ctrl+W (close), Ctrl+Tab (switch)
  - Navigation: Alt+Left (back), Alt+Right (forward), F5 (reload)
  - Address bar: Ctrl+L to focus, Enter to navigate
  - Scrolling: Mouse wheel, arrow keys, Page Up/Down, Space, Home/End
  - Input: file:// paths, local files, inline HTML, about:home

RENDERING ENGINE
----------------
  - HTML5 tokenizer (36 states) + tree builder (19 insertion modes)
  - CSS cascade: 412 properties, 74 shorthands, selector matching
  - Pairwise context transitions: 21 context fields per element
  - Block + inline layout with margin collapsing
  - Cairo + FreeType font rendering

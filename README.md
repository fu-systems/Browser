# Pane — A Minimal, Secure Web Browser

Pane is a from-scratch web browser built with one principle: **do nothing unless the user explicitly asks for it.**

No cookies. No JavaScript. No tracking. No telemetry. Just HTML rendered in a window. Everything beyond that is opt-in via plugins.

---

## Why?

Modern browsers are massive, opaque machines. They execute arbitrary code, store persistent state, fingerprint hardware, and phone home — all before you've finished reading the first paragraph of an article.

Pane takes the opposite approach:

- **Read-only by default.** The browser fetches HTML and renders it. It does not send data back to the server beyond the initial request.
- **No ambient authority.** Cookies, JavaScript, WebSockets, local storage — none of these exist until the user installs a plugin that provides them.
- **Small and auditable.** The core browser should stay small enough that a single developer can read and understand the entire codebase.

---

## Core Architecture

```
┌─────────────────────────────────────────────────────┐
│                      Pane UI                        │
│  ┌───────────────────────────────────────────────┐  │
│  │               Tab / Window Manager            │  │
│  └───────────────┬───────────────────────────────┘  │
│                  │                                   │
│  ┌───────────────▼───────────────────────────────┐  │
│  │              Render Engine                     │  │
│  │  ┌──────────┐  ┌───────────┐  ┌────────────┐ │  │
│  │  │  HTML     │  │  CSS      │  │  Layout    │ │  │
│  │  │  Parser   │  │  Parser   │  │  Engine    │ │  │
│  │  └──────────┘  └───────────┘  └────────────┘ │  │
│  └───────────────┬───────────────────────────────┘  │
│                  │                                   │
│  ┌───────────────▼───────────────────────────────┐  │
│  │             Network Layer                      │  │
│  │  HTTP/HTTPS requests only. No state persisted. │  │
│  └───────────────────────────────────────────────┘  │
│                                                      │
│  ┌──────────────────────────────────────────────┐   │
│  │            Plugin Interface                   │   │
│  │  Hooks into network, rendering, and storage   │   │
│  │  to extend capability when the user opts in.  │   │
│  └──────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────┘
```

---

## Roadmap

### Phase 1 — Fetch and Render (MVP)

The browser can open a URL, fetch the HTML over HTTP/HTTPS, and display it.

- [ ] **TCP/TLS networking** — Make raw HTTP/1.1 and HTTPS requests. No cookie headers. No referrer headers. Minimal request footprint.
- [ ] **HTML parser** — Parse HTML5 into a DOM tree. Handle malformed markup gracefully (the real web is messy).
- [ ] **CSS parser** — Parse inline styles and `<style>` blocks. External stylesheets loaded via `<link>`.
- [ ] **Layout engine** — Block and inline layout. Box model (margin, padding, border). Basic text flow and wrapping.
- [ ] **Painting** — Render the laid-out page to a window. Text rendering with system fonts. Background colors and borders.
- [ ] **Navigation** — Address bar, clickable links, back/forward history (in-memory only, not persisted).
- [ ] **Basic UI shell** — Window frame, address bar, back/forward/reload buttons, tab bar.

### Phase 2 — Make It Usable

Polish the core until everyday reading is comfortable.

- [ ] **Images** — Fetch and display `<img>` elements (PNG, JPEG, GIF, SVG).
- [ ] **Forms (display only)** — Render `<input>`, `<select>`, `<textarea>` so form-heavy pages don't look broken. No submission by default.
- [ ] **Text selection and copy** — Select text with the mouse, copy to clipboard.
- [ ] **Keyboard shortcuts** — Ctrl+L (address bar), Ctrl+T (new tab), Ctrl+W (close tab), Ctrl+C (copy), F5 (reload).
- [ ] **Scroll** — Smooth scrolling, scroll bar, page up/down, Home/End.
- [ ] **View source** — Show raw HTML for the current page.
- [ ] **Find on page** — Ctrl+F text search with highlighting.

### Phase 3 — Plugin System

Open the browser up to opt-in extensions without compromising the secure default.

- [ ] **Plugin API** — Define a stable interface that plugins can hook into: network requests, DOM post-processing, storage, and UI panels.
- [ ] **Plugin sandboxing** — Each plugin runs in an isolated process/sandbox. A misbehaving plugin cannot crash the browser or access another plugin's data.
- [ ] **Plugin manager UI** — Install, enable, disable, and remove plugins from within the browser.

### Phase 4 — First-Party Plugins

Ship official plugins for common needs.

- [ ] **Cookie plugin** — Opt-in cookie storage and sending. Per-site controls.
- [ ] **JavaScript plugin** — Embed a JS engine. Off by default. Per-site controls.
- [ ] **Form submission plugin** — Allow forms to POST data. Confirmation prompt before every submission.
- [ ] **Bookmarks plugin** — Save and organize URLs locally.
- [ ] **History plugin** — Persist browsing history locally. Clearable.
- [ ] **Ad/tracker blocker plugin** — Filter list-based blocking (e.g. EasyList-compatible).

### Phase 5 — Hardening

- [ ] **Process isolation** — Each tab runs in a separate sandboxed process.
- [ ] **Content Security Policy enforcement** — Even without JS, honor CSP headers to inform rendering decisions.
- [ ] **Certificate pinning** — TOFU or configurable pinning for TLS certificates.
- [ ] **Reproducible builds** — Deterministic compilation so users can verify the binary matches the source.

---

## Design Principles

1. **Secure by default, capable by choice.** The browser does the minimum. Plugins add the rest.
2. **No silent network traffic.** Every request the browser makes should be visible and explainable.
3. **No persistent state without a plugin.** Close the browser, and it forgets everything.
4. **Readable codebase.** Favor clarity over cleverness. Keep dependencies minimal.
5. **User owns their data.** No telemetry, no analytics, no phoning home. Ever.

---

## Tech Stack (Planned)

| Component        | Approach                                                  |
|------------------|-----------------------------------------------------------|
| Language         | C (core engine), with potential for Rust in security-critical modules |
| GUI toolkit      | Platform-native (X11/Wayland on Linux, Win32 on Windows, Cocoa on macOS) |
| TLS              | Minimal vendored library or OS-native TLS                 |
| Text rendering   | FreeType + HarfBuzz (Linux), DirectWrite (Windows), Core Text (macOS) |
| Build system     | CMake                                                     |
| Plugin interface | Shared libraries loaded at runtime with a C ABI           |

---

## Building

> **Note:** There is nothing to build yet. This section will be updated as code is added.

```sh
git clone <repo-url> pane
cd pane
mkdir build && cd build
cmake ..
make
./pane
```

---

## Contributing

This project is in its earliest stages. If you're interested in helping build a browser from scratch, open an issue to discuss before submitting a PR.

---

## License

TBD — will be chosen before the first code release.

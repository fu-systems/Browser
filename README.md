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

- [ ] **Networking** — HTTP/HTTPS requests via `dart:io`. No cookie headers. No referrer headers. Minimal request footprint.
- [ ] **HTML parser** — Parse HTML5 into a DOM tree. Handle malformed markup gracefully (the real web is messy). Written in pure Dart, no Flutter dependency, to stay portable.
- [ ] **CSS parser** — Parse inline styles and `<style>` blocks. External stylesheets loaded via `<link>`. Pure Dart, portable.
- [ ] **Layout engine** — Block and inline layout. Box model (margin, padding, border). Basic text flow and wrapping. Pure Dart, portable.
- [ ] **Painting** — Render the laid-out page via Flutter `CustomPainter` / `Canvas`. Text rendering with system fonts. Background colors and borders.
- [ ] **Navigation** — Address bar, clickable links, back/forward history (in-memory only, not persisted).
- [ ] **Basic UI shell** — Flutter-based window with address bar, back/forward/reload buttons, tab bar.

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

- [ ] **Process isolation** — Each tab runs in a separate Dart isolate (or OS process if pivoted to C).
- [ ] **Content Security Policy enforcement** — Even without JS, honor CSP headers to inform rendering decisions.
- [ ] **Certificate pinning** — TOFU or configurable pinning for TLS certificates.
- [ ] **Reproducible builds** — Deterministic compilation so users can verify the binary matches the source.

### Long-Term — C Pivot (Contingency)

If Flutter becomes unsustainable, migrate the core engine to C + SDL2.

- [ ] **Port HTML/CSS parsers to C** — The Dart parsers are intentionally written without Flutter-specific APIs, making translation straightforward.
- [ ] **SDL2 windowing and input** — Replace Flutter's window/input layer with SDL2.
- [ ] **FreeType + HarfBuzz text rendering** — Replace Flutter's text shaping with direct FreeType/HarfBuzz calls.
- [ ] **C ABI plugin interface** — Replace Dart isolate-based plugins with shared libraries loaded at runtime.

---

## Design Principles

1. **Secure by default, capable by choice.** The browser does the minimum. Plugins add the rest.
2. **No silent network traffic.** Every request the browser makes should be visible and explainable.
3. **No persistent state without a plugin.** Close the browser, and it forgets everything.
4. **Readable codebase.** Favor clarity over cleverness. Keep dependencies minimal.
5. **User owns their data.** No telemetry, no analytics, no phoning home. Ever.

---

## Tech Stack

| Component        | Approach                                                  |
|------------------|-----------------------------------------------------------|
| Language         | Dart                                                      |
| Framework        | Flutter (cross-platform UI, canvas rendering, text shaping) |
| Networking       | `dart:io` HttpClient for HTTP/HTTPS requests              |
| Rendering        | Flutter `CustomPainter` / `Canvas` for page painting      |
| Plugin isolation | Dart `Isolate`s for sandboxed plugin execution             |
| Build system     | Flutter CLI (`flutter build`)                              |
| Targets          | Linux, Windows, macOS (mobile targets possible later)      |

> **Planned pivot:** The Flutter/Dart stack is chosen for development speed and built-in cross-platform support. If Flutter's long-term viability becomes uncertain (e.g. Google deprioritizes or abandons it), the project will pivot to **C with SDL2** for windowing, FreeType + HarfBuzz for text rendering, and a C ABI plugin interface. The parser, layout engine, and plugin architecture are being designed with this potential migration in mind — keeping logic decoupled from Flutter-specific APIs wherever practical.

---

## Building

> **Note:** There is nothing to build yet. This section will be updated as code is added.

```sh
git clone <repo-url> pane
cd pane
flutter pub get
flutter run -d linux    # or: -d windows, -d macos
```

---

## Contributing

This project is in its earliest stages. If you're interested in helping build a browser from scratch, open an issue to discuss before submitting a PR.

---

## License

TBD — will be chosen before the first code release.

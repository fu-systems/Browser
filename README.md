# Pane — A Minimal, Secure Web Browser

Pane is a from-scratch web browser built with one principle: **do nothing unless the user explicitly asks for it.**

No cookies. No JavaScript. No tracking. No telemetry. Just HTML rendered in a window. Everything beyond that is opt-in via plugins.

---

## Current Phase: C++ Rendering Engine (Phase 2)

The project is pivoting from the Phase 1 Dart/Flutter prototype to a **ground-up C++ rendering engine** built directly from the HTML and CSS specifications.

The approach:

1. **Catalog** every HTML element and CSS property from the standards
2. **Scan** an existing open-source engine (Servo) to empirically identify which pairwise element interactions matter
3. **Prune** the ~435,600 possible pairs down to the ~5,000 that have distinct behavior
4. **Build** the engine as a chain of **pairwise context transitions** — a single state machine that walks the DOM tree, where each parent-child pair is a well-defined transition function
5. **Validate** against real-world page sources and systematically classify edge cases
6. **Formalize** a decision engine for edge-case handling — the project's unique contribution to reduce engine complexity

See **[ROADMAP.md](ROADMAP.md)** for the full phase breakdown.

---

## Architecture

```
(parent_context, child_element) → child_context
```

The entire layout engine is this function, applied iteratively down the DOM tree. No child walks the ancestor chain. Deeper nesting is just a chain of pairwise transitions. The context object carries formatting context, containing block, stacking context, and inherited properties — everything a child needs to lay itself out.

---

## Design Principles

1. **Secure by default, capable by choice.** The browser does the minimum. Plugins add the rest.
2. **No silent network traffic.** Every request is visible and explainable.
3. **No persistent state without a plugin.** Close the browser, and it forgets everything.
4. **Readable codebase.** Favor clarity over cleverness. Keep dependencies minimal.
5. **User owns their data.** No telemetry, no analytics, no phoning home. Ever.

---

## Repository Structure

```
ROADMAP.md              Phase 2 roadmap and methodology
License                 Project license
archive/phase1-dart/    Complete Phase 1 Dart/Flutter implementation (preserved, not modified)
```

---

## License

TBD — will be chosen before the first code release.

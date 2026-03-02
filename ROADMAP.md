# Pane — Phase 2 Roadmap: C Standards-First Rendering Engine

## Overview

Phase 1 (archived) proved the concept: a minimal, secure browser shell in Dart/Flutter.

Phase 2 is a ground-up rewrite of the rendering engine in C, built directly from the HTML and CSS specifications. The goal is not to replicate existing engines — it is to build a cleaner one by understanding the full interaction space *before* writing layout code.

---

## Core Thesis

Browser engine complexity is not inherent — it is the result of ad-hoc patches accumulated over decades. A principled engine built on a complete understanding of element interactions can be significantly smaller and more correct.

The unique selling point: **systematic edge-case handling through pairwise context transitions**, reducing the total number of code lines and architectural complexity compared to existing engines.

---

## Architecture: Pairwise Context Transitions

The rendering engine is modeled as a single state machine walking the DOM tree. Every parent-child relationship is a **pairwise transition**:

```
(parent_context, child_element) → child_context
```

The context object carries all information a child needs to lay itself out. No child ever needs to walk up the ancestor chain. Deeper nesting is just a chain of these pairwise transitions applied iteratively.

```
context = {
    formatting_context   // block | inline | flex | grid | table
    containing_block     // dimensions + reference for percentage resolution
    stacking_context     // z-ordering reference
    inherited_properties // cascade-resolved inherited values
    ...fields TBD by scanner analysis
}
```

The critical validation: if this context object stabilizes at a bounded number of fields (expected: 20-30), the architecture holds. If it grows without bound, it degenerates into a hidden ancestor-chain lookup and must be revised.

---

## Phase 2 Stages

### Stage 1 — Standards Catalog

Enumerate every element and property from the current HTML and CSS specifications.

- [ ] Catalog all HTML elements (~110) with their content models, categories, and default styles
- [ ] Catalog all CSS properties (~550+) with their value types, initial values, inheritance, and applicable elements
- [ ] Record which properties create new formatting contexts, stacking contexts, or containing blocks
- [ ] Output: machine-readable catalog (JSON or similar) for use by the scanner and test generator

### Stage 2 — Pairwise Interaction Scanner

Scan an existing open-source engine (Servo preferred — clean-room Rust implementation, ~500K lines vs Chromium's ~35M) to empirically identify which pairwise interactions are handled and how.

- [ ] Fork/clone Servo source for analysis
- [ ] Build a scanner that walks Servo's layout code and extracts every conditional branch on element type or property combination
- [ ] Identify every point where layout looks up the ancestor chain (these are chain-sensitive cases)
- [ ] Map each finding to a pairwise interaction in the standards catalog

**Pruning pipeline:**

```
Full space:     ~435,600 pairs (660 × 660)
                        ↓ remove meaningless pairs (elements that can't nest, properties that don't interact)
Structural:     ~50,000
                        ↓ remove pairs with identical behavior
Behavioral:     ~5,000
                        ↓ flag chain-sensitive interactions
Pairwise-clean: ~4,700  (handled by context propagation)
Chain-flagged:  ~300    (need context enrichment analysis)
```

### Stage 3 — Chain-Sensitive Formalization

For each chain-flagged interaction, determine whether it can be absorbed into the pairwise model by enriching the context object.

- [ ] For each flagged interaction, ask: can a context field make this reducible to pairwise?
- [ ] If yes — add the field to the context spec
- [ ] If no — document it as a genuine architectural limitation and design a minimal handler
- [ ] Validate that the context object converges to a stable, bounded structure
- [ ] Output: finalized context object specification + list of irreducible exceptions (expected: very few)

### Stage 4 — Test Generation

Generate automated test cases from the pairwise interaction list and validate against existing engines.

- [ ] Write a test harness that generates DOM trees for each meaningful pair
- [ ] Render in headless Chromium, Firefox, and Safari (WebKit)
- [ ] Capture computed styles + layout geometry for each test
- [ ] Where all three engines agree → that is the spec behavior (reference output)
- [ ] Where they diverge → flag as an ambiguity requiring a principled decision
- [ ] Output: reference test suite with expected results

### Stage 5 — C Engine Implementation

Build the rendering engine in C using the pairwise transition model.

- [ ] HTML parser (standards-compliant tokenizer + tree builder)
- [ ] CSS parser (property recognition, value parsing, cascade resolution)
- [ ] Pairwise transition engine (the core: context propagation down the DOM tree)
- [ ] Formatting context implementations (block, inline, flex, grid, table)
- [ ] Box model + painting
- [ ] Integration with the Pane UI shell (replacing the Dart rendering layer)

### Stage 6 — Real-World Dataset Analysis

Gather a large dataset of real page sources and run the engine against them to find what the pairwise model misses.

- [ ] Collect page sources from top sites, diverse layouts, edge-case-heavy pages
- [ ] Render with the C engine and compare against reference engine output
- [ ] Classify failures: pairwise gap, chain-sensitive gap, parser bug, spec ambiguity
- [ ] Feed findings back into the pairwise interaction list and context object

### Stage 7 — Edge-Case Decision Engine

Formalize the systematic method for detecting and handling edge cases — this is the project's unique contribution.

- [ ] Define a taxonomy of edge-case origins (pairwise interaction, chain sensitivity, spec ambiguity, real-world markup quirks)
- [ ] Build a detection system that classifies which category a rendering failure falls into
- [ ] For each category, define a resolution strategy that avoids ad-hoc patches
- [ ] Measure: total engine LOC vs Servo/Blink for equivalent coverage
- [ ] Document the methodology as a reusable approach for engine development

---

## Design Principles (Unchanged from Phase 1)

1. **Secure by default, capable by choice.** The browser does the minimum. Plugins add the rest.
2. **No silent network traffic.** Every request is visible and explainable.
3. **No persistent state without a plugin.** Close the browser, and it forgets everything.
4. **Readable codebase.** Favor clarity over cleverness. Keep dependencies minimal.
5. **User owns their data.** No telemetry, no analytics, no phoning home. Ever.

---

## Tech Stack (Phase 2)

| Component         | Approach                                                       |
|-------------------|----------------------------------------------------------------|
| Engine language   | C (C17)                                                        |
| Build system      | CMake                                                          |
| HTML/CSS parsing  | Custom, built from spec                                        |
| Layout engine     | Pairwise context transition model                              |
| Text rendering    | FreeType + HarfBuzz                                            |
| Windowing         | SDL2 (or retained Flutter shell during transition)             |
| Test oracle       | Headless Chromium / Firefox / WebKit for reference comparison  |
| Scanner target    | Servo (Mozilla's Rust-based engine) for interaction analysis   |

---

## What's in the Archive

The `archive/phase1-dart/` directory contains the complete Phase 1 Dart/Flutter implementation — HTML/CSS parsers, layout engine, plugin system, QuickJS integration, and test suites. This code is preserved as-is for reference. It is not being carried forward into Phase 2.

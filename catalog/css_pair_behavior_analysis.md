# CSS Property Pair Behavior Analysis

Analysis of `css_property_pairs_full.md` to extract compressible patterns,
behavioral equivalence classes, and the minimal rule set that covers all
169,744 property×property pairs.

---

## Finding 1: INDEPENDENT Dominates Everything

**94.8% of all pairs are INDEPENDENT — no interaction whatsoever.**

| Interaction Type | Count | % | Description |
|-----------------|-------|---|-------------|
| INDEPENDENT | 160,865 | 94.77% | No interaction. Both apply independently. |
| SAME_MODULE | 5,729 | 3.37% | Same CSS module, contribute to same visual feature |
| ALL_RESETS | 819 | 0.48% | `all` shorthand resets the property |
| SAME_SHORTHAND | 715 | 0.42% | Both are longhands of a common shorthand |
| SHORTHAND_SETS | 489 | 0.29% | One is a shorthand that sets the other |
| SELF | 413 | 0.24% | Same property paired with itself |
| PARENT_CHILD_CONTEXT | 217 | 0.13% | Container property ↔ item property |
| INTERACTION | 217 | 0.13% | Specific known conflict/dependency |
| DISPLAY_DEPENDENT | 87 | 0.05% | One only applies under certain display values |
| LOGICAL_PHYSICAL | 81 | 0.05% | Logical↔physical mapping to same computed value |
| AXIS_REMAP | 81 | 0.05% | writing-mode changes which axis a logical prop maps to |
| SAME_AXIS | 37 | 0.02% | Both affect the same dimensional axis |
| ALL_EXEMPT | 5 | <0.01% | `all` does NOT reset (direction, unicode-bidi) |
| ALIAS | 3 | <0.01% | Legacy alias (word-wrap = overflow-wrap) |

**Non-trivial pairs (excluding INDEPENDENT + SELF): 8,466 (5.0%)**

This means 95% of the pair matrix can be covered by a single DEFAULT rule.
The remaining 5% is highly structured and compressible.

---

## Finding 2: Structural Rules Cover Most Non-Trivial Pairs

Seven interaction types are entirely determined by structural relationships
between properties (shorthand/longhand, logical/physical, module membership).
No per-pair knowledge is needed — just reference tables.

### 2a. SELF (412 pairs)

Every property paired with itself. One rule: "last declaration wins."

### 2b. ALL_RESETS (819 pairs)

The `all` shorthand resets every property except `direction` and
`unicode-bidi`. This generates 2 × (412 − 3) = 818 pairs (bidirectional,
excluding `all` × `all` = SELF).

One rule + two exceptions.

### 2c. SHORTHAND_SETS (489 pairs)

74 shorthands, each setting 2-12 longhands. Each shorthand×longhand pair
(in both directions) is SHORTHAND_SETS. Fully determined by the shorthand
→ longhand mapping table.

**Top shorthands by longhand count:**

| Shorthand | # Longhands |
|-----------|-------------|
| `border` | 12 |
| `animation` | 8 |
| `background` | 8 |
| `mask` | 8 |
| `font` | 7 |
| `font-variant` | 6 |
| `border-block` | 6 |
| `border-inline` | 6 |
| `grid` | 6 |
| `mask-border` | 6 |
| `offset` | 5 |
| `border-image` | 5 |

### 2d. SAME_SHORTHAND (715 pairs)

When two longhands share a common shorthand, they're SAME_SHORTHAND.
For a shorthand with N longhands, this generates N×(N-1) pairs.
Fully determined by shorthand membership.

### 2e. LOGICAL_PHYSICAL (81 pairs)

40 logical→physical property mappings. Each generates 2 pairs (bidirectional)
plus 1 SELF if the mapping is reflexive. Fully determined by the mapping table.

**Mapping pattern:** Every logical property has exactly one physical counterpart
in default writing mode (horizontal-tb, ltr):

| Logical | Physical |
|---------|----------|
| `block-size` | `height` |
| `inline-size` | `width` |
| `margin-block-start` | `margin-top` |
| `margin-block-end` | `margin-bottom` |
| `margin-inline-start` | `margin-left` |
| `margin-inline-end` | `margin-right` |
| `padding-block-*` | `padding-top/bottom` |
| `padding-inline-*` | `padding-left/right` |
| `border-block-*-color/style/width` | `border-top/bottom-*` |
| `border-inline-*-color/style/width` | `border-left/right-*` |
| `inset-block-start/end` | `top` / `bottom` |
| `inset-inline-start/end` | `left` / `right` |
| `max/min-block-size` | `max/min-height` |
| `max/min-inline-size` | `max/min-width` |
| `overflow-block/inline` | `overflow-y/x` |
| *(40 total mappings)* | |

### 2f. AXIS_REMAP (81 pairs)

Every logical property × `writing-mode` = AXIS_REMAP. One rule: "all
logical properties are remapped by writing-mode." Count equals the number
of logical properties (40-ish, bidirectional with writing-mode).

### 2g. SAME_MODULE (5,729 pairs)

Properties within the same CSS module. 45 modules total, generating
pairs proportional to module size². Fully determined by module membership.

**Top modules by pair generation:**

| Module | Properties | Pairs (N×(N-1)) |
|--------|-----------|-----------------|
| CSS Logical Properties | 53 | 2,756 |
| CSS Backgrounds | 41 | 1,640 |
| CSS Scroll Snap | 25 | 600 |

---

## Finding 3: INTERACTION Pairs Group Into 7 Categories

The 108 unique INTERACTION pairs (217 total, counting both directions) fall
into well-defined behavioral categories:

### Category A: Stacking Context Creators (9 unique pairs)

Properties that establish a stacking context, making `z-index` meaningful:

```
backdrop-filter × z-index
contain         × z-index
filter          × z-index
isolation       × z-index      (also: isolation × mix-blend-mode)
mix-blend-mode  × z-index
opacity         × z-index
transform       × z-index
will-change     × z-index
```

**Rule:** The set `{backdrop-filter, contain, filter, isolation, mix-blend-mode,
opacity, transform, will-change}` creates stacking contexts when active,
making `z-index` apply regardless of `position`.

### Category B: Containing Block Creators (7 unique pairs)

Properties that create containing blocks for positioned descendants:

```
contain   × position
filter    × position
overflow  × position        (doesn't clip position:fixed)
overflow  × transform       (transform creates CB, affects clipping)
overflow  × will-change     (will-change:transform creates CB)
position  × transform       (transform creates CB for fixed descendants)
position  × will-change
```

**Rule:** The set `{transform, filter, contain, will-change}` creates
containing blocks, capturing `position:fixed` descendants.

### Category C: Forces transform-style:flat (7 unique pairs)

Properties that break `transform-style: preserve-3d`:

```
clip-path       × transform-style
contain         × transform-style
filter          × transform-style
isolation       × transform-style
mix-blend-mode  × transform-style
opacity         × transform-style
overflow        × transform-style
```

**Rule:** The set `{clip-path, contain, filter, isolation, mix-blend-mode,
opacity, overflow}` forces `transform-style: flat` when active.

### Category D: Display-Gated Properties (34 unique pairs)

Properties that only apply under certain `display` values. Plus some
non-display gating (e.g., `position` gating `top/right/bottom/left`).

This overlaps heavily with DISPLAY_DEPENDENT (87 pairs, covering 43
properties).

**Rule:** 43 properties only apply under specific display modes (flex items,
grid items, table elements, block containers, etc.).

### Category E: Sizing Constraint Resolution (8 unique pairs)

The width/height constraint resolution algorithm:

```
height × max-height    →  max-height wins when max < height
height × min-height    →  min-height wins when min > height
max-height × min-height → min-height wins when min > max
width  × max-width     →  max-width wins when max < width
width  × min-width     →  min-width wins when min > width
max-width × min-width  →  min-width wins when min > max
aspect-ratio × height  →  ignored if both width+height definite
aspect-ratio × width   →  ignored if both width+height definite
```

**Rule:** `effective = max(min, min(max, value))`. Plus: `aspect-ratio`
resolves the missing dimension, ignored when both are definite.

### Category F: Overflow / Clipping Interactions (7 unique pairs)

```
clip-path × overflow            →  intersection of clips
contain × overflow              →  contain:paint acts like overflow:clip
overflow × resize               →  resize requires overflow ≠ visible
overflow × text-overflow        →  text-overflow requires overflow:hidden/clip
overflow-wrap × white-space     →  white-space:nowrap prevents wrapping
hyphens × overflow-wrap         →  both allow breaking, hyphens adds chars
text-overflow × white-space     →  text-overflow needs nowrap
```

### Category G: Specific Property Interactions (~36 unique pairs)

These don't group neatly — each is a specific semantic relationship:

- **box-sizing × {width, height, border, padding}** — border-box includes
  border+padding in width/height
- **float × {position, margin, width, shape-outside}** — float behavior
  modifications
- **position × {top, z-index, margin, order, flex-grow, grid-row}** — position
  value gates many properties
- **contain × {container-type, content-visibility, counter-reset, float}** —
  containment side effects
- **writing-mode × {direction, text-orientation, text-combine-upright}** —
  writing mode interactions
- **Animation/transition conflicts** — animations win over transitions
- **Scroll snap relationships** — snap-type gates snap-align/margin/padding
- **Shape relationships** — shape-outside gates shape-margin, shape-threshold
- **Text decoration relationships** — text-decoration-line gates other
  text-decoration-* properties

---

## Finding 4: Property Classification

### By Interaction Profile

Properties can be classified by how many non-INDEPENDENT pairs they have:

**High-interaction properties (10+ interactions):**

| Property | Non-INDEPENDENT pairs | Key role |
|----------|----------------------|----------|
| `display` | ~60+ | Gates 43+ properties, interacts with layout |
| `all` | 411+ | Resets everything |
| `position` | ~20+ | Gates z-index, top/right/bottom/left, interacts with float/contain |
| `contain` | ~15+ | Creates CB, stacking context, forces flat, gates overflow |
| `overflow` | ~12+ | Interacts with resize, text-overflow, position, transform-style |
| `float` | ~10+ | Interacts with display, position, margin, shape, width, clear |
| `writing-mode` | ~10+ | Remaps all logical properties, interacts with text-orientation |
| `transform` | ~8+ | Creates CB, stacking context, interacts with perspective |
| `z-index` | ~8+ | Gated by stacking context creators |
| `transform-style` | ~7+ | Forced flat by 7 properties |

**Zero-interaction properties (INDEPENDENT with everything except SELF + all):**

The vast majority of properties (~300+) only interact with `all` (ALL_RESETS),
their own shorthand (SHORTHAND_SETS), and their module peers (SAME_MODULE).
They have no INTERACTION, DISPLAY_DEPENDENT, or PARENT_CHILD_CONTEXT pairs.

---

## Finding 5: Compression Ratio

### The Naive Model: 169,744 rules

412 properties × 412 properties = 169,744 unique pair rules.

### The Compressed Model: ~30 rules + reference tables

| Rule Type | # Rules | Pairs Covered |
|-----------|---------|---------------|
| DEFAULT: INDEPENDENT | 1 | 160,865 (94.8%) |
| SELF | 1 | 412 (0.2%) |
| ALL_RESETS + ALL_EXEMPT | 1 | 824 (0.5%) |
| SHORTHAND_SETS (table of 74 shorthands) | 1 | 489 (0.3%) |
| SAME_SHORTHAND (derived from shorthand table) | 1 | 715 (0.4%) |
| LOGICAL_PHYSICAL (table of 40 mappings) | 1 | 81 (0.05%) |
| AXIS_REMAP | 1 | 81 (0.05%) |
| SAME_MODULE (table of 45 modules) | 1 | 5,729 (3.4%) |
| SAME_AXIS | 1 | 37 (0.02%) |
| DISPLAY_DEPENDENT (set of 43 properties) | 1 | 87 (0.05%) |
| PARENT_CHILD_CONTEXT: flex | 1 | ~108 |
| PARENT_CHILD_CONTEXT: grid | 1 | ~109 |
| ALIAS | 1 | 3 |
| Stacking context creators | 1 | ~18 |
| Containing block creators | 1 | ~14 |
| transform-style:flat forcers | 1 | ~14 |
| Sizing constraint resolution | 1 | ~16 |
| Overflow/clipping interactions | ~3 | ~14 |
| Specific property interactions | ~12 | ~72 |
| **Total** | **~30** | **169,744** |

**Result: ~30 rules + 4 reference tables cover all 169,744 pairs — a 5,658× compression.**

The 4 reference tables are:
1. Shorthand → longhand mappings (74 entries)
2. Logical → physical mappings (40 entries)
3. Module membership (45 modules, 412 properties)
4. Display-dependent property set (43 properties)

---

## Finding 6: Resolution Priority

When a pair matches multiple rules, the most specific rule applies.
Priority order (highest first):

1. **SELF** — always `A × A`
2. **ALIAS** — `overflow-wrap × word-wrap`
3. **SHORTHAND_SETS** — shorthand sets its longhand
4. **SAME_SHORTHAND** — longhands share a shorthand
5. **LOGICAL_PHYSICAL** — logical maps to physical counterpart
6. **INTERACTION** — specific known conflict
7. **DISPLAY_DEPENDENT** — gated by display value
8. **PARENT_CHILD_CONTEXT** — container/item relationship
9. **AXIS_REMAP** — writing-mode remaps logical property
10. **SAME_AXIS** — same dimensional axis
11. **SAME_MODULE** — same CSS module
12. **ALL_RESETS / ALL_EXEMPT** — `all` shorthand behavior
13. **INDEPENDENT** — default, no interaction

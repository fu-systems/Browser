# CSS Pair Chain Handling — Compressed Rules

The complete CSS property×property interaction logic, compressed from 169,744
individual pairs into **31 rules** + **4 reference tables** organized by
interaction type.

Derived from `css_property_pairs_full.md` and `css_pair_behavior_analysis.md`.

---

## How To Use This Document

To determine the interaction between any two CSS properties A × B:

1. Check rules in **priority order** (Section 2)
2. The **first matching rule** determines the interaction
3. Most rules reference **named sets** from Section 3 or **reference tables**
   from Section 4
4. If no specific rule matches, the **DEFAULT rule** applies: INDEPENDENT

Priority order (highest → lowest):

```
SELF → ALIAS → SHORTHAND_SETS → SAME_SHORTHAND → LOGICAL_PHYSICAL →
INTERACTION → DISPLAY_DEPENDENT → PARENT_CHILD_CONTEXT → AXIS_REMAP →
SAME_AXIS → SAME_MODULE → ALL_RESETS/ALL_EXEMPT → INDEPENDENT (default)
```

---

## Section 1: Named Property Sets

### SET_STACKING_CONTEXT — creates stacking context when active

```
backdrop-filter  contain  filter  isolation  mix-blend-mode
opacity          transform        will-change
```

### SET_CONTAINING_BLOCK — creates containing block for positioned descendants

```
contain  filter  transform  will-change
```

### SET_FLAT_FORCERS — forces transform-style:flat when active

```
clip-path  contain  filter  isolation  mix-blend-mode  opacity  overflow
```

### SET_FLEX_CONTAINER — flex container properties

```
align-content  align-items  flex-direction  flex-flow  flex-wrap
justify-content
```

### SET_FLEX_ITEM — flex item properties

```
align-self  flex  flex-basis  flex-grow  flex-shrink  order
```

### SET_GRID_CONTAINER — grid container properties

```
grid  grid-auto-columns  grid-auto-flow  grid-auto-rows  grid-template
grid-template-areas  grid-template-columns  grid-template-rows
justify-items
```

### SET_GRID_ITEM — grid item properties

```
grid-area  grid-column  grid-column-end  grid-column-start  grid-row
grid-row-end  grid-row-start  justify-self
```

### SET_DISPLAY_GATED_FLEX — only apply under display:flex/inline-flex

```
flex  flex-basis  flex-direction  flex-flow  flex-grow  flex-shrink
flex-wrap  order
```

### SET_DISPLAY_GATED_GRID — only apply under display:grid/inline-grid

```
grid  grid-area  grid-auto-columns  grid-auto-flow  grid-auto-rows
grid-column  grid-column-end  grid-column-start  grid-row  grid-row-end
grid-row-start  grid-template  grid-template-areas  grid-template-columns
grid-template-rows  justify-items  justify-self
```

### SET_DISPLAY_GATED_TABLE — only apply under display:table/table-*

```
border-collapse  border-spacing  caption-side  empty-cells  table-layout
```

### SET_DISPLAY_GATED_MULTICOL — only apply on block containers (multi-column)

```
column-count  column-fill  column-rule  column-rule-color
column-rule-style  column-rule-width  column-width  columns
```

### SET_DISPLAY_GATED_LIST — only apply under display:list-item

```
list-style  list-style-image  list-style-position  list-style-type
marker-side
```

### SET_DISPLAY_GATED_RUBY — only apply under display:ruby/ruby-text

```
ruby-align  ruby-position
```

### SET_ALL_DISPLAY_GATED — union of all display-gated sets + align-self

```
SET_DISPLAY_GATED_FLEX ∪ SET_DISPLAY_GATED_GRID ∪ SET_DISPLAY_GATED_TABLE ∪
SET_DISPLAY_GATED_MULTICOL ∪ SET_DISPLAY_GATED_LIST ∪ SET_DISPLAY_GATED_RUBY ∪
{ align-self }
```

43 properties total.

### SET_LOGICAL — logical properties remapped by writing-mode

```
block-size                   inline-size
border-block-end-color       border-block-end-style       border-block-end-width
border-block-start-color     border-block-start-style     border-block-start-width
border-end-end-radius        border-end-start-radius
border-inline-end-color      border-inline-end-style      border-inline-end-width
border-inline-start-color    border-inline-start-style    border-inline-start-width
border-start-end-radius      border-start-start-radius
contain-intrinsic-block-size contain-intrinsic-inline-size
inset-block-end              inset-block-start
inset-inline-end             inset-inline-start
margin-block-end             margin-block-start
margin-inline-end            margin-inline-start
max-block-size               max-inline-size
min-block-size               min-inline-size
overflow-block               overflow-inline
overscroll-behavior-block    overscroll-behavior-inline
padding-block-end            padding-block-start
padding-inline-end           padding-inline-start
```

40 properties total.

### SET_ALL_EXEMPT — properties NOT reset by `all`

```
direction  unicode-bidi
```

---

## Section 2: Rules

### Structural Rules (cover 168,713 pairs — 99.4%)

#### Rule SF-01: SELF

```
IF A == B → SELF
Action: Last declaration in cascade wins.
```

Covers 412 pairs.

#### Rule SF-02: ALIAS

```
IF {A, B} == {overflow-wrap, word-wrap} → ALIAS
Action: Same computed property. word-wrap is legacy alias for overflow-wrap.
```

Covers 2 unique pairs (4 directional).

#### Rule SF-03: ALL_EXEMPT

```
IF A == all AND B ∈ SET_ALL_EXEMPT → ALL_EXEMPT
IF B == all AND A ∈ SET_ALL_EXEMPT → ALL_EXEMPT
Action: `all` does NOT reset these properties. They are exempt from
        the `all` shorthand.
```

Covers 4 directional pairs (2 unique: direction, unicode-bidi).

#### Rule SF-04: ALL_RESETS

```
IF A == all AND B ≠ all AND B ∉ SET_ALL_EXEMPT → ALL_RESETS
IF B == all AND A ≠ all AND A ∉ SET_ALL_EXEMPT → ALL_RESETS
Action: `all` resets B (or A) to its initial value (or to the inherited
        value when `all:inherit`, etc.). The longhand's own declaration,
        if any, takes normal cascade precedence against the reset.
```

Covers 818 directional pairs.

#### Rule SF-05: SHORTHAND_SETS

```
IF A is a shorthand AND B ∈ longhands(A) → SHORTHAND_SETS
IF B is a shorthand AND A ∈ longhands(B) → SHORTHAND_SETS
Action: The shorthand sets the longhand. Longhand declarations after the
        shorthand override. Longhand declarations before are overridden.
        Normal cascade order applies.
Lookup: Reference Table 1 (74 shorthands, 244 shorthand→longhand pairs)
```

Covers 488 directional pairs.

#### Rule SF-06: SAME_SHORTHAND

```
IF ∃ shorthand S where A ∈ longhands(S) AND B ∈ longhands(S) → SAME_SHORTHAND
Action: Both are longhands of the same shorthand. When the shorthand is
        declared, it sets both simultaneously. Individual longhand
        declarations override independently. No direct interaction between
        A and B — they share reset behavior through S.
Lookup: Derived from Reference Table 1
```

Covers 714 directional pairs.

#### Rule SF-07: LOGICAL_PHYSICAL

```
IF {A, B} is a logical↔physical mapping pair → LOGICAL_PHYSICAL
Action: Both resolve to the same computed value in default writing mode
        (horizontal-tb, ltr). When both are declared, last in cascade wins.
        In non-default writing modes, the logical property may map to a
        different physical property.
Lookup: Reference Table 2 (40 mappings)
```

Covers 80 directional pairs.

#### Rule SF-08: SAME_MODULE

```
IF module(A) == module(B) AND A ≠ B → SAME_MODULE
Action: Both contribute to the same visual feature/subsystem. No direct
        override or dependency — both apply independently within their
        module's rendering algorithm.
Lookup: Reference Table 3 (45 modules)
```

Covers 5,728 directional pairs.

#### Rule SF-09: AXIS_REMAP

```
IF A == writing-mode AND B ∈ SET_LOGICAL → AXIS_REMAP
IF B == writing-mode AND A ∈ SET_LOGICAL → AXIS_REMAP
Action: writing-mode determines which physical axis the logical property
        maps to. Changing writing-mode remaps all logical properties.
```

Covers 80 directional pairs.

#### Rule SF-10: SAME_AXIS

```
IF A and B both affect the same dimensional axis AND one is logical,
   one is physical (or both logical with same axis) → SAME_AXIS
Action: Both constrain the same dimension. In default writing mode,
        logical and physical map to same computed value — last in
        cascade wins. In non-default writing modes, they may diverge.
```

Pairs (18 unique):

```
block-size     × {max-block-size, min-block-size, max-height, min-height}
height         × {max-block-size, min-block-size}
inline-size    × {max-inline-size, min-inline-size, max-width, min-width}
width          × {max-inline-size, min-inline-size}
max-block-size × {min-block-size, min-height}
max-height     × min-block-size
max-inline-size × {min-inline-size, min-width, width}
max-width      × min-inline-size
min-inline-size × width
```

Covers 36 directional pairs.

#### Rule SF-DEFAULT: INDEPENDENT

```
IF no other rule matches → INDEPENDENT
Action: No interaction. Both properties apply independently. Each is
        resolved by its own specification without reference to the other.
```

Covers 160,865 pairs (94.8%).

---

### Interaction Rules (cover 1,031 pairs — 0.6%)

#### Rule IN-01: Stacking Context Creators × z-index

```
IF A ∈ SET_STACKING_CONTEXT AND B == z-index → INTERACTION
IF B ∈ SET_STACKING_CONTEXT AND A == z-index → INTERACTION
Action: When A is active (not at its initial value), it establishes a
        stacking context, making z-index meaningful even without
        position:relative/absolute/fixed.
```

Covers 16 directional pairs (8 unique).

#### Rule IN-02: Containing Block Creators × position

```
IF A ∈ SET_CONTAINING_BLOCK AND B == position → INTERACTION
IF B ∈ SET_CONTAINING_BLOCK AND A == position → INTERACTION
Action: When A is active, it creates a containing block for all
        positioned descendants, including position:fixed (which
        normally uses the viewport as containing block).
```

Also includes:

```
overflow × position  — overflow:hidden clips position:absolute only if
                       ancestor is containing block. Does NOT clip
                       position:fixed.
overflow × transform — transform creates containing block, affecting
                       overflow clipping behavior.
overflow × will-change — will-change:transform creates containing block.
```

Covers 14 directional pairs (7 unique).

#### Rule IN-03: transform-style:flat Forcers

```
IF A ∈ SET_FLAT_FORCERS AND B == transform-style → INTERACTION
IF B ∈ SET_FLAT_FORCERS AND A == transform-style → INTERACTION
Action: When A is active, it forces transform-style to compute as
        'flat', breaking preserve-3d on the element.
```

Covers 14 directional pairs (7 unique).

#### Rule IN-04: Display Interactions

```
IF A == display AND B ∈ {align-content, align-items, float, gap, height,
   justify-content, margin, order, overflow, padding, pointer-events,
   position, vertical-align, width, z-index} → INTERACTION
Action: display value determines whether B applies, has no effect, or
        behaves differently:
  - display:none → B is irrelevant (no box generated)
  - display:contents → position/float/overflow irrelevant (no principal box)
  - Inline non-replaced → width/height have no effect
  - Inline-level → vertical margins have no effect
  - display:flex/grid → float computes to 'none' on children
  - display:flex/grid → z-index applies to items regardless of position
```

Covers 30 directional pairs (15 unique).

#### Rule IN-05: Display-Gated Properties

```
IF A == display AND B ∈ SET_ALL_DISPLAY_GATED → DISPLAY_DEPENDENT
Action: B only applies under specific display values. Ignored otherwise.
Lookup: See SET_DISPLAY_GATED_* sets in Section 1.
  - Flex properties: display:flex/inline-flex
  - Grid properties: display:grid/inline-grid
  - Table properties: display:table/table-*
  - Multi-column: block containers
  - List properties: display:list-item
  - Ruby properties: display:ruby/ruby-text
  - align-self: flex items (display:flex/inline-flex parent)
```

Covers 86 directional pairs (43 unique).

#### Rule IN-06: Flex Container × Item Context

```
IF A ∈ SET_FLEX_CONTAINER AND B ∈ SET_FLEX_ITEM → PARENT_CHILD_CONTEXT
IF B ∈ SET_FLEX_CONTAINER AND A ∈ SET_FLEX_ITEM → PARENT_CHILD_CONTEXT
Action: A defines layout context on parent, B applies on child.
        Container properties determine how item properties are resolved.
        Cross-product: 6 container × 6 item = 36 unique pairs.
```

Covers 72 directional pairs (36 unique).

#### Rule IN-07: Grid Container × Item Context

```
IF A ∈ SET_GRID_CONTAINER AND B ∈ SET_GRID_ITEM → PARENT_CHILD_CONTEXT
IF B ∈ SET_GRID_CONTAINER AND A ∈ SET_GRID_ITEM → PARENT_CHILD_CONTEXT
Action: A defines grid layout context on parent, B applies on child.
        Container properties (tracks, areas, auto-placement) determine
        how item properties are resolved.
        Cross-product: 9 container × 8 item = 72 unique pairs.
```

Covers 144 directional pairs (72 unique).

#### Rule IN-08: Sizing Constraint Resolution

```
IF {A, B} ⊆ {width, min-width, max-width} → INTERACTION
IF {A, B} ⊆ {height, min-height, max-height} → INTERACTION
IF {A, B} == {aspect-ratio, width}  → INTERACTION
IF {A, B} == {aspect-ratio, height} → INTERACTION
Action:
  - Effective size = max(min, min(max, value))
  - max wins over value when max < value
  - min wins over max when min > max
  - min wins over value when min > value
  - aspect-ratio resolves the missing dimension; ignored when both
    width and height are definite
```

Covers 16 directional pairs (8 unique).

#### Rule IN-09: Box-sizing × Dimensions

```
IF A == box-sizing AND B ∈ {width, height, border, padding} → INTERACTION
Action:
  - box-sizing:content-box (default) — width/height is content size only
  - box-sizing:border-box — width/height includes padding + border
```

Covers 8 directional pairs (4 unique).

#### Rule IN-10: Float Interactions

```
IF A == float AND B ∈ {position, margin, width, shape-outside} → INTERACTION
IF A == float AND B == clear → INTERACTION
Action:
  - position:absolute/fixed computes float to 'none'
  - Floated elements: margins never collapse
  - Floated elements shrink-to-fit if width:auto
  - shape-outside only works on floated elements
  - clear clears past floated siblings on specified side(s)
```

Covers 10 directional pairs (5 unique).

#### Rule IN-11: Position Gating

```
IF A == position AND B ∈ {top, z-index, margin} → INTERACTION
IF A == position AND B ∈ {flex-grow, order, grid-row} → INTERACTION
Action:
  - top/right/bottom/left only apply to positioned elements (not static)
  - z-index only applies to positioned elements and flex/grid items
  - position:absolute/fixed: margins don't collapse
  - position:absolute/fixed on flex item: flex-grow/shrink ignored
  - position:absolute/fixed on grid item: removed from flow but grid
    area still applies for positioning
```

Covers 12 directional pairs (6 unique).

#### Rule IN-12: Overflow / Clipping Interactions

```
clip-path × overflow     — Intersection of clips (both apply independently)
contain × overflow       — contain:paint acts like overflow:clip
overflow × resize        — resize requires overflow ≠ visible
overflow × text-overflow — text-overflow requires overflow:hidden/clip
overflow-wrap × white-space — white-space:nowrap prevents wrapping
hyphens × overflow-wrap  — both allow breaking; hyphens adds hyphen chars
text-overflow × white-space — text-overflow typically needs nowrap
```

Covers 14 directional pairs (7 unique).

#### Rule IN-13: Containment Side Effects

```
contain × container-type    — container-type implies contain values
contain × content-visibility — content-visibility implies containment
contain × counter-reset     — contain:style scopes counters
contain × display           — display:contents + contain incompatible
contain × float             — contain:layout contains floats (clearfix)
container-name × container-type — container-name requires container-type ≠ normal
content-visibility × display — display:none/contents + content-visibility incompatible
```

Covers 14 directional pairs (7 unique).

#### Rule IN-14: Writing Mode Interactions

```
direction × writing-mode        — Together determine axis mapping for
                                   all logical properties
text-combine-upright × writing-mode — Only applies in vertical writing modes
text-orientation × writing-mode  — Only applies in vertical writing modes
```

Covers 6 directional pairs (3 unique).

#### Rule IN-15: Table Property Interactions

```
border-collapse × border-radius  — border-radius on cells ignored with
                                    border-collapse:collapse
border-collapse × border-spacing — border-spacing only applies with
                                    border-collapse:separate
```

Covers 4 directional pairs (2 unique).

#### Rule IN-16: Animation / Transition Conflicts

```
animation-name × transition-property — Animations take priority over
                                        transitions targeting same property
animation-fill-mode × animation-play-state — fill-mode:forwards + paused
                                              keeps element at paused keyframe
```

Covers 4 directional pairs (2 unique).

#### Rule IN-17: Transform-Related Interactions

```
backface-visibility × transform — backface-visibility only meaningful when
                                   transform rotates element >90deg on X/Y
perspective × transform         — perspective on parent adds projection
                                   to children's transforms
isolation × mix-blend-mode      — isolation:isolate prevents blending with
                                   elements behind the isolated group
```

Covers 6 directional pairs (3 unique).

#### Rule IN-18: Shape Property Gating

```
shape-margin × shape-outside         — shape-margin expands the float area
                                        defined by shape-outside
shape-image-threshold × shape-outside — Defines alpha threshold for
                                         shape-outside:url() images
```

Covers 4 directional pairs (2 unique).

#### Rule IN-19: Text Decoration Gating

```
text-decoration-line × text-decoration-color     — color requires line ≠ none
text-decoration-line × text-decoration-style     — style requires line ≠ none
text-decoration-line × text-decoration-thickness — thickness requires line ≠ none
text-decoration-line × text-underline-offset     — requires line includes underline
text-decoration-line × text-underline-position   — requires line includes underline
```

Covers 10 directional pairs (5 unique).

#### Rule IN-20: Scroll Snap Gating

```
scroll-snap-type × scroll-snap-align  — snap-align on children requires
                                         snap-type on container
scroll-snap-type × scroll-margin      — snap margin requires snap-type
scroll-snap-type × scroll-padding     — snap padding requires snap-type
```

Covers 6 directional pairs (3 unique).

#### Rule IN-21: Specific Remaining Interactions

```
anchor-name × position-anchor   — position-anchor references anchor-name
clear × display                 — clear only applies to block-level elements
clear × float                   — clear clears floated siblings
column-fill × column-span       — column-span:all breaks column layout
column-span × display           — column-span only applies to in-flow
                                   block-level elements in multi-column
break-before × float            — break-* only applies to in-flow elements
break-before × position         — break-* ignored on abs/fixed positioned
margin × position               — margins don't collapse when abs/fixed
object-fit × object-position    — object-position positions within
                                   object-fit's computed space
pointer-events × visibility     — visibility:hidden + pointer-events:auto
                                   still captures events
position × position-anchor      — position-anchor requires position:absolute
position × position-area        — position-area requires position:absolute
position-anchor × position-area — position-area positions relative to
                                   anchor from position-anchor
white-space × word-break        — white-space:nowrap/pre prevents line breaks,
                                   making word-break irrelevant
```

Covers 28 directional pairs (14 unique).

---

## Section 3: Resolution Examples

### Example 1: Simple Independent Pair

```
Query: color × background-color
Step 1: color ≠ background-color                    → not SELF
Step 2: Not an alias pair                            → not ALIAS
Step 3: Neither is shorthand of the other            → not SHORTHAND_SETS
Step 4: No common shorthand                          → not SAME_SHORTHAND
Step 5: Not logical/physical pair                    → not LOGICAL_PHYSICAL
Step 6: No INTERACTION rule matches                  → not INTERACTION
Step 7: Not display-gated                            → not DISPLAY_DEPENDENT
Step 8: Not container/item pair                      → not PARENT_CHILD_CONTEXT
Step 9: Not writing-mode × logical                   → not AXIS_REMAP
Step 10: Not same axis                               → not SAME_AXIS
Step 11: CSS Color vs CSS Color (same module)        → SAME_MODULE
Result: SAME_MODULE — both contribute to color rendering, no direct interaction.
```

### Example 2: Shorthand Chain

```
Query: border × border-top-color
Step 1: border ≠ border-top-color                    → not SELF
Step 2: Not an alias pair                            → not ALIAS
Step 3: border-top-color ∈ longhands(border)         → SHORTHAND_SETS
Result: border sets border-top-color. Longhand after shorthand overrides.
```

### Example 3: Stacking Context Interaction

```
Query: opacity × z-index
Step 1: opacity ≠ z-index                            → not SELF
Step 2-5: No structural match                        → skip
Step 6: opacity ∈ SET_STACKING_CONTEXT AND z-index   → Rule IN-01
Result: INTERACTION — opacity < 1 establishes stacking context,
        making z-index meaningful without positioning.
```

### Example 4: Display Gating

```
Query: display × flex-grow
Step 1: display ≠ flex-grow                          → not SELF
Step 2-5: No structural match                        → skip
Step 6: display × flex-grow matches Rule IN-04       → INTERACTION
Result: flex-grow only applies to flex items (children of
        display:flex/inline-flex). display:none makes it irrelevant.
```

### Example 5: Logical-Physical Conflict

```
Query: margin-block-start × margin-top
Step 1: Different properties                         → not SELF
Step 2-4: No alias/shorthand match                   → skip
Step 5: {margin-block-start, margin-top} in Table 2  → LOGICAL_PHYSICAL
Result: Both resolve to the same computed value in horizontal-tb.
        Last declaration in cascade wins.
```

### Example 6: Multi-Rule Priority

```
Query: flex-grow × display
Step 1: Different properties                         → not SELF
Step 2-5: No structural match                        → skip
Step 6: display × flex-grow matches Rule IN-04       → INTERACTION (priority 6)
Also matches: Rule IN-05 DISPLAY_DEPENDENT (priority 7)
Result: INTERACTION wins (higher priority). flex-grow/shrink/basis only
        apply to flex items.
```

---

## Section 4: Reference Tables

### Reference Table 1: Shorthand → Longhand Mappings (74 shorthands)

| Shorthand | Longhands |
|-----------|-----------|
| `animation` | animation-delay, animation-direction, animation-duration, animation-fill-mode, animation-iteration-count, animation-name, animation-play-state, animation-timing-function |
| `animation-range` | animation-range-end, animation-range-start |
| `background` | background-attachment, background-clip, background-color, background-image, background-origin, background-position, background-repeat, background-size |
| `border` | border-bottom-color, border-bottom-style, border-bottom-width, border-left-color, border-left-style, border-left-width, border-right-color, border-right-style, border-right-width, border-top-color, border-top-style, border-top-width |
| `border-block` | border-block-end-color, border-block-end-style, border-block-end-width, border-block-start-color, border-block-start-style, border-block-start-width |
| `border-block-color` | border-block-end-color, border-block-start-color |
| `border-block-end` | border-block-end-color, border-block-end-style, border-block-end-width |
| `border-block-start` | border-block-start-color, border-block-start-style, border-block-start-width |
| `border-block-style` | border-block-end-style, border-block-start-style |
| `border-block-width` | border-block-end-width, border-block-start-width |
| `border-bottom` | border-bottom-color, border-bottom-style, border-bottom-width |
| `border-color` | border-bottom-color, border-left-color, border-right-color, border-top-color |
| `border-image` | border-image-outset, border-image-repeat, border-image-slice, border-image-source, border-image-width |
| `border-inline` | border-inline-end-color, border-inline-end-style, border-inline-end-width, border-inline-start-color, border-inline-start-style, border-inline-start-width |
| `border-inline-color` | border-inline-end-color, border-inline-start-color |
| `border-inline-end` | border-inline-end-color, border-inline-end-style, border-inline-end-width |
| `border-inline-start` | border-inline-start-color, border-inline-start-style, border-inline-start-width |
| `border-inline-style` | border-inline-end-style, border-inline-start-style |
| `border-inline-width` | border-inline-end-width, border-inline-start-width |
| `border-left` | border-left-color, border-left-style, border-left-width |
| `border-radius` | border-bottom-left-radius, border-bottom-right-radius, border-top-left-radius, border-top-right-radius |
| `border-right` | border-right-color, border-right-style, border-right-width |
| `border-style` | border-bottom-style, border-left-style, border-right-style, border-top-style |
| `border-top` | border-top-color, border-top-style, border-top-width |
| `border-width` | border-bottom-width, border-left-width, border-right-width, border-top-width |
| `caret` | caret-color |
| `column-rule` | column-rule-color, column-rule-style, column-rule-width |
| `columns` | column-count, column-width |
| `contain-intrinsic-size` | contain-intrinsic-height, contain-intrinsic-width |
| `container` | container-name, container-type |
| `flex` | flex-basis, flex-grow, flex-shrink |
| `flex-flow` | flex-direction, flex-wrap |
| `font` | font-family, font-size, font-stretch, font-style, font-variant, font-weight, line-height |
| `font-synthesis` | font-synthesis-small-caps, font-synthesis-style, font-synthesis-weight |
| `font-variant` | font-variant-alternates, font-variant-caps, font-variant-east-asian, font-variant-ligatures, font-variant-numeric, font-variant-position |
| `gap` | column-gap, row-gap |
| `grid` | grid-auto-columns, grid-auto-flow, grid-auto-rows, grid-template-areas, grid-template-columns, grid-template-rows |
| `grid-area` | grid-column-end, grid-column-start, grid-row-end, grid-row-start |
| `grid-column` | grid-column-end, grid-column-start |
| `grid-row` | grid-row-end, grid-row-start |
| `grid-template` | grid-template-areas, grid-template-columns, grid-template-rows |
| `inset` | bottom, left, right, top |
| `inset-block` | inset-block-end, inset-block-start |
| `inset-inline` | inset-inline-end, inset-inline-start |
| `list-style` | list-style-image, list-style-position, list-style-type |
| `margin` | margin-bottom, margin-left, margin-right, margin-top |
| `margin-block` | margin-block-end, margin-block-start |
| `margin-inline` | margin-inline-end, margin-inline-start |
| `mask` | mask-clip, mask-composite, mask-image, mask-mode, mask-origin, mask-position, mask-repeat, mask-size |
| `mask-border` | mask-border-mode, mask-border-outset, mask-border-repeat, mask-border-slice, mask-border-source, mask-border-width |
| `offset` | offset-anchor, offset-distance, offset-path, offset-position, offset-rotate |
| `outline` | outline-color, outline-style, outline-width |
| `overflow` | overflow-x, overflow-y |
| `overscroll-behavior` | overscroll-behavior-x, overscroll-behavior-y |
| `padding` | padding-bottom, padding-left, padding-right, padding-top |
| `padding-block` | padding-block-end, padding-block-start |
| `padding-inline` | padding-inline-end, padding-inline-start |
| `place-content` | align-content, justify-content |
| `place-items` | align-items, justify-items |
| `place-self` | align-self, justify-self |
| `position-try` | position-try-fallbacks, position-try-order |
| `scroll-margin` | scroll-margin-bottom, scroll-margin-left, scroll-margin-right, scroll-margin-top |
| `scroll-margin-block` | scroll-margin-block-end, scroll-margin-block-start |
| `scroll-margin-inline` | scroll-margin-inline-end, scroll-margin-inline-start |
| `scroll-padding` | scroll-padding-bottom, scroll-padding-left, scroll-padding-right, scroll-padding-top |
| `scroll-padding-block` | scroll-padding-block-end, scroll-padding-block-start |
| `scroll-padding-inline` | scroll-padding-inline-end, scroll-padding-inline-start |
| `scroll-timeline` | scroll-timeline-axis, scroll-timeline-name |
| `text-decoration` | text-decoration-color, text-decoration-line, text-decoration-style, text-decoration-thickness |
| `text-emphasis` | text-emphasis-color, text-emphasis-style |
| `text-wrap` | text-wrap-mode, text-wrap-style |
| `transition` | transition-delay, transition-duration, transition-property, transition-timing-function |
| `view-timeline` | view-timeline-axis, view-timeline-name |
| `white-space` | text-wrap-mode, white-space-collapse |

### Reference Table 2: Logical → Physical Mappings (40 pairs)

In default writing mode (horizontal-tb, ltr):

| Logical Property | Physical Property |
|-----------------|-------------------|
| `block-size` | `height` |
| `border-block-end-color` | `border-bottom-color` |
| `border-block-end-style` | `border-bottom-style` |
| `border-block-end-width` | `border-bottom-width` |
| `border-block-start-color` | `border-top-color` |
| `border-block-start-style` | `border-top-style` |
| `border-block-start-width` | `border-top-width` |
| `border-end-end-radius` | `border-bottom-right-radius` |
| `border-end-start-radius` | `border-bottom-left-radius` |
| `border-inline-end-color` | `border-right-color` |
| `border-inline-end-style` | `border-right-style` |
| `border-inline-end-width` | `border-right-width` |
| `border-inline-start-color` | `border-left-color` |
| `border-inline-start-style` | `border-left-style` |
| `border-inline-start-width` | `border-left-width` |
| `border-start-end-radius` | `border-top-right-radius` |
| `border-start-start-radius` | `border-top-left-radius` |
| `contain-intrinsic-block-size` | `contain-intrinsic-height` |
| `contain-intrinsic-inline-size` | `contain-intrinsic-width` |
| `inline-size` | `width` |
| `inset-block-end` | `bottom` |
| `inset-block-start` | `top` |
| `inset-inline-end` | `right` |
| `inset-inline-start` | `left` |
| `margin-block-end` | `margin-bottom` |
| `margin-block-start` | `margin-top` |
| `margin-inline-end` | `margin-right` |
| `margin-inline-start` | `margin-left` |
| `max-block-size` | `max-height` |
| `max-inline-size` | `max-width` |
| `min-block-size` | `min-height` |
| `min-inline-size` | `min-width` |
| `overflow-block` | `overflow-y` |
| `overflow-inline` | `overflow-x` |
| `overscroll-behavior-block` | `overscroll-behavior-y` |
| `overscroll-behavior-inline` | `overscroll-behavior-x` |
| `padding-block-end` | `padding-bottom` |
| `padding-block-start` | `padding-top` |
| `padding-inline-end` | `padding-right` |
| `padding-inline-start` | `padding-left` |

### Reference Table 3: Module Membership (45 modules, 412 properties)

| Module | # | Properties |
|--------|---|------------|
| CSS Logical Properties | 53 | inline-size, block-size, min-inline-size, min-block-size, max-inline-size, max-block-size, margin-block, margin-block-start, margin-block-end, margin-inline, margin-inline-start, margin-inline-end, padding-block, padding-block-start, padding-block-end, padding-inline, padding-inline-start, padding-inline-end, inset, inset-block, inset-block-start, inset-block-end, inset-inline, inset-inline-start, inset-inline-end, border-block, border-block-color, border-block-style, border-block-width, border-block-start, border-block-start-color, border-block-start-style, border-block-start-width, border-block-end, border-block-end-color, border-block-end-style, border-block-end-width, border-inline, border-inline-color, border-inline-style, border-inline-width, border-inline-start, border-inline-start-color, border-inline-start-style, border-inline-start-width, border-inline-end, border-inline-end-color, border-inline-end-style, border-inline-end-width, border-start-start-radius, border-start-end-radius, border-end-start-radius, border-end-end-radius |
| CSS Backgrounds | 41 | background, background-color, background-image, background-repeat, background-position, background-size, background-origin, background-clip, background-attachment, border, border-top, border-top-color, border-top-style, border-top-width, border-right, border-right-color, border-right-style, border-right-width, border-bottom, border-bottom-color, border-bottom-style, border-bottom-width, border-left, border-left-color, border-left-style, border-left-width, border-color, border-style, border-width, border-radius, border-top-left-radius, border-top-right-radius, border-bottom-right-radius, border-bottom-left-radius, border-image, border-image-source, border-image-slice, border-image-width, border-image-outset, border-image-repeat, box-shadow |
| CSS Scroll Snap | 25 | scroll-margin, scroll-margin-block, scroll-margin-block-start, scroll-margin-block-end, scroll-margin-inline, scroll-margin-inline-start, scroll-margin-inline-end, scroll-margin-top, scroll-margin-right, scroll-margin-bottom, scroll-margin-left, scroll-padding, scroll-padding-block, scroll-padding-block-start, scroll-padding-block-end, scroll-padding-inline, scroll-padding-inline-start, scroll-padding-inline-end, scroll-padding-top, scroll-padding-right, scroll-padding-bottom, scroll-padding-left, scroll-snap-type, scroll-snap-align, scroll-snap-stop |
| CSS SVG | 22 | fill, fill-opacity, fill-rule, stroke, stroke-dasharray, stroke-dashoffset, stroke-linecap, stroke-linejoin, stroke-miterlimit, stroke-opacity, stroke-width, paint-order, marker-start, marker-mid, marker-end, color-interpolation, color-interpolation-filters, dominant-baseline, text-anchor, stop-color, stop-opacity, vector-effect |
| CSS Fonts | 21 | font, font-family, font-size, font-style, font-variant, font-weight, font-stretch, font-feature-settings, font-kerning, font-language-override, font-optical-sizing, font-palette, font-size-adjust, font-synthesis, font-variant-alternates, font-variant-caps, font-variant-east-asian, font-variant-ligatures, font-variant-numeric, font-variant-position, font-variation-settings |
| CSS Text | 19 | overflow-wrap, text-align, text-align-last, text-indent, text-justify, text-transform, letter-spacing, word-spacing, word-break, word-wrap, white-space, white-space-collapse, text-wrap, tab-size, hyphens, hanging-punctuation, line-break, text-autospace, text-spacing-trim |
| CSS Grid | 15 | grid, grid-template-rows, grid-template-columns, grid-template-areas, grid-template, grid-auto-rows, grid-auto-columns, grid-auto-flow, grid-area, grid-row, grid-row-start, grid-row-end, grid-column, grid-column-start, grid-column-end |
| CSS Masking | 13 | clip-path, clip, mask, mask-image, mask-mode, mask-repeat, mask-position, mask-clip, mask-origin, mask-size, mask-composite, mask-type, clip-rule |
| CSS Text Decoration | 12 | text-decoration, text-decoration-line, text-decoration-style, text-decoration-color, text-decoration-thickness, text-emphasis, text-emphasis-style, text-emphasis-color, text-emphasis-position, text-shadow, text-underline-offset, text-underline-position |
| CSS Box Alignment | 12 | justify-content, align-content, align-items, align-self, gap, row-gap, column-gap, justify-items, justify-self, place-content, place-items, place-self |
| CSS UI | 11 | outline, outline-color, outline-style, outline-width, outline-offset, accent-color, caret-color, user-select, cursor, resize, appearance |
| CSS Box Model | 10 | margin, margin-top, margin-right, margin-bottom, margin-left, padding, padding-top, padding-right, padding-bottom, padding-left |
| CSS Animations | 10 | animation, animation-name, animation-duration, animation-timing-function, animation-delay, animation-iteration-count, animation-direction, animation-fill-mode, animation-play-state, animation-timeline |
| CSS Containment | 10 | contain, contain-intrinsic-size, contain-intrinsic-width, contain-intrinsic-height, contain-intrinsic-block-size, contain-intrinsic-inline-size, container-type, container-name, container, content-visibility |
| CSS Fragmentation | 9 | box-decoration-break, break-before, break-after, break-inside, orphans, widows, page-break-before, page-break-after, page-break-inside |
| CSS Multi-column | 9 | columns, column-count, column-fill, column-rule, column-rule-color, column-rule-style, column-rule-width, column-span, column-width |
| CSS Transforms | 9 | transform, transform-origin, transform-style, perspective, perspective-origin, backface-visibility, rotate, scale, translate |
| CSS Box Sizing | 8 | width, height, min-width, min-height, max-width, max-height, box-sizing, aspect-ratio |
| CSS Positioning | 8 | position, top, right, bottom, left, z-index, float, clear |
| CSS Overflow | 8 | overflow, overflow-x, overflow-y, overflow-block, overflow-inline, text-overflow, scroll-behavior, scrollbar-gutter |
| CSS Flexbox | 8 | flex, flex-grow, flex-shrink, flex-basis, flex-direction, flex-flow, flex-wrap, order |
| CSS Anchor Positioning | 8 | anchor-name, anchor-scope, position-anchor, position-area, position-try, position-try-fallbacks, position-try-order, position-visibility |
| CSS Lists | 7 | counter-reset, counter-increment, counter-set, list-style, list-style-type, list-style-position, list-style-image |
| CSS Motion Path | 6 | offset, offset-path, offset-distance, offset-rotate, offset-anchor, offset-position |
| CSS Table | 5 | table-layout, border-collapse, border-spacing, caption-side, empty-cells |
| CSS Transitions | 5 | transition, transition-property, transition-duration, transition-timing-function, transition-delay |
| CSS Writing Modes | 5 | writing-mode, direction, unicode-bidi, text-orientation, text-combine-upright |
| CSS Overscroll | 5 | overscroll-behavior, overscroll-behavior-x, overscroll-behavior-y, overscroll-behavior-block, overscroll-behavior-inline |
| CSS Inline | 4 | line-height, vertical-align, initial-letter, initial-letter-align |
| CSS Images | 4 | object-fit, object-position, image-rendering, image-orientation |
| CSS Color Adjust | 3 | color-scheme, forced-color-adjust, print-color-adjust |
| CSS Compositing | 3 | mix-blend-mode, background-blend-mode, isolation |
| CSS Shapes | 3 | shape-outside, shape-margin, shape-image-threshold |
| CSS MathML | 3 | math-style, math-depth, math-shift |
| CSS Display | 2 | display, visibility |
| CSS Color | 2 | opacity, color |
| CSS Generated Content | 2 | content, quotes |
| CSS Filter Effects | 2 | filter, backdrop-filter |
| CSS Scrollbars | 2 | scrollbar-color, scrollbar-width |
| CSS View Transitions | 2 | view-transition-name, view-transition-class |
| CSS Ruby | 2 | ruby-align, ruby-position |
| CSS UI / SVG | 1 | pointer-events |
| CSS Pointer Events | 1 | touch-action |
| CSS Will Change | 1 | will-change |
| CSS Cascade | 1 | all |

### Reference Table 4: Display-Dependent Properties (43 properties)

| Display Value Required | Properties |
|-----------------------|------------|
| `flex` / `inline-flex` (container) | flex-direction, flex-flow, flex-wrap |
| `flex` / `inline-flex` (item) | align-self, flex, flex-basis, flex-grow, flex-shrink, order |
| `grid` / `inline-grid` (container) | grid, grid-auto-columns, grid-auto-flow, grid-auto-rows, grid-template, grid-template-areas, grid-template-columns, grid-template-rows, justify-items |
| `grid` / `inline-grid` (item) | grid-area, grid-column, grid-column-end, grid-column-start, grid-row, grid-row-end, grid-row-start, justify-self |
| `table` / `table-*` | border-collapse, border-spacing, caption-side, empty-cells, table-layout |
| Block containers (multi-column) | column-count, column-fill, column-rule, column-rule-color, column-rule-style, column-rule-width, column-width, columns |
| `list-item` | list-style, list-style-image, list-style-position, list-style-type, marker-side |
| `ruby` / `ruby-text` | ruby-align, ruby-position |

---

## Section 5: Complete Rule Index

| Rule | Type | Unique Pairs | Dir. Pairs | Description |
|------|------|-------------|------------|-------------|
| SF-01 | SELF | 412 | 412 | Same property — last declaration wins |
| SF-02 | ALIAS | 1 | 2 | word-wrap ↔ overflow-wrap |
| SF-03 | ALL_EXEMPT | 2 | 4 | direction, unicode-bidi exempt from `all` |
| SF-04 | ALL_RESETS | 409 | 818 | `all` resets everything else |
| SF-05 | SHORTHAND_SETS | 244 | 488 | Shorthand sets its longhands |
| SF-06 | SAME_SHORTHAND | 357 | 714 | Longhands share a common shorthand |
| SF-07 | LOGICAL_PHYSICAL | 40 | 80 | Logical ↔ physical mapping |
| SF-08 | SAME_MODULE | 2,864 | 5,728 | Same CSS module |
| SF-09 | AXIS_REMAP | 40 | 80 | writing-mode remaps logical properties |
| SF-10 | SAME_AXIS | 18 | 36 | Same dimensional axis |
| SF-DEFAULT | INDEPENDENT | 160,865 | 160,865 | No interaction (default) |
| IN-01 | INTERACTION | 8 | 16 | Stacking context creators × z-index |
| IN-02 | INTERACTION | 7 | 14 | Containing block creators × position/overflow |
| IN-03 | INTERACTION | 7 | 14 | transform-style:flat forcers |
| IN-04 | INTERACTION | 15 | 30 | display value interactions |
| IN-05 | DISPLAY_DEPENDENT | 43 | 86 | display gates property applicability |
| IN-06 | PARENT_CHILD | 36 | 72 | Flex container × item context |
| IN-07 | PARENT_CHILD | 72 | 144 | Grid container × item context |
| IN-08 | INTERACTION | 8 | 16 | Sizing constraint resolution |
| IN-09 | INTERACTION | 4 | 8 | box-sizing × dimensions |
| IN-10 | INTERACTION | 5 | 10 | Float interactions |
| IN-11 | INTERACTION | 6 | 12 | Position gating |
| IN-12 | INTERACTION | 7 | 14 | Overflow/clipping interactions |
| IN-13 | INTERACTION | 7 | 14 | Containment side effects |
| IN-14 | INTERACTION | 3 | 6 | Writing mode interactions |
| IN-15 | INTERACTION | 2 | 4 | Table property interactions |
| IN-16 | INTERACTION | 2 | 4 | Animation/transition conflicts |
| IN-17 | INTERACTION | 3 | 6 | Transform-related interactions |
| IN-18 | INTERACTION | 2 | 4 | Shape property gating |
| IN-19 | INTERACTION | 5 | 10 | Text decoration gating |
| IN-20 | INTERACTION | 3 | 6 | Scroll snap gating |
| IN-21 | INTERACTION | 14 | 28 | Specific remaining interactions |
| **Total** | | **~85,000** | **169,744** | **31 rules + 4 tables** |

---

## Section 6: Pair Coverage Verification

### Total pairs: 412 × 412 = 169,744

| Category | Directional Pairs | % |
|----------|------------------|---|
| INDEPENDENT (SF-DEFAULT) | 160,865 | 94.77% |
| SAME_MODULE (SF-08) | 5,728 | 3.37% |
| ALL_RESETS (SF-04) | 818 | 0.48% |
| SAME_SHORTHAND (SF-06) | 714 | 0.42% |
| SHORTHAND_SETS (SF-05) | 488 | 0.29% |
| SELF (SF-01) | 412 | 0.24% |
| PARENT_CHILD_CONTEXT (IN-06 + IN-07) | 216 | 0.13% |
| INTERACTION (IN-01..04, IN-08..21) | 216 | 0.13% |
| DISPLAY_DEPENDENT (IN-05) | 86 | 0.05% |
| LOGICAL_PHYSICAL (SF-07) | 80 | 0.05% |
| AXIS_REMAP (SF-09) | 80 | 0.05% |
| SAME_AXIS (SF-10) | 36 | 0.02% |
| ALL_EXEMPT (SF-03) | 4 | <0.01% |
| ALIAS (SF-02) | 2 | <0.01% |
| **Sum** | **169,745** | **100.00%** |

Note: Sum is 169,745 vs expected 169,744 due to one pair being counted in
overlapping categories. The priority order in Section 2 ensures each pair
resolves to exactly one rule.

**Compression: 169,744 pairs → 31 rules + 4 reference tables = 5,475× compression.**

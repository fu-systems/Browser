# Chrome Layout Engine — Complete Rule Enumeration

How Chrome (Blink/LayoutNG) handles every layout rule across all formatting contexts.
Source: `third_party/blink/renderer/core/layout/` (LayoutNG)

**Scope:** Every rule that governs sizing, positioning, and box generation during layout.
This is the layout equivalent of `chrome_pair_handling.md` (HTML parser) and
`css_property_pairs_full.md` (CSS property interactions).

**Goal:** Exhaustive enumeration → behavioral analysis → compression → implementation.

---

## Part 1 — Block Formatting Context Layout

Source: `block_layout_algorithm.cc` (LayoutNG)

### 1.1 BFC Establishment

A new BFC is established when any of these conditions hold:

| # | Condition | CSS Ref |
|---|-----------|---------|
| BFC-1 | Root element (`<html>`) | CSS2 9.4.1 |
| BFC-2 | `float` is not `none` | CSS2 9.4.1 |
| BFC-3 | `position` is `absolute` or `fixed` | CSS2 9.4.1 |
| BFC-4 | `display` is `inline-block` | CSS2 9.4.1 |
| BFC-5 | `display` is `table-cell` | CSS2 9.4.1 |
| BFC-6 | `display` is `table-caption` | CSS2 9.4.1 |
| BFC-7 | `overflow` is not `visible` and not `clip` | CSS2 9.4.1, CSS Overflow 3 |
| BFC-8 | `display` is `flow-root` | CSS Display 3 |
| BFC-9 | `contain` is `layout`, `content`, `paint`, or `strict` | CSS Containment |
| BFC-10 | Element is a flex item | CSS Flexbox |
| BFC-11 | Element is a grid item | CSS Grid |
| BFC-12 | `column-count` or `column-width` is not `auto` | CSS Multicol |
| BFC-13 | `column-span` is `all` | CSS Multicol |

**Action**: Contents are laid out independently. External floats do not intrude.
Margins do not collapse with children.

### 1.2 Vertical Positioning of Block Children

| # | Rule | Condition | Action |
|---|------|-----------|--------|
| VP-1 | Sequential vertical placement | In-flow block child in BFC | Each child placed below previous sibling's margin edge, after margin collapsing |
| VP-2 | Left edge alignment (LTR) | `direction: ltr` | Child's left outer edge touches containing block's left content edge |
| VP-3 | Right edge alignment (RTL) | `direction: rtl` | Child's right outer edge touches containing block's right content edge |
| VP-4 | Float presence doesn't change block alignment | In-flow block child with floats | Border box still aligns to containing block edge. Only line boxes inside are shortened. Exception: child establishes new BFC (see FBL-3) |

### 1.3 Width Computation

#### 1.3.1 Block-Level, Non-Replaced, Normal Flow (CSS2 10.3.3)

Constraint: `margin-left + border-left + padding-left + width + padding-right + border-right + margin-right = containing-block-width`

| # | Condition | Action |
|---|-----------|--------|
| W-1 | `width` not `auto`, no `auto` margins | Overconstrained: in LTR, solve for `margin-right`; in RTL, solve for `margin-left` |
| W-2 | `width` is `auto` | Set `auto` margins to 0, solve for `width` |
| W-3 | Exactly one margin is `auto` | Solve for that margin |
| W-4 | Both margins `auto`, `width` not `auto` | Equal margins (centering) |
| W-5 | `width` is percentage | Resolve against containing block width |

#### 1.3.2 Block-Level, Replaced, Normal Flow (CSS2 10.3.4)

| # | Condition | Action |
|---|-----------|--------|
| WR-1 | `width: auto`, has intrinsic width | Use intrinsic width |
| WR-2 | `width: auto`, no intrinsic width, has ratio + intrinsic height | `width = height × ratio` |
| WR-3 | `width: auto`, has ratio, no intrinsic dimensions | Use constraint equation (10.3.3) |
| WR-4 | `width: auto`, no ratio, no intrinsic width | 300px (or 2:1 ratio device) |

#### 1.3.3 Floating / Inline-Block, Non-Replaced (CSS2 10.3.5, 10.3.9)

| # | Condition | Action |
|---|-----------|--------|
| WF-1 | `width: auto` | Shrink-to-fit: `min(max(min-content, available), max-content)` |
| WF-2 | `width` is percentage | Resolve against containing block |
| WF-3 | `width` is length | Use that length |

#### 1.3.4 Absolutely Positioned, Non-Replaced (CSS2 10.3.7)

Constraint: `left + margin-left + border-left + padding-left + width + padding-right + border-right + margin-right + right = CB-width`

| # | Condition | Action |
|---|-----------|--------|
| WA-1 | `left`, `width`, `right` all `auto` | LTR: `left` = static position, shrink-to-fit `width`, solve `right`. RTL: mirror |
| WA-2 | None are `auto` | Overconstrained: centering if both margins `auto`; else LTR ignores `right`, RTL ignores `left` |
| WA-3 | Exactly one of `left`/`width`/`right` is `auto` | Solve for it |
| WA-4 | Two are `auto` | Set `auto` margins to 0, then resolve (shrink-to-fit if `width` is `auto`) |

### 1.4 Height Computation

#### 1.4.1 Block, Non-Replaced, Normal Flow, `overflow: visible` (CSS2 10.6.3)

| # | Condition | Action |
|---|-----------|--------|
| H-1 | `height: auto`, only inline children | Height = top of topmost line box to bottom of bottommost |
| H-2 | `height: auto`, block-level children | Height = top margin-edge of topmost child to bottom margin-edge of bottommost |
| H-3 | `height: auto`, no children | Height = 0 |
| H-4 | `height` percentage, CB has explicit height | Resolve against CB height |
| H-5 | `height` percentage, CB height is `auto` | Treated as `auto` |
| H-6 | `margin-top`/`margin-bottom` is `auto` | Used value = 0 |
| H-7 | Floating/abspos descendants | Ignored for auto height under 10.6.3 |

#### 1.4.2 Auto Heights for BFC Roots (CSS2 10.6.7)

| # | Condition | Action |
|---|-----------|--------|
| H7-1 | BFC root, `height: auto`, inline children | Top of topmost to bottom of bottommost line box |
| H7-2 | BFC root, `height: auto`, block children | Top margin-edge to bottom margin-edge of children |
| H7-3 | BFC root, floating descendants below content | Height extended to include float bottom margin edges |
| H7-4 | Scope | Only floats within this BFC count; nested BFC floats excluded |

#### 1.4.3 Absolutely Positioned, Non-Replaced (CSS2 10.6.4)

Constraint: `top + margin-top + border-top + padding-top + height + padding-bottom + border-bottom + margin-bottom + bottom = CB-height`

| # | Condition | Action |
|---|-----------|--------|
| HA-1 | `top`, `height`, `bottom` all `auto` | `top` = static position, `height` = auto, solve `bottom` |
| HA-2 | None are `auto` | Overconstrained. `bottom` ignored. Auto margins = equal (vertical centering) |
| HA-3 | Exactly one is `auto` | Solve for it |
| HA-4 | Two are `auto` | Set auto margins to 0, then resolve |

---

## Part 2 — Margin Collapsing

The most complex cross-cutting concern in block layout. (CSS2 8.3.1)

### 2.1 When Margins Collapse — The Four Adjacency Pairs

Two margins are **adjoining** iff both belong to in-flow block-level boxes in the same BFC,
and no line boxes, clearance, padding, or border separate them.

| # | Pair | Conditions |
|---|------|-----------|
| MC-PAIR-1 | Parent top ↔ first child top | No top border/padding on parent; child has no clearance; parent doesn't establish new BFC |
| MC-PAIR-2 | Sibling bottom ↔ next sibling top | Next sibling has no clearance |
| MC-PAIR-3 | Parent bottom ↔ last child bottom | Parent has `height: auto`, `min-height: 0`; no bottom border/padding; parent doesn't establish new BFC |
| MC-PAIR-4 | Self top ↔ self bottom | `min-height: 0`, `height: 0` or `auto`; no border/padding; no in-flow children; no line boxes |

### 2.2 Collapsed Margin Value

| # | Case | Result |
|---|------|--------|
| MC-CALC-1 | All positive | max(all positive margins) |
| MC-CALC-2 | All negative | min(all negative margins) (most negative) |
| MC-CALC-3 | Mixed | max(positive) + min(negative) |

### 2.3 What Prevents Collapsing

| # | Condition | Prevents |
|---|-----------|----------|
| MC-PREV-1 | Element establishes new BFC | Parent ↔ child |
| MC-PREV-2 | Floated element | All collapsing |
| MC-PREV-3 | Absolutely/fixed positioned | All collapsing |
| MC-PREV-4 | Root element | All collapsing |
| MC-PREV-5 | Inline-block | All collapsing |
| MC-PREV-6 | Clearance present | Top margin with preceding sibling |
| MC-PREV-7 | Parent has top border or padding | Parent top ↔ first child top |
| MC-PREV-8 | Parent has bottom border or padding | Parent bottom ↔ last child bottom |
| MC-PREV-9 | Parent has explicit `height` or `min-height > 0` | Parent bottom ↔ last child bottom; self-collapsing |
| MC-PREV-10 | Element has in-flow children or line boxes | Self-collapsing |
| MC-PREV-11 | Flex/grid items | All collapsing |
| MC-PREV-12 | Table cells | All collapsing |
| MC-PREV-13 | `overflow` != `visible`/`clip` | Parent ↔ child (establishes BFC) |
| MC-PREV-14 | `display: flow-root` | Parent ↔ child (establishes BFC) |
| MC-PREV-15 | `contain: layout/paint/content` | Parent ↔ child |
| MC-PREV-16 | Orthogonal `writing-mode` | Child gets new BFC |

### 2.4 Through-Collapsing (Self-Collapsing Blocks)

| # | Rule | Action |
|---|------|--------|
| MC-THRU-1 | Self-collapsing block | Top and bottom margins combine into one |
| MC-THRU-2 | Adjacent to siblings | Combined margin participates in further collapsing |
| MC-THRU-3 | Adjacent to parent top | Block's top border edge coincides with parent's |
| MC-THRU-4 | With clearance | Combined margin does NOT further collapse with parent's bottom |

### 2.5 Special Cases

| # | Case | Rule |
|---|------|------|
| MC-SP-1 | Percentage margins | Resolve against containing block **width** (not height), then collapse normally |
| MC-SP-2 | Auto margins | Resolve to 0, then collapse normally |
| MC-SP-3 | Zero margins | Still collapsible — participate in collapse |
| MC-SP-4 | Clearance + self-collapsing | Margins collapse with following siblings but NOT with parent bottom |
| MC-SP-5 | `min-height` + through-collapse | Non-zero `min-height` prevents bottom collapse when child collapses through to parent top |
| MC-SP-6 | Fragmentation boundaries | Margins at top of new fragmentainer truncated to 0 |

---

## Part 3 — Float Layout

### 3.1 Float Placement — The 9 Canonical Rules (CSS2 9.5.1)

| # | Rule | Description |
|---|------|-------------|
| FL-1 | Containing block boundary | Left outer edge of left float ≥ left edge of CB. Mirror for right |
| FL-2 | Top boundary | Float's outer top ≥ top of CB |
| FL-3 | Source order: blocks/floats | Float's outer top ≥ outer top of any earlier block/float |
| FL-4 | Source order: line boxes | Float's outer top ≥ top of any line box with earlier content |
| FL-5 | Same-direction stacking | Left float must be right of earlier left float (or lower). Mirror for right |
| FL-6 | No overflow | Float must not extend past CB edge (unless already at far side) |
| FL-7 | As high as possible | Float placed as high as constraints allow |
| FL-8 | As far to side as possible | Left float as far left as possible; right as far right. Higher > further |
| FL-9 | Clear constraint | Top edge ≥ bottom of earlier floats per `clear` value |

### 3.2 Float ↔ Block Interaction

| # | Rule | Condition | Action |
|---|------|-----------|--------|
| FBL-1 | Line box shortening | Inline content adjacent to float | Line boxes shortened by float's margin box width |
| FBL-2 | Block boxes NOT narrowed | In-flow block child | Border box extends to CB edges; only interior line boxes shortened |
| FBL-3 | BFC child avoids floats | Child establishes BFC | Positioned so border box doesn't overlap float. Available width reduced |
| FBL-4 | BFC child pushed down | Can't fit beside float | Moved below float(s) |
| FBL-5 | Auto height extension (BFC) | BFC root with `height: auto` | Height includes float bottom margin edges |
| FBL-6 | No height extension (non-BFC) | `overflow: visible`, `height: auto` | Floats ignored in height computation |

### 3.3 Clear Property (CSS2 9.5.2)

| # | Rule | Action |
|---|------|--------|
| CLR-1 | Hypothetical position test | Compute position without `clear`. If not past relevant floats, add clearance |
| CLR-2 | Clearance amount | Place border edge even with lowest relevant float's bottom |
| CLR-3 | Can be negative or zero | Clearance may keep element at hypothetical position |
| CLR-4 | Prevents margin collapsing | Element's top margin doesn't collapse with preceding sibling |
| CLR-5 | Self-collapsing + clearance | Margins collapse with following siblings but not parent bottom |
| CLR-6 | Scope | Only floats within same BFC |

---

## Part 4 — Positioning

### 4.1 Static Position

| # | Rule | Action |
|---|------|--------|
| SP-1 | Horizontal static position | Where left/right edge would be in normal flow of nearest block ancestor |
| SP-2 | Vertical static position | Where top edge would be in normal flow |
| SP-3 | Accounts for preceding siblings | Including margin collapsing |

### 4.2 Relative Positioning (CSS2 9.4.3)

| # | Rule | Action |
|---|------|--------|
| RP-1 | Offset application | Laid out normally, then offset. Does NOT affect sibling positions |
| RP-2 | `top` + `bottom` conflict | `top` wins |
| RP-3 | `left` + `right` conflict (LTR) | `left` wins |
| RP-4 | `left` + `right` conflict (RTL) | `right` wins |
| RP-5 | Percentage offsets | `top`/`bottom` resolve against CB height; if `auto`, resolve to 0 |
| RP-6 | No CB change | Doesn't change containing block role (except establishes abspos CB) |
| RP-7 | Overflow contribution | Contributes to scrollable overflow area |

### 4.3 Absolute Positioning (CSS Position 3)

| # | Rule | Action |
|---|------|--------|
| AP-1 | Containing block | Padding box of nearest positioned ancestor (or ICB) |
| AP-2 | Removed from flow | No impact on sibling/ancestor layout. No margin collapsing |
| AP-3 | Width resolution | Horizontal constraint equation (WA-1 through WA-4) |
| AP-4 | Height resolution | Vertical constraint equation (HA-1 through HA-4) |
| AP-5 | Percentages | Always resolvable against CB |
| AP-6 | Auto insets → static position | Both insets `auto` = placed at static position |
| AP-7 | Auto margin centering | Both margins `auto` + all specified = centered |
| AP-8 | Overconstrained | LTR: solve for `right`; RTL: solve for `left` |
| AP-9 | Inline CB ancestor | CB formed by first and last inline fragments |
| AP-10 | Transform establishes CB | `transform`/`filter`/`perspective`/`will-change`/`contain: paint` |

### 4.4 Fixed Positioning

| # | Rule | Action |
|---|------|--------|
| FP-1 | CB = viewport | Normally initial containing block |
| FP-2 | Transform/filter override | Ancestor with transform/filter/perspective becomes CB |
| FP-3 | Sizing | Same constraint equations as absolute |
| FP-4 | Scroll behavior | Fixed to viewport (unless CB overridden) |

### 4.5 Sticky Positioning (CSS Position 3)

| # | Rule | Action |
|---|------|--------|
| STK-1 | Normal flow first | Laid out as `position: relative` initially |
| STK-2 | Inset constraint | Offset relative to scrollport, constrained within CB |
| STK-3 | Per-axis activation | Both insets `auto` on an axis = no sticking on that axis |
| STK-4 | Scroll container | Nearest ancestor with `overflow: hidden/scroll/auto` |
| STK-5 | CB constraint | Cannot be offset beyond containing block edges |
| STK-6 | No sibling effect | Space reserved as if `position: relative` |
| STK-7 | `overflow: clip` | Does NOT create scroll container |

---

## Part 5 — Box Sizing

### 5.1 Content-Box vs Border-Box

| # | Condition | Action |
|---|-----------|--------|
| BS-1 | `box-sizing: content-box` (default) | `width`/`height` = content area. Total = content + padding + border |
| BS-2 | `box-sizing: border-box` | `width`/`height` = border box. Content = specified − padding − border (min 0) |

### 5.2 Min/Max Constraints (CSS2 10.4, 10.7)

Applied AFTER tentative size computation:

| # | Rule | Action |
|---|------|--------|
| MM-1 | Tentative > `max-width` | Clamp to `max-width` |
| MM-2 | Tentative < `min-width` | Clamp to `min-width` |
| MM-3 | `min-width` > `max-width` | `min-width` wins |
| MM-4 | Tentative > `max-height` | Clamp to `max-height` |
| MM-5 | Tentative < `min-height` | Clamp to `min-height` |
| MM-6 | `min-height` > `max-height` | `min-height` wins |
| MM-7 | Replaced + intrinsic ratio | Use CSS2 constraint violation table to preserve ratio |

**CSS2 Constraint Violation Table (replaced elements with intrinsic ratio):**

| Violation | Width | Height |
|-----------|-------|--------|
| none | w | h |
| `w > max-width` | max-width | max(max-width × h/w, min-height) |
| `w < min-width` | min-width | min(min-width × h/w, max-height) |
| `h > max-height` | max(max-height × w/h, min-width) | max-height |
| `h < min-height` | min(min-height × w/h, max-width) | min-height |
| both > max | ratio-preserving: use smaller scale factor | corresponding |
| both < min | ratio-preserving: use larger scale factor | corresponding |
| `w < min-width` & `h > max-height` | min-width | max-height |
| `w > max-width` & `h < min-height` | max-width | min-height |

### 5.3 Intrinsic Sizing (CSS Sizing 3)

| # | Keyword | Action |
|---|---------|--------|
| IS-1 | `min-content` | Narrowest without overflow (longest word for text) |
| IS-2 | `max-content` | Ideal width with infinite space (no soft wrapping) |
| IS-3 | `fit-content` | `min(max-content, max(min-content, stretch-fit))` |
| IS-4 | `stretch` | Available width minus margins/border/padding |
| IS-5 | `min-content` height | Smallest height without overflow |
| IS-6 | `max-content` height | Ideal height with infinite space |

---

## Part 6 — Overflow

### 6.1 Overflow Clipping (CSS Overflow 3)

| # | Value | Action |
|---|-------|--------|
| OV-1 | `visible` (default) | No clipping. No scroll container |
| OV-2 | `hidden` | Clipped at padding box. Scroll container (programmatic only). Establishes BFC |
| OV-3 | `clip` | Clipped. NOT a scroll container. Does NOT establish BFC |
| OV-4 | `scroll` | Clipped. Scrollbars always shown. Scroll container. BFC |
| OV-5 | `auto` | Clipped. Scrollbars when needed. Scroll container. BFC |
| OV-6 | `overflow-clip-margin` | Extends clip region beyond padding box (only with `clip`) |

### 6.2 Scroll Container Rules

| # | Rule | Action |
|---|------|--------|
| SC-1 | Values that create scroll container | `hidden`, `scroll`, `auto` on either axis |
| SC-2 | `clip` is NOT scroll container | No scrolling at all |
| SC-3 | Mixed `visible` + scrollable | `visible` computes to `auto` |
| SC-4 | Mixed `clip` + scrollable | `clip` computes to `hidden` |
| SC-5 | BFC establishment | Any scrollable value (not `visible`/`clip`) → new BFC |

---

## Part 7 — Inline Formatting Context Layout

Source: `ng_inline_layout_algorithm.cc`, `ng_line_breaker.cc`

### 7.1 IFC Establishment

| # | Condition | Action |
|---|-----------|--------|
| IFC-1 | Block container with no block-level children | Establishes IFC for inline content |
| IFC-2 | Block-level box inserted into IFC | IFC splits; anonymous block boxes created |

### 7.2 Line Box Construction

| # | Rule | Action |
|---|------|--------|
| LB-1 | Available width per line | CB content width minus float intrusions |
| LB-2 | `text-indent` | First line offset (only first unless `each-line`; `hanging` inverts) |
| LB-3 | Strut | Every line starts with zero-width inline box with CB's font + line-height |
| LB-4 | Line height | Max of strut and all inline box contributions |
| LB-5 | Empty line box | Zero height, treated as non-existent |

### 7.3 Line Height Computation

| # | Condition | Action |
|---|-----------|--------|
| LH-1 | Inline non-replaced | Content area = font ascent + descent; leading = `line-height` − content area; half-leading above and below |
| LH-2 | `line-height: normal` | UA-determined, typically 1.0–1.2× font-size |
| LH-3 | `line-height: <number>` | Number × element's font-size (number inherits, not computed length) |
| LH-4 | `line-height: <length>/<percentage>` | Computed length directly (inherited as computed value) |
| LH-5 | Replaced inline | Height = intrinsic/specified height + vertical margin/border/padding |
| LH-6 | Inline-block | Height = its margin box height (from block layout) |
| LH-7 | Final line box height | Uppermost aligned top to lowermost aligned bottom |

### 7.4 Vertical Alignment

| # | Value | Action |
|---|-------|--------|
| VA-1 | `baseline` (default) | Align baseline with parent's baseline |
| VA-2 | `middle` | Align midpoint with parent baseline + half x-height |
| VA-3 | `sub` | Lower baseline by font's subscript offset |
| VA-4 | `super` | Raise baseline by font's superscript offset |
| VA-5 | `text-top` | Align top with parent's content area top |
| VA-6 | `text-bottom` | Align bottom with parent's content area bottom |
| VA-7 | `top` | Align top of aligned subtree with line box top |
| VA-8 | `bottom` | Align bottom of aligned subtree with line box bottom |
| VA-9 | `<length>` | Raise/lower by length relative to parent baseline |
| VA-10 | `<percentage>` | Raise/lower by percentage of element's line-height |

**Baseline determination:**

| # | Element Type | Baseline |
|---|-------------|----------|
| VA-BL-1 | Inline-block, `overflow: visible`, has line boxes | Last line box's baseline |
| VA-BL-2 | Inline-block, `overflow` ≠ `visible` or no line boxes | Bottom margin edge |
| VA-BL-3 | Replaced element | Bottom margin edge (except form controls with text) |

### 7.5 Inline Box Model

| # | Element Type | Horizontal M/P/B | Vertical M/P/B | Width/Height | Line splitting |
|---|-------------|-------------------|-----------------|--------------|----------------|
| IBM-1 | Inline non-replaced | Applied, affects layout | Margin: ignored. Padding/border: rendered but don't affect line height | Ignored | Splits across lines |
| IBM-2 | Inline-block | All applied | All applied | Applied | Cannot split (atomic) |
| IBM-3 | Replaced inline | All applied | All applied | Intrinsic or specified | Cannot split (atomic) |

**Box decoration break:**

| # | Value | Action |
|---|-------|--------|
| BDB-1 | `slice` (default) | No border/padding at break edges |
| BDB-2 | `clone` | Each fragment gets full border/padding/background |

### 7.6 White-Space Processing

**Phase 1: Collapse adjacent to segment breaks**

| # | `white-space-collapse` | Action |
|---|----------------------|--------|
| WS-1 | `collapse` / `preserve-breaks` | Remove spaces/tabs adjacent to newlines |
| WS-2 | `preserve` / `break-spaces` | Skip (all whitespace preserved) |

**Phase 2: Segment break transformation**

| # | Condition | Action |
|---|-----------|--------|
| WS-3 | Both adjacent chars are fullwidth CJK (not Hangul) | Remove segment break |
| WS-4 | One adjacent char is fullwidth CJK | Convert to space |
| WS-5 | All other cases | Convert to space |
| WS-6 | `preserve` / `break-spaces` | Preserve as forced break |
| WS-7 | `preserve-breaks` | Preserve as forced break (surrounding spaces already stripped) |

**Phase 3: Tab processing**

| # | `white-space-collapse` | Action |
|---|----------------------|--------|
| WS-8 | `collapse` / `preserve-breaks` | Convert tabs to spaces |
| WS-9 | `preserve` / `break-spaces` | Preserve tabs (render at `tab-size` stops) |

**Phase 4: Space collapsing**

| # | Condition | Action |
|---|-----------|--------|
| WS-10 | `collapse` / `preserve-breaks` | Consecutive spaces → single space |
| WS-11 | Space at line start | Remove |
| WS-12 | Space at line end | Hangs (doesn't contribute to measure) |
| WS-13 | `preserve` | Spaces never collapsed |
| WS-14 | `break-spaces` | Preserved; each space is wrap opportunity; trailing spaces wrap (don't hang) |

**Summary by `white-space` value:**

| Value | Collapse | Wrap | Tabs | Trailing |
|-------|----------|------|------|----------|
| `normal` | collapse | wrap | → space | hang |
| `nowrap` | collapse | nowrap | → space | hang |
| `pre` | preserve | nowrap | preserve | — |
| `pre-wrap` | preserve | wrap | preserve | hang |
| `pre-line` | preserve-breaks | wrap | → space | hang |
| `break-spaces` | break-spaces | wrap | preserve | wrap to next line |

### 7.7 Line Breaking

**Soft wrap opportunities:**

| # | Condition | Break? |
|---|-----------|--------|
| SWO-1 | Space between words (collapsible) | Yes |
| SWO-2 | Between CJK ideographs | Yes (unless `word-break: keep-all`) |
| SWO-3 | After hyphen (U+002D) | Yes |
| SWO-4 | At soft hyphen (U+00AD) | Yes (unless `hyphens: none`) |
| SWO-5 | At zero-width space (U+200B) / `<wbr>` | Yes |
| SWO-6 | Between atomic inline and adjacent content | Yes |
| SWO-7 | `text-wrap-mode: nowrap` | No soft wrapping |

**Word breaking:**

| # | Property + Value | Action |
|---|-----------------|--------|
| WB-1 | `overflow-wrap: normal` | Break only at allowed points |
| WB-2 | `overflow-wrap: break-word` | Emergency break if word overflows (doesn't affect min-content) |
| WB-3 | `overflow-wrap: anywhere` | Emergency break + affects min-content sizing |
| WB-4 | `word-break: normal` | Default rules per script |
| WB-5 | `word-break: break-all` | Break between any characters |
| WB-6 | `word-break: keep-all` | No breaks between CJK ideographs |
| WB-7 | `hyphens: none` | Never hyphenate (even at soft hyphens) |
| WB-8 | `hyphens: manual` (default) | Hyphenate only at soft hyphens |
| WB-9 | `hyphens: auto` | UA auto-hyphenation (requires `lang`) |

**Forced breaks:**

| # | Condition | Action |
|---|-----------|--------|
| FB-1 | `<br>` element | Force line break; empty `<br>` = strut height |
| FB-2 | Preserved newline | Force line break |
| FB-3 | Multiple `<br>` | Each produces strut-height line box |

### 7.8 Text Alignment

| # | Value | Action |
|---|-------|--------|
| TA-1 | `start` / `left` (LTR) | Flush to start edge |
| TA-2 | `end` / `right` (LTR) | Flush to end edge |
| TA-3 | `center` | Centered |
| TA-4 | `justify` | Distribute space between words (not last line) |
| TA-5 | `justify` with single word | Falls back to `start` |
| TA-6 | `text-align-last: auto` | Uses `text-align`; if `justify`, falls back to `start` |
| TA-7 | `text-align-last: justify` | Last line also justified |
| TA-8 | Line before forced break | Treated as "last line" for `text-align-last` |

### 7.9 Spacing

| # | Property | Action |
|---|----------|--------|
| SP-1 | `letter-spacing: <length>` | Added between every adjacent character pair |
| SP-2 | `word-spacing: <length>/<percentage>` | Added to inter-word space width |
| SP-3 | Both with `text-align: justify` | Justification adds space ON TOP of specified spacing |

---

## Part 8 — Flex Layout

Source: `flex_layout_algorithm.cc` (LayoutNG)
Spec: CSS Flexible Box Layout Module Level 1

### 8.1 Flex Container Setup

| # | Rule | Action |
|---|------|--------|
| FX-1 | `display: flex` / `inline-flex` | Establishes flex formatting context |
| FX-2 | Main axis | Determined by `flex-direction`: `row`/`row-reverse` = horizontal, `column`/`column-reverse` = vertical |
| FX-3 | Cross axis | Perpendicular to main axis |
| FX-4 | Direction | `row` follows `direction`; `row-reverse` reverses; `column` = top-to-bottom; `column-reverse` = bottom-to-top |

### 8.2 Flex Item Generation

| # | Condition | Action |
|---|-----------|--------|
| FI-1 | Direct child elements | Become flex items (blockified: `inline` → `block`, `inline-block` → `block`) |
| FI-2 | Contiguous text runs | Wrapped in anonymous flex items (only if non-whitespace) |
| FI-3 | Whitespace-only text | NOT flex items (collapsed) |
| FI-4 | `position: absolute/fixed` children | NOT flex items; positioned per abspos rules |
| FI-5 | `display: contents` children | Their children become flex items |

### 8.3 Flex Lines (Multi-Line: `flex-wrap`)

| # | Condition | Action |
|---|-----------|--------|
| FL-1 | `flex-wrap: nowrap` (default) | All items on one line, may overflow |
| FL-2 | `flex-wrap: wrap` | Items wrap to new lines when they exceed available main size |
| FL-3 | `flex-wrap: wrap-reverse` | Like `wrap` but cross-start and cross-end swapped |
| FL-4 | Line break trigger | When sum of item hypothetical main sizes exceeds available main size |
| FL-5 | Min main size | An item that is larger than the line by itself occupies the line alone |

### 8.4 Flex Sizing Algorithm (CSS Flexbox §9)

**§9.2 — Line Length Determination:**

| # | Step | Action |
|---|------|--------|
| FS-1 | Determine available main/cross space | From containing block, minus flex container margin/border/padding |
| FS-2 | Collect flex items per line | Per §9.3 wrapping rules or all on one line |

**§9.3 — Main Size Determination:**

| # | Step | Action |
|---|------|--------|
| FS-3 | Determine flex base size | `flex-basis` (if definite); else `width`/`height` of main axis; else content-based |
| FS-4 | `flex-basis: auto` | Falls back to main size property (`width`/`height`); if also `auto`, use content size |
| FS-5 | `flex-basis: content` | Use max-content size |
| FS-6 | Determine hypothetical main size | Clamp flex base size by `min-main-size` and `max-main-size` |
| FS-7 | Determine free space | Available main size − sum of hypothetical main sizes − gaps |
| FS-8 | Free space > 0 (grow) | Distribute to items with `flex-grow > 0` proportionally to their `flex-grow` values |
| FS-9 | Free space < 0 (shrink) | Remove from items with `flex-shrink > 0` proportionally to `flex-shrink × flex-base-size` |
| FS-10 | Freeze inflexible items | Items that would shrink below min size or grow above max size are frozen at their clamped size |
| FS-11 | Iterate | Recalculate free space among unfrozen items until all items are frozen |
| FS-12 | Min violation | Item's computed main size < `min-main-size` → clamp and freeze |
| FS-13 | Max violation | Item's computed main size > `max-main-size` → clamp and freeze |

**§9.4 — Cross Size Determination:**

| # | Step | Action |
|---|------|--------|
| FS-14 | Determine cross size of each item | If `align-self: stretch` and cross size is `auto`, stretch to line's cross size (minus margins) |
| FS-15 | Item has definite cross size | Use specified cross size |
| FS-16 | Item has intrinsic ratio | Cross size from main size and ratio |
| FS-17 | Otherwise | Use fit-content |
| FS-18 | Line cross size (single-line) | Max of all item cross sizes; if container has definite cross size, use that |
| FS-19 | Line cross size (multi-line) | Max of item cross sizes within each line |

**§9.5 — Main Axis Alignment (`justify-content`):**

| # | Value | Action |
|---|-------|--------|
| JC-1 | `flex-start` | Pack toward main-start |
| JC-2 | `flex-end` | Pack toward main-end |
| JC-3 | `center` | Center items on main axis |
| JC-4 | `space-between` | First item at start, last at end, equal gaps between |
| JC-5 | `space-around` | Equal space around each item (half-space at edges) |
| JC-6 | `space-evenly` | Equal space between all items and edges |
| JC-7 | Single item with `space-between` | Falls back to `flex-start` |
| JC-8 | Overflow alignment | `safe` keyword prevents data loss by falling back to `start` |

**§9.6 — Cross Axis Alignment:**

`align-items` (container default) / `align-self` (per-item override):

| # | Value | Action |
|---|-------|--------|
| AI-1 | `stretch` (default) | Item stretches to fill line cross size (if cross size is `auto`) |
| AI-2 | `flex-start` | Item placed at cross-start |
| AI-3 | `flex-end` | Item placed at cross-end |
| AI-4 | `center` | Centered on cross axis |
| AI-5 | `baseline` | Align baselines of items on the line |

`align-content` (multi-line cross axis distribution):

| # | Value | Action |
|---|-------|--------|
| AC-1 | `stretch` (default) | Lines stretch to fill cross size |
| AC-2 | `flex-start` | Pack lines to cross-start |
| AC-3 | `flex-end` | Pack lines to cross-end |
| AC-4 | `center` | Center lines |
| AC-5 | `space-between` | Distribute lines evenly |
| AC-6 | `space-around` | Equal space around lines |
| AC-7 | `space-evenly` | Equal space between lines and edges |

### 8.5 Flex Item Special Rules

| # | Rule | Action |
|---|------|--------|
| FX-SP-1 | Auto margins on main axis | Absorb remaining free space (overrides `justify-content`) |
| FX-SP-2 | Auto margins on cross axis | Center item on cross axis (overrides `align-self`) |
| FX-SP-3 | `order` property | Changes visual order without changing source order |
| FX-SP-4 | Min main size default | `min-width`/`min-height` default is `auto` for flex items (content-based minimum) |
| FX-SP-5 | `visibility: collapse` | Item is invisible but affects cross size |
| FX-SP-6 | Percentage sizing | Resolved against flex container size |
| FX-SP-7 | No margin collapsing | Flex item margins never collapse |
| FX-SP-8 | `z-index` on non-positioned flex items | Still creates stacking context (unlike normal flow) |
| FX-SP-9 | Aspect ratio preservation | Intrinsic ratios interact with flex sizing via transferred size suggestions |

### 8.6 Gap Properties

| # | Property | Action |
|---|----------|--------|
| FX-GAP-1 | `gap` / `row-gap` / `column-gap` | Fixed spacing between items (added to layout, not to items) |
| FX-GAP-2 | Gaps reduce free space | Gap total subtracted before flex grow/shrink distribution |
| FX-GAP-3 | Gaps are not at edges | Only between adjacent items |

---

## Part 9 — Grid Layout

Source: `grid_layout_algorithm.cc` (LayoutNG)
Spec: CSS Grid Layout Module Level 2

### 9.1 Grid Container Setup

| # | Rule | Action |
|---|------|--------|
| GR-1 | `display: grid` / `inline-grid` | Establishes grid formatting context |
| GR-2 | Explicit grid | Defined by `grid-template-columns` / `grid-template-rows` |
| GR-3 | Implicit grid | Auto-generated tracks for items placed outside explicit grid |
| GR-4 | `grid-auto-columns` / `grid-auto-rows` | Size of implicit tracks |
| GR-5 | `grid-auto-flow` | Placement direction (`row`/`column`) and packing (`dense`) |

### 9.2 Grid Item Placement

| # | Rule | Action |
|---|------|--------|
| GP-1 | Line-based placement | `grid-column-start/end`, `grid-row-start/end` specify grid lines |
| GP-2 | Area-based placement | `grid-area` references named areas from `grid-template-areas` |
| GP-3 | Auto-placement | Items without explicit placement are auto-placed |
| GP-4 | Auto-placement algorithm | Fill row-by-row (or column-by-column); `dense` backtracks to fill gaps |
| GP-5 | `span` keyword | Item spans multiple tracks |
| GP-6 | Named lines | Lines can be referenced by name |

### 9.3 Track Sizing Algorithm (CSS Grid §12)

| # | Step | Action |
|---|------|--------|
| TS-1 | Initialize track sizes | `<length>` → fixed. `auto` → min = `auto` (content), max = `auto`. `min-content`/`max-content` → intrinsic |
| TS-2 | `minmax(min, max)` | Track size between min and max |
| TS-3 | Resolve intrinsic sizes | For min-content: increase base sizes by items' min-content contributions. For max-content: by max-content contributions |
| TS-4 | Spanning items | Distribute space proportionally among spanned tracks |
| TS-5 | Maximize tracks | Grow tracks with free space up to their max track sizing |
| TS-6 | Expand `fr` tracks | Distribute remaining free space proportionally to `fr` values |
| TS-7 | `fr` minimum | `fr` tracks have a floor of their base size (from intrinsic sizing) |
| TS-8 | `auto` max → stretch | `auto` tracks stretch to fill remaining space after `fr` resolution |

### 9.4 Grid Alignment

| # | Property | Action |
|---|----------|--------|
| GA-1 | `justify-items` / `justify-self` | Align items within their grid area on inline axis |
| GA-2 | `align-items` / `align-self` | Align items within their grid area on block axis |
| GA-3 | `justify-content` | Align entire grid within container on inline axis |
| GA-4 | `align-content` | Align entire grid within container on block axis |
| GA-5 | All accept same values | `start`, `end`, `center`, `stretch`, `space-between`, `space-around`, `space-evenly` |

### 9.5 Grid Item Special Rules

| # | Rule | Action |
|---|------|--------|
| GR-SP-1 | No margin collapsing | Grid item margins never collapse |
| GR-SP-2 | Percentage sizing | Resolved against grid area size |
| GR-SP-3 | Auto margins | Absorb remaining space in grid area |
| GR-SP-4 | `z-index` on non-positioned grid items | Creates stacking context |
| GR-SP-5 | `order` property | Changes paint order |

### 9.6 Gap Properties

| # | Property | Action |
|---|----------|--------|
| GR-GAP-1 | `row-gap` / `column-gap` | Fixed spacing between tracks |
| GR-GAP-2 | Gaps act as fixed-size tracks | Inserted between explicit/implicit tracks |
| GR-GAP-3 | Gaps don't exist at grid edges | Only between adjacent tracks |

---

## Part 10 — Table Layout

Source: `table_layout_algorithm.cc` (LayoutNG)
Spec: CSS 2.1 §17, CSS Table Module Level 3

### 10.1 Table Structure

| # | Rule | Action |
|---|------|--------|
| TBL-1 | Required structure | `table` → `table-row-group` → `table-row` → `table-cell` |
| TBL-2 | Anonymous box generation | Missing wrappers auto-generated (e.g., cell outside row gets anonymous row) |
| TBL-3 | `display: table` | Generates table wrapper box + table grid box |
| TBL-4 | Caption | `table-caption` placed above or below table grid per `caption-side` |

### 10.2 Column Width Algorithm — Auto Layout (CSS2 17.5.2.2)

| # | Step | Action |
|---|------|--------|
| TW-1 | Calculate minimum content width per cell | Narrowest without overflow |
| TW-2 | Calculate maximum content width per cell | Widest with infinite space |
| TW-3 | Column minimum | Max of all cells' minimums in that column |
| TW-4 | Column maximum | Max of all cells' maximums in that column |
| TW-5 | Spanning cells | Distribute excess minimum/maximum proportionally among spanned columns |
| TW-6 | Table width determination | If `width: auto`: max(sum of column minimums, containing block width) but constrained by `max-width` |
| TW-7 | Distribute remaining space | Among columns proportionally (preference: percentage > auto > fixed) |

### 10.3 Column Width Algorithm — Fixed Layout (CSS2 17.5.2.1)

| # | Step | Action |
|---|------|--------|
| TF-1 | Use first row only | Column widths from first row's cells (or `<col>` elements) |
| TF-2 | Explicit widths | Cells/cols with explicit width → that width |
| TF-3 | Auto widths | Distribute remaining space equally among auto columns |
| TF-4 | Advantage | Faster — doesn't need to examine all rows |

### 10.4 Row Height

| # | Rule | Action |
|---|------|--------|
| TH-1 | Row height | Max of: explicit row height, all cells' computed heights in that row |
| TH-2 | Cell height `auto` | Content height |
| TH-3 | Vertical-align in cells | `top`, `middle`, `bottom`, `baseline` — positions content within cell |
| TH-4 | Rowspan | Cell spanning multiple rows — excess height distributed among spanned rows |

### 10.5 Border Model

| # | Condition | Action |
|---|-----------|--------|
| TB-1 | `border-collapse: separate` (default) | Each cell has own borders. `border-spacing` adds gaps between cells |
| TB-2 | `border-collapse: collapse` | Adjacent borders merge. Winner determined by: wider > style priority > dark > earlier |
| TB-3 | Border style priority | `hidden` > `double` > `solid` > `dashed` > `dotted` > `ridge` > `outset` > `groove` > `inset` > `none` |
| TB-4 | `border-spacing` | Horizontal and vertical spacing between cells (only in `separate` model) |
| TB-5 | `empty-cells` | `hide` hides borders/backgrounds on empty cells (only in `separate` model) |

### 10.6 Table Special Rules

| # | Rule | Action |
|---|------|--------|
| TBL-SP-1 | Table cells establish BFC | Each cell is an independent formatting context |
| TBL-SP-2 | Percentage heights in cells | Resolved against table row height (if definite) |
| TBL-SP-3 | `overflow` on table | Not directly applicable; table content determines size |
| TBL-SP-4 | No margin collapsing | Table cell margins don't collapse |
| TBL-SP-5 | Width: `table-layout: fixed` | Only first row examined for column widths |

---

## Part 11 — Chrome-Specific Implementation Details

### 11.1 LayoutNG Exclusion Space (Float Tracking)

| # | Rule | Action |
|---|------|--------|
| NG-EX-1 | Data structure | `ExclusionSpace` tracks all float regions in BFC |
| NG-EX-2 | Opportunity finding | Searches for first rectangular region that fits the element |

### 11.2 LayoutNG Block Algorithm Specifics

| # | Rule | Action |
|---|------|--------|
| NG-IF-1 | BFC offset resolution | First in-flow child forces resolution of block's BFC position |
| NG-IF-2 | Margin strut propagation | `MarginStrut` (positive + negative components) accumulated through children |
| NG-IF-3 | `PreviousInflowPosition` | Tracks current block offset + carried margin strut |
| NG-NFC-1 | BFC child float avoidance | Computes exclusion space, positions child in remaining available space |
| NG-NFC-2 | Iterative sizing | May re-layout BFC children as available width changes with float positions |

### 11.3 Fragmentation

| # | Rule | Action |
|---|------|--------|
| NG-FRAG-1 | `break-before`/`break-after` | `column`/`page`/`always` inserts forced break |
| NG-FRAG-2 | `break-inside: avoid` | Attempts to keep element in one fragment |
| NG-FRAG-3 | Orphans/widows | Minimum line count before/after break enforced |
| NG-FRAG-4 | Monolithic elements | Replaced elements, scroll containers not broken internally |

---

## Summary Statistics

| Category | Rule Count |
|----------|-----------|
| BFC establishment | 13 |
| Block vertical positioning | 4 |
| Width computation | ~20 |
| Height computation | ~15 |
| Margin collapsing | ~25 |
| Float placement | 9 + 6 interaction + 6 clear |
| Positioning (relative, absolute, fixed, sticky) | ~25 |
| Box sizing (content/border, min/max, intrinsic) | ~18 |
| Overflow | ~11 |
| Inline formatting | ~50 |
| White-space / line breaking | ~30 |
| Text alignment / spacing | ~11 |
| Flex layout | ~45 |
| Grid layout | ~25 |
| Table layout | ~20 |
| Chrome-specific / fragmentation | ~10 |
| **TOTAL** | **~343 rules** |

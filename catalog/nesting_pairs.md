# HTML Element Nesting Pairs — Browser Behavior Catalog

This document catalogs every valid and notable invalid parent-child element nesting pair, and how Chrome and Firefox handle each case. This is the empirical input for the pairwise context transition engine.

## How This Document Is Organized

Elements are grouped into **nesting categories** based on their content model (what children they accept). For each parent category, every possible child category is listed with:

- **Valid?** — whether the spec allows this nesting
- **Browser behavior** — what Chrome/Firefox actually do (layout + parser)
- **Pairwise note** — what the transition engine needs to know

---

## Element-to-Category Mapping

### Category A — Void Elements (cannot be parents)

`area`, `base`, `br`, `col`, `embed`, `hr`, `img`, `input`, `link`, `meta`, `param`, `source`, `track`, `wbr`

These elements have no children. The parser ignores any content placed inside them. No nesting pairs originate from these as parents.

### Category B — Raw Text / Metadata (text-only content)

`title`, `style`, `script`, `textarea`, `option`, `rp`

These elements contain only raw text (no child elements parsed). The parser treats all markup inside them as text.

### Category C — Phrasing Content Containers

`abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `dfn`, `em`, `i`, `kbd`, `label`, `mark`, `output`, `q`, `rt`, `s`, `samp`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`

Accept phrasing content (inline elements + text). Cannot directly contain block elements.

### Category D — Phrasing Containers (Block Display)

`h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `p`, `pre`

Accept phrasing content only, but render as block-level. A `<div>` inside a `<p>` triggers auto-close.

### Category E — Flow Content Containers

`article`, `aside`, `blockquote`, `body`, `dd`, `div`, `figcaption`, `figure`, `footer`, `header`, `li`, `main`, `nav`, `search`, `section`, `address`, `dialog`, `details`, `fieldset`, `form`, `td`, `th`, `dt`, `legend`, `summary`, `caption`, `noscript`

Accept flow content (both block and inline elements). These are the most permissive parents.

### Category F — Transparent Content Model

`a`, `ins`, `del`, `map`, `canvas`, `object`, `slot`

Transparent means: inherits the content model of its parent. If `<a>` is inside a flow container, it can contain flow. If inside a phrasing container, it can only contain phrasing.

### Category G — List Containers

`ul`, `ol`, `menu` — accept only `li` (+ `script`, `template`)
`dl` — accepts only `dt`, `dd`, `div` (+ `script`, `template`)

### Category H — Table Containers

`table` — accepts `caption`, `colgroup`, `thead`, `tbody`, `tfoot`, `tr`
`thead`, `tbody`, `tfoot` — accept only `tr` (+ `script`, `template`)
`tr` — accepts only `td`, `th` (+ `script`, `template`)
`colgroup` — accepts only `col`

### Category I — Select/Option Containers

`select` — accepts `option`, `optgroup`, `hr`
`optgroup` — accepts only `option`
`datalist` — accepts `option` + phrasing

### Category J — Media Containers

`video`, `audio` — accept `source`, `track`, then transparent content
`picture` — accepts `source` elements then one `img`

### Category K — Special Containers

`html` — accepts `head` then `body` only
`head` — accepts metadata content (`title`, `meta`, `link`, `style`, `script`, `base`, `noscript`)
`ruby` — accepts phrasing + `rt` + `rp`
`hgroup` — accepts `h1`–`h6` + `p`
`template` — accepts anything (inert, not rendered)
`button` — accepts phrasing (no interactive descendants)
`svg` — accepts SVG namespace elements
`math` — accepts MathML namespace elements

---

## Nesting Pair Interactions

### 1. Flow Container (E) as Parent

**Parent elements:** `div`, `article`, `section`, `nav`, `aside`, `main`, `body`, `blockquote`, `dd`, `li`, `td`, `th`, `figcaption`, `figure`, `footer`, `header`, `form`, `fieldset`, `details`, `dialog`, `search`, `address`, `dt`, `legend`, `summary`, `caption`, `noscript`

#### 1.1 Flow (E) → Block Child (E, D, G, H)

- `div > div`, `section > article`, `li > blockquote`, `td > div`, etc.
- **Valid:** Yes
- **Chrome/Firefox:** Block child generates a block-level box. Stacks vertically in normal flow. Margins collapse with parent and siblings per CSS 2.1 §8.3.1. If parent has padding or border, margin collapse is blocked.
- **Pairwise note:** Formatting context remains `block`. Containing block is the parent's content box. Margin collapsing rules apply — context must carry a `margin_collapse_through` flag.

#### 1.2 Flow (E) → Inline Child (C, F)

- `div > span`, `section > em`, `li > a`, `td > strong`, etc.
- **Valid:** Yes
- **Chrome/Firefox:** Inline child participates in an anonymous inline formatting context. If block and inline siblings coexist, Chrome/Firefox generate anonymous block boxes around consecutive inline runs (CSS 2.1 §9.2.1.1).
- **Pairwise note:** Transition creates an inline formatting context inside the block. Engine must handle anonymous box generation when mixed block/inline children exist.

#### 1.3 Flow (E) → Heading Child (D)

- `div > h1`, `section > h3`, `article > h2`
- **Valid:** Yes
- **Chrome/Firefox:** Heading renders as a block box with UA margins and font-size. Participates in normal block flow. Outline algorithm uses headings for section structure.
- **Pairwise note:** Same as 1.1 (block in block). UA stylesheet overrides (font-size, weight, margins) are inherited property changes in the context.

#### 1.4 Flow (E) → Paragraph (D: `p`)

- `div > p`, `section > p`, `li > p`
- **Valid:** Yes
- **Chrome/Firefox:** `<p>` renders as block with UA margins (1em top/bottom). Key parser behavior: `<p>` auto-closes when it encounters another block-level start tag.
- **Pairwise note:** Standard block-in-block. The auto-close behavior is parser-level, not layout-level.

#### 1.5 Flow (E) → List Container (G)

- `div > ul`, `section > ol`, `td > dl`
- **Valid:** Yes
- **Chrome/Firefox:** List renders as block with UA padding-inline-start (40px). Nested lists suppress top/bottom margins. Chrome/Firefox both apply `margin-block-start: 0; margin-block-end: 0` on nested `<ul>/<ol>` (child of `<li>`).
- **Pairwise note:** Standard block-in-block. Nested list margin suppression is a UA stylesheet rule, not a layout algorithm change.

#### 1.6 Flow (E) → Table (H: `table`)

- `div > table`, `section > table`, `td > table`
- **Valid:** Yes
- **Chrome/Firefox:** Table establishes a **table formatting context**. The table box is a block-level box with `display: table`. Chrome/Firefox generate anonymous table parts if required (missing `tbody`, etc.). Table width algorithm is independent (fixed or auto). Table cells establish new BFCs.
- **Pairwise note:** **Context transition from block → table.** New formatting context type. Containing block changes to the table's content box for internal elements. This is a major pairwise transition.

#### 1.7 Flow (E) → Form Elements

- `div > input`, `div > button`, `div > select`, `div > textarea`
- **Valid:** Yes
- **Chrome/Firefox:** Form controls are replaced elements (or behave like them). `input`, `select`, `textarea` render as `inline-block`. `button` renders as `inline-block` and establishes a new BFC for its contents. Sizing depends on the control type and platform.
- **Pairwise note:** Replaced elements have intrinsic dimensions. The context transition notes that the child is replaced and uses its intrinsic size rather than flowing content.

#### 1.8 Flow (E) → Embedded Content (`img`, `video`, `iframe`, `canvas`, `svg`)

- `div > img`, `section > video`, `td > iframe`
- **Valid:** Yes
- **Chrome/Firefox:** These are inline-level replaced elements. `img` uses intrinsic dimensions. `iframe` defaults to 300×150. `svg` establishes an SVG formatting context. `canvas` defaults to 300×150.
- **Pairwise note:** Replaced element handling. Child has intrinsic aspect ratio. `svg` and `math` are special — they establish entirely different formatting contexts (SVG/MathML).

#### 1.9 Flow (E) → Void Element

- `div > br`, `div > hr`, `p > br`
- **Valid:** Yes
- **Chrome/Firefox:** `br` inserts a line break in inline flow. `hr` generates a block-level box with UA border/margins. `br` is special — it's inline but forces a newline.
- **Pairwise note:** `br` and `hr` are special cases. `br` affects inline layout. `hr` is a block box.

#### 1.10 Flow (E) → Invalid: Another `<form>` inside `<form>`

- `form > form`
- **Valid:** No (spec forbids nested forms)
- **Chrome/Firefox:** **Parser drops the inner `<form>` tag entirely.** The inner form's children become children of the outer form. The inner `</form>` tag closes nothing (it's already been ignored).
- **Pairwise note:** Parser-level restriction, not layout. Engine parser must track open form elements and reject nested ones.

#### 1.11 Flow (E) → `template`

- `div > template`
- **Valid:** Yes
- **Chrome/Firefox:** `template` content is parsed into a DocumentFragment but not rendered. `display: none` by default. The content exists in the DOM but generates no boxes.
- **Pairwise note:** No layout impact. Template children are inert.

#### 1.12 Flow (E) → `script`, `style`

- `div > script`, `div > style`
- **Valid:** Yes
- **Chrome/Firefox:** `display: none`. No boxes generated. Content is text (CSS or JavaScript). In Pane, JavaScript is disabled by default, so `script` is always inert.
- **Pairwise note:** No layout impact. Skip in layout tree.

---

### 2. Phrasing Container — Block Display (D) as Parent

**Parent elements:** `p`, `h1`–`h6`, `pre`

These accept **only phrasing content** (inline elements + text). Block elements are NOT valid children.

#### 2.1 Phrasing-Block (D) → Inline Child (C)

- `p > span`, `h1 > em`, `pre > code`
- **Valid:** Yes
- **Chrome/Firefox:** Normal inline layout. Text and inline elements flow left-to-right (or per direction), line-wrap when they hit the containing block edge.
- **Pairwise note:** Standard phrasing-in-phrasing. No context change.

#### 2.2 Phrasing-Block (D) → Inline Replaced

- `p > img`, `h1 > br`, `p > input`
- **Valid:** Yes
- **Chrome/Firefox:** Replaced element participates in inline flow. `img` uses intrinsic dimensions. `input` renders as inline-block. `br` forces a line break.
- **Pairwise note:** Same inline formatting context. Replaced element has intrinsic size.

#### 2.3 Phrasing-Block (D) → **INVALID: Block Child**

- `p > div`, `p > blockquote`, `p > ul`, `p > table`, `p > h2`, `p > p`
- **Valid:** No
- **Chrome/Firefox:** **Parser auto-closes the `<p>` tag.** When the parser encounters a block-level start tag inside `<p>`, it implicitly closes the `<p>` first, then opens the block element as a sibling.

  Example: `<p>Hello <div>World</div></p>`
  Parsed as: `<p>Hello</p> <div>World</div> <p></p>`

  Chrome and Firefox agree on this behavior. The trailing `</p>` creates an empty `<p>` after the div (or is ignored if no content follows).

- **Pairwise note:** **Critical parser rule.** The engine parser must know which elements auto-close `<p>`. The full list: `address`, `article`, `aside`, `blockquote`, `center`, `details`, `dialog`, `dir`, `div`, `dl`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`–`h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `table`, `ul`.

#### 2.4 Heading (D) → **INVALID: Another Heading**

- `h1 > h2`, `h2 > h3`
- **Valid:** No (headings accept phrasing only, headings are flow)
- **Chrome/Firefox:** **Parser auto-closes the first heading.** `<h1>foo <h2>bar</h2></h1>` becomes `<h1>foo</h1> <h2>bar</h2>`.
- **Pairwise note:** Same auto-close mechanism as `<p>`. Headings cannot nest.

---

### 3. Phrasing Container — Inline Display (C) as Parent

**Parent elements:** `span`, `em`, `strong`, `b`, `i`, `u`, `s`, `small`, `code`, `var`, `samp`, `kbd`, `sub`, `sup`, `abbr`, `bdi`, `bdo`, `cite`, `data`, `dfn`, `mark`, `q`, `time`, `output`, `label`

These accept phrasing content. They render as inline boxes.

#### 3.1 Inline (C) → Inline Child (C)

- `span > em`, `strong > code`, `em > span`
- **Valid:** Yes
- **Chrome/Firefox:** Nested inline boxes. Each creates an inline box. They participate in the same inline formatting context. CSS properties like `font-style`, `font-weight`, `color` cascade into children.
- **Pairwise note:** No context change. Inherited properties update in the context.

#### 3.2 Inline (C) → Inline Replaced

- `span > img`, `em > br`, `label > input`
- **Valid:** Yes
- **Chrome/Firefox:** Replaced element inline with text. `img` has intrinsic size. `input` renders as inline-block (creates a new BFC for its contents).
- **Pairwise note:** Inline context. Replaced elements get special sizing.

#### 3.3 Inline (C) → **INVALID: Block Child**

- `span > div`, `em > p`, `code > blockquote`
- **Valid:** No
- **Chrome/Firefox:** **Unlike `<p>`, inline elements do NOT auto-close.** The parser accepts the block element as a child. Chrome/Firefox then split the inline box around the block:

  `<span>Hello <div>World</div> Bye</span>` renders as:
  ```
  <span>Hello </span>
  <div>World</div>
  <span> Bye</span>
  ```

  The inline is split into fragments before and after the block. This is defined in CSS 2.1 §9.2.1.1 as "anonymous block boxes" wrapping the inline fragments.

- **Pairwise note:** **Major pairwise transition.** When a block appears inside an inline, the engine must:
  1. Close the current inline formatting context
  2. Generate anonymous block boxes around inline fragments
  3. Render the block child in normal block flow
  4. Resume the inline formatting context after the block
  This is one of the most complex interactions in layout.

---

### 4. Transparent Elements (F) as Parent

**Parent elements:** `a`, `ins`, `del`, `map`, `canvas`, `object`, `slot`

Transparent elements inherit the content model of their parent.

#### 4.1 Transparent in Flow → Block Child

- `<div><a href="#"><div>...</div></a></div>`
- **Valid:** Yes (because `<a>` is in flow context, so it inherits flow content model)
- **Chrome/Firefox:** The `<a>` wraps a block-level child. Chrome/Firefox render the `<a>` as a block-level anonymous box around the `<div>`. The link is clickable on the entire block.
- **Pairwise note:** Transparent element inherits parent's formatting context. The context object passes through unchanged.

#### 4.2 Transparent in Phrasing → Block Child

- `<p><a href="#"><div>...</div></a></p>`
- **Valid:** No (because `<a>` inherits phrasing-only from `<p>`)
- **Chrome/Firefox:** `<p>` auto-closes when `<div>` is encountered. Parser reconstructs as: `<p><a href="#"></a></p><div><a href="#">...</a></div><p></p>`. The `<a>` is split across the paragraph boundary.
- **Pairwise note:** Parser-level. The transparent element doesn't override its parent's content model.

#### 4.3 `<a>` → **INVALID: Interactive Child**

- `a > a`, `a > button`, `a > select`, `a > textarea`, `a > details`
- **Valid:** No (no interactive descendants allowed)
- **Chrome/Firefox:** Chrome: the inner interactive element is extracted from the `<a>` — the `<a>` is split around it. Firefox: similar reconstruction. The inner `<a>` is ignored and its content becomes part of the outer `<a>`.
- **Pairwise note:** Parser-level restriction. Track "active interactive ancestor" in parser state.

---

### 5. List Containers (G) as Parent

#### 5.1 `ul`/`ol`/`menu` → `li`

- **Valid:** Yes (this is the only valid child element)
- **Chrome/Firefox:** `<li>` renders as `display: list-item`, which creates a principal block box plus a marker box (bullet/number). Marker is positioned per `list-style-position` (inside or outside).
- **Pairwise note:** Context transition: `block → list-item`. The child context needs `list-style-type` and `list-style-position` from inherited properties. Marker box generation is child-side logic.

#### 5.2 `ul`/`ol` → **INVALID: Non-`li` Children**

- `ul > div`, `ol > p`, `ul > span`
- **Valid:** No
- **Chrome/Firefox:** **Parser auto-generates an anonymous `<li>` wrapper.** The non-`li` content is placed inside an implicit `<li>`. In practice, Chrome wraps orphaned content in the list into an anonymous box. Firefox behaves similarly.

  `<ul><div>text</div></ul>` → Chrome treats the `<div>` as if inside an anonymous `<li>`.

- **Pairwise note:** Parser error recovery. Engine should auto-wrap non-`li` children.

#### 5.3 `dl` → `dt`/`dd`

- **Valid:** Yes
- **Chrome/Firefox:** `<dt>` renders as block (no indentation). `<dd>` renders as block with `margin-inline-start: 40px`. They stack vertically in normal flow.
- **Pairwise note:** Standard block-in-block. UA stylesheet provides the indentation.

#### 5.4 `li` → Flow Content

- `li > div`, `li > p`, `li > ul` (nested list), `li > table`
- **Valid:** Yes (li accepts flow)
- **Chrome/Firefox:** Normal flow layout inside the list item. Nested lists (`li > ul`) get special UA styling: nested lists have `margin-block: 0` (no extra vertical spacing), and `list-style-type` changes by nesting depth (disc → circle → square for `<ul>`).
- **Pairwise note:** Li is a flow container. Nested list styling is UA stylesheet, not layout algorithm. Nesting depth affects `list-style-type` — this can be tracked in inherited properties.

---

### 6. Table Containers (H) as Parent

Table layout has the strictest nesting rules in HTML. The table model requires a specific hierarchy.

#### 6.1 Required Table Hierarchy

```
table
├── caption           (optional, first child)
├── colgroup          (optional)
│   └── col
├── thead             (optional)
│   └── tr
│       ├── th
│       └── td
├── tbody             (one or more, auto-generated if missing)
│   └── tr
│       ├── th
│       └── td
└── tfoot             (optional)
    └── tr
        ├── th
        └── td
```

#### 6.2 `table` → `caption`

- **Valid:** Yes (as first child)
- **Chrome/Firefox:** Caption renders as `display: table-caption`, positioned above or below the table per `caption-side`. Caption establishes a block formatting context for its contents.
- **Pairwise note:** Context transition: `table → table-caption`. Containing block width is the table's border box width.

#### 6.3 `table` → `thead`/`tbody`/`tfoot`

- **Valid:** Yes
- **Chrome/Firefox:** Row groups render as `display: table-*-group`. They don't generate visible boxes themselves — they group rows. `border-collapse: collapse` changes border rendering between groups.
- **Pairwise note:** Context stays in table. Row groups are structural groupings.

#### 6.4 `table` → **Missing `tbody`**

- `<table><tr>...</tr></table>` (no explicit tbody)
- **Valid:** Yes (implied)
- **Chrome/Firefox:** **Parser auto-inserts `<tbody>`.** All `<tr>` elements not inside `thead`/`tfoot` are wrapped in an anonymous `<tbody>`.
- **Pairwise note:** Parser-level. Always normalize to the full table hierarchy.

#### 6.5 `tr` → `td`/`th`

- **Valid:** Yes (only valid children)
- **Chrome/Firefox:** Cells render as `display: table-cell`. Each cell **establishes a new block formatting context** (CSS 2.1 §17.5.4). Cells size according to the table layout algorithm (fixed or automatic). `colspan`/`rowspan` attributes create spanning cells.
- **Pairwise note:** **Major context transition: table-row → table-cell (new BFC).** The cell's containing block dimensions come from the table layout algorithm, not from parent width propagation. This is one of the most complex transitions.

#### 6.6 `td`/`th` → Flow Content

- `td > div`, `td > p`, `td > table` (nested table)
- **Valid:** Yes (cells accept flow)
- **Chrome/Firefox:** Normal flow layout inside the cell's new BFC. Nested tables are fully supported — the inner table gets its own table formatting context. Percentage widths on children resolve against the cell's computed width.
- **Pairwise note:** Cell is a flow container with a new BFC. Context transition back to block formatting. Containing block is the cell's content area.

#### 6.7 `table` → **INVALID: Direct text or non-table elements**

- `<table>some text</table>`, `<table><div>x</div></table>`
- **Valid:** No
- **Chrome/Firefox:** **Table foster parenting.** Text and non-table elements that appear directly in `table`, `thead`, `tbody`, `tfoot`, or `tr` are "foster parented" — moved BEFORE the table in the DOM. This is defined in the HTML parsing spec §13.2.6.1.

  `<table><div>x</div><tr><td>y</td></tr></table>` renders as:
  `<div>x</div> <table><tbody><tr><td>y</td></tr></tbody></table>`

- **Pairwise note:** **Critical parser rule.** Foster parenting is one of the most counterintuitive behaviors. The engine parser must implement foster parenting for all non-table content found in table contexts.

#### 6.8 Table Cells → **Invalid in wrong table position**

- `<table><td>...</td></table>` (td without tr)
- **Valid:** No (implied)
- **Chrome/Firefox:** **Parser auto-generates `<tbody>` and `<tr>`.** The cell is wrapped: `<table><tbody><tr><td>...</td></tr></tbody></table>`.
- **Pairwise note:** Parser auto-generation. Normalize table structure.

---

### 7. Form Element Nesting

#### 7.1 `form` → Flow Content

- `form > div`, `form > p`, `form > input`, `form > fieldset`
- **Valid:** Yes (form accepts flow)
- **Chrome/Firefox:** Normal flow layout. `<form>` is a block-level container. It does not establish a new BFC (unlike `fieldset`).
- **Pairwise note:** `form` is a standard flow container. No special layout behavior.

#### 7.2 `fieldset` → `legend` (first child) + Flow

- **Valid:** Yes
- **Chrome/Firefox:** `<legend>` renders as a special block that overlaps the fieldset's border. The legend box is positioned at the block-start edge of the fieldset, with the fieldset's border going around/behind it. Content after `legend` flows in the fieldset's content area.
- **Pairwise note:** Fieldset establishes a new BFC. Legend has special positioning rules — it's not normal flow. This is a unique pairwise transition that requires fieldset-specific layout logic.

#### 7.3 `button` → Phrasing Content

- `button > span`, `button > img`, `button > em`
- **Valid:** Yes (phrasing, no interactive descendants)
- **Chrome/Firefox:** Button establishes a new BFC for its contents. Children are centered (per UA stylesheet). Button is a replaced-like element with special sizing.
- **Pairwise note:** Context transition: `inline-block` parent with new BFC. Children lay out in block flow inside the button's content area.

#### 7.4 `select` → `option`/`optgroup`

- **Valid:** Yes (only valid children)
- **Chrome/Firefox:** Select is a replaced element — its rendering is platform-native (OS dropdown). `<option>` elements are not laid out by CSS — they appear in the dropdown menu. `<optgroup>` creates a labeled group in the dropdown.
- **Pairwise note:** **No CSS layout applies.** Select/option rendering is platform-native. The engine must handle these as replaced elements with special UI, not via the pairwise layout engine.

#### 7.5 `label` → Phrasing (no nested label)

- `label > span`, `label > input`
- **Valid:** Yes
- **Chrome/Firefox:** Normal inline layout. `<label>` renders as inline. Clicking the label focuses its associated control. Nested `<label>` is invalid — Chrome ignores the inner label tag.
- **Pairwise note:** Standard inline container. No layout-level difference from `<span>`.

---

### 8. Media Container Nesting

#### 8.1 `video`/`audio` → `source`, `track`, then Transparent

- `video > source`, `video > track`, `video > div` (fallback)
- **Valid:** Yes
- **Chrome/Firefox:** `<source>` and `<track>` are void/hidden. Transparent fallback content is rendered only if the media element is unsupported. When the media renders, fallback content is hidden.
- **Pairwise note:** Media elements are replaced. If supported, children are invisible. If unsupported, transparent content model applies (inherits parent's model).

#### 8.2 `picture` → `source` + `img`

- **Valid:** Yes
- **Chrome/Firefox:** `<source>` elements provide responsive image candidates. The `<img>` is the actual rendered element. Picture itself generates no box — it's a wrapper for responsive image selection.
- **Pairwise note:** `picture` is effectively transparent for layout. The `img` inside it is what generates the box.

---

### 9. Special Container Nesting

#### 9.1 `html` → `head` + `body`

- **Valid:** Yes (only valid structure)
- **Chrome/Firefox:** `<html>` is the root element. `<head>` is `display: none`. `<body>` establishes the initial containing block with 8px UA margin. All visible content is in `<body>`.
- **Pairwise note:** Root context. The initial containing block dimensions come from the viewport.

#### 9.2 `details` → `summary` (first child) + Flow

- **Valid:** Yes
- **Chrome/Firefox:** `<summary>` renders as `display: list-item` with a disclosure triangle marker. When `details` lacks the `open` attribute, only `<summary>` is visible — all other children are hidden (`display: none` equivalent, but actually content-visibility based in modern browsers). When `open`, all children render in normal flow after the summary.
- **Pairwise note:** Details is a flow container. The `open` attribute controls visibility of non-summary children. Summary has special marker rendering (disclosure triangle).

#### 9.3 `ruby` → Text + `rt` + `rp`

- **Valid:** Yes
- **Chrome/Firefox:** Ruby base text renders inline. `<rt>` renders as `display: ruby-text` — small annotation text positioned above (or beside) the base. `<rp>` content is hidden (provides fallback parentheses for non-ruby-aware agents).
- **Pairwise note:** Context transition to **ruby formatting context.** Ruby has its own layout algorithm. The context must switch to ruby mode.

#### 9.4 `svg` → SVG Elements

- `svg > rect`, `svg > circle`, `svg > path`, `svg > g`, `svg > text`
- **Valid:** Yes (SVG namespace)
- **Chrome/Firefox:** Entirely different layout model. SVG uses a coordinate-based system, not CSS box model. Elements are positioned by `x`/`y`/`cx`/`cy` attributes. CSS properties like `fill`, `stroke` apply. `transform` uses SVG transform syntax.
- **Pairwise note:** **Major context transition: CSS → SVG formatting context.** This is a completely different layout algorithm. The pairwise context must carry SVG-specific state (viewBox, coordinate system, current transform).

#### 9.5 `math` → MathML Elements

- `math > mrow`, `math > mfrac`, `math > msup`
- **Valid:** Yes (MathML namespace)
- **Chrome/Firefox:** MathML has its own layout algorithm for mathematical notation. Chrome and Firefox both support MathML Core (Chrome since 109, Firefox much earlier).
- **Pairwise note:** **Major context transition: CSS → MathML formatting context.** Like SVG, this is a completely separate layout system.

#### 9.6 `iframe` → (nested browsing context)

- **Valid:** Content is ignored
- **Chrome/Firefox:** iframe is a replaced element (300×150 default). It creates a **nested browsing context** with its own document. No parent-child layout interaction — the iframe's content is independent.
- **Pairwise note:** Replaced element with fixed/specified dimensions. No layout interaction with iframe contents.

---

### 10. Cross-Category Problematic Pairs

These are the pairs where Chrome/Firefox behavior is complex or counterintuitive.

#### 10.1 Block inside Inline (the "anonymous block" problem)

- `span > div`, `em > blockquote`, `a > div` (when a is in inline context)
- **Browser behavior:** Inline element is split. Anonymous block boxes are generated around the inline fragments. The block child interrupts the inline flow.
- **Engine complexity:** HIGH. This requires:
  1. Breaking the inline box at the block child boundary
  2. Generating anonymous block boxes for the pre-block and post-block inline content
  3. Maintaining inline formatting state (decorations, etc.) across the split
- **Pairwise note:** This is the #1 most complex pairwise interaction for layout.

#### 10.2 Table foster parenting

- `table > div`, `table > "text"`, `tr > span`
- **Browser behavior:** Non-table content is moved before the table (foster parenting).
- **Engine complexity:** MEDIUM. Parser must detect table context and reparent nodes.
- **Pairwise note:** Parser-level only. No layout complexity once the DOM is correct.

#### 10.3 `<p>` auto-closing cascade

- `p > div`, `p > p`, `p > ul`, `p > table`
- **Browser behavior:** Parser closes `<p>`, inserts block element as sibling.
- **Engine complexity:** LOW (parser rule). But interacts with transparent elements: `<p><a><div>` triggers complex reconstruction.
- **Pairwise note:** Maintain a list of elements that close `<p>`. Handle interaction with active formatting elements (like `<a>`, `<b>`, etc. that are reconstructed across the boundary).

#### 10.4 Formatting element reconstruction

- `<p><b>Hello <div>World</div></b></p>`
- **Browser behavior:** `<b>` is an active formatting element. When `<p>` auto-closes at `<div>`, the parser reconstructs `<b>` inside the new context:
  Result: `<p><b>Hello </b></p><div><b>World</b></div>`
- **Engine complexity:** MEDIUM. Parser must maintain an active formatting element list and reconstruct them when the tree is split.
- **Pairwise note:** Parser-level. The "adoption agency algorithm" (HTML spec §13.2.6.4.7) handles this.

#### 10.5 `display: flex`/`display: grid` on any element

- Any element with `display: flex` or `display: grid` applied via CSS
- **Browser behavior:** Establishes a new formatting context. All children become flex/grid items. Anonymous flex/grid items are generated for text nodes. Some child properties change meaning (float is ignored on flex items, vertical-align is ignored on flex items, etc.).
- **Engine complexity:** HIGH. Flex and grid have their own layout algorithms.
- **Pairwise note:** This is a **CSS-driven context transition**, not an HTML nesting one. The pairwise transition function must check `display` computed value to determine the formatting context, not just the element tag.

#### 10.6 `position: absolute`/`fixed` on any child

- Any child with `position: absolute` or `position: fixed`
- **Browser behavior:** Child is taken out of normal flow. Its containing block is the nearest positioned ancestor (for absolute) or the viewport (for fixed, unless an ancestor has transform/filter/will-change). The child does not affect the parent's size.
- **Engine complexity:** MEDIUM. Requires containing block resolution up the tree.
- **Pairwise note:** The pairwise context carries `positioned_containing_block` and `fixed_containing_block` fields. Out-of-flow children are laid out after in-flow children.

---

## Summary Statistics

| Category | Valid Pairs (approx) | Notes |
|---|---|---|
| Flow → Any Child | ~2,800 | Most permissive; standard block/inline flow |
| Phrasing → Phrasing | ~800 | Inline formatting; no context change |
| Phrasing-Block → Invalid Block | ~300 | Auto-close in parser; no layout handling |
| Inline → Invalid Block | ~300 | Anonymous block generation; complex layout |
| Table Hierarchy | ~50 | Strict structure; auto-generation + foster parenting |
| List → li only | ~10 | Auto-wrap non-li children |
| Select/Option | ~10 | Platform-native rendering; no CSS layout |
| Transparent delegation | ~500 | Inherits parent behavior; pass-through |
| Media containers | ~30 | Replaced elements; fallback content |
| SVG/MathML children | (separate spec) | Different layout algorithm entirely |
| **Total meaningful pairs** | **~4,900** | |

### Pairs Requiring Special Engine Handling

1. **Block in inline** — anonymous block generation (~300 pairs)
2. **Table foster parenting** — parser reparenting (~100 pairs)
3. **`<p>` auto-close** — parser closes p on block element (~200 pairs)
4. **Formatting element reconstruction** — adoption agency algorithm (~200 pairs)
5. **Table auto-generation** — missing tbody/tr/td insertion (~50 pairs)
6. **Fieldset + legend** — special legend positioning (1 pair)
7. **Ruby layout** — ruby formatting context (~10 pairs)
8. **SVG context switch** — CSS → SVG layout (~50+ pairs, separate spec)
9. **MathML context switch** — CSS → MathML layout (~30+ pairs, separate spec)
10. **CSS-driven context changes** — flex, grid, float, position (~all pairs, property-dependent)

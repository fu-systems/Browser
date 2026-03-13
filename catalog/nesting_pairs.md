# HTML Element Nesting Pairs — Exhaustive Pair Rule Catalog

Research document for deriving the minimal chained-pair rule set for the Pane parser.

Every non-void HTML element is listed as a parent. For each parent, every possible child element is classified into an **action** — the exact thing Chrome/Firefox do when that child tag is encountered inside that parent. The goal: reduce ~14,000 raw pairs into a small set of **pair rules** that chain.

---

## Pair Rule Actions (the complete set)

Every parent→child pair resolves to exactly one of these actions:

| # | Action | Code | Description |
|---|--------|------|-------------|
| 1 | **Accept** | `ACCEPT` | Child is valid. Parser inserts it as a child node. |
| 2 | **Auto-close parent** | `CLOSE_PARENT` | Parent is implicitly closed first, child becomes a sibling of the (now-closed) parent. |
| 3 | **Foster parent** | `FOSTER` | Child is moved before the nearest table ancestor. Table-context only. |
| 4 | **Auto-generate wrapper** | `WRAP(x)` | Missing intermediate element(s) `x` are auto-generated. Child is inserted inside the generated wrapper. |
| 5 | **Drop tag** | `DROP` | The child's start tag is ignored entirely. Its content (if any) is adopted by the current parent. |
| 6 | **Reconstruct formatting** | `RECONSTRUCT` | Active formatting elements (b, i, em, strong, a, etc.) are reconstructed in the new insertion context after a tree split. |
| 7 | **Close-and-reopen** | `CLOSE_REOPEN` | Current element of same type is closed, new one opens (e.g., `<li>` inside `<li>`). |
| 8 | **Raw text mode** | `RAW_TEXT` | Parser switches to raw text / RCDATA mode. No child elements parsed — everything until the end tag is text. |
| 9 | **Foreign content** | `FOREIGN` | Switches to SVG or MathML parsing mode. |
| 10 | **Void — no children** | `VOID` | Parent is void. Child is impossible — parser never enters this state. |

---

## Part 1 — Every Element as Parent (Exhaustive)

For each parent, children are grouped by action. "All remaining" means every element not listed in a specific action group.

---

### `html`

**Content model:** `head` then `body`
**Insertion mode:** BeforeHead / AfterHead

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | `head`, `body` | Only valid direct children. `head` must come first, `body` second. |
| WRAP(head) | Any tag before `head` is seen | Parser auto-generates `<head>`, processes tag in "in head" mode. |
| WRAP(body) | Any tag after `head` closes | Parser auto-generates `<body>`, processes tag in "in body" mode. |

**Chained pair rule:** `html` never appears mid-document. Its behavior is bootstrapping — generates `head`/`body` wrappers. Not relevant for chained pair analysis.

---

### `head`

**Content model:** Metadata content only
**Insertion mode:** InHead

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | `title`, `meta`, `link`, `style`, `script`, `noscript`, `base`, `template` | Valid metadata children. Parsed normally. |
| CLOSE_PARENT | `body`, `html`, `br` | Encountering these closes `<head>` implicitly, parser switches to AfterHead/InBody mode. `br` is special — treated as if `</head>` was seen. |
| CLOSE_PARENT | Any flow/phrasing element (`div`, `p`, `span`, `h1`, `a`, etc.) | Parser implicitly closes `<head>`, opens `<body>`, and re-processes the tag in body mode. |
| DROP | `head` (duplicate) | Second `<head>` tag is ignored. |

**Chained pair rule:** `head` auto-closes on any non-metadata tag → becomes `body` context. Single rule covers all cases.

---

### `body`

**Content model:** Flow content
**Insertion mode:** InBody

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | All flow content elements | `body` accepts everything that's valid in InBody mode. This is the default context for the entire document. |

`body` is the universal flow container — see "Flow Containers" below for the full breakdown.

---

### Flow Containers (shared rules)

**Elements:** `body`, `div`, `article`, `section`, `nav`, `aside`, `main`, `search`, `blockquote`, `dialog`, `figcaption`, `figure`, `dd`, `noscript`

**Content model:** Flow content (block + inline + everything)
**Insertion mode:** InBody

These all share identical pair rules:

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | **Block elements:** `div`, `article`, `section`, `nav`, `aside`, `main`, `search`, `blockquote`, `dialog`, `figure`, `figcaption`, `details`, `summary`, `address`, `hgroup`, `header`, `footer` | Block box, normal flow. Margins collapse with parent/siblings unless parent establishes BFC. |
| ACCEPT | **Headings:** `h1`, `h2`, `h3`, `h4`, `h5`, `h6` | Block box with UA font-size/weight/margins. |
| ACCEPT | **Paragraphs:** `p`, `pre` | Block box. `p` has 1em vertical margins. `pre` preserves whitespace (`white-space: pre`). |
| ACCEPT | **Lists:** `ul`, `ol`, `menu`, `dl` | Block box with UA `padding-inline-start: 40px`. |
| ACCEPT | **Table:** `table` | `display: table`. Establishes table formatting context. Block-level wrapper box. |
| ACCEPT | **Form:** `form`, `fieldset` | Block boxes. `fieldset` establishes new BFC with special border/legend rendering. `form` does not establish BFC. |
| ACCEPT | **Inline/phrasing:** `span`, `em`, `strong`, `b`, `i`, `u`, `s`, `small`, `cite`, `code`, `var`, `samp`, `kbd`, `sub`, `sup`, `abbr`, `bdi`, `bdo`, `data`, `dfn`, `mark`, `q`, `time`, `output`, `ruby`, `rt`, `rp` | Inline box. Creates anonymous inline formatting context. If siblings are blocks, anonymous block boxes wrap consecutive inline runs. |
| ACCEPT | **Interactive inline:** `a`, `button`, `label` | `a` is inline (transparent content model). `button` is `inline-block` establishing BFC. `label` is inline. |
| ACCEPT | **Embedded/replaced:** `img`, `iframe`, `embed`, `object`, `video`, `audio`, `canvas`, `picture`, `svg`, `math`, `portal` | Inline-level replaced elements. Have intrinsic dimensions. `svg`/`math` establish foreign formatting contexts. |
| ACCEPT | **Form controls:** `input`, `select`, `textarea`, `meter`, `progress`, `datalist` | Replaced/inline-block elements. Platform-native rendering for `input`, `select`, `textarea`. |
| ACCEPT | **Transparent:** `ins`, `del`, `map`, `slot` | Inherit flow content model. Children processed as if in flow context. |
| ACCEPT | **Text-level:** `br`, `wbr` | `br` forces line break. `wbr` provides optional break point. |
| ACCEPT | **Separators:** `hr` | Block-level box with UA border. |
| ACCEPT | **Hidden:** `script`, `style`, `template`, `noscript` | No layout boxes generated. Content is raw text (script/style) or inert (template). |
| ACCEPT | **Definition parts as orphans:** `dt`, `dd`, `li` | Technically invalid outside their list parents, but Chrome/Firefox accept them in flow context as block boxes. `li` renders as `list-item`, `dt`/`dd` as blocks. |
| DROP | `html`, `head` | Ignored. These structural tags can't appear mid-body. |
| ACCEPT | `caption`, `colgroup`, `col`, `thead`, `tbody`, `tfoot`, `tr`, `td`, `th` | These are accepted by the parser in InBody mode but only make sense in table context. Chrome/Firefox render them as block or inline depending on UA stylesheet. They don't get table layout unless inside `<table>`. |

**Pair rule reduction:** All flow containers share ONE rule set. A single `FLOW_PARENT` rule covers `body`, `div`, `article`, `section`, `nav`, `aside`, `main`, `search`, `blockquote`, `dialog`, `figcaption`, `figure`, `dd`, and `noscript`.

---

### Flow Containers (restricted variants)

These are flow containers with specific exclusions:

#### `header`, `footer`

Same as Flow Containers above, **except:**

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| DROP (spec) | `header`, `footer` (as descendants) | Spec forbids `header`/`footer` nesting, but Chrome/Firefox do NOT enforce this at the parser level. They accept it and render normally. **Spec violation tolerated.** |

**Pair rule:** Same as `FLOW_PARENT`. No parser-level difference.

#### `address`

Same as Flow Containers, **except:**

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| DROP (spec) | `address`, headings, sectioning elements | Spec forbids these as descendants. Chrome/Firefox do NOT enforce this — they accept and render. |

**Pair rule:** Same as `FLOW_PARENT`. No parser-level difference.

#### `dt`, `th`

Same as Flow Containers, **except:**

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| DROP (spec) | `header`, `footer`, sectioning, heading elements | Spec forbids in `dt` and `th`. Not parser-enforced. |

**Pair rule:** Same as `FLOW_PARENT`. No parser-level difference.

#### `form`

Same as Flow Containers, **except:**

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| DROP | `form` (nested) | **Parser enforced.** Inner `<form>` tag is completely ignored. Its children are adopted by the outer form's parent. The `</form>` close tag is also ignored (doesn't close the outer form). |

**Pair rule:** `FLOW_PARENT` + special rule: `form → form = DROP`.

#### `caption`

Same as Flow Containers, **except:**

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| CLOSE_PARENT | `caption`, `colgroup`, `col`, `thead`, `tbody`, `tfoot`, `tr`, `td`, `th` | Any table-structure tag inside `<caption>` implicitly closes the caption and is processed in the table context. |

**Pair rule:** `FLOW_PARENT` + `caption → table_structure_tag = CLOSE_PARENT`.

---

### `li`

**Content model:** Flow content
**Special behavior:** `li` inside `ul`/`ol` auto-closes a preceding open `li`.

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| (all) | (same as Flow Containers) | `li` is a flow container. Identical child handling to `div`. |
| CLOSE_REOPEN | `li` (sibling) | When `<li>` is encountered and there's already an open `<li>` in scope, the open `<li>` is closed first. This is NOT a parent→child rule — it's a sibling rule. |

**Pair rule:** `FLOW_PARENT`. The `li`→`li` close is a **scope rule**, not a nesting rule.

---

### `p`

**Content model:** Phrasing content only
**Insertion mode:** InBody (with auto-close rules)

This is the most important element for parser pair rules because of its extensive auto-close list.

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | `span`, `em`, `strong`, `b`, `i`, `u`, `s`, `small`, `cite`, `code`, `var`, `samp`, `kbd`, `sub`, `sup`, `abbr`, `bdi`, `bdo`, `data`, `dfn`, `mark`, `q`, `time`, `output`, `ruby`, `rt`, `rp`, `label` | Phrasing content. Inline boxes in the paragraph's inline formatting context. |
| ACCEPT | `a`, `ins`, `del`, `map` | Transparent elements inheriting phrasing model. Can contain only phrasing. |
| ACCEPT | `img`, `br`, `wbr`, `input`, `select`, `textarea`, `button`, `meter`, `progress`, `embed`, `iframe`, `object`, `video`, `audio`, `canvas`, `picture`, `svg`, `math`, `portal` | Inline-level replaced/embedded elements. |
| ACCEPT | `script`, `style`, `template`, `noscript`, `slot`, `datalist` | Hidden/inert/transparent. No layout impact. |
| CLOSE_PARENT | `address` | `</p>` auto-generated, `<address>` becomes sibling. |
| CLOSE_PARENT | `article` | `</p>` auto-generated, `<article>` becomes sibling. |
| CLOSE_PARENT | `aside` | `</p>` auto-generated. |
| CLOSE_PARENT | `blockquote` | `</p>` auto-generated. |
| CLOSE_PARENT | `details` | `</p>` auto-generated. |
| CLOSE_PARENT | `dialog` | `</p>` auto-generated. |
| CLOSE_PARENT | `div` | `</p>` auto-generated. |
| CLOSE_PARENT | `dl` | `</p>` auto-generated. |
| CLOSE_PARENT | `fieldset` | `</p>` auto-generated. |
| CLOSE_PARENT | `figcaption` | `</p>` auto-generated. |
| CLOSE_PARENT | `figure` | `</p>` auto-generated. |
| CLOSE_PARENT | `footer` | `</p>` auto-generated. |
| CLOSE_PARENT | `form` | `</p>` auto-generated. |
| CLOSE_PARENT | `h1`, `h2`, `h3`, `h4`, `h5`, `h6` | `</p>` auto-generated. |
| CLOSE_PARENT | `header` | `</p>` auto-generated. |
| CLOSE_PARENT | `hgroup` | `</p>` auto-generated. |
| CLOSE_PARENT | `hr` | `</p>` auto-generated. `<hr>` becomes sibling block. |
| CLOSE_PARENT | `li` | `</p>` auto-generated (only if `li` has `p` in button scope). |
| CLOSE_PARENT | `main` | `</p>` auto-generated. |
| CLOSE_PARENT | `menu` | `</p>` auto-generated. |
| CLOSE_PARENT | `nav` | `</p>` auto-generated. |
| CLOSE_PARENT | `ol` | `</p>` auto-generated. |
| CLOSE_PARENT | `p` | `</p>` auto-generated. A new `<p>` opens immediately after. |
| CLOSE_PARENT | `pre` | `</p>` auto-generated. |
| CLOSE_PARENT | `search` | `</p>` auto-generated. |
| CLOSE_PARENT | `section` | `</p>` auto-generated. |
| CLOSE_PARENT | `summary` | `</p>` auto-generated. |
| CLOSE_PARENT | `table` | `</p>` auto-generated. |
| CLOSE_PARENT | `ul` | `</p>` auto-generated. |

**The p-closing set (39 elements):**
```
address, article, aside, blockquote, details, dialog, div, dl, fieldset,
figcaption, figure, footer, form, h1, h2, h3, h4, h5, h6, header, hgroup,
hr, li, main, menu, nav, ol, p, pre, search, section, summary, table, ul
```

**Pair rule:** `p → [p-closing-set] = CLOSE_PARENT`. Single rule with a set lookup.

---

### `h1`, `h2`, `h3`, `h4`, `h5`, `h6`

**Content model:** Phrasing content only
**Same ACCEPT set as `p`.**

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | (same phrasing set as `p`) | Identical to `p` — inline formatting context. |
| CLOSE_PARENT | (same p-closing set as `p`) | Headings auto-close on the same set of block elements. |
| CLOSE_PARENT | `h1`, `h2`, `h3`, `h4`, `h5`, `h6` | **Any heading closes any open heading.** `<h1>foo <h2>bar` → `<h1>foo</h1><h2>bar</h2>`. All six close each other. |

**Pair rule:** Same as `p` + additional rule: `heading → heading = CLOSE_PARENT`.

---

### `pre`

**Content model:** Phrasing content only
**Same ACCEPT and CLOSE_PARENT sets as `p`.**

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | (same phrasing set as `p`) | Identical to `p`. Additionally: `white-space: pre` is inherited, so text preserves whitespace. |
| CLOSE_PARENT | (same p-closing set as `p`) | Identical auto-close behavior. |

**Special:** First newline character after `<pre>` is stripped (parser rule, not layout).

**Pair rule:** Identical to `p`. The `white-space: pre` is a CSS inherited property, not a pair rule.

---

### Inline Phrasing Containers (shared rules)

**Elements:** `span`, `em`, `strong`, `b`, `i`, `u`, `s`, `small`, `cite`, `code`, `var`, `samp`, `kbd`, `sub`, `sup`, `abbr`, `bdi`, `bdo`, `data`, `dfn`, `mark`, `q`, `time`, `output`

**Content model:** Phrasing content
**Insertion mode:** InBody (no special mode change)

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | (all phrasing elements — same set as `p` ACCEPT list) | Inline children nest inside the inline box. Inherited properties cascade (font-style, font-weight, color, text-decoration). |
| ACCEPT (parser) | **Block elements** (`div`, `p`, `table`, etc.) | **Parser does NOT auto-close inline elements.** The block is accepted as a child in the DOM tree. Layout then performs **anonymous block box generation** — the inline is split around the block. |

**This is the critical difference from `p`:** inline elements do NOT auto-close. They accept block children at the parser level, and layout handles the split.

**Layout behavior for invalid block-in-inline:**

```html
<span>AAA <div>BBB</div> CCC</span>
```

Chrome/Firefox render this as:

```
[anonymous block] → [inline: <span>AAA </span>]
[block: <div>BBB</div>]
[anonymous block] → [inline: <span> CCC</span>]
```

The `<span>` is split into two fragments. CSS properties (background, border, text-decoration) are carried across both fragments. `text-decoration` notably continues visually across the split (it "paints through").

**Pair rule:** `INLINE_PHRASING_PARENT → any = ACCEPT (parser)`. Layout handles splits via anonymous block generation. This is ONE rule.

**Special restrictions (spec-level, NOT parser-enforced):**
- `dfn` cannot contain `dfn` descendants
- `label` cannot contain `label` descendants
- These are not enforced by Chrome/Firefox parsers.

---

### `label`

Same as Inline Phrasing Containers, **except:**

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| DROP (spec) | `label` (nested) | Spec forbids. Chrome/Firefox do NOT enforce at parser level — they accept nested labels. |

**Pair rule:** Same as `INLINE_PHRASING_PARENT`.

---

### `a` (anchor)

**Content model:** Transparent (inherits from parent)
**Insertion mode:** InBody

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | (whatever the parent's content model allows) | If `<a>` is in a flow container, it can contain flow elements. If in a phrasing container, phrasing only. |
| DROP | `a` (nested) | **Parser enforced.** Adoption agency algorithm runs: the outer `<a>` is closed at the point where the inner `<a>` starts. Inner `<a>` opens. This prevents true nesting. |
| DROP (spec) | `button`, `details`, `embed`, `iframe`, `label`, `select`, `textarea` | Interactive content forbidden inside `<a>`. Chrome/Firefox behavior varies: some are parser-enforced (inner `<a>`), others are spec-only. |

**Pair rule:** `TRANSPARENT_PARENT` (inherits parent's rule set) + `a → a = DROP (adoption agency)`.

---

### `ins`, `del`

**Content model:** Transparent
**Same as `a`** except no interactive content restriction.

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | (whatever the parent's content model allows) | Transparent — inherits parent. |

**Pair rule:** `TRANSPARENT_PARENT`.

---

### `map`, `slot`

**Content model:** Transparent

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | (whatever the parent's content model allows) | Transparent — inherits parent. |

**Pair rule:** `TRANSPARENT_PARENT`.

---

### `canvas`, `object`

**Content model:** Transparent (fallback content)

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | (whatever the parent's content model allows) | Content is fallback. If the element renders (canvas context available, object loaded), children are not displayed. If not, children render per parent's content model. |

**Pair rule:** `TRANSPARENT_PARENT`. Display depends on element state.

---

### `button`

**Content model:** Phrasing content (no interactive descendants)
**Establishes:** New BFC (inline-block)

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | (phrasing elements — same as `p` ACCEPT list) | Inline content inside button's BFC. |
| CLOSE_REOPEN | `button` (nested) | **Parser enforced.** Encountering `<button>` when a `<button>` is already open closes the first button. Like `li`→`li`. |
| DROP (spec) | `a`, `button`, `details`, `embed`, `iframe`, `label`, `select`, `textarea` | Interactive content forbidden. `button`→`button` is parser-enforced (close-reopen). Others are spec-only. |
| ACCEPT (parser) | Block elements | Parser accepts them (like inline elements do). Layout generates anonymous blocks inside the button's BFC. |

**Pair rule:** `PHRASING_PARENT` + `button → button = CLOSE_REOPEN`.

---

### `ul`, `ol`, `menu`

**Content model:** `li` only (+ `script`, `template`)
**Insertion mode:** InBody

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | `li` | Valid child. `li` renders as `display: list-item`. |
| ACCEPT | `script`, `template` | Valid. Hidden/inert. |
| CLOSE_REOPEN (li) | `li` (when prev `li` is open) | New `<li>` closes the previous `<li>`. Sibling rule. |
| ACCEPT (parser) | Any other element | **Parser does NOT reject non-li children.** Chrome/Firefox accept `<ul><div>x</div></ul>` — the `div` becomes a direct child of `ul`. Layout renders it as a block box without list-item styling (no marker). |

**Pair rule:** `LIST_PARENT → li = ACCEPT`, `LIST_PARENT → other = ACCEPT (parser, invalid but tolerated)`. Chrome/Firefox are lenient here.

---

### `dl`

**Content model:** `dt`/`dd` groups (or `div` wrapping dt/dd groups)
**Insertion mode:** InBody

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | `dt`, `dd`, `div` | Valid children. `dt` is block, `dd` is block with `margin-inline-start: 40px`. `div` can wrap dt/dd groups. |
| ACCEPT | `script`, `template` | Valid. Hidden/inert. |
| CLOSE_REOPEN | `dt` when `dt` is open | New `<dt>` closes previous `<dt>`. |
| CLOSE_REOPEN | `dd` when `dt` is open | `<dd>` closes preceding `<dt>`. |
| CLOSE_REOPEN | `dt` when `dd` is open | `<dt>` closes preceding `<dd>`. |
| CLOSE_REOPEN | `dd` when `dd` is open | `<dd>` closes preceding `<dd>`. |
| ACCEPT (parser) | Any other element | Tolerated. Rendered per its default display. |

**Pair rule:** `DL_PARENT → dt/dd = ACCEPT + CLOSE_REOPEN (prev dt/dd)`. Cross-closing between dt↔dd.

---

### `table`

**Content model:** `caption`, `colgroup`, `thead`, `tbody`, `tfoot`, `tr` (in order)
**Insertion mode:** InTable

This is where pair rules get complex. The parser switches to InTable mode.

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | `caption` | Valid. Switches to InCaption mode. |
| ACCEPT | `colgroup` | Valid. Switches to InColumnGroup mode. |
| ACCEPT | `col` | WRAP(colgroup). Auto-generates `<colgroup>` wrapper. |
| ACCEPT | `thead`, `tfoot` | Valid. Switches to InTableBody mode. |
| ACCEPT | `tbody` | Valid. Switches to InTableBody mode. |
| WRAP(tbody) | `tr` | If `<tr>` appears directly in `<table>`, parser auto-generates `<tbody>` and inserts `<tr>` inside it. |
| WRAP(tbody, tr) | `td`, `th` | If cell appears directly in `<table>`, parser auto-generates `<tbody><tr>` and inserts cell inside. |
| ACCEPT | `script`, `template`, `style` | Valid in InTable mode. Processed normally. |
| FOSTER | `div`, `p`, `span`, `a`, `img`, `input`, text, **all other elements** | **Foster parenting.** Non-table content is moved BEFORE the table in the DOM tree. Parser processes the element in InBody mode but inserts it before the table. |
| DROP | `table` (nested via parser) | **Encountering `<table>` inside `<table>` closes the inner table context.** Actually, it closes the current table and starts a new one. Result: tables become siblings, not nested. **EXCEPT** if `<table>` appears inside `<td>`/`<th>` — then it's a legitimately nested table. |
| ACCEPT | `form` | **DROP.** `<form>` is ignored inside table context (InTable mode). |

**Foster parenting in detail:**

```html
<table>
  <div>foster me</div>
  <tr><td>cell</td></tr>
</table>
```

DOM result:
```
<div>foster me</div>
<table>
  <tbody>
    <tr><td>cell</td></tr>
  </tbody>
</table>
```

The `<div>` is foster-parented before the table.

**Pair rule:** `TABLE_PARENT → [table-children] = ACCEPT/WRAP`. `TABLE_PARENT → [everything else] = FOSTER`.

The table-children set: `caption`, `colgroup`, `col`, `thead`, `tbody`, `tfoot`, `tr`, `td`, `th`, `script`, `template`, `style`.

---

### `thead`, `tbody`, `tfoot`

**Content model:** `tr` only (+ `script`, `template`)
**Insertion mode:** InTableBody

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | `tr` | Valid. Switches to InRow mode. |
| ACCEPT | `script`, `template` | Valid. |
| WRAP(tr) | `td`, `th` | Auto-generates `<tr>` wrapper for orphaned cells. |
| CLOSE_PARENT | `caption`, `colgroup`, `thead`, `tbody`, `tfoot` | These close the current row group. Parser returns to InTable mode. |
| CLOSE_PARENT | `</table>` | Closes row group and table. |
| FOSTER | Everything else | Same foster parenting as `table`. Non-table content moves before the table. |

**Pair rule:** `TABLE_BODY_PARENT → tr = ACCEPT`, `→ td/th = WRAP(tr)`, `→ table-structure = CLOSE_PARENT`, `→ other = FOSTER`.

---

### `tr`

**Content model:** `td`, `th` only (+ `script`, `template`)
**Insertion mode:** InRow

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | `td`, `th` | Valid. Switches to InCell mode. Each cell establishes a new BFC. |
| ACCEPT | `script`, `template` | Valid. |
| CLOSE_PARENT | `tr` | New `<tr>` closes the current row. |
| CLOSE_PARENT | `caption`, `colgroup`, `thead`, `tbody`, `tfoot` | Close row and return to table context. |
| CLOSE_PARENT | `</table>` | Close row, row group, and table. |
| FOSTER | Everything else | Foster parent before the table. |

**Pair rule:** `TABLE_ROW_PARENT → td/th = ACCEPT`, `→ tr = CLOSE_PARENT`, `→ table-structure = CLOSE_PARENT`, `→ other = FOSTER`.

---

### `td`, `th` (table cells)

**Content model:** Flow content
**Insertion mode:** InCell (which delegates to InBody)

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| (all) | Same as Flow Containers | Cells are flow containers. They accept all InBody content. |
| CLOSE_PARENT | `td`, `th` | New cell closes the current cell. |
| CLOSE_PARENT | `tr`, `caption`, `colgroup`, `thead`, `tbody`, `tfoot` | Table structure tags close the cell. |
| CLOSE_PARENT | `</table>` | Closes cell, row, row group, table. |

**Pair rule:** `FLOW_PARENT` + `cell → cell = CLOSE_PARENT` + `cell → table-structure = CLOSE_PARENT`.

---

### `colgroup`

**Content model:** `col` only (or empty with `span` attribute)
**Insertion mode:** InColumnGroup

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | `col` | Valid. |
| ACCEPT | `template` | Valid. |
| CLOSE_PARENT | Everything else | Any non-`col` tag closes `<colgroup>`. Parser returns to InTable mode and reprocesses the tag. |

**Pair rule:** `COLGROUP_PARENT → col/template = ACCEPT`, `→ other = CLOSE_PARENT`.

---

### `caption`

See "Flow Containers (restricted variants)" above. Summary:

**Pair rule:** `FLOW_PARENT` + `caption → table-structure = CLOSE_PARENT`.

---

### `select`

**Content model:** `option`, `optgroup`, `hr`
**Insertion mode:** InSelect

The parser switches to a completely different mode.

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | `option` | Valid. Closes any previous open `<option>`. |
| ACCEPT | `optgroup` | Valid. Closes any previous open `<optgroup>` (and its `<option>`). |
| ACCEPT | `hr` | Valid (as a separator). Renders as a horizontal line in the dropdown. |
| ACCEPT | `script`, `template` | Valid. |
| CLOSE_PARENT | `select` (nested) | Closes the current `<select>`. |
| CLOSE_PARENT | `input`, `textarea` | Close `<select>` and process in InBody mode. |
| DROP | Everything else | **Ignored entirely.** `<div>`, `<p>`, `<span>`, text — all tags (not text content) are dropped in InSelect mode. Text IS accepted as content of options. |

**Pair rule:** `SELECT_PARENT → option/optgroup/hr = ACCEPT`, `→ select/input/textarea = CLOSE_PARENT`, `→ other = DROP`.

---

### `optgroup`

**Content model:** `option` only
**Insertion mode:** InSelect

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | `option` | Valid. Closes any previous open `<option>`. |
| CLOSE_PARENT | `optgroup` | New optgroup closes current one. |
| CLOSE_PARENT | `select` (closing) | Closes optgroup and select. |
| DROP | Everything else | Same as `select` — ignored in InSelect mode. |

**Pair rule:** `OPTGROUP_PARENT → option = ACCEPT`, `→ optgroup = CLOSE_PARENT`, `→ other = DROP`.

---

### `option`

**Content model:** Text only
**Insertion mode:** InSelect

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| RAW_TEXT (effectively) | Text content | Only text is accepted. |
| CLOSE_PARENT | `option` | New option closes current one. |
| CLOSE_PARENT | `optgroup` | Closes option (and current optgroup opens new one). |
| DROP | Everything else | Tags are dropped. Only text content is preserved. |

**Pair rule:** `OPTION_PARENT → text = ACCEPT`, `→ option/optgroup = CLOSE_PARENT`, `→ other = DROP`.

---

### `textarea`

**Content model:** Text only (RCDATA)
**Insertion mode:** Text (RCDATA)

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| RAW_TEXT | Everything | Parser switches to RCDATA mode. No tags are parsed inside textarea (except `</textarea>`). All content is raw text. HTML entities ARE decoded. |

**Pair rule:** `RAW_TEXT_PARENT`. No child elements possible.

---

### `title`

**Content model:** Text only (RCDATA)

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| RAW_TEXT | Everything | Same as `textarea`. RCDATA mode — entities decoded, no tags parsed. |

**Pair rule:** `RAW_TEXT_PARENT`.

---

### `script`

**Content model:** Text only (raw text)

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| RAW_TEXT | Everything | Parser switches to Script Data mode. Nothing is parsed — even entities are NOT decoded. Only `</script>` ends it. |

**Pair rule:** `RAW_TEXT_PARENT`.

---

### `style`

**Content model:** Text only (raw text)

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| RAW_TEXT | Everything | Same as `script`. Raw text mode. Only `</style>` ends it. |

**Pair rule:** `RAW_TEXT_PARENT`.

---

### `template`

**Content model:** Anything (inert)
**Special behavior:** Content is parsed into a DocumentFragment, not rendered.

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | Everything | Template can contain any HTML content. It's parsed normally but stored in an inert DocumentFragment. `display: none` — no layout boxes generated. |

**Pair rule:** `TEMPLATE_PARENT → any = ACCEPT (inert)`. No layout implications.

---

### `noscript`

**Content model:** Varies — when scripting is disabled, acts as flow/phrasing container; when enabled, raw text.

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| RAW_TEXT | (when scripting enabled) | Content is raw text — not parsed. |
| ACCEPT | (when scripting disabled) | Content is parsed as flow content (if in `body`) or metadata (if in `head`). |

**Pair rule:** Depends on scripting state. For Pane (scripting disabled): `FLOW_PARENT`.

---

### `ruby`

**Content model:** Phrasing + `rt` + `rp`

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | Phrasing elements | Base text of the ruby annotation. |
| ACCEPT | `rt` | Ruby text — annotation displayed above/beside base. `display: ruby-text`. |
| ACCEPT | `rp` | Fallback parentheses — `display: none` in ruby-aware browsers. |
| CLOSE_REOPEN | `rt` when `rt` is open | New `<rt>` closes previous `<rt>`. |
| CLOSE_REOPEN | `rp` when `rt` is open | `<rp>` closes `<rt>`. |

**Pair rule:** `RUBY_PARENT → phrasing = ACCEPT`, `→ rt/rp = ACCEPT + CLOSE_REOPEN(prev)`.

---

### `hgroup`

**Content model:** `h1`–`h6` and `p` elements

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | `h1`, `h2`, `h3`, `h4`, `h5`, `h6` | Headings inside hgroup. Only one is the "heading" — others are subheadings. |
| ACCEPT | `p` | Used for subtitles/taglines. |
| ACCEPT | `script`, `template` | Valid. |
| ACCEPT (parser) | Other elements | Parser does not enforce hgroup content model strictly. Other elements are accepted but invalid per spec. |

**Pair rule:** `HGROUP_PARENT → headings/p = ACCEPT`, `→ other = ACCEPT (parser tolerant)`.

---

### `details`

**Content model:** `summary` (first child) then flow

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | `summary` (as first child) | Summary is the visible toggle element. If no `<summary>` present, Chrome/Firefox auto-generate one with text "Details". |
| ACCEPT | Flow content (after summary) | Only visible when `open` attribute is present. Hidden otherwise. |

**Pair rule:** `FLOW_PARENT` with display-toggling for non-summary children based on `open` attribute.

---

### `summary`

**Content model:** Phrasing content or one heading element

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | Phrasing elements | Normal inline content. |
| ACCEPT | `h1`–`h6` (one) | Heading as the summary text. |
| ACCEPT (parser) | Block elements | Parser accepts them. Layout renders normally (summary acts as a flow container in practice). |

**Pair rule:** Effectively `FLOW_PARENT` (parser is lenient).

---

### `fieldset`

**Content model:** `legend` (optional first child) then flow
**Establishes:** New BFC

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | `legend` (first child) | Special positioning: legend overlaps the fieldset border at block-start edge. |
| ACCEPT | Flow content | Normal flow after legend. Fieldset establishes BFC. |

**Pair rule:** `FLOW_PARENT` + `fieldset → legend (first) = special positioning`.

---

### `legend`

**Content model:** Phrasing content and heading content

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | Phrasing elements | Inline content. |
| ACCEPT | `h1`–`h6` | Heading inside legend. |
| ACCEPT (parser) | Block elements | Parser accepts. Layout treats legend as a flow container. |

**Pair rule:** Effectively `FLOW_PARENT` (parser is lenient).

---

### `figure`

**Content model:** `figcaption` (optional first or last child) then flow

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | `figcaption` | Caption for the figure. Block box. |
| ACCEPT | Flow content | Normal flow layout. |

**Pair rule:** `FLOW_PARENT`.

---

### `video`, `audio`

**Content model:** `source`, `track`, then transparent (fallback)
**Replaced elements**

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | `source` | Media source candidates. Not rendered. |
| ACCEPT | `track` | Text tracks (subtitles). Not rendered visually in parent. |
| ACCEPT | (transparent — fallback) | Only displayed if media fails to load. Inherits parent's content model. |

**Pair rule:** `MEDIA_PARENT → source/track = ACCEPT (hidden)`, `→ other = TRANSPARENT_PARENT (fallback)`.

---

### `picture`

**Content model:** `source` elements then one `img`

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | `source` | Responsive image candidates. Not rendered. |
| ACCEPT | `img` | The actual rendered image. |

**Pair rule:** `PICTURE_PARENT → source/img = ACCEPT`.

---

### `svg`

**Content model:** SVG namespace elements
**Insertion mode:** InForeignContent

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| FOREIGN | SVG elements (`rect`, `circle`, `path`, `g`, `text`, `defs`, `use`, `line`, `polyline`, `polygon`, `ellipse`, `image`, `clipPath`, `mask`, `filter`, `linearGradient`, `radialGradient`, `pattern`, `marker`, `symbol`, `foreignObject`, `switch`, `a`, `tspan`, etc.) | Parsed in SVG namespace. Different attribute handling (case-sensitive, different default attributes). |
| ACCEPT | `foreignObject` → then HTML content | `foreignObject` re-enters HTML parsing mode. Its children are HTML. |
| CLOSE_PARENT | HTML block elements (`div`, `p`, `table`, etc.) | If an HTML element appears directly in SVG (not in `foreignObject`), Chrome/Firefox close the SVG context and process the element in InBody mode. This is the "integration point" behavior. |

**Pair rule:** `SVG_PARENT → svg-elements = FOREIGN`, `→ html-block = CLOSE_PARENT (exit foreign)`.

---

### `math`

**Content model:** MathML namespace elements
**Insertion mode:** InForeignContent

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| FOREIGN | MathML elements (`mrow`, `mi`, `mn`, `mo`, `msup`, `msub`, `mfrac`, `mroot`, `msqrt`, `mtext`, `mspace`, `mtable`, `mtr`, `mtd`, `mover`, `munder`, `munderover`, `mmultiscripts`, `mprescripts`, `none`, `annotation`, `annotation-xml`, `semantics`, etc.) | Parsed in MathML namespace. |
| ACCEPT | `annotation-xml` → then HTML content | If `annotation-xml` has `encoding="text/html"`, re-enters HTML parsing. |
| CLOSE_PARENT | HTML block elements | Same exit-foreign behavior as SVG. |

**Pair rule:** `MATHML_PARENT → mathml-elements = FOREIGN`, `→ html-block = CLOSE_PARENT (exit foreign)`.

---

### `iframe`

**Content model:** Text (fallback, ignored by modern browsers)

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| RAW_TEXT | Everything | Content between `<iframe>` and `</iframe>` is raw text — not parsed as HTML. Modern browsers ignore it entirely (it was fallback for non-iframe-supporting browsers). |

**Pair rule:** `RAW_TEXT_PARENT`.

---

### `datalist`

**Content model:** Phrasing content or `option` elements

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| ACCEPT | `option` | Options for the datalist suggestions. |
| ACCEPT | Phrasing elements | Fallback content for non-datalist-aware browsers. |

**Pair rule:** `PHRASING_PARENT` + `option`.

---

### Void Elements (cannot be parents)

**Elements:** `area`, `base`, `br`, `col`, `embed`, `hr`, `img`, `input`, `link`, `meta`, `param`, `portal`, `source`, `track`, `wbr`

| Action | Child elements | Chrome/Firefox behavior |
|--------|---------------|------------------------|
| VOID | Everything | These elements cannot have children. Parser self-closes them immediately. Any content "inside" them actually becomes a sibling. |

**Pair rule:** `VOID → any = N/A`. No nesting possible.

---

## Part 2 — The Reduced Pair Rule Set

The ~14,000 raw element×element pairs collapse into **17 parent rules** and **7 parser actions**:

### Parent Rules (17)

| # | Rule Name | Elements | Child Handling |
|---|-----------|----------|----------------|
| 1 | `FLOW_PARENT` | `body`, `div`, `article`, `section`, `nav`, `aside`, `main`, `search`, `blockquote`, `dialog`, `figcaption`, `figure`, `dd`, `dt`, `li`, `noscript`, `address`, `header`, `footer`, `form`, `fieldset`, `details`, `summary`, `legend`, `td`, `th`, `caption` | Accept all flow+phrasing. No auto-close. |
| 2 | `PHRASING_BLOCK_PARENT` | `p`, `h1`–`h6`, `pre` | Accept phrasing. Auto-close on 39-element block set. |
| 3 | `INLINE_PHRASING_PARENT` | `span`, `em`, `strong`, `b`, `i`, `u`, `s`, `small`, `cite`, `code`, `var`, `samp`, `kbd`, `sub`, `sup`, `abbr`, `bdi`, `bdo`, `data`, `dfn`, `mark`, `q`, `time`, `output`, `label` | Accept phrasing. Parser accepts blocks too (layout splits). |
| 4 | `TRANSPARENT_PARENT` | `a`, `ins`, `del`, `map`, `canvas`, `object`, `slot` | Inherit parent's rule. |
| 5 | `LIST_PARENT` | `ul`, `ol`, `menu` | Expect `li`. Tolerate anything. |
| 6 | `DL_PARENT` | `dl` | Expect `dt`/`dd`/`div`. Tolerate anything. Cross-close dt↔dd. |
| 7 | `TABLE_PARENT` | `table` | Accept table children. Foster-parent everything else. |
| 8 | `TABLE_BODY_PARENT` | `thead`, `tbody`, `tfoot` | Accept `tr`. Wrap cells. Foster-parent rest. |
| 9 | `TABLE_ROW_PARENT` | `tr` | Accept `td`/`th`. Foster-parent rest. |
| 10 | `TABLE_CELL_PARENT` | `td`, `th` | FLOW_PARENT + auto-close on cells/table-structure. |
| 11 | `COLGROUP_PARENT` | `colgroup` | Accept `col`. Close on anything else. |
| 12 | `SELECT_PARENT` | `select` | Accept `option`/`optgroup`/`hr`. Drop rest. |
| 13 | `OPTGROUP_PARENT` | `optgroup` | Accept `option`. Close on `optgroup`. Drop rest. |
| 14 | `RAW_TEXT_PARENT` | `script`, `style`, `title`, `textarea`, `iframe`, `option` | No child elements. Raw text mode. |
| 15 | `RUBY_PARENT` | `ruby` | Accept phrasing + `rt`/`rp`. Cross-close rt↔rp. |
| 16 | `SVG_PARENT` | `svg` | Foreign content mode. Exit on HTML blocks. |
| 17 | `MATHML_PARENT` | `math` | Foreign content mode. Exit on HTML blocks. |

(Void elements don't need a parent rule — they can't have children.)

### Chained Pair Actions (7)

| # | Action | Trigger | Effect on tree |
|---|--------|---------|----------------|
| 1 | `ACCEPT` | Valid child | Insert as child. No tree modification. |
| 2 | `CLOSE_PARENT` | Block tag in phrasing-block; table-structure in caption/cell; cell in cell/row; etc. | Close current element. Re-process tag in parent's context. **Chain: rule(grandparent, child)** |
| 3 | `FOSTER` | Non-table content in table context | Move child before table ancestor. **Chain: rule(table's parent, child)** |
| 4 | `WRAP(x)` | Missing intermediate (e.g., `tr` in `table` → wrap in `tbody`; `td` in `table` → wrap in `tbody` + `tr`) | Generate wrapper element(s). Insert child inside wrapper. **Chain: rule(wrapper, child)** |
| 5 | `DROP` | Duplicate structural tag (`form`→`form`, select context rejects) | Ignore start tag. Content adopted by current parent. |
| 6 | `CLOSE_REOPEN` | Same-type sibling (`li`→`li`, `dt`→`dd`, `option`→`option`, `button`→`button`) | Close current element of that type. Open new one. Sibling relationship. |
| 7 | `RAW_TEXT` | Parent is raw text element | Switch to raw text/RCDATA mode. No child elements parsed. |

### How Chaining Works

A three-deep nesting `A → B → C` is resolved by:
1. Look up `rule(A, B)` → get action for B
2. If ACCEPT: look up `rule(B, C)` → get action for C
3. If CLOSE_PARENT: look up `rule(A.parent, B)` → reprocess B in grandparent context
4. If FOSTER: look up `rule(table.parent, B)` → reprocess B before table
5. If WRAP(x): look up `rule(x, B)` → process B inside generated wrapper

Every nesting depth is resolved pair-by-pair. No three-element rules needed. **The 17 parent rules × 7 actions are sufficient to handle all HTML nesting.**

---

## Part 3 — The Auto-Close Element Sets

These are the exact element sets used in the pair rules above:

### Set 1: Elements that close `<p>` (the p-closing set)

```
address, article, aside, blockquote, center, details, dialog, dir, div, dl,
fieldset, figcaption, figure, footer, form, h1, h2, h3, h4, h5, h6, header,
hgroup, hr, li, main, menu, nav, ol, p, pre, search, section, summary,
table, ul
```

(39 elements. Also used for `h1`–`h6` and `pre`.)

### Set 2: Elements that close `<li>`

```
li
```

(`<li>` only closes a preceding `<li>` if it's in scope.)

### Set 3: Elements that close `<dt>` and `<dd>`

```
dt, dd
```

(`<dt>` closes `<dd>`, `<dd>` closes `<dt>`, `<dt>` closes `<dt>`, `<dd>` closes `<dd>`.)

### Set 4: Elements that close `<option>`

```
option, optgroup
```

### Set 5: Elements that close table cell (`<td>`/`<th>`)

```
td, th, tr, caption, colgroup, thead, tbody, tfoot
```

(Anything that moves to a different table structural level.)

### Set 6: Elements that close `<tr>`

```
tr, caption, colgroup, thead, tbody, tfoot
```

### Set 7: Elements that close `<thead>`/`<tbody>`/`<tfoot>`

```
caption, colgroup, thead, tbody, tfoot
```

### Set 8: Table-valid children (not foster-parented)

```
caption, colgroup, col, thead, tbody, tfoot, tr, td, th, script, template, style
```

### Set 9: Elements that exit foreign content (SVG/MathML)

```
b, big, blockquote, body, br, center, code, dd, div, dl, dt, em, embed,
h1, h2, h3, h4, h5, h6, head, hr, i, img, li, listing, menu, meta, nobr,
ol, p, pre, ruby, s, small, span, strong, strike, sub, sup, table, tt, u,
ul, var
```

(Any of these encountered in SVG/MathML context causes the parser to exit foreign content.)

### Set 10: Active formatting elements (reconstructed across splits)

```
a, b, big, code, em, font, i, nobr, s, small, strike, strong, tt, u
```

These are pushed onto the "list of active formatting elements" and reconstructed when the tree is split by auto-closing or foster parenting.

---

## Part 4 — Layout-Level Pair Interactions

After the parser builds the correct tree, layout must handle these pair interactions. These are **independent of parser rules** — they depend on the computed `display` value.

### Layout Pair Rule 1: Block in Block

**When:** Parent `display` is `block`/`flow`, child `display` is `block`/`flow`
**Action:** Stack vertically. Margin collapse between siblings and with parent (unless BFC boundary).
**Elements:** ~2,000 pairs

### Layout Pair Rule 2: Inline in Block

**When:** Parent `display` is `block`/`flow`, child `display` is `inline`
**Action:** Create anonymous inline formatting context. Line boxes are generated.
**Elements:** ~1,500 pairs

### Layout Pair Rule 3: Mixed Block+Inline in Block

**When:** Parent `display` is `block`/`flow`, children are mix of block and inline
**Action:** Wrap consecutive inline runs in anonymous block boxes, then stack all blocks vertically.
**Elements:** Triggered by sibling relationships, not parent-child.

### Layout Pair Rule 4: Block in Inline (anonymous block generation)

**When:** Parent `display` is `inline`, child `display` is `block`
**Action:** Split inline parent into fragments. Wrap fragments in anonymous block boxes. Block child becomes a sibling of the anonymous blocks.
**Elements:** ~300 pairs (inline parents × block children)

### Layout Pair Rule 5: Inline in Inline

**When:** Parent `display` is `inline`, child `display` is `inline`
**Action:** Nested inline boxes. Participate in same inline formatting context.
**Elements:** ~800 pairs

### Layout Pair Rule 6: Table Internal

**When:** Any table `display` value (`table`, `table-row`, `table-cell`, etc.)
**Action:** Table layout algorithm. Fixed or automatic width computation. Cell sizing. Border collapse.
**Elements:** ~50 pairs (strict hierarchy)

### Layout Pair Rule 7: Flex/Grid (CSS-driven)

**When:** Parent `display` is `flex`/`inline-flex`/`grid`/`inline-grid`
**Action:** All children become flex/grid items. Float, clear, vertical-align are ignored on items. Anonymous items generated for text nodes.
**Elements:** Any parent×child pair where CSS applies `display: flex/grid`.

### Layout Pair Rule 8: Replaced Element as Child

**When:** Child is a replaced element (`img`, `input`, `select`, `textarea`, `iframe`, `canvas`, `video`, `embed`, `object`)
**Action:** Use intrinsic dimensions. Aspect ratio constraints. No flow of child content into parent's formatting context.
**Elements:** ~200 pairs

### Layout Pair Rule 9: Out-of-Flow (positioned/float)

**When:** Child has `position: absolute/fixed` or `float: left/right`
**Action:** Remove from normal flow. Containing block changes. Does not affect parent's size (for absolute/fixed). Float affects sibling layout.
**Elements:** Any parent×child pair where CSS applies positioning.

### Layout Pair Rule 10: Establishes BFC

**When:** Parent establishes a new block formatting context (overflow≠visible, display:flow-root, float, position:absolute/fixed, inline-block, table-cell, flex/grid item, etc.)
**Action:** Contains floats, prevents margin collapse with children, independent formatting context.
**Elements:** ~500 pairs (specific parents that establish BFC × any child)

---

## Part 5 — Complete Pair Count

### Parser Level

| Parent Rule | # Parents | # Distinct Outcomes | # Effective Pairs |
|-------------|-----------|--------------------|--------------------|
| FLOW_PARENT | 27 | 2 (accept/drop html,head) | 27 × 147 = 3,969 |
| PHRASING_BLOCK_PARENT | 8 | 2 (accept phrasing / close on block) | 8 × 147 = 1,176 |
| INLINE_PHRASING_PARENT | 25 | 1 (accept all) | 25 × 147 = 3,675 |
| TRANSPARENT_PARENT | 7 | (delegates to parent) | 7 × 147 = 1,029 |
| LIST_PARENT | 3 | 2 (accept li / tolerate rest) | 3 × 147 = 441 |
| DL_PARENT | 1 | 2 (accept dt,dd / tolerate rest) | 1 × 147 = 147 |
| TABLE_PARENT | 1 | 3 (accept / wrap / foster) | 1 × 147 = 147 |
| TABLE_BODY_PARENT | 3 | 3 (accept / wrap / foster) | 3 × 147 = 441 |
| TABLE_ROW_PARENT | 1 | 3 (accept / close / foster) | 1 × 147 = 147 |
| TABLE_CELL_PARENT | 2 | 2 (flow + close on table tags) | 2 × 147 = 294 |
| COLGROUP_PARENT | 1 | 2 (accept col / close rest) | 1 × 147 = 147 |
| SELECT_PARENT | 1 | 3 (accept / close / drop) | 1 × 147 = 147 |
| OPTGROUP_PARENT | 1 | 2 (accept option / drop rest) | 1 × 147 = 147 |
| RAW_TEXT_PARENT | 6 | 1 (raw text) | 6 × 147 = 882 |
| RUBY_PARENT | 1 | 2 (phrasing + rt/rp) | 1 × 147 = 147 |
| SVG_PARENT | 1 | 2 (foreign / exit) | 1 × SVG elements |
| MATHML_PARENT | 1 | 2 (foreign / exit) | 1 × MathML elements |
| VOID (no children) | 15 | 0 | 0 |
| **Total** | **105** | | **~12,936 raw pairs** |

Reduced to: **17 parent rules × 7 actions + 10 element sets = 129 rule components**

### Layout Level

| Layout Rule | Trigger | Pairs Affected |
|-------------|---------|----------------|
| Block in Block | display pairs | ~2,000 |
| Inline in Block | display pairs | ~1,500 |
| Block in Inline | display pairs | ~300 |
| Inline in Inline | display pairs | ~800 |
| Table layout | table display | ~50 |
| Replaced child | replaced elements | ~200 |
| Anonymous box gen | mixed children | (sibling-triggered) |
| BFC establishment | overflow/float/etc. | ~500 |
| Flex/Grid | CSS-driven | (any pair) |
| Positioned | CSS-driven | (any pair) |

Reduced to: **10 layout rules keyed on computed `display` value**

---

## Grand Total: Minimal Chained Pair Rule Set

```
Parser:  17 parent rules + 7 actions + 10 element sets
Layout:  10 display-based rules
─────────────────────────────────────────────────────
Total:   44 rule components handle all ~14,000 element pairs
```

Every HTML nesting scenario resolves by:
1. Look up parent's rule (17 options)
2. Check child against element sets (10 sets)
3. Execute action (7 options)
4. Chain to next pair if tree was modified (CLOSE_PARENT/FOSTER/WRAP)
5. After tree is built, apply layout rule based on computed display (10 options)

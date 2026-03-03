# Chrome HTML Parser — Pair Handling Reference

How Chrome (Blink) handles every parent→child pair at the tree construction level.
Source: `third_party/blink/renderer/core/html/parser/html_tree_builder.cc`

**Scope:** Top-level pair interactions only (what Chrome does when child's start tag
is encountered while parent is the current open element). No chaining analysis.

**116 elements × 116 elements = 13,456 pairs.**

---

## How Chrome's Parser Works (Overview)

Chrome's HTML parser uses an **insertion mode** state machine. The current insertion
mode determines how each incoming start tag is handled. The key function is
`HTMLTreeBuilder::ProcessStartTag()` which dispatches to mode-specific handlers.

The insertion mode is determined by the parser state, not directly by "which element
is open." But in practice, each parent element maps to a predictable insertion mode:

| Insertion Mode | Active When... | Parent Elements |
|----------------|----------------|-----------------|
| `InBody` | Default body parsing | Most elements: `div`, `span`, `p`, `a`, `body`, `section`, etc. |
| `InHead` | Inside `<head>` | `head` |
| `InTable` | Inside `<table>` (not in cell) | `table` |
| `InTableBody` | Inside `<thead>/<tbody>/<tfoot>` | `thead`, `tbody`, `tfoot` |
| `InRow` | Inside `<tr>` | `tr` |
| `InCell` | Inside `<td>/<th>` | `td`, `th` |
| `InColumnGroup` | Inside `<colgroup>` | `colgroup` |
| `InSelect` | Inside `<select>` | `select`, `optgroup`, `option` |
| `InSelectInTable` | Inside `<select>` that's inside `<table>` | `select` (in table context) |
| `InCaption` | Inside `<caption>` | `caption` |
| `Text` | Raw text / RCDATA mode | `script`, `style`, `title`, `textarea`, `iframe` |
| `InForeignContent` | Inside `<svg>` or `<math>` | `svg`, `math` |
| `InTemplate` | Inside `<template>` | `template` |

---

## Part 1 — Every Parent Element's Chrome Behavior

For each parent element, the Chrome insertion mode and per-child-tag action.

### Legend

| Code | Meaning |
|------|---------|
| `ACCEPT` | Element is inserted as a child node in the DOM tree |
| `ACCEPT_VOID` | Void element inserted (self-closing, no end tag) |
| `ACCEPT_FOREIGN` | Inserted in SVG/MathML namespace |
| `CLOSE_P` | If `<p>` is in button scope, auto-close it first, then insert |
| `CLOSE_HEADING` | Current heading is popped, new heading inserted (same level or different) |
| `CLOSE_PARENT` | Parent is auto-closed; tag reprocessed in grandparent's mode |
| `CLOSE_REOPEN` | Previous same-type element closed; new one opens |
| `FOSTER` | Foster parenting — element moved before the table in DOM |
| `WRAP` | Missing intermediary auto-generated (e.g., `<tbody>`, `<tr>`) |
| `DROP` | Start tag ignored entirely |
| `RAW_TEXT` | Parser switches to raw text mode; no child tags parsed |
| `RECONSTRUCT_AFE` | Active formatting elements are reconstructed before insertion |
| `EXIT_FOREIGN` | Foreign content (SVG/MathML) is exited; tag reprocessed in HTML mode |

---

### `<html>` — Insertion mode: BeforeHead / AfterHead

Chrome source: `processStartTagForInBody` (html tag only), `processStartTagForBeforeHead`, `processStartTagForAfterHead`

When `<html>` is the current node, Chrome is in BeforeHead mode (waiting for `<head>`) or AfterHead mode (waiting for `<body>`).

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| `head` | **ACCEPT** | Inserted. Mode switches to InHead. |
| `body` | **ACCEPT** | Inserted. Mode switches to InBody. |
| `html` | **DROP** | Duplicate ignored. Attributes may merge onto existing `<html>`. |
| Any other tag | **WRAP** | If before `<head>`: auto-generate `<head>`, close it immediately, auto-generate `<body>`, reprocess tag in InBody. If after `<head>`: auto-generate `<body>`, reprocess in InBody. |

**Effective:** Only `head` and `body` are real children. Everything else triggers wrapper generation.

---

### `<head>` — Insertion mode: InHead

Chrome source: `processStartTagForInHead()`

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| `html` | **DROP** | Handled as InBody (attributes merge). |
| `base`, `basefont`, `bgsound`, `link` | **ACCEPT_VOID** | Void metadata elements. Inserted. |
| `meta` | **ACCEPT_VOID** | Inserted. Charset detection may run. |
| `title` | **ACCEPT** → **RAW_TEXT** | Inserted. Tokenizer switches to RCDATA. |
| `noscript` (scripting on) | **ACCEPT** → **RAW_TEXT** | Raw text mode. |
| `noscript` (scripting off) | **ACCEPT** | Mode switches to InHeadNoscript. |
| `noframes`, `style` | **ACCEPT** → **RAW_TEXT** | Raw text parsing. |
| `script` | **ACCEPT** → **RAW_TEXT** | Script data parsing mode. |
| `template` | **ACCEPT** | Pushed onto stack. Formatting marker inserted. Mode pushed to InTemplate. |
| `head` | **DROP** | Duplicate `<head>` ignored entirely. |
| Everything else | **CLOSE_PARENT** | `<head>` is auto-closed (popped). Mode switches to AfterHead. Tag is reprocessed (usually triggers `<body>` generation, then InBody processing). |

**Chrome source detail:** In `processStartTagForInHead`, the catch-all default is:
```
pop head off stack → switch to AfterHead → reprocess token
```

---

### `<body>` — Insertion mode: InBody

Chrome source: `processStartTagForInBody()`

`<body>` accepts virtually everything. It's the universal flow container. All the InBody rules below apply. See **"InBody Mode — Full Tag Dispatch"** section.

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| `html` | **DROP** | Ignored. Attributes merge. |
| `body` | **DROP** | Duplicate ignored. Attributes may merge. |
| `head` | **DROP** | Ignored in body. |
| `frameset` | **DROP** | Ignored (unless only `html`+`head`+`body` on stack and frameset-ok). |
| All block elements | **ACCEPT** | `address`, `article`, `aside`, `blockquote`, `center`, `details`, `dialog`, `dir`, `div`, `dl`, `fieldset`, `figcaption`, `figure`, `footer`, `header`, `hgroup`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `listing`, `search`, `section`, `summary`, `ul` — close `<p>` if in button scope, then insert. |
| `h1`–`h6` | **ACCEPT** | Close `<p>` if in button scope. If current node is a heading, pop it first (heading→heading = close-reopen). Insert. |
| `form` | **ACCEPT** or **DROP** | If form pointer is already set (and no `<template>` on stack), **DROP**. Otherwise close `<p>` if in scope, insert, set form pointer. |
| `li` | **ACCEPT** | Walk stack backwards — if `<li>` found, generate implied end tags (excluding `<li>`), pop to `<li>`. Close `<p>` if in scope. Insert. |
| `dd`, `dt` | **ACCEPT** | Same as `li` but checks for both `<dd>` and `<dt>`. |
| `a` | **ACCEPT** + **RECONSTRUCT_AFE** | If `<a>` in active formatting list: run adoption agency for `<a>`, remove from list/stack. Then reconstruct active formatting, insert `<a>`, push onto active formatting list. |
| `b`, `big`, `code`, `em`, `font`, `i`, `s`, `small`, `strike`, `strong`, `tt`, `u` | **ACCEPT** + **RECONSTRUCT_AFE** | Reconstruct active formatting. Insert element. Push onto active formatting list. |
| `nobr` | **ACCEPT** + **RECONSTRUCT_AFE** | If `<nobr>` in scope, adoption agency for it. Then reconstruct, insert, push onto formatting list. |
| `button` | **CLOSE_REOPEN** or **ACCEPT** | If `<button>` in scope, generate implied end tags, pop to `<button>` (close it). Then reconstruct active formatting, insert new `<button>`. |
| `applet`, `marquee`, `object` | **ACCEPT** + **RECONSTRUCT_AFE** | Reconstruct formatting. Insert. Push formatting marker. |
| `table` | **ACCEPT** | Close `<p>` if in scope (not in quirks mode). Insert. Switch to InTable mode. |
| `area`, `br`, `embed`, `img`, `keygen`, `wbr` | **ACCEPT_VOID** + **RECONSTRUCT_AFE** | Reconstruct formatting. Insert void element. |
| `input` | **ACCEPT_VOID** + **RECONSTRUCT_AFE** | Reconstruct formatting. Insert void element. |
| `param`, `source`, `track` | **ACCEPT_VOID** | Insert void element (no formatting reconstruction). |
| `hr` | **ACCEPT_VOID** | Close `<p>` if in button scope. Insert void element. |
| `textarea` | **ACCEPT** → **RAW_TEXT** | Insert element. Tokenizer switches to RCDATA. Parser mode → Text. |
| `xmp` | **ACCEPT** → **RAW_TEXT** | Close `<p>` if in scope. Reconstruct formatting. Raw text parsing. |
| `iframe` | **ACCEPT** → **RAW_TEXT** | Raw text parsing (content is fallback text). |
| `noembed` | **ACCEPT** → **RAW_TEXT** | Raw text parsing. |
| `noscript` (scripting on) | **ACCEPT** → **RAW_TEXT** | Raw text parsing. |
| `select` | **ACCEPT** | Reconstruct formatting. Insert. Switch to InSelect (or InSelectInTable if in table context). |
| `optgroup`, `option` | **ACCEPT** | If current node is `<option>`, pop it. Reconstruct formatting. Insert. |
| `rb`, `rtc` | **ACCEPT** | If `<ruby>` in scope, generate implied end tags. Insert. |
| `rp`, `rt` | **ACCEPT** | If `<ruby>` in scope, generate implied end tags (excluding `<rtc>`). Insert. |
| `math` | **ACCEPT_FOREIGN** | Reconstruct formatting. Adjust MathML attributes. Insert in MathML namespace. Switch to InForeignContent. |
| `svg` | **ACCEPT_FOREIGN** | Reconstruct formatting. Adjust SVG attributes. Insert in SVG namespace. Switch to InForeignContent. |
| `caption`, `col`, `colgroup`, `frame`, `tbody`, `td`, `tfoot`, `th`, `thead`, `tr` | **DROP** | Parse error. Ignored entirely in InBody mode. |
| `script`, `style`, `template` | **ACCEPT** | Processed via InHead handler (same rules as in `<head>`). |
| `base`, `basefont`, `bgsound`, `link`, `meta`, `noframes`, `title` | **ACCEPT** | Also processed via InHead handler. |
| Any other tag | **ACCEPT** + **RECONSTRUCT_AFE** | Reconstruct active formatting elements. Insert element normally. |

---

### Flow Container Elements — Insertion mode: InBody

**Elements:** `div`, `article`, `section`, `nav`, `aside`, `main`, `search`, `blockquote`, `dialog`, `figcaption`, `figure`, `dd`, `dt`, `noscript`, `address`, `header`, `footer`, `details`, `summary`, `legend`, `fieldset`, `li`

All of these are in InBody mode when open. The child tag behavior is **identical to `<body>` above** — Chrome does not change insertion mode for these elements. The InBody dispatch is the same regardless of which flow container is the current node.

**Key nuance:** The insertion mode is InBody for all of these, but some InBody rules check what's on the *stack of open elements* (not just the current node):

| Parent-specific behavior | Detail |
|-------------------------|--------|
| `form` → `form` | **DROP.** Inner `<form>` dropped if form pointer is already set. |
| Any → `li` | Chrome walks the stack looking for an open `<li>` to close. |
| Any → `dd`/`dt` | Chrome walks the stack looking for open `<dd>` or `<dt>` to close. |
| Any → `button` | Chrome checks if `<button>` is in scope; if so, closes it first. |
| Any → `a` | Chrome checks active formatting list for `<a>`; if found, runs adoption agency. |

**For the purposes of top-level pair analysis, every flow container parent handles every child tag identically to `<body>`.** The stack-walking rules are scope-based, not parent-based.

---

### `<p>` — Insertion mode: InBody

`<p>` is open in InBody mode. Chrome does NOT change mode when entering `<p>`. The critical difference from flow containers is what happens when certain child tags arrive:

**What Chrome actually does:** When a "block" tag arrives in InBody mode, Chrome checks "is there a `<p>` in button scope?" If yes, it generates an implied `</p>` end tag (closing the `<p>`), then inserts the block element.

| Child Tag Category | Chrome Action | Detail |
|-------------------|--------------|--------|
| **Block elements that close `<p>`:** `address`, `article`, `aside`, `blockquote`, `center`, `details`, `dialog`, `dir`, `div`, `dl`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `header`, `hgroup`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `listing`, `search`, `section`, `summary`, `ul` | **CLOSE_P** | Chrome generates implied `</p>`, closes `<p>`, inserts new element as sibling. |
| `h1`–`h6` | **CLOSE_P** | Same — close `<p>`, insert heading as sibling. |
| `hr` | **CLOSE_P** | Close `<p>`, insert `<hr>` as sibling. |
| `table` | **CLOSE_P** (unless quirks) | Close `<p>` (not in quirks mode), insert table. |
| `li` | **CLOSE_P** | After stack walk for previous `<li>`, close `<p>`, insert. |
| Phrasing elements (`span`, `em`, `strong`, `b`, `i`, `u`, `s`, `small`, `cite`, `code`, `var`, `samp`, `kbd`, `sub`, `sup`, `abbr`, `bdi`, `bdo`, `data`, `dfn`, `mark`, `q`, `time`, `output`, `label`, `ruby`, `rt`, `rp`) | **ACCEPT** | Normal inline child in `<p>`. |
| `a` | **ACCEPT** (with adoption agency if nested) | If active `<a>` in formatting list, adoption agency runs. Otherwise reconstruct formatting + insert. |
| `b`, `big`, `code`, `em`, `font`, `i`, `s`, `small`, `strike`, `strong`, `tt`, `u` | **ACCEPT** + **RECONSTRUCT_AFE** | Active formatting elements. |
| Form controls (`input`, `select`, `textarea`, `button`, `meter`, `progress`) | **ACCEPT** | Inline-level. `textarea` switches to RCDATA mode. `button` may close-reopen. |
| Replaced elements (`img`, `iframe`, `embed`, `canvas`, `picture`, `object`, `video`, `audio`, `portal`) | **ACCEPT** | Inline-level in the `<p>`. |
| `br`, `wbr`, `area` | **ACCEPT_VOID** | Void elements. |
| `script`, `style`, `template`, `noscript`, `link`, `meta`, `base`, `title` | **ACCEPT** | Processed via InHead handler. |
| `svg`, `math` | **ACCEPT_FOREIGN** | Foreign content insertion. |
| `caption`, `col`, `colgroup`, `frame`, `tbody`, `td`, `tfoot`, `th`, `thead`, `tr` | **DROP** | Table-internal tags ignored in InBody. |
| `html`, `head`, `body` | **DROP** | Ignored. |

**The `<p>`-closing set (Chrome source — `closeParagraph()` is called from InBody for these tags):**
```
address, article, aside, blockquote, center, details, dialog, dir, div, dl,
fieldset, figcaption, figure, footer, form, h1, h2, h3, h4, h5, h6, header,
hgroup, hr, li, main, menu, nav, ol, p, pre, listing, search, section,
summary, table, ul
```

---

### `<h1>` through `<h6>` — Insertion mode: InBody

Same as `<p>` with one additional rule:

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| Any `h1`–`h6` | **CLOSE_HEADING** | Chrome checks: "is the current node a heading element?" If yes, pops it (parse error). Then closes `<p>` if in scope. Then inserts the new heading. Result: `<h1>A<h2>B</h2>` → `<h1>A</h1><h2>B</h2>`. |
| All other tags | Same as `<p>` | Identical p-closing behavior + phrasing acceptance. |

---

### `<pre>` — Insertion mode: InBody

Identical to `<p>` in terms of parser pair rules. Two minor differences:

1. First newline after `<pre>` opening tag is stripped by the tokenizer.
2. `white-space: pre` is a CSS property (not a parser concern).

All child tag actions are the same as `<p>`.

---

### Inline Phrasing Elements — Insertion mode: InBody

**Elements:** `span`, `em`, `strong`, `b`, `i`, `u`, `s`, `small`, `cite`, `code`, `var`, `samp`, `kbd`, `sub`, `sup`, `abbr`, `bdi`, `bdo`, `data`, `dfn`, `mark`, `q`, `time`, `output`

**Critical difference from `<p>`:** Chrome does NOT auto-close these elements when a block tag arrives.

When `<span>` is open and `<div>` arrives:
- Chrome checks "is `<p>` in button scope?" — no (assuming `<span>` is inside something other than `<p>`)
- Chrome inserts `<div>` as a child of `<span>` in the DOM
- Layout handles the block-in-inline split (anonymous block generation)

| Child Tag Category | Chrome Action | Detail |
|-------------------|--------------|--------|
| Block elements | **ACCEPT** (parser level) | Chrome inserts them as children. **No auto-close.** Layout splits the inline. |
| Phrasing elements | **ACCEPT** | Normal nesting. |
| Active formatting elements | **ACCEPT** + **RECONSTRUCT_AFE** | Same as InBody. |
| All others | Same as InBody | No mode change, same dispatch. |

**The parser does not distinguish inline parents from block parents in InBody mode.** The "close `<p>`" check is scope-based — it only fires if `<p>` is in button scope. Inline elements don't create scope boundaries.

**What this means for pairs:** Every child tag gets the same treatment regardless of whether the current node is `<span>` or `<div>` — the InBody rules are the same. The only behavioral difference is in the `<p>`-in-button-scope check, which depends on what's on the stack, not the immediate parent.

---

### `<label>` — Insertion mode: InBody

Same as inline phrasing elements. Chrome does not enforce the spec restriction against nested `<label>` elements at the parser level. Pair behavior identical to `<span>`.

---

### `<a>` (anchor) — Insertion mode: InBody

**Transparent content model.** Chrome treats `<a>` as a regular InBody element with one special rule:

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| `a` | **ADOPTION_AGENCY** | Chrome checks the active formatting list. If there's already an `<a>`: runs the adoption agency algorithm (closes the outer `<a>` at the point of the inner one), removes from formatting list, then reconstructs formatting and inserts the new `<a>`. Result: `<a href=1>X<a href=2>Y` → `<a href=1>X</a><a href=2>Y</a>`. |
| All other tags | Same as InBody | Identical to flow containers. Chrome does not enforce "no interactive content inside `<a>`" at the parser level. |

---

### `<ins>`, `<del>` — Insertion mode: InBody

Transparent content model. No special parser rules. Same as a regular InBody element. All child tags handled identically to `<div>`.

---

### `<map>`, `<slot>` — Insertion mode: InBody

Transparent. No special parser rules. Same InBody dispatch.

---

### `<canvas>`, `<object>` — Insertion mode: InBody

`<object>` has special handling: it's in the `applet, marquee, object` group which inserts a formatting marker. But for child tag dispatch, same InBody rules apply.

`<canvas>` has no special parser rules. Same InBody dispatch.

| Child Tag | Chrome Action for `<object>` | Detail |
|-----------|------------------------------|--------|
| All tags | Same as InBody | `<object>` pushes a formatting marker, but child dispatch is unchanged. |

---

### `<button>` — Insertion mode: InBody

Same InBody dispatch. No mode change.

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| `button` | **CLOSE_REOPEN** | Chrome checks "is `<button>` in scope?" If yes: generate implied end tags, pop to `<button>`. Then reconstruct formatting, insert new `<button>`. |
| All other tags | Same as InBody | No difference from other flow containers at the parser level. |

---

### `<ul>`, `<ol>`, `<menu>` — Insertion mode: InBody

No mode change. Standard InBody dispatch.

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| `li` | **ACCEPT** | Chrome walks the stack for open `<li>` to close. Closes `<p>` if in scope. Inserts `<li>`. |
| All other tags | Same as InBody | Chrome does NOT restrict `<ul>` to `<li>` children at the parser level. `<div>`, `<span>`, etc. are accepted. |

---

### `<dl>` — Insertion mode: InBody

No mode change. Standard InBody dispatch.

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| `dt` | **ACCEPT** | Chrome walks stack for open `<dd>` or `<dt>` to close. Closes `<p>` if in scope. Inserts `<dt>`. |
| `dd` | **ACCEPT** | Same stack walk. Closes open `<dd>` or `<dt>`. Closes `<p>` if in scope. Inserts `<dd>`. |
| All other tags | Same as InBody | Parser is lenient — accepts anything. |

---

### `<ruby>` — Insertion mode: InBody

No mode change. Standard InBody dispatch with specific handling for ruby children:

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| `rb`, `rtc` | **ACCEPT** | If `<ruby>` is in scope, generate implied end tags. Insert. |
| `rp`, `rt` | **ACCEPT** | If `<ruby>` is in scope, generate implied end tags (excluding `<rtc>`). Insert. |
| All other tags | Same as InBody | Standard dispatch. |

---

### `<hgroup>` — Insertion mode: InBody

No mode change. Standard InBody dispatch. Chrome does NOT enforce the content model restriction (only headings + `<p>`) at the parser level. All child tags handled per InBody rules.

---

### `<table>` — Insertion mode: InTable

Chrome source: `processStartTagForInTable()`

This is where pair rules diverge significantly from InBody.

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| `caption` | **ACCEPT** | Clear stack back to table context. Insert marker on formatting list. Insert `<caption>`. Switch to InCaption. |
| `colgroup` | **ACCEPT** | Clear stack back to table context. Insert `<colgroup>`. Switch to InColumnGroup. |
| `col` | **WRAP** | Clear stack back to table context. Auto-generate `<colgroup>`. Reprocess `<col>` in InColumnGroup. |
| `tbody`, `tfoot`, `thead` | **ACCEPT** | Clear stack back to table context. Insert element. Switch to InTableBody. |
| `tr` | **WRAP** | Clear stack back to table context. Auto-generate `<tbody>`. Reprocess `<tr>` in InTableBody. |
| `td`, `th` | **WRAP** | Clear stack back to table context. Auto-generate `<tbody>`. Reprocess in InTableBody (which will then auto-generate `<tr>`). |
| `table` | **CLOSE_PARENT** | Parse error. If `<table>` not in scope, ignore. Otherwise: pop elements until `<table>` is popped. Reset insertion mode. Reprocess `<table>` token (starts a new sibling table). |
| `style`, `script`, `template` | **ACCEPT** | Processed via InHead handler. |
| `input` | **ACCEPT_VOID** (if type=hidden) or **FOSTER** | If `type="hidden"`: insert void element directly in table (acknowledged self-closing). Otherwise: foster parent. |
| `form` | **DROP** | Parse error. If form pointer set or `<template>` on stack, ignore. Otherwise insert form (removed from stack immediately — not added to open elements). |
| `html` | **DROP** | Processed via InBody (attributes merge). |
| Everything else | **FOSTER** | Parse error. Set foster parenting flag. Process token using InBody rules. The InBody insertion uses foster parent location (before the table) instead of current node. Clear foster flag. |

**Foster parenting detail:** When foster parenting is active, the insertion location is changed from "inside the current table" to "before the table element in its parent." The element is still processed with InBody rules but inserted at the foster parent location.

---

### `<thead>`, `<tbody>`, `<tfoot>` — Insertion mode: InTableBody

Chrome source: `processStartTagForInTableBody()`

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| `tr` | **ACCEPT** | Clear stack back to table body context. Insert `<tr>`. Switch to InRow. |
| `td`, `th` | **WRAP** | Parse error. Clear stack to table body context. Auto-generate `<tr>`. Reprocess tag in InRow. |
| `caption`, `col`, `colgroup`, `tbody`, `tfoot`, `thead` | **CLOSE_PARENT** | If `<tbody>`/`<thead>`/`<tfoot>` not in table scope, ignore. Otherwise: clear to table body context. Pop current element. Switch to InTable. Reprocess tag. |
| Everything else | **FOSTER** | Process as InTable (which foster-parents non-table content). |

---

### `<tr>` — Insertion mode: InRow

Chrome source: `processStartTagForInRow()`

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| `td`, `th` | **ACCEPT** | Clear stack back to table row context. Insert cell. Switch to InCell. Push formatting marker. |
| `caption`, `col`, `colgroup`, `tbody`, `tfoot`, `thead`, `tr` | **CLOSE_PARENT** | If `<tr>` not in table scope, ignore. Otherwise: clear to row context. Pop `<tr>`. Switch to InTableBody. Reprocess tag (for `tr`: opens new row; for others: closes tbody too). |
| Everything else | **FOSTER** | Process as InTable (foster parent). |

---

### `<td>`, `<th>` — Insertion mode: InCell

Chrome source: `processStartTagForInCell()`

InCell delegates to InBody for most tags, but intercepts table-structure tags:

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| `caption`, `col`, `colgroup`, `tbody`, `td`, `tfoot`, `th`, `thead`, `tr` | **CLOSE_PARENT** | If `<td>`/`<th>` not in table scope, ignore. Otherwise: close the cell (generate implied end tags, pop to cell, pop cell). Clear formatting to marker. Switch to InRow. Reprocess tag. |
| Everything else | Same as InBody | Cell is a flow container. Full InBody dispatch applies. |

---

### `<caption>` — Insertion mode: InCaption

Chrome source: `processStartTagForInCaption()`

InCaption delegates to InBody for most tags, but intercepts table-structure tags:

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| `caption`, `col`, `colgroup`, `tbody`, `td`, `tfoot`, `th`, `thead`, `tr` | **CLOSE_PARENT** | If `<caption>` not in table scope, ignore. Otherwise: generate implied end tags. Pop to `<caption>`. Pop caption. Clear formatting to marker. Switch to InTable. Reprocess tag. |
| Everything else | Same as InBody | Caption is a flow container. Full InBody dispatch applies. |

---

### `<colgroup>` — Insertion mode: InColumnGroup

Chrome source: `processStartTagForInColumnGroup()`

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| `html` | **DROP** | Processed via InBody (attributes merge). |
| `col` | **ACCEPT_VOID** | Insert void `<col>` element. |
| `template` | **ACCEPT** | Processed via InHead handler. |
| Everything else | **CLOSE_PARENT** | If current node is not `<colgroup>`, ignore (e.g., if it's a fragment case). Otherwise: pop `<colgroup>`. Switch to InTable. Reprocess tag. |

---

### `<select>` — Insertion mode: InSelect

Chrome source: `processStartTagForInSelect()`

This is one of the most restrictive modes. Almost everything is dropped.

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| `html` | **DROP** | Processed via InBody (attributes merge). |
| `option` | **ACCEPT** | If current node is `<option>`, pop it first (auto-close previous option). Insert new `<option>`. |
| `optgroup` | **ACCEPT** | If current is `<option>`, pop it. If current is `<optgroup>`, pop it. Insert new `<optgroup>`. |
| `hr` | **ACCEPT_VOID** | If current is `<option>`, pop. If current is `<optgroup>`, pop. Insert void `<hr>`. |
| `select` | **CLOSE_PARENT** | Parse error. If `<select>` not in scope, ignore. Otherwise: pop to `<select>`. Pop `<select>`. Reset insertion mode. (Effectively closes the select.) |
| `input`, `keygen`, `textarea` | **CLOSE_PARENT** | Parse error. If `<select>` not in scope, ignore. Otherwise: pop to `<select>`. Pop `<select>`. Reset insertion mode. Reprocess tag in new mode. |
| `script`, `template` | **ACCEPT** | Processed via InHead handler. |
| Everything else | **DROP** | Parse error. Token ignored. |

**InSelectInTable variant:** If `<select>` is inside a `<table>`, Chrome uses InSelectInTable mode which additionally handles:

| Child Tag | Chrome Action in InSelectInTable | Detail |
|-----------|----------------------------------|--------|
| `caption`, `table`, `tbody`, `tfoot`, `thead`, `tr`, `td`, `th` | **CLOSE_PARENT** | Pop to `<select>`, pop it, reset mode, reprocess. |
| Everything else | Same as InSelect | Standard InSelect dispatch. |

---

### `<optgroup>` — Insertion mode: InSelect

Same mode as `<select>`. Pair behavior:

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| `option` | **ACCEPT** | If current is `<option>`, pop. Insert option. |
| `optgroup` | **CLOSE_PARENT** | If current is `<option>`, pop. If current is `<optgroup>`, pop (close current optgroup). Insert new `<optgroup>`. |
| `select` | **CLOSE_PARENT** | Pop to select, close it. |
| `hr` | **ACCEPT_VOID** | Pop option/optgroup if current. Insert void hr. |
| Everything else | Same as InSelect | Most things are **DROP**ped. |

---

### `<option>` — Insertion mode: InSelect

Same InSelect mode.

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| `option` | **CLOSE_PARENT** | Current `<option>` popped. New option inserted. |
| `optgroup` | **CLOSE_PARENT** | Current `<option>` popped. Then optgroup handling. |
| Everything else | Same as InSelect | Most things **DROP**ped. Text content IS accepted. |

---

### `<script>` — Insertion mode: Text (Script Data)

Chrome source: Tokenizer switches to Script Data state.

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| **Everything** | **RAW_TEXT** | No tags are parsed. All content is treated as raw text until `</script>` is encountered. Even HTML entities are NOT decoded. |

---

### `<style>` — Insertion mode: Text (Raw Text)

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| **Everything** | **RAW_TEXT** | No tags parsed. Raw text until `</style>`. Entities NOT decoded. |

---

### `<title>` — Insertion mode: Text (RCDATA)

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| **Everything** | **RAW_TEXT** | RCDATA mode — tags not parsed, but HTML entities ARE decoded. Until `</title>`. |

---

### `<textarea>` — Insertion mode: Text (RCDATA)

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| **Everything** | **RAW_TEXT** | RCDATA mode. Entities decoded. First newline stripped. Until `</textarea>`. |

---

### `<iframe>` — Insertion mode: Text (Raw Text)

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| **Everything** | **RAW_TEXT** | Raw text mode. Content is fallback (ignored by modern browsers). Until `</iframe>`. |

---

### `<template>` — Insertion mode: InTemplate

Chrome source: `processStartTagForInTemplate()`

Template uses a stack of template insertion modes. The initial mode depends on what tag arrives:

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| `base`, `basefont`, `bgsound`, `link`, `meta`, `noframes`, `script`, `style`, `template`, `title` | **ACCEPT** | Process via InHead. |
| `caption`, `colgroup`, `col`, `tbody`, `tfoot`, `thead` | **ACCEPT** | Pop template mode. Push InTable. Reprocess. |
| `tr` | **ACCEPT** | Pop template mode. Push InTableBody. Reprocess. |
| `td`, `th` | **ACCEPT** | Pop template mode. Push InRow. Reprocess. |
| Everything else | **ACCEPT** | Pop template mode. Push InBody. Reprocess. |

**Effectively:** `<template>` accepts everything. Content is parsed into a DocumentFragment. The template adapts its internal mode based on what tags appear.

---

### `<svg>` — Insertion mode: InForeignContent

Chrome source: `processStartTagForInForeignContent()`

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| `b`, `big`, `blockquote`, `body`, `br`, `center`, `code`, `dd`, `div`, `dl`, `dt`, `em`, `embed`, `h1`–`h6`, `head`, `hr`, `i`, `img`, `li`, `listing`, `menu`, `meta`, `nobr`, `ol`, `p`, `pre`, `ruby`, `s`, `small`, `span`, `strong`, `strike`, `sub`, `sup`, `table`, `tt`, `u`, `ul`, `var` | **EXIT_FOREIGN** | Parse error. Pop elements from the stack until reaching a MathML text integration point, HTML integration point, or HTML namespace element. Reprocess the tag in the new (HTML) insertion mode. |
| `font` (with `color`, `face`, or `size` attribute) | **EXIT_FOREIGN** | Same as above — exits foreign content. |
| All other tags (including SVG elements like `rect`, `circle`, `path`, `g`, `text`, `defs`, `use`, `line`, `polyline`, `polygon`, `ellipse`, `image`, `clipPath`, `mask`, `filter`, etc.) | **ACCEPT_FOREIGN** | Inserted in SVG namespace. Attribute names are adjusted (camelCase for SVG: `viewBox`, `preserveAspectRatio`, etc.). |

**The EXIT_FOREIGN set (46 elements):**
```
b, big, blockquote, body, br, center, code, dd, div, dl, dt, em, embed,
font (with specific attrs), h1, h2, h3, h4, h5, h6, head, hr, i, img,
li, listing, menu, meta, nobr, ol, p, pre, ruby, s, small, span, strong,
strike, sub, sup, table, tt, u, ul, var
```

---

### `<math>` — Insertion mode: InForeignContent

Same rules as `<svg>`. The EXIT_FOREIGN set is identical.

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| EXIT_FOREIGN set | **EXIT_FOREIGN** | Pops back to HTML context. |
| MathML elements and unknown tags | **ACCEPT_FOREIGN** | Inserted in MathML namespace. |

**Additional integration points:** `<annotation-xml>` with `encoding="text/html"` or `encoding="application/xhtml+xml"` is an HTML integration point — its children are parsed in HTML mode.

---

### Void Elements — Cannot be parents

**Elements:** `area`, `base`, `br`, `col`, `embed`, `hr`, `img`, `input`, `link`, `meta`, `param`, `portal`, `source`, `track`, `wbr`

| Child Tag | Chrome Action | Detail |
|-----------|--------------|--------|
| **Everything** | **N/A (VOID)** | Void elements are self-closing. Chrome never enters a state where a void element is "the current node" expecting children. Any content after a void element becomes a sibling. |

---

### `<rp>` — Insertion mode: InBody

`<rp>` is a text-only element per spec, but Chrome does not switch to a special mode. It remains in InBody. The spec says `<rp>` should only contain text, but Chrome accepts any InBody content.

For pair analysis purposes: **same as InBody flow container.** Chrome doesn't restrict children.

---

### `<datalist>` — Insertion mode: InBody

No special mode. Standard InBody dispatch. Chrome accepts `<option>` elements and phrasing content without restriction.

---

### `<meter>`, `<progress>` — Insertion mode: InBody

No special mode. Standard InBody dispatch. These are replaced elements but Chrome doesn't restrict their parser-level children.

---

### `<video>`, `<audio>` — Insertion mode: InBody

Transparent content model. No special parser mode. Standard InBody dispatch. `<source>` and `<track>` are accepted as void elements within, and any other content is fallback.

---

### `<picture>` — Insertion mode: InBody

Transparent/special content model. No special parser mode. Standard InBody dispatch. `<source>` elements and `<img>` are the expected children, but Chrome doesn't restrict others.

---

## Part 2 — Complete Pair Action Matrix

Every parent classified by its Chrome insertion mode, with the action for every child.

### Parent Group A: InBody Mode (89 elements)

These all share the **same Chrome parser dispatch** (InBody mode):

```
a, abbr, address, article, aside, audio, b, bdi, bdo, blockquote, body,
button, canvas, cite, code, data, datalist, dd, del, details, dfn, dialog,
div, dl, dt, em, fieldset, figcaption, figure, footer, form, h1, h2, h3,
h4, h5, h6, header, hgroup, hr (as parent: void), i, ins, kbd, label,
legend, li, main, map, mark, meter, nav, noscript, object, ol, output,
p, picture, pre, progress, q, rp, rt, ruby, s, samp, search, section,
select (InSelect override), slot, small, span, strong, sub, summary, sup,
table (InTable override), tbody/thead/tfoot (InTableBody override),
td/th (InCell override), time, tr (InRow override), u, ul, var, video, wbr (void)
```

**Excluding elements with mode overrides** (table/select/void/rawtext/foreign), there are **73 elements** that share identical InBody child-handling:

For these 73 parents, here is the **complete child action map** (116 children):

| Child | Action | Chrome Detail |
|-------|--------|---------------|
| `a` | ACCEPT (adoption agency if nested) | Reconstruct AFE. If `<a>` in formatting list, run adoption agency first. Insert. Push to formatting list. |
| `abbr` | ACCEPT | Reconstruct AFE. Insert. |
| `address` | CLOSE_P + ACCEPT | Close `<p>` if in button scope. Insert. |
| `area` | ACCEPT_VOID | Reconstruct AFE. Insert void. |
| `article` | CLOSE_P + ACCEPT | Close `<p>` if in button scope. Insert. |
| `aside` | CLOSE_P + ACCEPT | Close `<p>` if in button scope. Insert. |
| `audio` | ACCEPT | Reconstruct AFE. Insert. (Transparent, any-other-start-tag.) |
| `b` | ACCEPT + AFE | Reconstruct AFE. Insert. Push to formatting list. |
| `base` | ACCEPT | Processed via InHead handler. |
| `bdi` | ACCEPT | Reconstruct AFE. Insert. |
| `bdo` | ACCEPT | Reconstruct AFE. Insert. |
| `blockquote` | CLOSE_P + ACCEPT | Close `<p>` if in button scope. Insert. |
| `body` | DROP | Ignored. Attributes may merge. |
| `br` | ACCEPT_VOID | Reconstruct AFE. Insert void. |
| `button` | CLOSE_REOPEN or ACCEPT | If `<button>` in scope: close it. Reconstruct AFE. Insert. |
| `canvas` | ACCEPT | Reconstruct AFE. Insert. |
| `caption` | DROP | Ignored in InBody. |
| `cite` | ACCEPT | Reconstruct AFE. Insert. |
| `code` | ACCEPT + AFE | Reconstruct AFE. Insert. Push to formatting list. |
| `col` | DROP | Ignored in InBody. |
| `colgroup` | DROP | Ignored in InBody. |
| `data` | ACCEPT | Reconstruct AFE. Insert. |
| `datalist` | ACCEPT | Reconstruct AFE. Insert. |
| `dd` | ACCEPT | Walk stack for open dd/dt, close. Close `<p>` in scope. Insert. |
| `del` | ACCEPT | Reconstruct AFE. Insert. |
| `details` | CLOSE_P + ACCEPT | Close `<p>` if in button scope. Insert. |
| `dfn` | ACCEPT | Reconstruct AFE. Insert. |
| `dialog` | CLOSE_P + ACCEPT | Close `<p>` if in button scope. Insert. |
| `div` | CLOSE_P + ACCEPT | Close `<p>` if in button scope. Insert. |
| `dl` | CLOSE_P + ACCEPT | Close `<p>` if in button scope. Insert. |
| `dt` | ACCEPT | Walk stack for open dd/dt, close. Close `<p>` in scope. Insert. |
| `em` | ACCEPT + AFE | Reconstruct AFE. Insert. Push to formatting list. |
| `embed` | ACCEPT_VOID | Reconstruct AFE. Insert void. |
| `fieldset` | CLOSE_P + ACCEPT | Close `<p>` if in button scope. Insert. |
| `figcaption` | CLOSE_P + ACCEPT | Close `<p>` if in button scope. Insert. |
| `figure` | CLOSE_P + ACCEPT | Close `<p>` if in button scope. Insert. |
| `footer` | CLOSE_P + ACCEPT | Close `<p>` if in button scope. Insert. |
| `form` | ACCEPT or DROP | If form pointer set (no template): DROP. Else close `<p>`, insert, set pointer. |
| `h1` | CLOSE_P + CLOSE_HEADING + ACCEPT | Close `<p>` in scope. If current is heading, pop it. Insert. |
| `h2` | CLOSE_P + CLOSE_HEADING + ACCEPT | Same as h1. |
| `h3` | CLOSE_P + CLOSE_HEADING + ACCEPT | Same as h1. |
| `h4` | CLOSE_P + CLOSE_HEADING + ACCEPT | Same as h1. |
| `h5` | CLOSE_P + CLOSE_HEADING + ACCEPT | Same as h1. |
| `h6` | CLOSE_P + CLOSE_HEADING + ACCEPT | Same as h1. |
| `head` | DROP | Ignored in InBody. |
| `header` | CLOSE_P + ACCEPT | Close `<p>` if in button scope. Insert. |
| `hgroup` | CLOSE_P + ACCEPT | Close `<p>` if in button scope. Insert. |
| `hr` | CLOSE_P + ACCEPT_VOID | Close `<p>` if in button scope. Insert void. |
| `html` | DROP | Ignored. Attributes merge. |
| `i` | ACCEPT + AFE | Reconstruct AFE. Insert. Push to formatting list. |
| `iframe` | RAW_TEXT | Insert. Switch tokenizer to raw text. Mode → Text. |
| `img` | ACCEPT_VOID | Reconstruct AFE. Insert void. |
| `input` | ACCEPT_VOID | Reconstruct AFE. Insert void. |
| `ins` | ACCEPT | Reconstruct AFE. Insert. |
| `kbd` | ACCEPT | Reconstruct AFE. Insert. |
| `label` | ACCEPT | Reconstruct AFE. Insert. |
| `legend` | ACCEPT | Reconstruct AFE. Insert. (any-other-start-tag path.) |
| `li` | ACCEPT | Walk stack for open `<li>`, close. Close `<p>` in scope. Insert. |
| `link` | ACCEPT | Processed via InHead handler. |
| `main` | CLOSE_P + ACCEPT | Close `<p>` if in button scope. Insert. |
| `map` | ACCEPT | Reconstruct AFE. Insert. |
| `mark` | ACCEPT | Reconstruct AFE. Insert. |
| `math` | ACCEPT_FOREIGN | Reconstruct AFE. Insert in MathML namespace. |
| `menu` | CLOSE_P + ACCEPT | Close `<p>` if in button scope. Insert. |
| `meta` | ACCEPT | Processed via InHead handler. |
| `meter` | ACCEPT | Reconstruct AFE. Insert. |
| `nav` | CLOSE_P + ACCEPT | Close `<p>` if in button scope. Insert. |
| `noscript` | ACCEPT or RAW_TEXT | Scripting on: raw text. Scripting off: InHead handler. |
| `object` | ACCEPT + AFE_MARKER | Reconstruct AFE. Insert. Push formatting marker. |
| `ol` | CLOSE_P + ACCEPT | Close `<p>` if in button scope. Insert. |
| `optgroup` | ACCEPT | If current is `<option>`, pop it. Reconstruct AFE. Insert. |
| `option` | ACCEPT | If current is `<option>`, pop it. Reconstruct AFE. Insert. |
| `output` | ACCEPT | Reconstruct AFE. Insert. |
| `p` | CLOSE_P + ACCEPT | Close `<p>` if in button scope (i.e., close previous `<p>`). Insert new `<p>`. |
| `param` | ACCEPT_VOID | Insert void. (No AFE reconstruction.) |
| `picture` | ACCEPT | Reconstruct AFE. Insert. |
| `portal` | ACCEPT_VOID | Reconstruct AFE. Insert void. (Treated like area/embed group or any-other.) |
| `pre` | CLOSE_P + ACCEPT | Close `<p>` if in button scope. Insert. Skip next newline. |
| `progress` | ACCEPT | Reconstruct AFE. Insert. |
| `q` | ACCEPT | Reconstruct AFE. Insert. |
| `rp` | ACCEPT | If `<ruby>` in scope, generate implied end tags (excl. rtc). Insert. |
| `rt` | ACCEPT | If `<ruby>` in scope, generate implied end tags (excl. rtc). Insert. |
| `ruby` | ACCEPT | Reconstruct AFE. Insert. |
| `s` | ACCEPT + AFE | Reconstruct AFE. Insert. Push to formatting list. |
| `samp` | ACCEPT | Reconstruct AFE. Insert. |
| `script` | ACCEPT | Processed via InHead handler. Raw text mode. |
| `search` | CLOSE_P + ACCEPT | Close `<p>` if in button scope. Insert. |
| `section` | CLOSE_P + ACCEPT | Close `<p>` if in button scope. Insert. |
| `select` | ACCEPT → InSelect | Reconstruct AFE. Insert. Switch to InSelect (or InSelectInTable). |
| `slot` | ACCEPT | Reconstruct AFE. Insert. |
| `small` | ACCEPT + AFE | Reconstruct AFE. Insert. Push to formatting list. |
| `source` | ACCEPT_VOID | Insert void. (No AFE reconstruction.) |
| `span` | ACCEPT | Reconstruct AFE. Insert. |
| `strong` | ACCEPT + AFE | Reconstruct AFE. Insert. Push to formatting list. |
| `style` | ACCEPT | Processed via InHead handler. Raw text mode. |
| `sub` | ACCEPT | Reconstruct AFE. Insert. |
| `summary` | CLOSE_P + ACCEPT | Close `<p>` if in button scope. Insert. |
| `sup` | ACCEPT | Reconstruct AFE. Insert. |
| `svg` | ACCEPT_FOREIGN | Reconstruct AFE. Insert in SVG namespace. |
| `table` | CLOSE_P + ACCEPT → InTable | Close `<p>` if in scope (not quirks). Insert. Switch to InTable. |
| `tbody` | DROP | Ignored in InBody. |
| `td` | DROP | Ignored in InBody. |
| `template` | ACCEPT | Processed via InHead handler. |
| `textarea` | ACCEPT → RCDATA | Insert. Skip next newline. Tokenizer → RCDATA. Mode → Text. |
| `tfoot` | DROP | Ignored in InBody. |
| `th` | DROP | Ignored in InBody. |
| `thead` | DROP | Ignored in InBody. |
| `time` | ACCEPT | Reconstruct AFE. Insert. |
| `title` | ACCEPT | Processed via InHead handler. RCDATA mode. |
| `tr` | DROP | Ignored in InBody. |
| `track` | ACCEPT_VOID | Insert void. |
| `u` | ACCEPT + AFE | Reconstruct AFE. Insert. Push to formatting list. |
| `ul` | CLOSE_P + ACCEPT | Close `<p>` if in button scope. Insert. |
| `var` | ACCEPT | Reconstruct AFE. Insert. |
| `video` | ACCEPT | Reconstruct AFE. Insert. |
| `wbr` | ACCEPT_VOID | Reconstruct AFE. Insert void. |

### Parent Group B: InTable Mode (1 element: `table`)

See `<table>` section above for full 116-child breakdown.

**Summary counts:**
- ACCEPT: 9 children (`caption`, `colgroup`, `tbody`, `tfoot`, `thead`, `script`, `style`, `template`, hidden `input`)
- WRAP: 4 children (`col` → colgroup, `tr` → tbody, `td` → tbody+tr, `th` → tbody+tr)
- CLOSE_PARENT: 1 child (`table` → closes current table)
- DROP: 2 children (`form`, `html`)
- FOSTER: 100 children (everything else)

### Parent Group C: InTableBody Mode (3 elements: `thead`, `tbody`, `tfoot`)

**Summary counts:**
- ACCEPT: 3 children (`tr`, `script`, `template`)
- WRAP: 2 children (`td`, `th` → auto-generate `<tr>`)
- CLOSE_PARENT: 5 children (`caption`, `col`, `colgroup`, `tbody`/`thead`/`tfoot` — close current, reprocess)
- FOSTER: 106 children (everything else → process as InTable → foster parent)

### Parent Group D: InRow Mode (1 element: `tr`)

**Summary counts:**
- ACCEPT: 4 children (`td`, `th`, `script`, `template`)
- CLOSE_PARENT: 7 children (`caption`, `col`, `colgroup`, `tbody`, `tfoot`, `thead`, `tr`)
- FOSTER: 105 children (everything else)

### Parent Group E: InCell Mode (2 elements: `td`, `th`)

**Summary counts:**
- CLOSE_PARENT: 9 children (`caption`, `col`, `colgroup`, `tbody`, `td`, `tfoot`, `th`, `thead`, `tr`)
- Same as InBody: 107 children (full flow container behavior)

### Parent Group F: InColumnGroup Mode (1 element: `colgroup`)

**Summary counts:**
- ACCEPT: 2 children (`col`, `template`)
- DROP: 1 child (`html`)
- CLOSE_PARENT: 113 children (everything else closes colgroup)

### Parent Group G: InSelect Mode (3 elements: `select`, `optgroup`, `option`)

**Summary counts for `select`:**
- ACCEPT: 5 children (`option`, `optgroup`, `hr`, `script`, `template`)
- CLOSE_PARENT: 4 children (`select`, `input`, `keygen`, `textarea`)
- DROP: 1 child (`html`)
- DROP: 106 children (everything else is ignored)

### Parent Group H: InCaption Mode (1 element: `caption`)

**Summary counts:**
- CLOSE_PARENT: 9 children (`caption`, `col`, `colgroup`, `tbody`, `td`, `tfoot`, `th`, `thead`, `tr`)
- Same as InBody: 107 children

### Parent Group I: Raw Text Mode (5 elements: `script`, `style`, `title`, `textarea`, `iframe`)

**Summary counts:**
- RAW_TEXT: 116 children (nothing is parsed as tags)

### Parent Group J: InForeignContent Mode (2 elements: `svg`, `math`)

**Summary counts:**
- EXIT_FOREIGN: ~46 children (the breakout set)
- ACCEPT_FOREIGN: ~70 children (inserted in foreign namespace)

### Parent Group K: Void Elements (15 elements)

**Summary counts:**
- VOID (N/A): 116 children (cannot have children)

### Parent Group L: InTemplate Mode (1 element: `template`)

**Summary counts:**
- ACCEPT: 116 children (everything accepted — mode adapts internally)

---

## Part 3 — Summary Statistics

### Action Distribution Across All 13,456 Pairs

| Action | Pairs | % | Description |
|--------|-------|---|-------------|
| ACCEPT (InBody) | ~7,500 | 55.7% | Standard child insertion in flow/phrasing context |
| CLOSE_P + ACCEPT | ~2,400 | 17.8% | Block element closes open `<p>` then inserts |
| RAW_TEXT | ~580 | 4.3% | Raw text parents (5) × all children |
| VOID | ~1,740 | 12.9% | Void parents (15) × all children — no children possible |
| FOSTER | ~500 | 3.7% | Non-table content in table context |
| DROP | ~600 | 4.5% | Ignored tags (table internals in InBody, most tags in InSelect, structural duplicates) |
| CLOSE_PARENT | ~200 | 1.5% | Auto-close (table structure, colgroup, select, foreign exit) |
| WRAP | ~30 | 0.2% | Auto-generate wrappers (tbody, tr, colgroup) |
| CLOSE_REOPEN | ~10 | <0.1% | Same-type close (button, li, dt/dd, option) |
| ACCEPT_FOREIGN | ~140 | 1.0% | SVG/MathML namespace insertion |
| EXIT_FOREIGN | ~90 | 0.7% | Break out of foreign content |
| ACCEPT (InTemplate) | ~116 | 0.9% | Template accepts everything |

### The 7 Chrome Code Paths That Cover All 13,456 Pairs

1. **InBody any-other-start-tag** (reconstruct AFE + insert) — covers ~55% of pairs
2. **InBody block-element** (close-p + insert) — covers ~18% of pairs
3. **Void parent** (no children) — covers ~13% of pairs
4. **InBody drop** (table internals + structural duplicates) — covers ~5% of pairs
5. **Text mode** (raw text / RCDATA) — covers ~4% of pairs
6. **Table modes** (foster + wrap + close + accept) — covers ~4% of pairs
7. **Foreign content** (exit or accept in namespace) — covers ~2% of pairs

---

## Part 4 — Chrome Source File References

| File | Role |
|------|------|
| `html_tree_builder.cc` | Main tree construction: `ProcessStartTag()`, `ProcessEndTag()`, per-mode dispatch |
| `html_tree_builder.h` | Insertion mode enum, tree builder state |
| `html_element_stack.cc` | Stack of open elements, scope checking (`HasInScope`, `HasInButtonScope`, etc.) |
| `html_formatting_element_list.cc` | Active formatting element list, reconstruction, adoption agency |
| `html_token.h` | Token types (start tag, end tag, character, comment, DOCTYPE) |
| `html_tokenizer.cc` | Tokenizer states (Data, RCDATA, RawText, ScriptData, etc.) |
| `html_construction_site.cc` | DOM tree manipulation: `InsertHTMLElement()`, `InsertForeignElement()`, foster parenting |

All files under: `third_party/blink/renderer/core/html/parser/`

### Verified Function Signatures (from source)

| Function | Lines | Purpose |
|----------|-------|---------|
| `ProcessStartTagForInBody()` | 687–1104 | All start tag handling in InBody mode |
| `ProcessEndTagForInBody()` | 1934–2102 | All end tag handling in InBody mode |
| `ProcessCloseWhenNestedTag<IsLi>()` | 550–567 | `<li>` auto-close stack walk |
| `ProcessCloseWhenNestedTag<IsDdOrDt>()` | 550–567 | `<dd>`/`<dt>` auto-close stack walk |
| `ProcessFakePEndTagIfPInButtonScope()` | — | The `<p>`-closing check used by all block elements |
| `CallTheAdoptionAgency()` | 1638–1758 | Formatting element tree restructuring |
| `ProcessAnyOtherEndTagForInBody()` | 1618–1635 | Default end tag: walk stack for match |
| `ProcessStartTagForInTable()` | — | InTable mode start tags |
| `ProcessStartTagForInSelect()` | — | InSelect mode start tags |
| `ProcessStartTagForInForeignContent()` | — | Foreign content breakout logic |

### Chrome-Specific Implementation Details (Verified from Source)

**1. Adoption agency hard limits:**
```
Outer iteration limit: 8
Inner iteration limit: 3
```
If the tree restructuring exceeds these limits, Chrome stops. This prevents pathological
markup from causing O(n²) parser behavior.

**2. `<li>` stack walk skips `<address>`, `<div>`, `<p>`:**
When Chrome walks the stack looking for an open `<li>` to auto-close, it normally stops
at any "special node." But it makes exceptions for `<address>`, `<div>`, and `<p>` —
these are not treated as stop boundaries. Same logic for `<dd>`/`<dt>`.

**3. `<option>` and `<optgroup>` have dual InBody behavior:**
When these tags arrive in InBody mode (not in InSelect), Chrome checks if `<select>` is
in scope. If so, it generates implied end tags and does scope-aware handling. If not, they
fall through to reconstruct-AFE + insert. This is relevant when `<option>` appears
orphaned outside a `<select>`.

**4. `<table>` quirks mode exception:**
In quirks mode, `<table>` does NOT auto-close `<p>`. In standards/almost-standards mode,
it does. This is the only tag where the `<p>`-closing behavior depends on the document's
compat mode.

**5. `<select>` nesting prevention:**
When `<select>` start tag arrives and `<select>` is already in scope, Chrome auto-closes
the existing `<select>` (parse error). New Chrome feature (`InputInSelectEnabled`) also
allows `<input>` to auto-close `<select>` when in scope.

**6. `<image>` → `<img>` rewriting:**
Chrome rewrites `<image>` start tags to `<img>` (with comment: "Apparently we're not
supposed to ask."). Not in our 116-element list but worth noting for robustness.

---

## Part 5 — Corrections to generate_nesting_pairs.py Model

The existing script models Chrome's behavior well, but a few classifications deviate
from Chrome's actual parser:

### 1. `<rp>` is NOT raw text in Chrome

**Script says:** `RAW_TEXT` (rp treated as text-only)
**Chrome actually does:** `<rp>` is a normal InBody element. Chrome does NOT switch to
any special mode. `<rp>` accepts all children just like `<span>`. The HTML spec says
`<rp>` should contain only text, but Chrome does not enforce this at the parser level.

**Impact:** 116 pairs misclassified. `rp → [any child]` should be InBody rules, not RAW_TEXT.

### 2. `<option>` context-dependent behavior

**Script says:** `RAW_TEXT` (option treated as text-only)
**Chrome actually does:** When inside `<select>` (InSelect mode), `<option>` effectively
only contains text because InSelect drops all other tags. But this is InSelect mode's
restriction, not `<option>`'s own behavior. When `<option>` appears orphaned outside
`<select>` (in InBody mode), Chrome treats it as a regular element that can have children.

**For top-level pair analysis:** The script's RAW_TEXT classification is functionally
correct for `<option>` inside `<select>` (the only spec-valid position), because InSelect
mode blocks all child tags. But the mechanism is different: it's the mode (InSelect) that
drops tags, not `<option>` itself switching to raw text.

**Impact:** Functionally equivalent for spec-valid HTML. No pairs misclassified in practice.

### 3. `<p>`-closing is child-triggered, not parent-triggered

**Script model:** `PHRASING_BLOCK_PARENT → block child = CLOSE_PARENT`
**Chrome model:** When ANY block element tag arrives in InBody, Chrome checks "is `<p>` in
button scope?" If yes, closes `<p>` first. This check runs regardless of the current node.

**For top-level pair analysis:** When `<p>` IS the current node (direct parent), the
scope check always finds it → CLOSE_PARENT is correct. When `<span>` is the current
node (with no `<p>` ancestor), the check finds nothing → ACCEPT is correct. So the
script's pair classifications are correct for single-level analysis.

**The nuance matters only for multi-level nesting:** If `<p>` contains `<span>` and
`<div>` arrives, Chrome closes `<p>` (popping `<span>` too). This is a stack-scope
behavior, not a parent-child pair rule. It's one of the chain-sensitive cases the
pairwise model will need to address in Stage 3.

### 4. CLOSE_REOPEN is a scope check, not parent-child

**Script model:** `button → button = CLOSE_REOPEN` (parent is button)
**Chrome model:** When `<button>` tag arrives in InBody, Chrome checks "is `<button>` in
scope?" — walking the entire stack. If there's a `<button>` anywhere up the ancestor
chain (within scope boundaries), Chrome closes it.

**For top-level pair analysis:** When `<button>` is the direct parent, the scope check
always finds it → CLOSE_REOPEN is correct. Same for `li → li`, `dt → dd`, etc.

### 5. Adoption agency for `<a>` is formatting-list-based

**Script model:** `a → a = DROP`
**Chrome model:** When `<a>` arrives in InBody, Chrome checks the *active formatting
element list* (not the stack of open elements) for an existing `<a>`. If found, it runs
the adoption agency algorithm, which restructures the tree to close the outer `<a>`.

**For top-level pair analysis:** When `<a>` is the direct parent and inner `<a>` arrives,
the outer `<a>` IS on the formatting list → adoption agency runs → outer closes → inner
opens. The script's DROP is a simplification — the actual mechanism is adoption agency,
which preserves content differently than a simple drop.

### Summary of corrections needed

| Pair Set | Script Classification | Chrome Actual | Fix Needed? |
|----------|----------------------|---------------|-------------|
| `rp → *` (116 pairs) | RAW_TEXT | InBody (ACCEPT all) | **Yes** — rp should be FLOW_PARENT or INLINE_PHRASING_PARENT |
| `option → *` (116 pairs) | RAW_TEXT | InSelect mode drops tags | **No** — functionally equivalent |
| `a → a` (1 pair) | DROP | Adoption agency | **Minor** — mechanism differs but top-level result is same |
| P-closing pairs | CLOSE_PARENT | Scope-based close | **No** — correct for top-level |
| CLOSE_REOPEN pairs | CLOSE_REOPEN | Scope-based close | **No** — correct for top-level |

# HTML Element Nesting Pairs — Full Enumeration

Every parent × child pair with parser action and browser behavior.
Generated from the HTML parsing spec rules. For analysis by the Pane pair-rule engine.

**Total elements:** 116
**Total pairs:** 13456

## Action Legend

| Code | Meaning |
|------|---------|
| `ACCEPT` | Valid child. Parser inserts as child node. Normal layout per display values. |
| `ACCEPT_BLOCK_IN_FLOW` | Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings. |
| `ACCEPT_CAPTION` | Valid. display:table-caption, positioned per caption-side. Establishes BFC. |
| `ACCEPT_CELL` | Valid. Table cell establishes new BFC. Sized by table layout algorithm. |
| `ACCEPT_COL` | Valid. Column definition for table layout. |
| `ACCEPT_DD` | Valid. Block box with margin-inline-start:40px (UA default). |
| `ACCEPT_DT` | Valid. Block box. No indentation. |
| `ACCEPT_FOREIGN` | Valid. Parsed in SVG/MathML namespace with foreign content rules. |
| `ACCEPT_HEADING_IN_HGROUP` | Valid. Heading element inside hgroup for multi-level heading. |
| `ACCEPT_HIDDEN` | Valid. Element generates no layout boxes (display:none or inert content). |
| `ACCEPT_INERT` | Valid. Parsed into DocumentFragment but not rendered (template content). |
| `ACCEPT_INLINE_IN_FLOW` | Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs. |
| `ACCEPT_LIST_ITEM` | Valid. Child renders as display:list-item with marker box (bullet/number). |
| `ACCEPT_OPTGROUP` | Valid. Labeled group inside dropdown. Platform-native rendering. |
| `ACCEPT_OPTION` | Valid. Platform-native rendering inside dropdown. Not CSS-laid-out. |
| `ACCEPT_PHRASING` | Valid phrasing child. Inline box in parent's inline formatting context. |
| `ACCEPT_REPLACED` | Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides. |
| `ACCEPT_RUBY_PAREN` | Valid. display:none in ruby-aware browsers. Fallback parentheses. |
| `ACCEPT_RUBY_TEXT` | Valid. display:ruby-text. Small annotation above/beside base text. |
| `ACCEPT_SOURCE` | Valid. Media source candidate. Not rendered. |
| `ACCEPT_TABLE_CHILD` | Valid table structural child. Processed in table formatting context. |
| `ACCEPT_TABLE_IN_FLOW` | Valid. Table establishes table formatting context as block-level box. |
| `ACCEPT_TR` | Valid. Table row processed in row context. |
| `ACCEPT_TRACK` | Valid. Text track for media. Not visually rendered in parent. |
| `BLOCK_IN_INLINE` | Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated. |
| `CLOSE_PARENT` | Parser auto-closes parent. Child becomes sibling of (now closed) parent. Re-processed in grandparent context. |
| `CLOSE_PARENT_CAPTION` | Parser auto-closes caption. Tag processed in table context. |
| `CLOSE_PARENT_CELL` | Parser auto-closes table cell. Tag processed in row/table context. |
| `CLOSE_PARENT_COLGROUP` | Parser auto-closes colgroup. Tag processed in table context. |
| `CLOSE_PARENT_FOREIGN` | HTML element exits foreign content. SVG/MathML context closed. Tag processed in HTML mode. |
| `CLOSE_PARENT_HEADING` | Parser auto-closes heading. New heading becomes sibling of closed heading. |
| `CLOSE_PARENT_OPTGROUP` | Parser auto-closes optgroup. Tag processed in select context. |
| `CLOSE_PARENT_OPTION` | Parser auto-closes option. New option/optgroup opens in select context. |
| `CLOSE_PARENT_P` | Parser auto-closes <p>. Block child becomes sibling after the closed paragraph. |
| `CLOSE_PARENT_ROW` | Parser auto-closes table row. Tag processed in table-body context. |
| `CLOSE_PARENT_SELECT` | Parser auto-closes select. Tag processed in parent context. |
| `CLOSE_PARENT_TBODY` | Parser auto-closes row group. Tag processed in table context. |
| `CLOSE_REOPEN_BUTTON` | Previous <button> auto-closed. New <button> opens. |
| `CLOSE_REOPEN_DD` | Previous <dt> or <dd> auto-closed. New <dd> opens as sibling in dl. |
| `CLOSE_REOPEN_DT` | Previous <dt> or <dd> auto-closed. New <dt> opens as sibling in dl. |
| `CLOSE_REOPEN_LI` | Previous <li> auto-closed. New <li> opens as sibling in same list. |
| `CLOSE_REOPEN_RP` | Previous <rt> auto-closed. <rp> opens. |
| `CLOSE_REOPEN_RT` | Previous <rt> auto-closed. New <rt> opens. |
| `DROP` | Start tag ignored entirely. Content (if any) adopted by current parent. |
| `DROP_BODY` | Duplicate <body> tag ignored. Attributes may merge. |
| `DROP_FORM` | Nested <form> tag dropped. Children become part of outer form. |
| `DROP_HEAD` | Duplicate <head> tag ignored. |
| `DROP_SELECT` | Tag ignored in select context. Only option/optgroup/hr accepted. |
| `FOSTER` | Foster parenting. Non-table content moved BEFORE the table ancestor in DOM. |
| `RAW_TEXT` | Parent is in raw text mode. No child elements parsed. All content is text until end tag. |
| `RECONSTRUCT` | Active formatting element reconstructed across tree split boundary. |
| `VOID` | Parent is void element. Cannot have children. Content becomes sibling. |
| `WRAP_TBODY` | Parser auto-generates <tbody>. Child inserted inside generated tbody. |
| `WRAP_TBODY_TR` | Parser auto-generates <tbody> and <tr>. Cell inserted inside generated row. |
| `WRAP_TR` | Parser auto-generates <tr>. Cell inserted inside generated row. |

---

### `<a>` — Rule: `TRANSPARENT_PARENT`

- **ACCEPT** → `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **DROP** → `a`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<abbr>` — Rule: `INLINE_PHRASING_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<address>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<area>` — Rule: `VOID`

- **VOID** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Parent is void element. Cannot have children. Content becomes sibling.

---

### `<article>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<aside>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<audio>` — Rule: `MEDIA_PARENT`

- **ACCEPT** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `u`, `ul`, `var`, `video`, `wbr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `source`
  - Valid. Media source candidate. Not rendered.

- **ACCEPT** → `track`
  - Valid. Text track for media. Not visually rendered in parent.

---

### `<b>` — Rule: `INLINE_PHRASING_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<base>` — Rule: `VOID`

- **VOID** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Parent is void element. Cannot have children. Content becomes sibling.

---

### `<bdi>` — Rule: `INLINE_PHRASING_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<bdo>` — Rule: `INLINE_PHRASING_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<blockquote>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<body>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<br>` — Rule: `VOID`

- **VOID** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Parent is void element. Cannot have children. Content becomes sibling.

---

### `<button>` — Rule: `BUTTON_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **CLOSE_REOPEN** → `button`
  - Previous <button> auto-closed. New <button> opens.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<canvas>` — Rule: `TRANSPARENT_PARENT`

- **ACCEPT** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

---

### `<caption>` — Rule: `CAPTION_PARENT`

- **ACCEPT** → `legend`, `optgroup`, `option`, `title`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **CLOSE_PARENT** → `caption`, `col`, `colgroup`, `tbody`, `td`, `tfoot`, `th`, `thead`, `tr`
  - Parser auto-closes caption. Tag processed in table context.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<cite>` — Rule: `INLINE_PHRASING_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<code>` — Rule: `INLINE_PHRASING_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<col>` — Rule: `VOID`

- **VOID** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Parent is void element. Cannot have children. Content becomes sibling.

---

### `<colgroup>` — Rule: `COLGROUP_PARENT`

- **ACCEPT** → `col`
  - Valid. Column definition for table layout.

- **ACCEPT** → `template`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **CLOSE_PARENT** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Parser auto-closes colgroup. Tag processed in table context.

---

### `<data>` — Rule: `INLINE_PHRASING_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<datalist>` — Rule: `DATALIST_PARENT`

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `body`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `option`
  - Valid. Platform-native rendering inside dropdown. Not CSS-laid-out.

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

---

### `<dd>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<del>` — Rule: `TRANSPARENT_PARENT`

- **ACCEPT** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

---

### `<details>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<dfn>` — Rule: `INLINE_PHRASING_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<dialog>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<div>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<dl>` — Rule: `DL_PARENT`

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `body`, `caption`, `col`, `colgroup`, `details`, `dialog`, `div`, `dl`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `dd`
  - Valid. Block box with margin-inline-start:40px (UA default).

- **ACCEPT** → `dt`
  - Valid. Block box. No indentation.

- **ACCEPT** → `script`, `template`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

---

### `<dt>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<em>` — Rule: `INLINE_PHRASING_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<embed>` — Rule: `VOID`

- **VOID** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Parent is void element. Cannot have children. Content becomes sibling.

---

### `<fieldset>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<figcaption>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<figure>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<footer>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<form>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

- **DROP** → `form`
  - Nested <form> tag dropped. Children become part of outer form.

---

### `<h1>` — Rule: `PHRASING_BLOCK_PARENT`

- **ACCEPT** → `a`, `abbr`, `area`, `audio`, `b`, `base`, `bdi`, `bdo`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `dfn`, `dt`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `link`, `map`, `mark`, `math`, `meta`, `meter`, `noscript`, `object`, `optgroup`, `option`, `output`, `param`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **CLOSE_PARENT** → `h1`, `h2`, `h3`, `h4`, `h5`, `h6`
  - Parser auto-closes heading. New heading becomes sibling of closed heading.

- **CLOSE_PARENT** → `address`, `article`, `aside`, `blockquote`, `details`, `dialog`, `div`, `dl`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `table`, `ul`
  - Parser auto-closes <p>. Block child becomes sibling after the closed paragraph.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<h2>` — Rule: `PHRASING_BLOCK_PARENT`

- **ACCEPT** → `a`, `abbr`, `area`, `audio`, `b`, `base`, `bdi`, `bdo`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `dfn`, `dt`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `link`, `map`, `mark`, `math`, `meta`, `meter`, `noscript`, `object`, `optgroup`, `option`, `output`, `param`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **CLOSE_PARENT** → `h1`, `h2`, `h3`, `h4`, `h5`, `h6`
  - Parser auto-closes heading. New heading becomes sibling of closed heading.

- **CLOSE_PARENT** → `address`, `article`, `aside`, `blockquote`, `details`, `dialog`, `div`, `dl`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `table`, `ul`
  - Parser auto-closes <p>. Block child becomes sibling after the closed paragraph.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<h3>` — Rule: `PHRASING_BLOCK_PARENT`

- **ACCEPT** → `a`, `abbr`, `area`, `audio`, `b`, `base`, `bdi`, `bdo`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `dfn`, `dt`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `link`, `map`, `mark`, `math`, `meta`, `meter`, `noscript`, `object`, `optgroup`, `option`, `output`, `param`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **CLOSE_PARENT** → `h1`, `h2`, `h3`, `h4`, `h5`, `h6`
  - Parser auto-closes heading. New heading becomes sibling of closed heading.

- **CLOSE_PARENT** → `address`, `article`, `aside`, `blockquote`, `details`, `dialog`, `div`, `dl`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `table`, `ul`
  - Parser auto-closes <p>. Block child becomes sibling after the closed paragraph.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<h4>` — Rule: `PHRASING_BLOCK_PARENT`

- **ACCEPT** → `a`, `abbr`, `area`, `audio`, `b`, `base`, `bdi`, `bdo`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `dfn`, `dt`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `link`, `map`, `mark`, `math`, `meta`, `meter`, `noscript`, `object`, `optgroup`, `option`, `output`, `param`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **CLOSE_PARENT** → `h1`, `h2`, `h3`, `h4`, `h5`, `h6`
  - Parser auto-closes heading. New heading becomes sibling of closed heading.

- **CLOSE_PARENT** → `address`, `article`, `aside`, `blockquote`, `details`, `dialog`, `div`, `dl`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `table`, `ul`
  - Parser auto-closes <p>. Block child becomes sibling after the closed paragraph.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<h5>` — Rule: `PHRASING_BLOCK_PARENT`

- **ACCEPT** → `a`, `abbr`, `area`, `audio`, `b`, `base`, `bdi`, `bdo`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `dfn`, `dt`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `link`, `map`, `mark`, `math`, `meta`, `meter`, `noscript`, `object`, `optgroup`, `option`, `output`, `param`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **CLOSE_PARENT** → `h1`, `h2`, `h3`, `h4`, `h5`, `h6`
  - Parser auto-closes heading. New heading becomes sibling of closed heading.

- **CLOSE_PARENT** → `address`, `article`, `aside`, `blockquote`, `details`, `dialog`, `div`, `dl`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `table`, `ul`
  - Parser auto-closes <p>. Block child becomes sibling after the closed paragraph.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<h6>` — Rule: `PHRASING_BLOCK_PARENT`

- **ACCEPT** → `a`, `abbr`, `area`, `audio`, `b`, `base`, `bdi`, `bdo`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `dfn`, `dt`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `link`, `map`, `mark`, `math`, `meta`, `meter`, `noscript`, `object`, `optgroup`, `option`, `output`, `param`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **CLOSE_PARENT** → `h1`, `h2`, `h3`, `h4`, `h5`, `h6`
  - Parser auto-closes heading. New heading becomes sibling of closed heading.

- **CLOSE_PARENT** → `address`, `article`, `aside`, `blockquote`, `details`, `dialog`, `div`, `dl`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `table`, `ul`
  - Parser auto-closes <p>. Block child becomes sibling after the closed paragraph.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<head>` — Rule: `HEAD`

- **ACCEPT** → `base`, `link`, `meta`, `noscript`, `script`, `style`, `template`, `title`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **CLOSE_PARENT** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `main`, `map`, `mark`, `math`, `menu`, `meter`, `nav`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `textarea`, `tfoot`, `th`, `thead`, `time`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Parser auto-closes parent. Child becomes sibling of (now closed) parent. Re-processed in grandparent context.

- **DROP** → `head`
  - Duplicate <head> tag ignored.

---

### `<header>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<hgroup>` — Rule: `HGROUP_PARENT`

- **ACCEPT** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `h1`, `h2`, `h3`, `h4`, `h5`, `h6`
  - Valid. Heading element inside hgroup for multi-level heading.

- **ACCEPT** → `script`, `template`
  - Valid. Element generates no layout boxes (display:none or inert content).

---

### `<hr>` — Rule: `VOID`

- **VOID** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Parent is void element. Cannot have children. Content becomes sibling.

---

### `<html>` — Rule: `HTML_ROOT`

- **ACCEPT** → `body`, `head`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **WRAP_BODY** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

---

### `<i>` — Rule: `INLINE_PHRASING_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<iframe>` — Rule: `RAW_TEXT`

- **RAW_TEXT** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Parent is in raw text mode. No child elements parsed. All content is text until end tag.

---

### `<img>` — Rule: `VOID`

- **VOID** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Parent is void element. Cannot have children. Content becomes sibling.

---

### `<input>` — Rule: `VOID`

- **VOID** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Parent is void element. Cannot have children. Content becomes sibling.

---

### `<ins>` — Rule: `TRANSPARENT_PARENT`

- **ACCEPT** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

---

### `<kbd>` — Rule: `INLINE_PHRASING_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<label>` — Rule: `INLINE_PHRASING_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<legend>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<li>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<link>` — Rule: `VOID`

- **VOID** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Parent is void element. Cannot have children. Content becomes sibling.

---

### `<main>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<map>` — Rule: `TRANSPARENT_PARENT`

- **ACCEPT** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

---

### `<mark>` — Rule: `INLINE_PHRASING_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<math>` — Rule: `MATHML_PARENT`

- **ACCEPT** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `base`, `bdi`, `bdo`, `button`, `canvas`, `caption`, `cite`, `col`, `colgroup`, `data`, `datalist`, `del`, `details`, `dfn`, `dialog`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `header`, `hgroup`, `html`, `iframe`, `input`, `ins`, `kbd`, `label`, `legend`, `link`, `main`, `map`, `mark`, `math`, `meter`, `nav`, `noscript`, `object`, `optgroup`, `option`, `output`, `param`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `samp`, `script`, `search`, `section`, `select`, `slot`, `source`, `style`, `summary`, `svg`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `video`, `wbr`
  - Valid. Parsed in SVG/MathML namespace with foreign content rules.

- **CLOSE_PARENT** → `b`, `blockquote`, `body`, `br`, `code`, `dd`, `div`, `dl`, `dt`, `em`, `embed`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `hr`, `i`, `img`, `li`, `menu`, `meta`, `ol`, `p`, `pre`, `ruby`, `s`, `small`, `span`, `strong`, `sub`, `sup`, `table`, `u`, `ul`, `var`
  - HTML element exits foreign content. SVG/MathML context closed. Tag processed in HTML mode.

---

### `<menu>` — Rule: `LIST_PARENT`

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `body`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `legend`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `script`, `template`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `li`
  - Valid. Child renders as display:list-item with marker box (bullet/number).

---

### `<meta>` — Rule: `VOID`

- **VOID** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Parent is void element. Cannot have children. Content becomes sibling.

---

### `<meter>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<nav>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<noscript>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<object>` — Rule: `TRANSPARENT_PARENT`

- **ACCEPT** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

---

### `<ol>` — Rule: `LIST_PARENT`

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `body`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `legend`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `script`, `template`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `li`
  - Valid. Child renders as display:list-item with marker box (bullet/number).

---

### `<optgroup>` — Rule: `OPTGROUP_PARENT`

- **ACCEPT** → `option`
  - Valid. Platform-native rendering inside dropdown. Not CSS-laid-out.

- **CLOSE_PARENT** → `optgroup`
  - Parser auto-closes optgroup. Tag processed in select context.

- **CLOSE_PARENT** → `select`
  - Parser auto-closes select. Tag processed in parent context.

- **DROP** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Tag ignored in select context. Only option/optgroup/hr accepted.

---

### `<option>` — Rule: `RAW_TEXT`

- **RAW_TEXT** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Parent is in raw text mode. No child elements parsed. All content is text until end tag.

---

### `<output>` — Rule: `INLINE_PHRASING_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<p>` — Rule: `PHRASING_BLOCK_PARENT`

- **ACCEPT** → `a`, `abbr`, `area`, `audio`, `b`, `base`, `bdi`, `bdo`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `dfn`, `dt`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `link`, `map`, `mark`, `math`, `meta`, `meter`, `noscript`, `object`, `optgroup`, `option`, `output`, `param`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **CLOSE_PARENT** → `address`, `article`, `aside`, `blockquote`, `details`, `dialog`, `div`, `dl`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `table`, `ul`
  - Parser auto-closes <p>. Block child becomes sibling after the closed paragraph.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<param>` — Rule: `VOID`

- **VOID** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Parent is void element. Cannot have children. Content becomes sibling.

---

### `<picture>` — Rule: `PICTURE_PARENT`

- **ACCEPT** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `search`, `section`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `script`, `template`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `img`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `source`
  - Valid. Media source candidate. Not rendered.

---

### `<portal>` — Rule: `VOID`

- **VOID** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Parent is void element. Cannot have children. Content becomes sibling.

---

### `<pre>` — Rule: `PHRASING_BLOCK_PARENT`

- **ACCEPT** → `a`, `abbr`, `area`, `audio`, `b`, `base`, `bdi`, `bdo`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `dfn`, `dt`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `link`, `map`, `mark`, `math`, `meta`, `meter`, `noscript`, `object`, `optgroup`, `option`, `output`, `param`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **CLOSE_PARENT** → `address`, `article`, `aside`, `blockquote`, `details`, `dialog`, `div`, `dl`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `table`, `ul`
  - Parser auto-closes <p>. Block child becomes sibling after the closed paragraph.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<progress>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<q>` — Rule: `INLINE_PHRASING_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<rp>` — Rule: `RAW_TEXT`

- **RAW_TEXT** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Parent is in raw text mode. No child elements parsed. All content is text until end tag.

---

### `<rt>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<ruby>` — Rule: `RUBY_PARENT`

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `body`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `rp`
  - Valid. display:none in ruby-aware browsers. Fallback parentheses.

- **ACCEPT** → `rt`
  - Valid. display:ruby-text. Small annotation above/beside base text.

---

### `<s>` — Rule: `INLINE_PHRASING_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<samp>` — Rule: `INLINE_PHRASING_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<script>` — Rule: `RAW_TEXT`

- **RAW_TEXT** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Parent is in raw text mode. No child elements parsed. All content is text until end tag.

---

### `<search>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<section>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<select>` — Rule: `SELECT_PARENT`

- **ACCEPT** → `hr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `script`, `template`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `optgroup`
  - Valid. Labeled group inside dropdown. Platform-native rendering.

- **ACCEPT** → `option`
  - Valid. Platform-native rendering inside dropdown. Not CSS-laid-out.

- **CLOSE_PARENT** → `input`, `select`, `textarea`
  - Parser auto-closes select. Tag processed in parent context.

- **DROP** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `html`, `i`, `iframe`, `img`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `search`, `section`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Tag ignored in select context. Only option/optgroup/hr accepted.

---

### `<slot>` — Rule: `TRANSPARENT_PARENT`

- **ACCEPT** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

---

### `<small>` — Rule: `INLINE_PHRASING_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<source>` — Rule: `VOID`

- **VOID** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Parent is void element. Cannot have children. Content becomes sibling.

---

### `<span>` — Rule: `INLINE_PHRASING_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<strong>` — Rule: `INLINE_PHRASING_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<style>` — Rule: `RAW_TEXT`

- **RAW_TEXT** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Parent is in raw text mode. No child elements parsed. All content is text until end tag.

---

### `<sub>` — Rule: `INLINE_PHRASING_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<summary>` — Rule: `FLOW_PARENT`

- **ACCEPT** → `caption`, `colgroup`, `legend`, `optgroup`, `option`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<sup>` — Rule: `INLINE_PHRASING_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<svg>` — Rule: `SVG_PARENT`

- **ACCEPT** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `base`, `bdi`, `bdo`, `button`, `canvas`, `caption`, `cite`, `col`, `colgroup`, `data`, `datalist`, `del`, `details`, `dfn`, `dialog`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `header`, `hgroup`, `html`, `iframe`, `input`, `ins`, `kbd`, `label`, `legend`, `link`, `main`, `map`, `mark`, `math`, `meter`, `nav`, `noscript`, `object`, `optgroup`, `option`, `output`, `param`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `samp`, `script`, `search`, `section`, `select`, `slot`, `source`, `style`, `summary`, `svg`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `video`, `wbr`
  - Valid. Parsed in SVG/MathML namespace with foreign content rules.

- **CLOSE_PARENT** → `b`, `blockquote`, `body`, `br`, `code`, `dd`, `div`, `dl`, `dt`, `em`, `embed`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `hr`, `i`, `img`, `li`, `menu`, `meta`, `ol`, `p`, `pre`, `ruby`, `s`, `small`, `span`, `strong`, `sub`, `sup`, `table`, `u`, `ul`, `var`
  - HTML element exits foreign content. SVG/MathML context closed. Tag processed in HTML mode.

---

### `<table>` — Rule: `TABLE_PARENT`

- **ACCEPT** → `caption`
  - Valid. display:table-caption, positioned per caption-side. Establishes BFC.

- **ACCEPT** → `script`, `style`, `template`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `colgroup`, `tbody`, `tfoot`, `thead`
  - Valid table structural child. Processed in table formatting context.

- **DROP** → `form`
  - Nested <form> tag dropped. Children become part of outer form.

- **FOSTER** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `sub`, `summary`, `sup`, `svg`, `table`, `textarea`, `time`, `title`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Foster parenting. Non-table content moved BEFORE the table ancestor in DOM.

- **WRAP_COLGROUP** → `col`
  - Valid. Column definition for table layout.

- **WRAP_TBODY** → `tr`
  - Parser auto-generates <tbody>. Child inserted inside generated tbody.

- **WRAP_TBODY_TR** → `td`, `th`
  - Parser auto-generates <tbody> and <tr>. Cell inserted inside generated row.

---

### `<tbody>` — Rule: `TABLE_BODY_PARENT`

- **ACCEPT** → `script`, `template`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `tr`
  - Valid. Table row processed in row context.

- **CLOSE_PARENT** → `caption`, `colgroup`, `tbody`, `tfoot`, `thead`
  - Parser auto-closes row group. Tag processed in table context.

- **FOSTER** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `cite`, `code`, `col`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `textarea`, `time`, `title`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Foster parenting. Non-table content moved BEFORE the table ancestor in DOM.

- **WRAP_TR** → `td`, `th`
  - Parser auto-generates <tr>. Cell inserted inside generated row.

---

### `<td>` — Rule: `TABLE_CELL_PARENT`

- **ACCEPT** → `legend`, `optgroup`, `option`, `title`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **CLOSE_PARENT** → `caption`, `colgroup`, `tbody`, `td`, `tfoot`, `th`, `thead`, `tr`
  - Parser auto-closes table cell. Tag processed in row/table context.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<template>` — Rule: `TEMPLATE_PARENT`

- **ACCEPT** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Valid. Parsed into DocumentFragment but not rendered (template content).

---

### `<textarea>` — Rule: `RAW_TEXT`

- **RAW_TEXT** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Parent is in raw text mode. No child elements parsed. All content is text until end tag.

---

### `<tfoot>` — Rule: `TABLE_BODY_PARENT`

- **ACCEPT** → `script`, `template`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `tr`
  - Valid. Table row processed in row context.

- **CLOSE_PARENT** → `caption`, `colgroup`, `tbody`, `tfoot`, `thead`
  - Parser auto-closes row group. Tag processed in table context.

- **FOSTER** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `cite`, `code`, `col`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `textarea`, `time`, `title`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Foster parenting. Non-table content moved BEFORE the table ancestor in DOM.

- **WRAP_TR** → `td`, `th`
  - Parser auto-generates <tr>. Cell inserted inside generated row.

---

### `<th>` — Rule: `TABLE_CELL_PARENT`

- **ACCEPT** → `legend`, `optgroup`, `option`, `title`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `address`, `article`, `aside`, `blockquote`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `li`, `main`, `menu`, `nav`, `ol`, `p`, `pre`, `search`, `section`, `summary`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `area`, `base`, `col`, `input`, `link`, `meta`, `noscript`, `param`, `script`, `source`, `style`, `template`, `track`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `b`, `bdi`, `bdo`, `cite`, `code`, `data`, `del`, `dfn`, `em`, `i`, `ins`, `kbd`, `label`, `map`, `mark`, `output`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `slot`, `small`, `span`, `strong`, `sub`, `sup`, `time`, `u`, `var`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `br`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `audio`, `button`, `canvas`, `datalist`, `embed`, `iframe`, `img`, `math`, `meter`, `object`, `picture`, `portal`, `progress`, `select`, `svg`, `textarea`, `video`
  - Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.

- **ACCEPT** → `table`
  - Valid. Table establishes table formatting context as block-level box.

- **CLOSE_PARENT** → `caption`, `colgroup`, `tbody`, `td`, `tfoot`, `th`, `thead`, `tr`
  - Parser auto-closes table cell. Tag processed in row/table context.

- **DROP** → `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

- **DROP** → `body`
  - Duplicate <body> tag ignored. Attributes may merge.

---

### `<thead>` — Rule: `TABLE_BODY_PARENT`

- **ACCEPT** → `script`, `template`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `tr`
  - Valid. Table row processed in row context.

- **CLOSE_PARENT** → `caption`, `colgroup`, `tbody`, `tfoot`, `thead`
  - Parser auto-closes row group. Tag processed in table context.

- **FOSTER** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `cite`, `code`, `col`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `textarea`, `time`, `title`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Foster parenting. Non-table content moved BEFORE the table ancestor in DOM.

- **WRAP_TR** → `td`, `th`
  - Parser auto-generates <tr>. Cell inserted inside generated row.

---

### `<time>` — Rule: `INLINE_PHRASING_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<title>` — Rule: `RAW_TEXT`

- **RAW_TEXT** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Parent is in raw text mode. No child elements parsed. All content is text until end tag.

---

### `<tr>` — Rule: `TABLE_ROW_PARENT`

- **ACCEPT** → `td`, `th`
  - Valid. Table cell establishes new BFC. Sized by table layout algorithm.

- **ACCEPT** → `script`, `template`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **CLOSE_PARENT** → `caption`, `colgroup`, `tbody`, `tfoot`, `thead`, `tr`
  - Parser auto-closes table row. Tag processed in table-body context.

- **FOSTER** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `cite`, `code`, `col`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `textarea`, `time`, `title`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Foster parenting. Non-table content moved BEFORE the table ancestor in DOM.

---

### `<track>` — Rule: `VOID`

- **VOID** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Parent is void element. Cannot have children. Content becomes sibling.

---

### `<u>` — Rule: `INLINE_PHRASING_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<ul>` — Rule: `LIST_PARENT`

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `body`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `legend`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.

- **ACCEPT** → `script`, `template`
  - Valid. Element generates no layout boxes (display:none or inert content).

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.

- **ACCEPT** → `li`
  - Valid. Child renders as display:list-item with marker box (bullet/number).

---

### `<var>` — Rule: `INLINE_PHRASING_PARENT`

- **ACCEPT** → `a`, `abbr`, `audio`, `b`, `bdi`, `bdo`, `br`, `button`, `canvas`, `cite`, `code`, `data`, `datalist`, `del`, `dfn`, `em`, `embed`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `map`, `mark`, `math`, `meter`, `noscript`, `object`, `output`, `picture`, `portal`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `sup`, `svg`, `template`, `textarea`, `time`, `u`, `var`, `video`, `wbr`
  - Valid phrasing child. Inline box in parent's inline formatting context.

- **ACCEPT** → `address`, `area`, `article`, `aside`, `base`, `blockquote`, `caption`, `col`, `colgroup`, `dd`, `details`, `dialog`, `div`, `dl`, `dt`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `header`, `hgroup`, `hr`, `legend`, `li`, `link`, `main`, `menu`, `meta`, `nav`, `ol`, `optgroup`, `option`, `p`, `param`, `pre`, `search`, `section`, `source`, `summary`, `table`, `tbody`, `td`, `tfoot`, `th`, `thead`, `title`, `tr`, `track`, `ul`
  - Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.

- **DROP** → `body`, `head`, `html`
  - Start tag ignored entirely. Content (if any) adopted by current parent.

---

### `<video>` — Rule: `MEDIA_PARENT`

- **ACCEPT** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `u`, `ul`, `var`, `video`, `wbr`
  - Valid child. Parser inserts as child node. Normal layout per display values.

- **ACCEPT** → `source`
  - Valid. Media source candidate. Not rendered.

- **ACCEPT** → `track`
  - Valid. Text track for media. Not visually rendered in parent.

---

### `<wbr>` — Rule: `VOID`

- **VOID** → `a`, `abbr`, `address`, `area`, `article`, `aside`, `audio`, `b`, `base`, `bdi`, `bdo`, `blockquote`, `body`, `br`, `button`, `canvas`, `caption`, `cite`, `code`, `col`, `colgroup`, `data`, `datalist`, `dd`, `del`, `details`, `dfn`, `dialog`, `div`, `dl`, `dt`, `em`, `embed`, `fieldset`, `figcaption`, `figure`, `footer`, `form`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `head`, `header`, `hgroup`, `hr`, `html`, `i`, `iframe`, `img`, `input`, `ins`, `kbd`, `label`, `legend`, `li`, `link`, `main`, `map`, `mark`, `math`, `menu`, `meta`, `meter`, `nav`, `noscript`, `object`, `ol`, `optgroup`, `option`, `output`, `p`, `param`, `picture`, `portal`, `pre`, `progress`, `q`, `rp`, `rt`, `ruby`, `s`, `samp`, `script`, `search`, `section`, `select`, `slot`, `small`, `source`, `span`, `strong`, `style`, `sub`, `summary`, `sup`, `svg`, `table`, `tbody`, `td`, `template`, `textarea`, `tfoot`, `th`, `thead`, `time`, `title`, `tr`, `track`, `u`, `ul`, `var`, `video`, `wbr`
  - Parent is void element. Cannot have children. Content becomes sibling.

---

## Summary — Action Distribution

| Action | Count | % of Total |
|--------|-------|-----------|
| `ACCEPT` | 9215 | 68.5% |
| `VOID` | 1740 | 12.9% |
| `RAW_TEXT` | 812 | 6.0% |
| `CLOSE_PARENT` | 620 | 4.6% |
| `FOSTER` | 527 | 3.9% |
| `DROP` | 417 | 3.1% |
| `WRAP_BODY` | 114 | 0.8% |
| `WRAP_TR` | 6 | 0.0% |
| `WRAP_TBODY_TR` | 2 | 0.0% |
| `CLOSE_REOPEN` | 1 | 0.0% |
| `WRAP_COLGROUP` | 1 | 0.0% |
| `WRAP_TBODY` | 1 | 0.0% |
| **Total** | **13456** | **100%** |

## Summary — Parent Rule Distribution

| Parent Rule | # Pairs | # Elements Using Rule |
|-------------|---------|----------------------|
| `FLOW_PARENT` | 3132 | 27 (address, article, aside, blockquote, body...) |
| `INLINE_PHRASING_PARENT` | 2900 | 25 (abbr, b, bdi, bdo, cite...) |
| `VOID` | 1740 | 15 (area, base, br, col, embed...) |
| `PHRASING_BLOCK_PARENT` | 928 | 8 (h1, h2, h3, h4, h5...) |
| `TRANSPARENT_PARENT` | 812 | 7 (a, canvas, del, ins, map...) |
| `RAW_TEXT` | 812 | 7 (iframe, option, rp, script, style...) |
| `LIST_PARENT` | 348 | 3 (menu, ol, ul) |
| `TABLE_BODY_PARENT` | 348 | 3 (tbody, tfoot, thead) |
| `MEDIA_PARENT` | 232 | 2 (audio, video) |
| `TABLE_CELL_PARENT` | 232 | 2 (td, th) |
| `BUTTON_PARENT` | 116 | 1 (button) |
| `CAPTION_PARENT` | 116 | 1 (caption) |
| `COLGROUP_PARENT` | 116 | 1 (colgroup) |
| `DATALIST_PARENT` | 116 | 1 (datalist) |
| `DL_PARENT` | 116 | 1 (dl) |
| `HEAD` | 116 | 1 (head) |
| `HGROUP_PARENT` | 116 | 1 (hgroup) |
| `HTML_ROOT` | 116 | 1 (html) |
| `MATHML_PARENT` | 116 | 1 (math) |
| `OPTGROUP_PARENT` | 116 | 1 (optgroup) |
| `PICTURE_PARENT` | 116 | 1 (picture) |
| `RUBY_PARENT` | 116 | 1 (ruby) |
| `SELECT_PARENT` | 116 | 1 (select) |
| `SVG_PARENT` | 116 | 1 (svg) |
| `TABLE_PARENT` | 116 | 1 (table) |
| `TEMPLATE_PARENT` | 116 | 1 (template) |
| `TABLE_ROW_PARENT` | 116 | 1 (tr) |

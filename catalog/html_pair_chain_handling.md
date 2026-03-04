# HTML Pair Chain Handling — Compressed Rules

The complete Chrome HTML parser pair-handling logic, compressed from 13,456
individual parent×child pairs into **62 rules** organized by insertion mode.

Derived from `chrome_pair_handling.md` and `pair_behavior_analysis.md`.

---

## How To Use This Document

To determine what Chrome does for any parent→child pair:

1. Look up the **parent's mode** in Section 1
2. Go to that mode's rules in Section 3
3. Find the **first rule whose child set matches** the child tag
4. The action tells you what Chrome does
5. If the action says REPROCESS, follow the chain to the target mode

---

## Section 1: Parent → Mode Mapping

Every HTML element maps to exactly one insertion mode when it is the current
open element on the stack.

### Mode: InBody (78+ elements)

All elements not assigned to another mode below. Includes flow containers,
phrasing elements, and obsolete elements that Chrome processes in InBody:

```
a          abbr       address    applet     article    aside
audio      b          bdi        bdo        big        blockquote
body       button     canvas     center     cite       code
data       datalist   dd         del        details    dfn
dialog     dir        div        dl         dt         em
fieldset   figcaption figure     footer     form       h1
h2         h3         h4         h5         h6         header
hgroup     i          ins        kbd        label      legend
li         listing    main       map        mark       marquee
menu       meter      nav        nobr       noembed    noframes
noscript   object     ol         output     p          picture
pre        progress   q          rb         rp         rt
rtc        ruby       s          samp       search     section
slot       small      span       strike     strong     sub
summary    sup        time       tt         u          ul
var        video
```

### Mode: Void — no children possible (15 elements)

```
area  base  br   col    embed  hr   img   input
link  meta  param portal source track wbr
```

### Mode: RawText — all content is unparsed text (5 elements)

```
script  style  title  textarea  iframe
```

### Mode: InTable (1 element)

```
table
```

### Mode: InTableBody (3 elements)

```
thead  tbody  tfoot
```

### Mode: InRow (1 element)

```
tr
```

### Mode: InCell (2 elements)

```
td  th
```

### Mode: InCaption (1 element)

```
caption
```

### Mode: InColumnGroup (1 element)

```
colgroup
```

### Mode: InSelect (3 elements)

```
select  optgroup  option
```

### Mode: InForeignContent (2 elements)

```
svg  math
```

### Mode: InTemplate (1 element)

```
template
```

### Mode: InHead (1 element)

```
head
```

### Mode: Initial (1 element)

```
html
```

**Total: 14 modes covering all 116 elements.**

---

## Section 2: Named Element Sets

These named sets are referenced by the rules in Section 3. Every child tag
belongs to one or more of these sets.

### SET_P_CLOSING — Block elements that close `<p>` (27 tags)

```
address  article  aside  blockquote  center  details  dialog  dir
div      dl       fieldset  figcaption  figure  footer  header  hgroup
listing  main     menu   nav         ol      p       pre     search
section  summary  ul
```

### SET_HEADINGS — Heading elements (6 tags)

```
h1  h2  h3  h4  h5  h6
```

### SET_AFE — Active Formatting Elements (12 tags)

```
b  big  code  em  font  i  s  small  strike  strong  tt  u
```

### SET_VOID_AFE — Void elements with AFE reconstruction (7 tags)

```
area  br  embed  img  input  keygen  wbr
```

### SET_VOID_PLAIN — Void elements without AFE reconstruction (3 tags)

```
param  source  track
```

### SET_HEAD_DELEGATED — Processed via InHead handler (10 tags)

```
base  basefont  bgsound  link  meta  noframes  script  style  template  title
```

### SET_TABLE_INTERNAL — Dropped in InBody mode (10 tags)

```
caption  col  colgroup  frame  tbody  td  tfoot  th  thead  tr
```

### SET_STRUCTURAL — Top-level structural elements (3 tags)

```
html  head  body
```

### SET_AFE_MARKER — Elements that push a formatting marker (3 tags)

```
applet  marquee  object
```

### SET_TABLE_DIRECT — Directly accepted in InTable (5 tags)

```
caption  colgroup  tbody  tfoot  thead
```

### SET_TABLE_WRAP — Auto-wrapped in InTable (4 tags)

```
col  tr  td  th
```

### SET_TABLE_ESCAPE — Close InCell/InCaption (9 tags)

```
caption  col  colgroup  tbody  td  tfoot  th  thead  tr
```

### SET_EXIT_FOREIGN — Break out of SVG/MathML (46 tags)

```
b    big   blockquote  body   br     center  code  dd    div   dl
dt   em    embed       font*  h1     h2      h3    h4    h5    h6
head hr    i           img    li     listing menu  meta  nobr  ol
p    pre   ruby        s      small  span    strong strike sub  sup
table tt   u           ul     var
```

*`font` only exits foreign content when it has a `color`, `face`, or `size` attribute.

### SET_INSELECT_ACCEPTED — Valid children in InSelect (5 tags)

```
option  optgroup  hr  script  template
```

### SET_GENERIC_PHRASING — Catch-all "any other start tag" (~27 tags)

```
abbr   audio    bdi    bdo     canvas  cite   data     datalist
del    dfn      ins    kbd     label   legend map      mark
meter  output   picture  progress  q    ruby   samp    slot
span   sub      sup    time    var     video
```

These are the InBody tags not covered by any named set above. They all follow
the same code path: reconstruct AFE → insert.

---

## Section 3: Rules By Mode

Rules are evaluated **in order** — the first matching rule wins. Each rule
specifies the child set, the action, and where reprocessing continues (if
applicable).

### Actions Legend

| Action | Meaning |
|--------|---------|
| INSERT | Create element, append to current node |
| INSERT_VOID | Create void element (self-closing), append to current node |
| INSERT_FOREIGN | Create element in SVG/MathML namespace, append to current node |
| DROP | Ignore the start tag entirely (parse error) |
| CLOSE_P | If `<p>` is in button scope, generate implied `</p>` first |
| CLOSE_HEADING | If current node is h1-h6, pop it off the stack |
| CLOSE_SAME | Close same-type element if in scope, then insert new one |
| CLOSE_PARENT | Pop current element off stack, reprocess child in new mode |
| RECONSTRUCT_AFE | Reconstruct active formatting elements before insertion |
| PUSH_AFE | Push element onto active formatting element list |
| PUSH_MARKER | Push a marker onto active formatting element list |
| ADOPTION_AGENCY | Run the adoption agency algorithm for this element |
| FOSTER | Process with InBody rules, but insert before the table (not inside it) |
| STACK_WALK | Walk the stack of open elements for a matching ancestor to close |
| SWITCH(mode) | Change insertion mode after insertion |
| REPROCESS_IN(mode) | Do not insert; instead reprocess this token in the specified mode |
| WRAP(tag) | Auto-generate the specified wrapper element, then reprocess |
| PROCESS_IN_HEAD | Delegate to InHead mode's handler |
| RAW_TEXT | No child tags parsed; all content is raw text until closing tag |

---

### Mode: InBody (78+ parents, 22 rules)

Applies when current node is any InBody-mode element.

```
RULE IB-01  child ∈ SET_P_CLOSING
            → CLOSE_P + INSERT

RULE IB-02  child ∈ SET_HEADINGS
            → CLOSE_P + CLOSE_HEADING + INSERT

RULE IB-03  child ∈ SET_AFE
            → RECONSTRUCT_AFE + INSERT + PUSH_AFE

RULE IB-04  child ∈ SET_VOID_AFE
            → RECONSTRUCT_AFE + INSERT_VOID

RULE IB-05  child ∈ SET_VOID_PLAIN
            → INSERT_VOID

RULE IB-06  child = hr
            → CLOSE_P + INSERT_VOID

RULE IB-07  child ∈ SET_HEAD_DELEGATED
            → PROCESS_IN_HEAD

RULE IB-08  child ∈ SET_TABLE_INTERNAL
            → DROP

RULE IB-09  child ∈ SET_STRUCTURAL
            → DROP  (html/body may merge attributes)

RULE IB-10  child = li
            → STACK_WALK(li) + CLOSE_P + INSERT

RULE IB-11  child ∈ {dd, dt}
            → STACK_WALK(dd, dt) + CLOSE_P + INSERT

RULE IB-12  child ∈ {option, optgroup}
            → if current node is <option>: pop it
            → RECONSTRUCT_AFE + INSERT

RULE IB-13  child = a
            → if <a> in formatting list: ADOPTION_AGENCY
            → RECONSTRUCT_AFE + INSERT + PUSH_AFE

RULE IB-14  child = nobr
            → if <nobr> in scope: ADOPTION_AGENCY
            → RECONSTRUCT_AFE + INSERT + PUSH_AFE

RULE IB-15  child = button
            → if <button> in scope: generate implied end tags, pop to <button>
            → RECONSTRUCT_AFE + INSERT

RULE IB-16  child = form
            → CLOSE_P
            → if form pointer already set (no <template> on stack): DROP
            → else: INSERT + set form pointer

RULE IB-17  child = table
            → CLOSE_P (skipped in quirks mode)
            → INSERT + SWITCH(InTable)

RULE IB-18  child = select
            → RECONSTRUCT_AFE + INSERT
            → SWITCH(InSelect) — or SWITCH(InSelectInTable) if table ancestor

RULE IB-19  child ∈ {textarea, iframe, xmp, noembed, noscript}
            → textarea/xmp: CLOSE_P (xmp only) + INSERT + SWITCH(Text)
              textarea uses RCDATA tokenizer, xmp uses RawText tokenizer
            → iframe/noembed/noscript: INSERT + SWITCH(Text/RawText)
            → first newline stripped for textarea

RULE IB-20  child ∈ {rb, rtc, rp, rt}
            → if <ruby> in scope: generate implied end tags
              (rp/rt: exclude <rtc> from implied end tags)
            → INSERT

RULE IB-21  child ∈ SET_AFE_MARKER
            → RECONSTRUCT_AFE + INSERT + PUSH_MARKER

RULE IB-22  child ∈ {math, svg}
            → RECONSTRUCT_AFE
            → INSERT_FOREIGN (math→MathML namespace, svg→SVG namespace)
            → SWITCH(InForeignContent)

DEFAULT     child ∈ SET_GENERIC_PHRASING (or any unmatched tag)
            → RECONSTRUCT_AFE + INSERT
```

**Note on context-sensitivity:** Rules IB-01, IB-02, IB-06, IB-10, IB-11,
IB-16, IB-17 all include CLOSE_P which checks "is `<p>` in button scope?"
This is a stack check, not a parent check. When `<p>` IS the current node,
it is always in button scope, so CLOSE_P always fires. When the current node
is an inline element like `<span>` (with no `<p>` ancestor in button scope),
CLOSE_P is a no-op.

---

### Mode: InTable (1 parent: `table`, 7 rules)

```
RULE IT-01  child ∈ SET_TABLE_DIRECT {caption, colgroup, tbody, tfoot, thead}
            → clear stack back to table context
            → INSERT + SWITCH:
              caption  → InCaption  (also push AFE marker)
              colgroup → InColumnGroup
              tbody/tfoot/thead → InTableBody

RULE IT-02  child ∈ SET_TABLE_WRAP {col, tr, td, th}
            → clear stack back to table context
            → col: WRAP(colgroup) → REPROCESS_IN(InColumnGroup)
            → tr:  WRAP(tbody)    → REPROCESS_IN(InTableBody)
            → td/th: WRAP(tbody)  → REPROCESS_IN(InTableBody)
              ↳ chains to: InTableBody wraps tr → InRow accepts td/th

RULE IT-03  child = table
            → CLOSE_PARENT (pop elements until <table> popped)
            → reset insertion mode
            → REPROCESS in new mode (typically starts sibling table)

RULE IT-04  child ∈ {style, script, template}
            → PROCESS_IN_HEAD

RULE IT-05  child = input  (only if type="hidden")
            → INSERT_VOID (directly in table)
            → if NOT type="hidden": falls through to DEFAULT (foster)

RULE IT-06  child ∈ {form, html}
            → form: DROP (if form pointer set or <template> on stack)
              otherwise: insert + immediately pop (not added to stack)
            → html: DROP (attributes merge)

DEFAULT     everything else (~100 tags)
            → FOSTER: set foster-parent flag → process with InBody rules
              → element is inserted BEFORE the <table> in the DOM, not inside it
            → clear foster-parent flag
```

**Foster parenting detail:** The foster parent location is:
1. If the last `<table>` has a parent element → insert before `<table>` in that parent
2. Otherwise → append to the element one above `<table>` in the stack

---

### Mode: InTableBody (3 parents: `thead`, `tbody`, `tfoot`, 4 rules)

```
RULE TB-01  child = tr
            → clear stack back to table body context
            → INSERT + SWITCH(InRow)

RULE TB-02  child ∈ {td, th}
            → clear stack back to table body context
            → WRAP(tr) → REPROCESS_IN(InRow)
            ↳ chains to: Rule IR-01 (InRow accepts td/th)

RULE TB-03  child ∈ {caption, col, colgroup, tbody, tfoot, thead}
            → if none of {tbody, thead, tfoot} in table scope: DROP
            → else: clear stack to table body context
            → pop current element (the tbody/thead/tfoot)
            → SWITCH(InTable) → REPROCESS_IN(InTable)
            ↳ chains to: Rule IT-01 or IT-02 depending on child

DEFAULT     everything else (~106 tags)
            → process using InTable rules (Rule IT-04/IT-05/IT-06 or FOSTER)
```

---

### Mode: InRow (1 parent: `tr`, 3 rules)

```
RULE IR-01  child ∈ {td, th}
            → clear stack back to table row context
            → INSERT + PUSH_MARKER + SWITCH(InCell)

RULE IR-02  child ∈ {caption, col, colgroup, tbody, tfoot, thead, tr}
            → if <tr> not in table scope: DROP
            → else: clear stack to row context → pop <tr>
            → SWITCH(InTableBody) → REPROCESS_IN(InTableBody)
            ↳ chains to:
              tr → TB-01 (opens new row)
              tbody/tfoot/thead → TB-03 (closes current tbody, reprocess in InTable)
              caption/col/colgroup → TB-03 → IT-01/IT-02

DEFAULT     everything else (~105 tags)
            → process using InTable rules (IT-04/IT-05/IT-06 or FOSTER)
```

---

### Mode: InCell (2 parents: `td`, `th`, 2 rules)

```
RULE IC-01  child ∈ SET_TABLE_ESCAPE {caption, col, colgroup, tbody, td,
                                       tfoot, th, thead, tr}
            → if neither <td> nor <th> in table scope: DROP
            → else: close the cell:
              generate implied end tags → pop to <td>/<th> → pop it
              → clear AFE to last marker
            → SWITCH(InRow) → REPROCESS_IN(InRow)
            ↳ chains to:
              td/th → IR-01 (opens new cell in same row)
              tr    → IR-02 (closes row, opens new row)
              other → IR-02 → TB-03 → IT-01/IT-02

DEFAULT     everything else (107 tags)
            → process using InBody rules (Rules IB-01 through IB-22 + default)
            → cell is a full flow container — all InBody content works
```

---

### Mode: InCaption (1 parent: `caption`, 2 rules)

```
RULE CP-01  child ∈ SET_TABLE_ESCAPE {caption, col, colgroup, tbody, td,
                                       tfoot, th, thead, tr}
            → if <caption> not in table scope: DROP
            → else: generate implied end tags → pop to <caption> → pop it
              → clear AFE to last marker
            → SWITCH(InTable) → REPROCESS_IN(InTable)
            ↳ chains to: Rule IT-01 or IT-02

DEFAULT     everything else (107 tags)
            → process using InBody rules (Rules IB-01 through IB-22 + default)
            → caption is a full flow container
```

---

### Mode: InColumnGroup (1 parent: `colgroup`, 4 rules)

```
RULE CG-01  child = html
            → DROP (attributes merge via InBody handler)

RULE CG-02  child = col
            → INSERT_VOID

RULE CG-03  child = template
            → PROCESS_IN_HEAD

DEFAULT     everything else (113 tags)
            → CLOSE_PARENT: pop <colgroup> off stack
            → SWITCH(InTable) → REPROCESS_IN(InTable)
            ↳ chains to: InTable rules (IT-01 through IT-06 or foster)
            → if colgroup can't be popped (fragment case): DROP
```

---

### Mode: InSelect (3 parents: `select`, `optgroup`, `option`, 6 rules)

```
RULE IS-01  child = html
            → DROP (attributes merge)

RULE IS-02  child = option
            → if current node is <option>: pop it (auto-close previous option)
            → INSERT

RULE IS-03  child = optgroup
            → if current node is <option>: pop it
            → if current node is <optgroup>: pop it (auto-close previous group)
            → INSERT

RULE IS-04  child = hr
            → if current node is <option>: pop it
            → if current node is <optgroup>: pop it
            → INSERT_VOID

RULE IS-05  child = select
            → CLOSE_PARENT: pop until <select> is popped
            → reset insertion mode appropriately
            ↳ chains to: whatever mode the document is now in

RULE IS-06  child ∈ {input, keygen, textarea}
            → if <select> not in select scope: DROP
            → else: CLOSE_PARENT (pop until <select> popped)
            → reset insertion mode → REPROCESS in new mode
            ↳ chains to: InBody rules for input/textarea, etc.

RULE IS-07  child ∈ {script, template}
            → PROCESS_IN_HEAD

DEFAULT     everything else (~106 tags)
            → DROP (parse error, token ignored)
```

**InSelectInTable variant:** When `<select>` is inside a `<table>`, Chrome
uses InSelectInTable mode which adds one rule before the default:

```
RULE IST-01 child ∈ {caption, table, tbody, tfoot, thead, tr, td, th}
            → CLOSE_PARENT (pop until <select> popped)
            → reset insertion mode → REPROCESS in new mode
            ↳ chains to: InTable/InTableBody/InRow rules
```

---

### Mode: InForeignContent (2 parents: `svg`, `math`, 2 rules)

```
RULE FC-01  child ∈ SET_EXIT_FOREIGN (46 tags)
            → parse error
            → pop elements from stack until reaching:
              a MathML text integration point, OR
              an HTML integration point, OR
              an HTML namespace element
            → REPROCESS in the new (HTML) insertion mode
            ↳ chains to: whatever HTML mode is now active (usually InBody)

RULE FC-02  child = font (with color, face, or size attribute)
            → same as FC-01: EXIT_FOREIGN + REPROCESS

DEFAULT     everything else (~70 tags, including SVG/MathML-specific elements)
            → INSERT_FOREIGN in parent's namespace
            → SVG: adjust attribute names (viewBox, preserveAspectRatio, etc.)
            → MathML: adjust MathML attributes
```

---

### Mode: RawText (5 parents: `script`, `style`, `title`, `textarea`, `iframe`, 1 rule)

```
RULE RT-01  ALL children (116 tags)
            → RAW_TEXT — no tags are parsed
            → content is treated as text until the matching closing tag
            → title/textarea: RCDATA mode (HTML entities ARE decoded)
            → script/style/iframe: true raw text (entities NOT decoded)
```

---

### Mode: Void (15 parents, 1 rule)

```
RULE VD-01  ALL children (116 tags)
            → N/A — void elements cannot have children
            → any content after a void element becomes a sibling
            → Chrome never enters a state where a void element is
              "the current node" expecting children
```

---

### Mode: InTemplate (1 parent: `template`, 5 rules)

Template accepts everything. The internal mode adapts based on the child:

```
RULE TM-01  child ∈ SET_HEAD_DELEGATED
            → PROCESS_IN_HEAD

RULE TM-02  child ∈ {caption, colgroup, col, tbody, tfoot, thead}
            → pop template insertion mode
            → push InTable onto template mode stack
            → SWITCH(InTable) → REPROCESS_IN(InTable)

RULE TM-03  child = tr
            → pop template insertion mode
            → push InTableBody onto template mode stack
            → SWITCH(InTableBody) → REPROCESS_IN(InTableBody)

RULE TM-04  child ∈ {td, th}
            → pop template insertion mode
            → push InRow onto template mode stack
            → SWITCH(InRow) → REPROCESS_IN(InRow)

DEFAULT     everything else
            → pop template insertion mode
            → push InBody onto template mode stack
            → SWITCH(InBody) → REPROCESS_IN(InBody)
```

---

### Mode: InHead (1 parent: `head`, 5 rules)

```
RULE HD-01  child = html
            → DROP (attributes merge via InBody handler)

RULE HD-02  child ∈ {base, basefont, bgsound, link}
            → INSERT_VOID (metadata void elements)

RULE HD-03  child = meta
            → INSERT_VOID (+ charset detection may run)

RULE HD-04  child ∈ {title, noscript, noframes, style, script, template}
            → INSERT + switch tokenizer:
              title → RCDATA
              noscript (scripting on) → RawText
              noframes/style → RawText
              script → ScriptData
              template → push to stack, push InTemplate mode

RULE HD-05  child = head
            → DROP (duplicate <head> ignored)

DEFAULT     everything else
            → CLOSE_PARENT: pop <head> off stack
            → SWITCH(AfterHead) → REPROCESS
            ↳ chains to: AfterHead auto-generates <body> → REPROCESS_IN(InBody)
```

---

### Mode: Initial (1 parent: `html`, 4 rules)

When `<html>` is the current node, parser is in BeforeHead or AfterHead mode.

```
RULE HT-01  child = head
            → INSERT + SWITCH(InHead)

RULE HT-02  child = body
            → INSERT + SWITCH(InBody)

RULE HT-03  child = html
            → DROP (duplicate, attributes may merge)

DEFAULT     everything else
            → if before <head>: auto-generate <head>, close it, auto-generate <body>
            → if after <head>: auto-generate <body>
            → REPROCESS_IN(InBody)
```

---

## Section 4: Rule Chain Resolution

Many rules end with REPROCESS, meaning the child token is re-evaluated in a
different mode. This creates chains of rule firings. Here are the common chains.

### Table Structure Chains

**`<table>` → `<td>` (3-rule chain):**
```
1. InTable:    td matches IT-02 → WRAP(tbody) → REPROCESS_IN(InTableBody)
2. InTableBody: td matches TB-02 → WRAP(tr) → REPROCESS_IN(InRow)
3. InRow:      td matches IR-01 → INSERT + SWITCH(InCell)
Result: <table><tbody><tr><td>  (tbody and tr auto-generated)
```

**`<table>` → `<th>` (3-rule chain):**
```
Same as above, th follows identical path.
Result: <table><tbody><tr><th>
```

**`<table>` → `<tr>` (2-rule chain):**
```
1. InTable:     tr matches IT-02 → WRAP(tbody) → REPROCESS_IN(InTableBody)
2. InTableBody: tr matches TB-01 → INSERT + SWITCH(InRow)
Result: <table><tbody><tr>  (tbody auto-generated)
```

**`<table>` → `<col>` (2-rule chain):**
```
1. InTable:       col matches IT-02 → WRAP(colgroup) → REPROCESS_IN(InColumnGroup)
2. InColumnGroup: col matches CG-02 → INSERT_VOID
Result: <table><colgroup><col>  (colgroup auto-generated)
```

### Cell Escape Chains

**`<td>` → `<td>` (2-rule chain, same-row new cell):**
```
1. InCell: td matches IC-01 → close current cell → SWITCH(InRow) → REPROCESS_IN(InRow)
2. InRow:  td matches IR-01 → INSERT + SWITCH(InCell)
Result: </td><td>  (previous cell closed, new cell in same row)
```

**`<td>` → `<tr>` (3-rule chain, close cell + close row + new row):**
```
1. InCell:      tr matches IC-01 → close cell → REPROCESS_IN(InRow)
2. InRow:       tr matches IR-02 → close <tr> → REPROCESS_IN(InTableBody)
3. InTableBody: tr matches TB-01 → INSERT + SWITCH(InRow)
Result: </td></tr><tr>
```

**`<td>` → `<caption>` (4-rule chain):**
```
1. InCell:      caption matches IC-01 → close cell → REPROCESS_IN(InRow)
2. InRow:       caption matches IR-02 → close <tr> → REPROCESS_IN(InTableBody)
3. InTableBody: caption matches TB-03 → close tbody → REPROCESS_IN(InTable)
4. InTable:     caption matches IT-01 → INSERT + SWITCH(InCaption)
Result: </td></tr></tbody><caption>
```

### Select Escape Chains

**`<select>` → `<input>` (2-rule chain):**
```
1. InSelect: input matches IS-06 → close <select> → reset mode → REPROCESS
2. InBody:   input matches IB-04 → RECONSTRUCT_AFE + INSERT_VOID
Result: </select><input>
```

**`<option>` → `<div>` (1 rule, DROP):**
```
1. InSelect: div matches DEFAULT → DROP
Result: <div> is silently ignored
```

### Foreign Content Escape Chains

**`<svg>` → `<div>` (2-rule chain):**
```
1. InForeignContent: div matches FC-01 → pop to HTML namespace → REPROCESS
2. InBody:           div matches IB-01 → CLOSE_P + INSERT
Result: </svg><div>  (exits SVG, inserts div in HTML)
```

**`<svg>` → `<rect>` (1 rule, no chain):**
```
1. InForeignContent: rect matches DEFAULT → INSERT_FOREIGN
Result: <rect> inserted in SVG namespace
```

### Head Close Chains

**`<head>` → `<div>` (2-rule chain):**
```
1. InHead: div matches DEFAULT → CLOSE_PARENT (pop head) → REPROCESS
2. AfterHead auto-generates <body> → REPROCESS_IN(InBody)
3. InBody: div matches IB-01 → CLOSE_P + INSERT
Result: </head><body><div>
```

### Template Chains

**`<template>` → `<td>` (2-rule chain):**
```
1. InTemplate: td matches TM-04 → push InRow → REPROCESS_IN(InRow)
2. InRow:      td matches IR-01 → INSERT + SWITCH(InCell)
Result: template internal mode becomes InRow, <td> inserted
```

### Foster Parenting (not a chain, but a redirect)

**`<table>` → `<div>` (1 rule, foster):**
```
1. InTable: div matches DEFAULT → FOSTER
   → set foster flag → process with InBody rules (IB-01: CLOSE_P + INSERT)
   → element is inserted BEFORE <table> in DOM, not inside it
   → clear foster flag
Result: <div> appears as a sibling before <table> in the DOM tree
```

**`<tr>` → `<span>` (1 rule via delegation, foster):**
```
1. InRow: span matches DEFAULT → process using InTable rules
   → InTable DEFAULT → FOSTER → process with InBody rules
   → span inserted before table
Result: <span> foster-parented before <table>
```

---

## Section 5: Complete Rule Index

All rules in one flat list for reference.

| ID | Mode | Child Set | Action |
|----|------|-----------|--------|
| IB-01 | InBody | SET_P_CLOSING (27) | CLOSE_P + INSERT |
| IB-02 | InBody | SET_HEADINGS (6) | CLOSE_P + CLOSE_HEADING + INSERT |
| IB-03 | InBody | SET_AFE (12) | RECONSTRUCT_AFE + INSERT + PUSH_AFE |
| IB-04 | InBody | SET_VOID_AFE (7) | RECONSTRUCT_AFE + INSERT_VOID |
| IB-05 | InBody | SET_VOID_PLAIN (3) | INSERT_VOID |
| IB-06 | InBody | hr | CLOSE_P + INSERT_VOID |
| IB-07 | InBody | SET_HEAD_DELEGATED (10) | PROCESS_IN_HEAD |
| IB-08 | InBody | SET_TABLE_INTERNAL (10) | DROP |
| IB-09 | InBody | SET_STRUCTURAL (3) | DROP |
| IB-10 | InBody | li | STACK_WALK(li) + CLOSE_P + INSERT |
| IB-11 | InBody | dd, dt | STACK_WALK(dd,dt) + CLOSE_P + INSERT |
| IB-12 | InBody | option, optgroup | auto-close option + RECONSTRUCT_AFE + INSERT |
| IB-13 | InBody | a | ADOPTION_AGENCY(a) + RECONSTRUCT_AFE + INSERT + PUSH_AFE |
| IB-14 | InBody | nobr | ADOPTION_AGENCY(nobr) + RECONSTRUCT_AFE + INSERT + PUSH_AFE |
| IB-15 | InBody | button | CLOSE_SAME(button) + RECONSTRUCT_AFE + INSERT |
| IB-16 | InBody | form | CLOSE_P + conditional DROP or INSERT |
| IB-17 | InBody | table | CLOSE_P (not quirks) + INSERT + SWITCH(InTable) |
| IB-18 | InBody | select | RECONSTRUCT_AFE + INSERT + SWITCH(InSelect) |
| IB-19 | InBody | textarea, iframe, xmp, noembed, noscript | INSERT + SWITCH(Text) |
| IB-20 | InBody | rb, rtc, rp, rt | implied end tags (ruby) + INSERT |
| IB-21 | InBody | SET_AFE_MARKER (3) | RECONSTRUCT_AFE + INSERT + PUSH_MARKER |
| IB-22 | InBody | math, svg | RECONSTRUCT_AFE + INSERT_FOREIGN + SWITCH(InForeignContent) |
| IB-DF | InBody | *(default)* | RECONSTRUCT_AFE + INSERT |
| IT-01 | InTable | SET_TABLE_DIRECT (5) | clear stack + INSERT + SWITCH(mode) |
| IT-02 | InTable | SET_TABLE_WRAP (4) | clear stack + WRAP → REPROCESS |
| IT-03 | InTable | table | CLOSE_PARENT → REPROCESS |
| IT-04 | InTable | style, script, template | PROCESS_IN_HEAD |
| IT-05 | InTable | input[type=hidden] | INSERT_VOID |
| IT-06 | InTable | form, html | DROP |
| IT-DF | InTable | *(default)* | FOSTER (InBody rules at foster location) |
| TB-01 | InTableBody | tr | clear stack + INSERT + SWITCH(InRow) |
| TB-02 | InTableBody | td, th | WRAP(tr) → REPROCESS_IN(InRow) |
| TB-03 | InTableBody | caption, col, colgroup, tbody, tfoot, thead | CLOSE_PARENT → REPROCESS_IN(InTable) |
| TB-DF | InTableBody | *(default)* | process as InTable |
| IR-01 | InRow | td, th | clear stack + INSERT + PUSH_MARKER + SWITCH(InCell) |
| IR-02 | InRow | caption, col, colgroup, tbody, tfoot, thead, tr | CLOSE_PARENT → REPROCESS_IN(InTableBody) |
| IR-DF | InRow | *(default)* | process as InTable |
| IC-01 | InCell | SET_TABLE_ESCAPE (9) | close cell → REPROCESS_IN(InRow) |
| IC-DF | InCell | *(default)* | process as InBody |
| CP-01 | InCaption | SET_TABLE_ESCAPE (9) | close caption → REPROCESS_IN(InTable) |
| CP-DF | InCaption | *(default)* | process as InBody |
| CG-01 | InColumnGroup | html | DROP |
| CG-02 | InColumnGroup | col | INSERT_VOID |
| CG-03 | InColumnGroup | template | PROCESS_IN_HEAD |
| CG-DF | InColumnGroup | *(default)* | CLOSE_PARENT → REPROCESS_IN(InTable) |
| IS-01 | InSelect | html | DROP |
| IS-02 | InSelect | option | auto-close option + INSERT |
| IS-03 | InSelect | optgroup | auto-close option/optgroup + INSERT |
| IS-04 | InSelect | hr | auto-close option/optgroup + INSERT_VOID |
| IS-05 | InSelect | select | CLOSE_PARENT → reset mode |
| IS-06 | InSelect | input, keygen, textarea | CLOSE_PARENT → REPROCESS |
| IS-07 | InSelect | script, template | PROCESS_IN_HEAD |
| IS-DF | InSelect | *(default)* | DROP |
| IST-01 | InSelectInTable | caption, table, tbody, tfoot, thead, tr, td, th | CLOSE_PARENT → REPROCESS |
| FC-01 | InForeignContent | SET_EXIT_FOREIGN (46) | pop to HTML → REPROCESS |
| FC-02 | InForeignContent | font (with color/face/size) | pop to HTML → REPROCESS |
| FC-DF | InForeignContent | *(default)* | INSERT_FOREIGN |
| RT-01 | RawText | *(all)* | RAW_TEXT |
| VD-01 | Void | *(all)* | N/A (no children) |
| TM-01 | InTemplate | SET_HEAD_DELEGATED (10) | PROCESS_IN_HEAD |
| TM-02 | InTemplate | caption, colgroup, col, tbody, tfoot, thead | push InTable → REPROCESS |
| TM-03 | InTemplate | tr | push InTableBody → REPROCESS |
| TM-04 | InTemplate | td, th | push InRow → REPROCESS |
| TM-DF | InTemplate | *(default)* | push InBody → REPROCESS |
| HD-01 | InHead | html | DROP |
| HD-02 | InHead | base, basefont, bgsound, link | INSERT_VOID |
| HD-03 | InHead | meta | INSERT_VOID (+ charset) |
| HD-04 | InHead | title, noscript, noframes, style, script, template | INSERT + mode switch |
| HD-05 | InHead | head | DROP |
| HD-DF | InHead | *(default)* | CLOSE_PARENT → REPROCESS |
| HT-01 | Initial | head | INSERT + SWITCH(InHead) |
| HT-02 | Initial | body | INSERT + SWITCH(InBody) |
| HT-03 | Initial | html | DROP |
| HT-DF | Initial | *(default)* | auto-generate head+body → REPROCESS_IN(InBody) |

**Total: 62 rules** (23 InBody + 7 InTable + 4 InTableBody + 3 InRow +
2 InCell + 2 InCaption + 4 InColumnGroup + 8 InSelect + 1 InSelectInTable +
3 InForeignContent + 1 RawText + 1 Void + 5 InTemplate + 6 InHead +
4 Initial — including defaults)

These 62 rules, combined with the 14-mode parent mapping, fully determine
the behavior of all 13,456 parent×child pairs.

---

## Section 6: Pair Coverage Verification

The `html_elements.json` catalog contains 147 elements. The original
`chrome_pair_handling.md` analysis used a curated set of 116 elements. These
rules cover **all** elements through default/catch-all rules — any element
not explicitly listed in a named set falls into the DEFAULT rule for whatever
mode its parent is in.

**Coverage by mode (using 116-element curated set):**

| Mode | Parents | × | Children | = Pairs |
|------|---------|---|----------|---------|
| InBody | 78 | × | 116 | 9,048 |
| Void | 15 | × | 116 | 1,740 |
| RawText | 5 | × | 116 | 580 |
| InSelect | 3 | × | 116 | 348 |
| InTableBody | 3 | × | 116 | 348 |
| InCell | 2 | × | 116 | 232 |
| InForeignContent | 2 | × | 116 | 232 |
| InTable | 1 | × | 116 | 116 |
| InRow | 1 | × | 116 | 116 |
| InColumnGroup | 1 | × | 116 | 116 |
| InCaption | 1 | × | 116 | 116 |
| InTemplate | 1 | × | 116 | 116 |
| InHead | 1 | × | 116 | 116 |
| Initial | 1 | × | 116 | 116 |
| **Total** | **116** | | | **13,456** |

Every mode has a DEFAULT rule, so even elements not explicitly named in any
child set (e.g., obscure obsolete tags like `blink`, `spacer`, `multicol`)
are handled correctly — they hit the catch-all path for their parent's mode.

# Pair Behavior Analysis — Rules and Groupings

Analysis of `chrome_pair_handling.md` to extract compressible patterns,
behavioral equivalence classes, and the minimal rule set that covers all
13,456 parent→child pairs.

---

## Finding 1: The Mode Dominance Rule

**The parent element's identity almost never matters. Only its insertion mode does.**

116 parent elements map to just 14 insertion modes. 73 of the 116 (63%) share
a single mode (InBody), meaning they handle every child tag identically:

| Insertion Mode | # Parents | % of 116 | Child dispatch |
|----------------|-----------|----------|----------------|
| **InBody** | **73** | **62.9%** | Same 116-child action map for all 73 |
| Void (no children) | 15 | 12.9% | N/A — no children possible |
| Raw Text | 5 | 4.3% | Same: all 116 children → RAW_TEXT |
| InSelect | 3 | 2.6% | Nearly identical across select/optgroup/option |
| InTableBody | 3 | 2.6% | Identical across thead/tbody/tfoot |
| InCell | 2 | 1.7% | Identical across td/th |
| InForeignContent | 2 | 1.7% | Identical across svg/math |
| InTable | 1 | 0.9% | table only |
| InRow | 1 | 0.9% | tr only |
| InColumnGroup | 1 | 0.9% | colgroup only |
| InCaption | 1 | 0.9% | caption only |
| InTemplate | 1 | 0.9% | template only |
| InHead | 1 | 0.9% | head only |
| BeforeHead/AfterHead | 1 | 0.9% | html only |

**Consequence:** Instead of 116 parent rules, we need only 14 mode rules.
The parent→mode mapping is a simple lookup table.

### The 73 InBody Parents

These elements all produce **byte-for-byte identical** child handling:

```
a, abbr, address, article, aside, audio, b, bdi, bdo, blockquote, body,
button, canvas, cite, code, data, datalist, dd, del, details, dfn, dialog,
div, dl, dt, em, fieldset, figcaption, figure, footer, form, h1, h2, h3,
h4, h5, h6, header, hgroup, i, ins, kbd, label, legend, li, main, map,
mark, meter, nav, noscript, object, ol, output, p, picture, pre, progress,
q, rp, rt, ruby, s, samp, search, section, slot, small, span, strong, sub,
summary, sup, time, u, ul, var, video
```

The parser doesn't care if the current node is `<div>`, `<span>`, `<p>`, or
`<body>` — the InBody dispatch table is the same. The *stack-scope checks*
(like "is `<p>` in button scope?") create context-sensitivity, but the
dispatching logic itself is identical.

---

## Finding 2: Children Fall Into 18 Behavioral Classes

Every child tag follows one of 18 behavior patterns in InBody mode. Since
InBody covers 73 parents, this classification alone covers 73 × 116 = 8,468
pairs (63% of all pairs).

### Class 1: P-Closing Block Elements (27 tags)

```
address, article, aside, blockquote, center, details, dialog, dir, div, dl,
fieldset, figcaption, figure, footer, header, hgroup, listing, main, menu,
nav, ol, p, pre, search, section, summary, ul
```

**Action:** Close `<p>` if in button scope → insert element.

All 27 behave identically. A `<div>` arriving in InBody triggers the exact
same code path as `<blockquote>` or `<section>`.

### Class 2: Headings (6 tags)

```
h1, h2, h3, h4, h5, h6
```

**Action:** Close `<p>` if in button scope → if current node is a heading, pop it → insert.

Superset of Class 1's behavior. All six headings are interchangeable in the
parser — `h1` through `h6` trigger identical logic (Chrome doesn't distinguish
heading levels).

### Class 3: Active Formatting Elements (12 tags)

```
b, big, code, em, font, i, s, small, strike, strong, tt, u
```

**Action:** Reconstruct AFE → insert → push to active formatting list.

These are the elements that participate in the adoption agency algorithm. They
persist across tag boundaries via the formatting list.

### Class 4: Void + AFE Reconstruction (6 tags)

```
area, br, embed, img, keygen, wbr
```

**Action:** Reconstruct AFE → insert void element.

Plus `input` follows the same pattern (Class 4b: 1 tag).

### Class 5: Void, No AFE (3 tags)

```
param, source, track
```

**Action:** Insert void element. No AFE reconstruction.

### Class 6: P-Closing Void (1 tag)

```
hr
```

**Action:** Close `<p>` if in button scope → insert void element.

Unique hybrid: combines P-closing (Class 1) with void insertion.

### Class 7: Head-Delegated (10 tags)

```
base, basefont, bgsound, link, meta, noframes, script, style, template, title
```

**Action:** Process via InHead handler (same rules as inside `<head>`).

These are accepted in BOTH InBody AND InTable modes — they transcend context.
They're the only children accepted in nearly every insertion mode.

### Class 8: Table-Internal Dropped (10 tags)

```
caption, col, colgroup, frame, tbody, td, tfoot, th, thead, tr
```

**Action:** DROP — parse error, ignored entirely.

These only function inside their respective table modes. In InBody, they're
invisible to the parser.

### Class 9: Structural Dropped (3 tags)

```
html, head, body
```

**Action:** DROP. (`html` and `body` may merge attributes onto existing elements.)

### Class 10: Self-Closing Siblings — List Items (3 tags)

```
li, dd, dt
```

**Action:**
- `li`: Walk stack for open `<li>`, close it → close `<p>` → insert.
- `dd`/`dt`: Walk stack for open `<dd>` or `<dt>`, close it → close `<p>` → insert.

These combine P-closing (Class 1) with a stack walk for same-type siblings.
The stack walk skips `<address>`, `<div>`, and `<p>` (they're not treated as
stop boundaries).

### Class 11: Self-Closing Siblings — Options (2 tags)

```
option, optgroup
```

**Action:** If current node is `<option>`, pop it → reconstruct AFE → insert.

In InBody mode (outside `<select>`), these auto-close a previous `<option>`
on the current node. In InSelect mode, they have different behavior (see
Finding 4).

### Class 12: Close-Reopen Elements (2 tags)

```
button, nobr
```

**Action:**
- `button`: If `<button>` in scope → generate implied end tags, pop to `<button>` → reconstruct AFE → insert new `<button>`.
- `nobr`: If `<nobr>` in scope → adoption agency → reconstruct AFE → insert.

### Class 13: Anchor (1 tag)

```
a
```

**Action:** If `<a>` in active formatting list → adoption agency → reconstruct AFE → insert → push to formatting list.

### Class 14: Mode-Switching Containers (4 tags)

```
table    → Close <p> (not quirks) → insert → switch to InTable
select   → Reconstruct AFE → insert → switch to InSelect
textarea → Insert → skip newline → tokenizer RCDATA → mode Text
iframe   → Insert → tokenizer raw text → mode Text
```

Each switches the parser into a completely different mode upon insertion.

### Class 15: Ruby Children (4 tags)

```
rb, rtc → If <ruby> in scope, generate implied end tags → insert
rp, rt  → If <ruby> in scope, generate implied end tags (excl. rtc) → insert
```

### Class 16: AFE Marker Elements (3 tags)

```
applet, marquee, object
```

**Action:** Reconstruct AFE → insert → push formatting **marker** (not the
element itself). The marker prevents AFE reconstruction from crossing this
element's boundary.

### Class 17: Foreign Content (2 tags)

```
math → Reconstruct AFE → insert in MathML namespace → InForeignContent
svg  → Reconstruct AFE → insert in SVG namespace → InForeignContent
```

### Class 18: Generic Phrasing — "Any Other Start Tag" (~30 tags)

```
abbr, audio, bdi, bdo, canvas, cite, data, datalist, del, dfn, ins, kbd,
label, legend, map, mark, meter, output, picture, progress, q, ruby, samp,
slot, span, sub, sup, time, var, video
```

**Action:** Reconstruct AFE → insert.

This is the catch-all. Every tag not explicitly listed falls into this path.

### Special Cases

- `form`: Close `<p>` → if form pointer already set (no template): DROP. Else insert + set form pointer.
- `xmp`: Close `<p>` → reconstruct AFE → raw text mode.
- `noembed`, `noscript` (scripting on): Raw text mode.

---

## Finding 3: Cross-Mode Behavior Matrix

Each child class has a predictable behavior pattern across all 14 parent modes.
Here's the cross-mode matrix for the major child classes:

### P-Closing Blocks (e.g., `div`) across all parent modes

| Parent Mode | Action |
|-------------|--------|
| InBody (73 parents) | CLOSE_P + ACCEPT |
| InTable | FOSTER |
| InTableBody | FOSTER (via InTable) |
| InRow | FOSTER (via InTable) |
| InCell | CLOSE_P + ACCEPT (InBody rules) |
| InCaption | CLOSE_P + ACCEPT (InBody rules) |
| InColumnGroup | CLOSE_PARENT → InTable → FOSTER |
| InSelect | DROP |
| Raw Text | RAW_TEXT (not parsed) |
| InForeignContent | EXIT_FOREIGN |
| Void | N/A |
| InTemplate | ACCEPT (pushes InBody) |
| InHead | CLOSE_PARENT → reprocess |

### Generic Phrasing (e.g., `span`) across all parent modes

| Parent Mode | Action |
|-------------|--------|
| InBody (73 parents) | ACCEPT |
| InTable | FOSTER |
| InTableBody | FOSTER |
| InRow | FOSTER |
| InCell | ACCEPT |
| InCaption | ACCEPT |
| InColumnGroup | CLOSE_PARENT → FOSTER |
| InSelect | DROP |
| Raw Text | RAW_TEXT |
| InForeignContent | EXIT_FOREIGN |
| Void | N/A |
| InTemplate | ACCEPT |
| InHead | CLOSE_PARENT → reprocess |

### Head-Delegated (e.g., `script`, `template`) across all parent modes

| Parent Mode | Action |
|-------------|--------|
| InBody | ACCEPT (via InHead handler) |
| InTable | ACCEPT (via InHead handler) |
| InTableBody | ACCEPT (via InTable → InHead) |
| InRow | ACCEPT (via InTable → InHead) |
| InCell | ACCEPT (via InBody → InHead) |
| InCaption | ACCEPT (via InBody → InHead) |
| InColumnGroup | ACCEPT (template only, via InHead) |
| InSelect | ACCEPT (script/template only, via InHead) |
| Raw Text | RAW_TEXT |
| InForeignContent | ACCEPT_FOREIGN (stays in foreign) |
| Void | N/A |
| InTemplate | ACCEPT (via InHead) |
| InHead | ACCEPT |

**Key pattern:** `script` and `template` are accepted in almost every mode.
They are the most universally accepted children.

### Table-Internal (e.g., `tr`, `td`) across all parent modes

| Parent Mode | Action |
|-------------|--------|
| InBody | DROP |
| InTable | ACCEPT or WRAP (depending on specific tag) |
| InTableBody | ACCEPT (tr) or WRAP (td/th) |
| InRow | ACCEPT (td/th) or CLOSE_PARENT (tr) |
| InCell | CLOSE_PARENT |
| InCaption | CLOSE_PARENT |
| InColumnGroup | CLOSE_PARENT → InTable → (specific action) |
| InSelect | DROP (or CLOSE_PARENT in InSelectInTable) |
| Raw Text | RAW_TEXT |
| InForeignContent | EXIT_FOREIGN (for some like `table`) or ACCEPT_FOREIGN |
| Void | N/A |
| InTemplate | ACCEPT (adapts internal mode) |
| InHead | CLOSE_PARENT → reprocess |

---

## Finding 4: Structural Symmetries

### Symmetry A: InCell ≡ InCaption

`td`, `th` (InCell) and `caption` (InCaption) have **nearly identical** child
handling:

- Same 9-tag "close parent" set: `caption`, `col`, `colgroup`, `tbody`, `td`,
  `tfoot`, `th`, `thead`, `tr`
- Same "everything else = InBody" fallthrough
- Same 107 children handled via InBody rules

The ONLY difference: the close-parent mechanism. InCell closes the cell
(via `CloseTheCell()`), InCaption closes the caption. But the trigger set
and fallthrough are identical.

**Rule:** InCell and InCaption are the same pattern — "InBody + table-
structural escape hatch."

### Symmetry B: thead ≡ tbody ≡ tfoot

All three share InTableBody mode with identical behavior. There is zero
difference in child handling.

### Symmetry C: td ≡ th

Both share InCell mode with identical behavior. The parser doesn't distinguish
between data cells and header cells.

### Symmetry D: svg ≡ math (nearly)

Both use InForeignContent mode with the same EXIT_FOREIGN tag set (~46 tags).
The only difference is namespace (SVG vs MathML) and some attribute
adjustments. The behavioral rules are identical.

### Symmetry E: All 5 Raw Text parents

`script`, `style`, `title`, `textarea`, `iframe` — identical behavior (no
child parsing). The only nuance: `title`/`textarea` use RCDATA (entities
decoded), while `script`/`style`/`iframe` use true raw text (entities NOT
decoded). But from a pair-matching perspective, all 116 children are RAW_TEXT
for all 5.

---

## Finding 5: The Table Hierarchy Cascade

Table elements form a strict hierarchy with predictable cascade rules:

```
table → thead/tbody/tfoot → tr → td/th → [InBody content]
```

At each level, the parser enforces:

| Situation | Rule | Mechanism |
|-----------|------|-----------|
| Child belongs to **this** level | CLOSE_PARENT (close current, open new sibling) | `<tr>` inside `<tr>` closes current row |
| Child belongs to a **deeper** level | WRAP (auto-generate missing intermediary) | `<td>` inside `<table>` auto-generates `<tbody>` + `<tr>` |
| Child belongs to a **higher** level | CLOSE_PARENT (close current, reprocess) | `</table>` inside `<tr>` closes row + tbody + table |
| Child is **unrelated** to tables | FOSTER (move before table) | `<div>` inside `<table>` foster-parented |
| Child is head-delegated | ACCEPT (via InHead) | `<script>` inside `<table>` accepted directly |

### Wrap Chains (auto-generated intermediaries)

| Pair | Auto-generated |
|------|----------------|
| `table` → `col` | `<colgroup>` wrapper |
| `table` → `tr` | `<tbody>` wrapper |
| `table` → `td`/`th` | `<tbody>` + `<tr>` wrappers |
| `thead`/`tbody`/`tfoot` → `td`/`th` | `<tr>` wrapper |

### The Foster Parenting Catch-All

In InTable, InTableBody, and InRow modes, the "anything else" handler is
**always foster parenting**. The content is processed with InBody rules but
inserted **before** the table element in the DOM, not inside it.

This means:
- InTable fosters ~100 of 116 child tags
- InTableBody fosters ~106 of 116 child tags
- InRow fosters ~105 of 116 child tags

Foster parenting is the dominant action in all table-context modes.

---

## Finding 6: The Escape Hatch Pattern

Four insertion modes share a common pattern: "restricted mode with a set of
tags that break out to a different mode."

| Mode | Escape Tags | Escape Action | Fallthrough |
|------|-------------|---------------|-------------|
| InCell | 9 table-structural tags | Close cell → InRow | InBody |
| InCaption | Same 9 tags | Close caption → InTable | InBody |
| InSelect | `input`, `keygen`, `textarea` | Close select → reset mode | DROP (most tags) |
| InSelect | `select` (self) | Close select → reset mode | DROP |
| InSelectInTable | 8 table-structural tags | Close select → reset mode | InSelect rules |
| InForeignContent | ~46 common HTML tags | Exit foreign → HTML mode | ACCEPT_FOREIGN |
| InColumnGroup | Everything except `col`/`template` | Close colgroup → InTable | N/A |

**The pattern:** Each restricted mode has:
1. A small set of tags it handles natively
2. An "escape set" that closes the current context
3. A fallthrough for everything else

---

## Finding 7: Universal Children and Universal Rejects

### Tags accepted in the MOST parent modes

| Tag | Accepted in modes | Notes |
|-----|-------------------|-------|
| `script` | InBody, InHead, InTable, InTableBody*, InRow*, InCell, InCaption, InColumnGroup*, InSelect, InTemplate | Only RAW_TEXT and Void modes exclude it. * = via delegation chain |
| `template` | Same as `script` | Near-universal acceptance |
| `style` | InBody, InHead, InTable, InCell, InCaption, InTemplate | Via InHead delegation |

### Tags dropped/ignored in the MOST parent modes

| Tag | Dropped/ignored in | Notes |
|-----|--------------------|-------|
| `frame` | InBody (DROP), InTable (FOSTER), InSelect (DROP), Raw Text (RAW_TEXT) | No valid parent except `<frameset>` (not in our model) |
| `html` | Dropped/ignored in every mode | Attributes may merge, but element never inserted |
| `head` | Dropped in InBody, InTable (via foster → InBody → drop) | Only accepted initially |
| `body` | Dropped in InBody | Only accepted initially |

---

## Finding 8: Compression Ratio

### The Naive Model: 13,456 rules

116 parents × 116 children = 13,456 unique pair rules.

### The Compressed Model: ~50 rules

Using mode grouping + child classes:

| Step | Compression |
|------|-------------|
| 116 parents → 14 modes | 8.3× (parent dimension) |
| 116 children → 18 classes (InBody) | 6.4× (child dimension in InBody) |
| 116 children → 3-6 classes (other modes) | Varies per mode |

**Unique (mode, child-class) → action mappings:**

| Mode | # Child Classes | # Rules |
|------|-----------------|---------|
| InBody | 18 | 18 |
| InTable | 6 | 6 |
| InTableBody | 5 | 5 |
| InRow | 4 | 4 |
| InCell | 2 (escape set + InBody) | 2* |
| InCaption | 2 (escape set + InBody) | 2* |
| InColumnGroup | 4 | 4 |
| InSelect | 5 | 5 |
| Raw Text | 1 | 1 |
| InForeignContent | 2 | 2 |
| Void | 1 | 1 |
| InTemplate | 1 | 1 |
| InHead | 4 | 4 |
| BeforeHead/AfterHead | 3 | 3 |
| **Total** | | **~58** |

*InCell/InCaption reuse InBody's 18 classes for their fallthrough, so the
18 InBody rules aren't counted again.

**Result: ~58 rules cover all 13,456 pairs — a 232× compression.**

---

## Finding 9: The Three Fundamental Actions

Despite 12 action codes in the legend, every pair ultimately resolves to one
of three primitive operations:

### Primitive 1: INSERT

The child element is created and attached to the DOM tree. This covers:
- ACCEPT, ACCEPT_VOID, ACCEPT_FOREIGN
- CLOSE_P + ACCEPT (close `<p>`, then insert)
- CLOSE_HEADING + ACCEPT (close heading, then insert)
- WRAP + ACCEPT (generate wrapper, then insert child within it)
- RECONSTRUCT_AFE + ACCEPT (rebuild formatting context, then insert)
- RAW_TEXT (element inserted, then tokenizer mode changes)

### Primitive 2: REJECT

The child's start tag is discarded with no DOM effect:
- DROP (explicit ignore)
- DROP within InSelect (parse error + ignore)

### Primitive 3: RESTRUCTURE

The existing DOM structure is modified before/instead of simple insertion:
- CLOSE_PARENT (pop parent off stack, reprocess child in new context)
- CLOSE_REOPEN (close same-type ancestor, then insert new instance)
- FOSTER (insert at foster parent location instead of current node)
- EXIT_FOREIGN (pop out of foreign namespace, reprocess)
- ADOPTION_AGENCY (complex tree restructuring for formatting elements)

### Distribution

| Primitive | % of 13,456 pairs |
|-----------|-------------------|
| INSERT | ~82% (ACCEPT + CLOSE_P + AFE + WRAP + RAW_TEXT + FOREIGN_ACCEPT + TEMPLATE) |
| REJECT | ~10% (DROP + VOID-N/A) |
| RESTRUCTURE | ~8% (FOSTER + CLOSE_PARENT + EXIT_FOREIGN + ADOPTION_AGENCY) |

---

## Finding 10: Context-Sensitive vs. Context-Free Rules

Most pair rules are **context-free** — they depend only on the parent's mode
and the child's tag name. But some rules are **context-sensitive**, depending
on the stack of open elements:

### Context-Free Rules (determine action from parent mode + child tag alone)

- All Raw Text rules
- All Void rules
- All InTable FOSTER rules
- All InSelect DROP rules
- All InForeignContent EXIT/ACCEPT rules
- InBody: Classes 1-9 (P-closing blocks, headings, AFE, void, head-delegated, table-dropped, structural-dropped)
- InBody: Classes 17-18 (foreign, generic phrasing)

### Context-Sensitive Rules (need stack information)

| Rule | What it checks | Affected children |
|------|---------------|-------------------|
| P-closing | "Is `<p>` in button scope?" | 27 block elements + h1-h6 + hr + table + li + dd/dt |
| Button close-reopen | "Is `<button>` in scope?" | `button` |
| Li stack walk | "Is `<li>` on the stack?" | `li` |
| Dd/dt stack walk | "Is `<dd>` or `<dt>` on the stack?" | `dd`, `dt` |
| Anchor adoption | "Is `<a>` in formatting list?" | `a` |
| Nobr adoption | "Is `<nobr>` in scope?" | `nobr` |
| Form pointer | "Is form pointer set?" | `form` |
| Ruby scope | "Is `<ruby>` in scope?" | `rb`, `rtc`, `rp`, `rt` |
| Option auto-close | "Is current node `<option>`?" | `option`, `optgroup` |
| Table quirks | "Is document in quirks mode?" | `table` (p-closing only) |

**Key insight for pair analysis:** For **top-level pair matching** (parent is
the direct current node), most context-sensitive rules resolve deterministically.
If `<p>` is the parent and a block arrives, `<p>` IS in button scope → always
CLOSE_P. If `<button>` is the parent and `<button>` arrives, it IS in scope →
always CLOSE_REOPEN.

Context-sensitivity only matters for **multi-level nesting** (grandparent
effects), which is outside the scope of pairwise analysis.

---

## Finding 11: Child Class Overlap Between Modes

Several child classes recur across modes with the same semantics:

### The "Head-Delegated Set" appears in 5+ modes

`script`, `style`, `template` (and `base`, `link`, `meta`, etc.) are
processed via InHead handler in InBody, InTable, InColumnGroup, InSelect,
and InTemplate modes. The delegation chain differs but the end result
(ACCEPT via InHead rules) is the same.

### The "Table-Structural Escape Set" appears in 3 modes

The set `{caption, col, colgroup, tbody, td, tfoot, th, thead, tr}` triggers
CLOSE_PARENT in InCell, InCaption, and (partially) InTableBody/InRow.

### The "Foster Set" is just "everything else" in table modes

The FOSTER action in InTable, InTableBody, and InRow is not a specific tag
set — it's the complement of the explicitly handled tags. In each mode:

| Mode | Explicitly handled | Foster = 116 minus handled |
|------|-------------------|---------------------------|
| InTable | 16 tags | ~100 tags |
| InTableBody | 10 tags | ~106 tags |
| InRow | 11 tags | ~105 tags |

### The EXIT_FOREIGN set is fixed across svg AND math

The same ~46 HTML tags break out of foreign content regardless of whether
the parent is `<svg>` or `<math>`.

---

## Finding 12: Rule Priority Chains

Within InBody mode, when a child tag arrives, Chrome checks rules in a
specific priority order. Several child classes combine multiple checks:

### Combined check chains

| Child | Check 1 | Check 2 | Check 3 |
|-------|---------|---------|---------|
| `h1`-`h6` | Close `<p>` in button scope | Pop current heading | Insert |
| `li` | Walk stack for `<li>` | Close `<p>` in button scope | Insert |
| `dd`/`dt` | Walk stack for `<dd>`/`<dt>` | Close `<p>` in button scope | Insert |
| `a` | Adoption agency for existing `<a>` | Reconstruct AFE | Insert + push AFE |
| `button` | Close `<button>` in scope | Reconstruct AFE | Insert |
| `form` | Check form pointer | Close `<p>` in button scope | Insert + set pointer |
| `table` | Close `<p>` (not quirks) | Insert | Switch to InTable |

### Single-action children (no chain)

| Child class | Action |
|-------------|--------|
| Generic phrasing (30 tags) | Reconstruct AFE → insert |
| Void+AFE (6 tags) | Reconstruct AFE → insert void |
| Head-delegated (10 tags) | Delegate to InHead |
| Table-internal (10 tags) | DROP |
| Structural (3 tags) | DROP |

---

## Summary: The Minimal Mental Model

To predict Chrome's behavior for any parent→child pair:

### Step 1: What mode is the parent in?

```
InBody (73 elements)  →  Step 2a
InTable (table)       →  Step 2b
InTableBody (3)       →  Step 2c
InRow (tr)            →  Step 2d
InCell (td, th)       →  Step 2a + table-structural escape
InCaption (caption)   →  Step 2a + table-structural escape
InColumnGroup (colgroup) → Accept col/template, close for rest
InSelect (3)          →  Accept option/optgroup/hr, drop rest
Raw Text (5)          →  Everything is raw text
Foreign (svg, math)   →  Exit for ~46 HTML tags, accept foreign for rest
Void (15)             →  No children
Template              →  Accept everything
Head                  →  Accept head-content, close for rest
```

### Step 2a: What class is the child in? (InBody mode)

```
P-closing block (27)  →  Close <p> + insert
Heading (6)           →  Close <p> + close heading + insert
AFE formatting (12)   →  Reconstruct AFE + insert + push AFE
Void+AFE (7)          →  Reconstruct AFE + insert void
Void no-AFE (3)       →  Insert void
P-closing void: hr    →  Close <p> + insert void
Head-delegated (10)   →  Process via InHead
Table-internal (10)   →  DROP
Structural (3)        →  DROP
Self-closing (li/dd/dt) → Stack walk + close <p> + insert
Options (2)           →  Auto-close option + insert
Close-reopen (2)      →  Close same-type + insert
Anchor                →  Adoption agency + insert
Mode-switch (4)       →  Insert + change mode
Ruby (4)              →  Implied end tags + insert
AFE marker (3)        →  Insert + push marker
Foreign (2)           →  Insert in foreign namespace
Generic (30)          →  Reconstruct AFE + insert
```

### Step 2b: InTable child classes

```
Table-structural (9)  →  ACCEPT or WRAP (with auto-generated intermediaries)
Self (table)          →  CLOSE_PARENT
Head-delegated (3)    →  ACCEPT via InHead
input[type=hidden]    →  ACCEPT_VOID
form                  →  DROP (or limited insert)
html                  →  DROP
Everything else (100) →  FOSTER
```

### Step 2c/2d: InTableBody / InRow child classes

```
Next-level child      →  ACCEPT (tr in tbody, td/th in row)
Deeper-level child    →  WRAP (td/th in tbody)
Same/higher level     →  CLOSE_PARENT
Head-delegated        →  ACCEPT via InHead delegation chain
Everything else       →  FOSTER
```

---

## Appendix: Element Sets Quick Reference

### Set: P-Closing Triggers (33 tags)

Tags that close `<p>` when they arrive in InBody:

```
address, article, aside, blockquote, center, dd, details, dialog, dir, div,
dl, dt, fieldset, figcaption, figure, footer, form, h1, h2, h3, h4, h5, h6,
header, hgroup, hr, li, listing, main, menu, nav, ol, p, pre, search,
section, summary, table, ul
```

### Set: Active Formatting Elements (12 tags)

```
b, big, code, em, font, i, s, small, strike, strong, tt, u
```

### Set: Head-Delegated (10 tags)

```
base, basefont, bgsound, link, meta, noframes, script, style, template, title
```

### Set: Table-Internal Dropped in InBody (10 tags)

```
caption, col, colgroup, frame, tbody, td, tfoot, th, thead, tr
```

### Set: EXIT_FOREIGN (~46 tags)

Tags that break out of SVG/MathML:

```
b, big, blockquote, body, br, center, code, dd, div, dl, dt, em, embed,
font (with color/face/size attr), h1, h2, h3, h4, h5, h6, head, hr, i,
img, li, listing, menu, meta, nobr, ol, p, pre, ruby, s, small, span,
strong, strike, sub, sup, table, tt, u, ul, var
```

### Set: Table-Structural Escape (9 tags)

Tags that close InCell/InCaption:

```
caption, col, colgroup, tbody, td, tfoot, th, thead, tr
```

### Set: InSelect Accepted (5 tags)

```
option, optgroup, hr, script, template
```

### Set: Void Elements (15 tags)

```
area, base, br, col, embed, hr, img, input, link, meta, param, portal,
source, track, wbr
```

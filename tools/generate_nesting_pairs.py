#!/usr/bin/env python3
"""
Generate the exhaustive HTML element nesting pair list.
Every parent x child = ACTION, with Chrome/Firefox behavior note.
"""

# ─── All 147 standard HTML elements ───

ALL_ELEMENTS = [
    "a", "abbr", "address", "area", "article", "aside", "audio",
    "b", "base", "bdi", "bdo", "blockquote", "body", "br", "button",
    "canvas", "caption", "cite", "code", "col", "colgroup",
    "data", "datalist", "dd", "del", "details", "dfn", "dialog", "div", "dl", "dt",
    "em", "embed",
    "fieldset", "figcaption", "figure", "footer", "form",
    "h1", "h2", "h3", "h4", "h5", "h6", "head", "header", "hgroup", "hr", "html",
    "i", "iframe", "img", "input", "ins",
    "kbd",
    "label", "legend", "li", "link",
    "main", "map", "mark", "math", "menu", "meta", "meter",
    "nav", "noscript",
    "object", "ol", "optgroup", "option", "output",
    "p", "param", "picture", "portal", "pre", "progress",
    "q",
    "rp", "rt", "ruby",
    "s", "samp", "script", "search", "section", "select", "slot", "small",
    "source", "span", "strong", "style", "sub", "summary", "sup", "svg",
    "table", "tbody", "td", "template", "textarea", "tfoot", "th", "thead",
    "time", "title", "tr", "track",
    "u", "ul",
    "var", "video", "wbr",
]

# ─── Element sets (from HTML parsing spec) ───

VOID = {
    "area", "base", "br", "col", "embed", "hr", "img", "input",
    "link", "meta", "param", "portal", "source", "track", "wbr",
}

RAW_TEXT_PARENTS = {"script", "style", "title", "textarea", "iframe"}

# Phrasing content elements (can appear inside <p>)
PHRASING = {
    "a", "abbr", "audio", "b", "bdi", "bdo", "br", "button", "canvas",
    "cite", "code", "data", "datalist", "del", "dfn", "em", "embed",
    "i", "iframe", "img", "input", "ins", "kbd", "label", "map", "mark",
    "math", "meter", "noscript", "object", "output", "picture", "portal",
    "progress", "q", "ruby", "rp", "rt", "s", "samp", "script", "select",
    "slot", "small", "span", "strong", "style", "sub", "sup", "svg",
    "template", "textarea", "time", "u", "var", "video", "wbr",
}

# Elements that auto-close <p> (and h1-h6, pre)
P_CLOSING_SET = {
    "address", "article", "aside", "blockquote", "center", "details",
    "dialog", "dir", "div", "dl", "fieldset", "figcaption", "figure",
    "footer", "form", "h1", "h2", "h3", "h4", "h5", "h6", "header",
    "hgroup", "hr", "li", "main", "menu", "nav", "ol", "p", "pre",
    "search", "section", "summary", "table", "ul",
}

HEADINGS = {"h1", "h2", "h3", "h4", "h5", "h6"}

# Table-valid children (not foster-parented)
TABLE_CHILDREN = {
    "caption", "colgroup", "col", "thead", "tbody", "tfoot", "tr",
    "script", "template", "style",
}

# Elements that close table cells
CELL_CLOSING = {"td", "th", "tr", "caption", "colgroup", "thead", "tbody", "tfoot"}

# Elements that close <tr>
ROW_CLOSING = {"tr", "caption", "colgroup", "thead", "tbody", "tfoot"}

# Elements that close thead/tbody/tfoot
BODY_CLOSING = {"caption", "colgroup", "thead", "tbody", "tfoot"}

# Elements that exit SVG/MathML foreign content
EXIT_FOREIGN = {
    "b", "big", "blockquote", "body", "br", "center", "code", "dd", "div",
    "dl", "dt", "em", "embed", "h1", "h2", "h3", "h4", "h5", "h6", "head",
    "hr", "i", "img", "li", "listing", "menu", "meta", "nobr", "ol", "p",
    "pre", "ruby", "s", "small", "span", "strong", "strike", "sub", "sup",
    "table", "tt", "u", "ul", "var",
}

# Active formatting elements (reconstructed across splits)
ACTIVE_FORMATTING = {
    "a", "b", "big", "code", "em", "font", "i", "nobr", "s", "small",
    "strike", "strong", "tt", "u",
}

# Select-valid children
SELECT_CHILDREN = {"option", "optgroup", "hr", "script", "template"}

# Elements that close <select>
SELECT_CLOSING = {"select", "input", "textarea"}

# Interactive elements (forbidden inside <a>)
INTERACTIVE = {"a", "button", "details", "embed", "iframe", "label", "select", "textarea"}

# ─── UA default styles per element (Chromium/Firefox consensus) ───
# Each entry: { property: value, ... }
# Only non-initial values listed (initial = what CSS spec defaults to)

UA_STYLES = {
    "a":          {"display": "inline", "color": "linkcolor", "text-decoration": "underline", "cursor": "pointer"},
    "abbr":       {"display": "inline"},
    "address":    {"display": "block", "font-style": "italic"},
    "area":       {"display": "none"},
    "article":    {"display": "block"},
    "aside":      {"display": "block"},
    "audio":      {"display": "none"},  # without controls attr
    "b":          {"display": "inline", "font-weight": "bold"},
    "base":       {"display": "none"},
    "bdi":        {"display": "inline", "unicode-bidi": "isolate"},
    "bdo":        {"display": "inline", "unicode-bidi": "bidi-override"},
    "blockquote": {"display": "block", "margin-block": "1em", "margin-inline": "40px"},
    "body":       {"display": "block", "margin": "8px"},
    "br":         {"display": "inline"},  # generates line break
    "button":     {"display": "inline-block", "text-align": "center", "cursor": "default", "border": "outset", "padding": "1px 6px", "font": "inherit"},
    "canvas":     {"display": "inline", "width": "300px", "height": "150px"},  # replaced
    "caption":    {"display": "table-caption", "text-align": "center"},
    "cite":       {"display": "inline", "font-style": "italic"},
    "code":       {"display": "inline", "font-family": "monospace"},
    "col":        {"display": "table-column"},
    "colgroup":   {"display": "table-column-group"},
    "data":       {"display": "inline"},
    "datalist":   {"display": "none"},
    "dd":         {"display": "block", "margin-inline-start": "40px"},
    "del":        {"display": "inline", "text-decoration": "line-through"},
    "details":    {"display": "block"},
    "dfn":        {"display": "inline", "font-style": "italic"},
    "dialog":     {"display": "block", "position": "absolute", "inset-inline": "0", "width": "fit-content", "height": "fit-content", "margin": "auto", "border": "solid", "padding": "1em", "background": "canvas", "color": "canvastext"},
    "div":        {"display": "block"},
    "dl":         {"display": "block", "margin-block": "1em"},
    "dt":         {"display": "block"},
    "em":         {"display": "inline", "font-style": "italic"},
    "embed":      {"display": "inline"},  # replaced
    "fieldset":   {"display": "block", "border": "2px groove", "padding": "0.35em 0.75em 0.625em", "margin-inline": "2px", "min-inline-size": "min-content"},
    "figcaption": {"display": "block"},
    "figure":     {"display": "block", "margin-block": "1em", "margin-inline": "40px"},
    "footer":     {"display": "block"},
    "form":       {"display": "block"},
    "h1":         {"display": "block", "font-size": "2em", "font-weight": "bold", "margin-block": "0.67em"},
    "h2":         {"display": "block", "font-size": "1.5em", "font-weight": "bold", "margin-block": "0.83em"},
    "h3":         {"display": "block", "font-size": "1.17em", "font-weight": "bold", "margin-block": "1em"},
    "h4":         {"display": "block", "font-weight": "bold", "margin-block": "1.33em"},
    "h5":         {"display": "block", "font-size": "0.83em", "font-weight": "bold", "margin-block": "1.67em"},
    "h6":         {"display": "block", "font-size": "0.67em", "font-weight": "bold", "margin-block": "2.33em"},
    "head":       {"display": "none"},
    "header":     {"display": "block"},
    "hgroup":     {"display": "block"},
    "hr":         {"display": "block", "border-style": "inset", "border-width": "1px", "margin-block": "0.5em", "overflow": "hidden"},
    "html":       {"display": "block"},
    "i":          {"display": "inline", "font-style": "italic"},
    "iframe":     {"display": "inline", "border": "2px inset", "width": "300px", "height": "150px"},  # replaced
    "img":        {"display": "inline"},  # replaced
    "input":      {"display": "inline-block", "cursor": "text", "border": "1px solid", "padding": "1px", "font": "inherit"},  # type-dependent
    "ins":        {"display": "inline", "text-decoration": "underline"},
    "kbd":        {"display": "inline", "font-family": "monospace"},
    "label":      {"display": "inline", "cursor": "default"},
    "legend":     {"display": "block", "padding-inline": "2px"},
    "li":         {"display": "list-item"},
    "link":       {"display": "none"},
    "main":       {"display": "block"},
    "map":        {"display": "inline"},
    "mark":       {"display": "inline", "background": "yellow", "color": "black"},
    "math":       {"display": "inline"},  # MathML namespace
    "menu":       {"display": "block", "list-style-type": "disc", "margin-block": "1em", "padding-inline-start": "40px"},
    "meta":       {"display": "none"},
    "meter":      {"display": "inline-block", "width": "5em", "height": "1em", "vertical-align": "-0.2em"},
    "nav":        {"display": "block"},
    "noscript":   {"display": "inline"},  # block when contains block children
    "object":     {"display": "inline"},  # replaced
    "ol":         {"display": "block", "list-style-type": "decimal", "margin-block": "1em", "padding-inline-start": "40px"},
    "optgroup":   {"display": "block", "font-weight": "bold"},  # inside select; platform-native
    "option":     {"display": "block"},  # inside select; platform-native
    "output":     {"display": "inline"},
    "p":          {"display": "block", "margin-block": "1em"},
    "param":      {"display": "none"},
    "picture":    {"display": "contents"},  # effectively transparent
    "portal":     {"display": "inline"},  # replaced
    "pre":        {"display": "block", "font-family": "monospace", "white-space": "pre", "margin-block": "1em"},
    "progress":   {"display": "inline-block", "width": "10em", "height": "1em", "vertical-align": "-0.2em"},
    "q":          {"display": "inline"},  # ::before{content:open-quote} ::after{content:close-quote}
    "rp":         {"display": "none"},  # ruby-aware; inline fallback
    "rt":         {"display": "ruby-text", "font-size": "0.5em"},  # display:ruby-text
    "ruby":       {"display": "ruby"},
    "s":          {"display": "inline", "text-decoration": "line-through"},
    "samp":       {"display": "inline", "font-family": "monospace"},
    "script":     {"display": "none"},
    "search":     {"display": "block"},
    "section":    {"display": "block"},
    "select":     {"display": "inline-block", "border": "1px solid", "white-space": "pre", "cursor": "default"},
    "slot":       {"display": "contents"},
    "small":      {"display": "inline", "font-size": "smaller"},
    "source":     {"display": "none"},
    "span":       {"display": "inline"},
    "strong":     {"display": "inline", "font-weight": "bold"},
    "style":      {"display": "none"},
    "sub":        {"display": "inline", "vertical-align": "sub", "font-size": "smaller"},
    "summary":    {"display": "block", "cursor": "pointer"},  # display:list-item with disclosure marker
    "sup":        {"display": "inline", "vertical-align": "super", "font-size": "smaller"},
    "svg":        {"display": "inline", "overflow": "hidden"},  # SVG namespace
    "table":      {"display": "table", "border-collapse": "separate", "border-spacing": "2px", "border-color": "gray", "text-indent": "0"},
    "tbody":      {"display": "table-row-group", "vertical-align": "middle", "border-color": "inherit"},
    "td":         {"display": "table-cell", "padding": "1px", "vertical-align": "inherit", "text-align": "inherit"},
    "template":   {"display": "none"},
    "textarea":   {"display": "inline-block", "border": "1px solid", "padding": "2px", "font-family": "monospace", "white-space": "pre-wrap", "overflow": "auto", "resize": "both", "cursor": "text"},
    "tfoot":      {"display": "table-footer-group", "vertical-align": "middle", "border-color": "inherit"},
    "th":         {"display": "table-cell", "padding": "1px", "font-weight": "bold", "text-align": "center", "vertical-align": "inherit"},
    "thead":      {"display": "table-header-group", "vertical-align": "middle", "border-color": "inherit"},
    "time":       {"display": "inline"},
    "title":      {"display": "none"},
    "tr":         {"display": "table-row", "vertical-align": "inherit", "border-color": "inherit"},
    "track":      {"display": "none"},
    "u":          {"display": "inline", "text-decoration": "underline"},
    "ul":         {"display": "block", "list-style-type": "disc", "margin-block": "1em", "padding-inline-start": "40px"},
    "var":        {"display": "inline", "font-style": "italic"},
    "video":      {"display": "inline", "object-fit": "contain"},  # replaced
    "wbr":        {"display": "inline"},  # generates line break opportunity
}

# Formatting context established by parent
PARENT_CONTEXT = {
    "block":              "BFC (block formatting context)",
    "inline":             "IFC (inline formatting context of ancestor BFC)",
    "inline-block":       "BFC (establishes own BFC)",
    "table":              "TFC (table formatting context)",
    "table-row-group":    "TFC row-group context",
    "table-header-group": "TFC row-group context",
    "table-footer-group": "TFC row-group context",
    "table-row":          "TFC row context",
    "table-cell":         "BFC (cell establishes own BFC)",
    "table-caption":      "BFC (caption establishes own BFC)",
    "table-column-group": "TFC column-group context (no child layout)",
    "table-column":       "TFC column context (no child layout)",
    "flex":               "FFC (flex formatting context)",
    "inline-flex":        "FFC (flex formatting context)",
    "grid":               "GFC (grid formatting context)",
    "inline-grid":        "GFC (grid formatting context)",
    "list-item":          "BFC (block + marker box)",
    "ruby":               "ruby formatting context",
    "ruby-text":          "ruby-text context",
    "contents":           "none (replaced by children in parent context)",
    "none":               "none (no boxes generated)",
}


def css_str(element):
    """Return compact CSS string for an element's UA defaults."""
    styles = UA_STYLES.get(element, {})
    if not styles:
        return "display:inline"
    parts = []
    for prop, val in styles.items():
        parts.append(f"{prop}:{val}")
    return "; ".join(parts)


def parent_context_str(element):
    """Return the formatting context the parent establishes for children."""
    styles = UA_STYLES.get(element, {})
    display = styles.get("display", "inline")
    return PARENT_CONTEXT.get(display, f"{display} context")


# ─── Parent rule classification ───

FLOW_PARENTS = {
    "body", "div", "article", "section", "nav", "aside", "main", "search",
    "blockquote", "dialog", "figcaption", "figure", "dd", "noscript",
    "address", "header", "footer", "form", "fieldset", "details",
    "summary", "legend", "li", "dt",
}

PHRASING_BLOCK_PARENTS = {"p", "h1", "h2", "h3", "h4", "h5", "h6", "pre"}

INLINE_PHRASING_PARENTS = {
    "span", "em", "strong", "b", "i", "u", "s", "small", "cite", "code",
    "var", "samp", "kbd", "sub", "sup", "abbr", "bdi", "bdo", "data",
    "dfn", "mark", "q", "time", "output", "label",
}

TRANSPARENT_PARENTS = {"a", "ins", "del", "map", "canvas", "object", "slot"}

LIST_PARENTS = {"ul", "ol", "menu"}

TABLE_CELL_PARENTS = {"td", "th"}

# ─── Behavior descriptions ───

BEHAVIOR = {
    "ACCEPT":
        "Valid child. Parser inserts as child node. Normal layout per display values.",
    "ACCEPT_PHRASING":
        "Valid phrasing child. Inline box in parent's inline formatting context.",
    "ACCEPT_BLOCK_IN_FLOW":
        "Valid flow child. Block box stacking vertically in normal flow. Margins collapse with siblings.",
    "ACCEPT_INLINE_IN_FLOW":
        "Valid flow child. Inline box. If mixed with block siblings, anonymous block boxes wrap inline runs.",
    "ACCEPT_TABLE_IN_FLOW":
        "Valid. Table establishes table formatting context as block-level box.",
    "ACCEPT_REPLACED":
        "Valid. Replaced element with intrinsic dimensions. Inline-level unless CSS overrides.",
    "ACCEPT_HIDDEN":
        "Valid. Element generates no layout boxes (display:none or inert content).",
    "ACCEPT_LIST_ITEM":
        "Valid. Child renders as display:list-item with marker box (bullet/number).",
    "ACCEPT_DT":
        "Valid. Block box. No indentation.",
    "ACCEPT_DD":
        "Valid. Block box with margin-inline-start:40px (UA default).",
    "ACCEPT_TABLE_CHILD":
        "Valid table structural child. Processed in table formatting context.",
    "ACCEPT_TR":
        "Valid. Table row processed in row context.",
    "ACCEPT_CELL":
        "Valid. Table cell establishes new BFC. Sized by table layout algorithm.",
    "ACCEPT_COL":
        "Valid. Column definition for table layout.",
    "ACCEPT_OPTION":
        "Valid. Platform-native rendering inside dropdown. Not CSS-laid-out.",
    "ACCEPT_OPTGROUP":
        "Valid. Labeled group inside dropdown. Platform-native rendering.",
    "ACCEPT_CAPTION":
        "Valid. display:table-caption, positioned per caption-side. Establishes BFC.",
    "ACCEPT_RUBY_TEXT":
        "Valid. display:ruby-text. Small annotation above/beside base text.",
    "ACCEPT_RUBY_PAREN":
        "Valid. display:none in ruby-aware browsers. Fallback parentheses.",
    "ACCEPT_SOURCE":
        "Valid. Media source candidate. Not rendered.",
    "ACCEPT_TRACK":
        "Valid. Text track for media. Not visually rendered in parent.",
    "ACCEPT_FOREIGN":
        "Valid. Parsed in SVG/MathML namespace with foreign content rules.",
    "ACCEPT_HEADING_IN_HGROUP":
        "Valid. Heading element inside hgroup for multi-level heading.",
    "ACCEPT_INERT":
        "Valid. Parsed into DocumentFragment but not rendered (template content).",
    "CLOSE_PARENT":
        "Parser auto-closes parent. Child becomes sibling of (now closed) parent. Re-processed in grandparent context.",
    "CLOSE_PARENT_P":
        "Parser auto-closes <p>. Block child becomes sibling after the closed paragraph.",
    "CLOSE_PARENT_HEADING":
        "Parser auto-closes heading. New heading becomes sibling of closed heading.",
    "CLOSE_PARENT_CELL":
        "Parser auto-closes table cell. Tag processed in row/table context.",
    "CLOSE_PARENT_ROW":
        "Parser auto-closes table row. Tag processed in table-body context.",
    "CLOSE_PARENT_TBODY":
        "Parser auto-closes row group. Tag processed in table context.",
    "CLOSE_PARENT_CAPTION":
        "Parser auto-closes caption. Tag processed in table context.",
    "CLOSE_PARENT_COLGROUP":
        "Parser auto-closes colgroup. Tag processed in table context.",
    "CLOSE_PARENT_SELECT":
        "Parser auto-closes select. Tag processed in parent context.",
    "CLOSE_PARENT_OPTGROUP":
        "Parser auto-closes optgroup. Tag processed in select context.",
    "CLOSE_PARENT_OPTION":
        "Parser auto-closes option. New option/optgroup opens in select context.",
    "CLOSE_PARENT_FOREIGN":
        "HTML element exits foreign content. SVG/MathML context closed. Tag processed in HTML mode.",
    "CLOSE_REOPEN_LI":
        "Previous <li> auto-closed. New <li> opens as sibling in same list.",
    "CLOSE_REOPEN_DT":
        "Previous <dt> or <dd> auto-closed. New <dt> opens as sibling in dl.",
    "CLOSE_REOPEN_DD":
        "Previous <dt> or <dd> auto-closed. New <dd> opens as sibling in dl.",
    "CLOSE_REOPEN_BUTTON":
        "Previous <button> auto-closed. New <button> opens.",
    "CLOSE_REOPEN_RT":
        "Previous <rt> auto-closed. New <rt> opens.",
    "CLOSE_REOPEN_RP":
        "Previous <rt> auto-closed. <rp> opens.",
    "FOSTER":
        "Foster parenting. Non-table content moved BEFORE the table ancestor in DOM.",
    "WRAP_TBODY":
        "Parser auto-generates <tbody>. Child inserted inside generated tbody.",
    "WRAP_TBODY_TR":
        "Parser auto-generates <tbody> and <tr>. Cell inserted inside generated row.",
    "WRAP_TR":
        "Parser auto-generates <tr>. Cell inserted inside generated row.",
    "DROP":
        "Start tag ignored entirely. Content (if any) adopted by current parent.",
    "DROP_FORM":
        "Nested <form> tag dropped. Children become part of outer form.",
    "DROP_HEAD":
        "Duplicate <head> tag ignored.",
    "DROP_BODY":
        "Duplicate <body> tag ignored. Attributes may merge.",
    "DROP_SELECT":
        "Tag ignored in select context. Only option/optgroup/hr accepted.",
    "RAW_TEXT":
        "Parent is in raw text mode. No child elements parsed. All content is text until end tag.",
    "VOID":
        "Parent is void element. Cannot have children. Content becomes sibling.",
    "BLOCK_IN_INLINE":
        "Parser accepts (no auto-close). Layout splits inline parent into fragments around the block child. Anonymous block boxes generated.",
    "RECONSTRUCT":
        "Active formatting element reconstructed across tree split boundary.",
}


def get_parent_rule(parent):
    """Return the parent rule name for an element."""
    if parent in VOID:
        return "VOID"
    if parent in RAW_TEXT_PARENTS:
        return "RAW_TEXT"
    if parent == "option":
        return "RAW_TEXT"  # option is text-only in select context
    if parent == "rp":
        return "RAW_TEXT"  # rp is text-only
    if parent == "html":
        return "HTML_ROOT"
    if parent == "head":
        return "HEAD"
    if parent in FLOW_PARENTS:
        return "FLOW_PARENT"
    if parent in TABLE_CELL_PARENTS:
        return "TABLE_CELL_PARENT"
    if parent == "caption":
        return "CAPTION_PARENT"
    if parent in PHRASING_BLOCK_PARENTS:
        return "PHRASING_BLOCK_PARENT"
    if parent in INLINE_PHRASING_PARENTS:
        return "INLINE_PHRASING_PARENT"
    if parent in TRANSPARENT_PARENTS:
        return "TRANSPARENT_PARENT"
    if parent in LIST_PARENTS:
        return "LIST_PARENT"
    if parent == "dl":
        return "DL_PARENT"
    if parent == "table":
        return "TABLE_PARENT"
    if parent in {"thead", "tbody", "tfoot"}:
        return "TABLE_BODY_PARENT"
    if parent == "tr":
        return "TABLE_ROW_PARENT"
    if parent == "colgroup":
        return "COLGROUP_PARENT"
    if parent == "select":
        return "SELECT_PARENT"
    if parent == "optgroup":
        return "OPTGROUP_PARENT"
    if parent == "ruby":
        return "RUBY_PARENT"
    if parent == "hgroup":
        return "HGROUP_PARENT"
    if parent == "svg":
        return "SVG_PARENT"
    if parent == "math":
        return "MATHML_PARENT"
    if parent == "template":
        return "TEMPLATE_PARENT"
    if parent == "button":
        return "BUTTON_PARENT"
    if parent == "datalist":
        return "DATALIST_PARENT"
    if parent in {"video", "audio"}:
        return "MEDIA_PARENT"
    if parent == "picture":
        return "PICTURE_PARENT"
    # Fallback
    return "FLOW_PARENT"


def get_action(parent, child):
    """Return (action_code, behavior_key) for a parent→child pair."""
    rule = get_parent_rule(parent)

    # ─── VOID ───
    if rule == "VOID":
        return ("VOID", "VOID")

    # ─── RAW TEXT ───
    if rule == "RAW_TEXT":
        return ("RAW_TEXT", "RAW_TEXT")

    # ─── HTML ROOT ───
    if rule == "HTML_ROOT":
        if child == "head":
            return ("ACCEPT", "ACCEPT")
        if child == "body":
            return ("ACCEPT", "ACCEPT")
        return ("WRAP_BODY", "ACCEPT")  # anything else auto-generates body

    # ─── HEAD ───
    if rule == "HEAD":
        if child in {"title", "meta", "link", "style", "script", "noscript", "base", "template"}:
            return ("ACCEPT", "ACCEPT_HIDDEN")
        if child == "head":
            return ("DROP", "DROP_HEAD")
        return ("CLOSE_PARENT", "CLOSE_PARENT")

    # ─── TEMPLATE ───
    if rule == "TEMPLATE_PARENT":
        return ("ACCEPT", "ACCEPT_INERT")

    # ─── SELECT ───
    if rule == "SELECT_PARENT":
        if child == "option":
            return ("ACCEPT", "ACCEPT_OPTION")
        if child == "optgroup":
            return ("ACCEPT", "ACCEPT_OPTGROUP")
        if child == "hr":
            return ("ACCEPT", "ACCEPT")
        if child in {"script", "template"}:
            return ("ACCEPT", "ACCEPT_HIDDEN")
        if child in SELECT_CLOSING:
            return ("CLOSE_PARENT", "CLOSE_PARENT_SELECT")
        return ("DROP", "DROP_SELECT")

    # ─── OPTGROUP ───
    if rule == "OPTGROUP_PARENT":
        if child == "option":
            return ("ACCEPT", "ACCEPT_OPTION")
        if child == "optgroup":
            return ("CLOSE_PARENT", "CLOSE_PARENT_OPTGROUP")
        if child == "select":
            return ("CLOSE_PARENT", "CLOSE_PARENT_SELECT")
        return ("DROP", "DROP_SELECT")

    # ─── TABLE ───
    if rule == "TABLE_PARENT":
        if child == "caption":
            return ("ACCEPT", "ACCEPT_CAPTION")
        if child == "colgroup":
            return ("ACCEPT", "ACCEPT_TABLE_CHILD")
        if child == "col":
            return ("WRAP_COLGROUP", "ACCEPT_COL")
        if child in {"thead", "tbody", "tfoot"}:
            return ("ACCEPT", "ACCEPT_TABLE_CHILD")
        if child == "tr":
            return ("WRAP_TBODY", "WRAP_TBODY")
        if child in {"td", "th"}:
            return ("WRAP_TBODY_TR", "WRAP_TBODY_TR")
        if child in {"script", "template", "style"}:
            return ("ACCEPT", "ACCEPT_HIDDEN")
        if child == "form":
            return ("DROP", "DROP_FORM")
        return ("FOSTER", "FOSTER")

    # ─── TABLE BODY (thead/tbody/tfoot) ───
    if rule == "TABLE_BODY_PARENT":
        if child == "tr":
            return ("ACCEPT", "ACCEPT_TR")
        if child in {"td", "th"}:
            return ("WRAP_TR", "WRAP_TR")
        if child in {"script", "template"}:
            return ("ACCEPT", "ACCEPT_HIDDEN")
        if child in BODY_CLOSING:
            return ("CLOSE_PARENT", "CLOSE_PARENT_TBODY")
        return ("FOSTER", "FOSTER")

    # ─── TABLE ROW ───
    if rule == "TABLE_ROW_PARENT":
        if child in {"td", "th"}:
            return ("ACCEPT", "ACCEPT_CELL")
        if child in {"script", "template"}:
            return ("ACCEPT", "ACCEPT_HIDDEN")
        if child == "tr":
            return ("CLOSE_PARENT", "CLOSE_PARENT_ROW")
        if child in BODY_CLOSING:
            return ("CLOSE_PARENT", "CLOSE_PARENT_ROW")
        return ("FOSTER", "FOSTER")

    # ─── TABLE CELL ───
    if rule == "TABLE_CELL_PARENT":
        if child in CELL_CLOSING:
            return ("CLOSE_PARENT", "CLOSE_PARENT_CELL")
        # Otherwise same as FLOW_PARENT
        return _flow_action(parent, child)

    # ─── CAPTION ───
    if rule == "CAPTION_PARENT":
        if child in {"caption", "colgroup", "col", "thead", "tbody", "tfoot", "tr", "td", "th"}:
            return ("CLOSE_PARENT", "CLOSE_PARENT_CAPTION")
        return _flow_action(parent, child)

    # ─── COLGROUP ───
    if rule == "COLGROUP_PARENT":
        if child == "col":
            return ("ACCEPT", "ACCEPT_COL")
        if child == "template":
            return ("ACCEPT", "ACCEPT_HIDDEN")
        return ("CLOSE_PARENT", "CLOSE_PARENT_COLGROUP")

    # ─── LIST PARENT (ul/ol/menu) ───
    if rule == "LIST_PARENT":
        if child == "li":
            return ("ACCEPT", "ACCEPT_LIST_ITEM")
        if child in {"script", "template"}:
            return ("ACCEPT", "ACCEPT_HIDDEN")
        # Parser is lenient — accepts other elements
        return ("ACCEPT", "ACCEPT_INLINE_IN_FLOW" if child in PHRASING else "ACCEPT_BLOCK_IN_FLOW")

    # ─── DL ───
    if rule == "DL_PARENT":
        if child == "dt":
            return ("ACCEPT", "ACCEPT_DT")
        if child == "dd":
            return ("ACCEPT", "ACCEPT_DD")
        if child == "div":
            return ("ACCEPT", "ACCEPT_BLOCK_IN_FLOW")
        if child in {"script", "template"}:
            return ("ACCEPT", "ACCEPT_HIDDEN")
        # Parser is lenient
        return ("ACCEPT", "ACCEPT_INLINE_IN_FLOW" if child in PHRASING else "ACCEPT_BLOCK_IN_FLOW")

    # ─── FLOW PARENT ───
    if rule == "FLOW_PARENT":
        # Special: form nesting
        if parent == "form" and child == "form":
            return ("DROP", "DROP_FORM")
        return _flow_action(parent, child)

    # ─── PHRASING BLOCK PARENT (p, h1-h6, pre) ───
    if rule == "PHRASING_BLOCK_PARENT":
        if child in P_CLOSING_SET:
            if parent in HEADINGS and child in HEADINGS:
                return ("CLOSE_PARENT", "CLOSE_PARENT_HEADING")
            return ("CLOSE_PARENT", "CLOSE_PARENT_P")
        if child in {"html", "head", "body"}:
            return ("DROP", "DROP")
        # Everything else is accepted (phrasing content or tolerated)
        return ("ACCEPT", "ACCEPT_PHRASING")

    # ─── INLINE PHRASING PARENT ───
    if rule == "INLINE_PHRASING_PARENT":
        if child in PHRASING:
            return ("ACCEPT", "ACCEPT_PHRASING")
        if child in {"html", "head", "body"}:
            return ("DROP", "DROP")
        # Block elements — parser accepts, layout splits
        return ("ACCEPT", "BLOCK_IN_INLINE")

    # ─── TRANSPARENT PARENT ───
    if rule == "TRANSPARENT_PARENT":
        # Special: a → a is adoption agency
        if parent == "a" and child == "a":
            return ("DROP", "DROP")
        if parent == "a" and child in INTERACTIVE:
            return ("ACCEPT", "ACCEPT")  # spec violation but parser accepts
        # Transparent inherits parent — we list as ACCEPT since it delegates
        return ("ACCEPT", "ACCEPT")

    # ─── BUTTON ───
    if rule == "BUTTON_PARENT":
        if child == "button":
            return ("CLOSE_REOPEN", "CLOSE_REOPEN_BUTTON")
        if child in PHRASING:
            return ("ACCEPT", "ACCEPT_PHRASING")
        if child in {"html", "head", "body"}:
            return ("DROP", "DROP")
        # Block elements — parser accepts, layout splits (button establishes BFC)
        return ("ACCEPT", "BLOCK_IN_INLINE")

    # ─── RUBY ───
    if rule == "RUBY_PARENT":
        if child == "rt":
            return ("ACCEPT", "ACCEPT_RUBY_TEXT")
        if child == "rp":
            return ("ACCEPT", "ACCEPT_RUBY_PAREN")
        if child in PHRASING:
            return ("ACCEPT", "ACCEPT_PHRASING")
        return ("ACCEPT", "ACCEPT")

    # ─── HGROUP ───
    if rule == "HGROUP_PARENT":
        if child in HEADINGS:
            return ("ACCEPT", "ACCEPT_HEADING_IN_HGROUP")
        if child == "p":
            return ("ACCEPT", "ACCEPT")
        if child in {"script", "template"}:
            return ("ACCEPT", "ACCEPT_HIDDEN")
        return ("ACCEPT", "ACCEPT")  # parser is lenient

    # ─── SVG ───
    if rule == "SVG_PARENT":
        if child in EXIT_FOREIGN:
            return ("CLOSE_PARENT", "CLOSE_PARENT_FOREIGN")
        return ("ACCEPT", "ACCEPT_FOREIGN")

    # ─── MATHML ───
    if rule == "MATHML_PARENT":
        if child in EXIT_FOREIGN:
            return ("CLOSE_PARENT", "CLOSE_PARENT_FOREIGN")
        return ("ACCEPT", "ACCEPT_FOREIGN")

    # ─── MEDIA (video/audio) ───
    if rule == "MEDIA_PARENT":
        if child == "source":
            return ("ACCEPT", "ACCEPT_SOURCE")
        if child == "track":
            return ("ACCEPT", "ACCEPT_TRACK")
        # Transparent fallback — accept anything
        return ("ACCEPT", "ACCEPT")

    # ─── PICTURE ───
    if rule == "PICTURE_PARENT":
        if child == "source":
            return ("ACCEPT", "ACCEPT_SOURCE")
        if child == "img":
            return ("ACCEPT", "ACCEPT_REPLACED")
        if child in {"script", "template"}:
            return ("ACCEPT", "ACCEPT_HIDDEN")
        return ("ACCEPT", "ACCEPT")  # parser lenient

    # ─── DATALIST ───
    if rule == "DATALIST_PARENT":
        if child == "option":
            return ("ACCEPT", "ACCEPT_OPTION")
        if child in PHRASING:
            return ("ACCEPT", "ACCEPT_PHRASING")
        return ("ACCEPT", "ACCEPT")

    return ("ACCEPT", "ACCEPT")


def _flow_action(parent, child):
    """Action for a flow-content parent."""
    if child in {"html", "head"}:
        return ("DROP", "DROP")
    if child == "body":
        return ("DROP", "DROP_BODY")
    if child in {"script", "style", "template", "noscript"}:
        return ("ACCEPT", "ACCEPT_HIDDEN")

    # Replaced/embedded elements
    if child in {"img", "iframe", "embed", "portal", "canvas", "picture", "object"}:
        return ("ACCEPT", "ACCEPT_REPLACED")
    if child in {"svg", "math"}:
        return ("ACCEPT", "ACCEPT_REPLACED")
    if child in {"video", "audio"}:
        return ("ACCEPT", "ACCEPT_REPLACED")

    # Void elements
    if child in VOID:
        if child == "hr":
            return ("ACCEPT", "ACCEPT_BLOCK_IN_FLOW")
        if child == "br":
            return ("ACCEPT", "ACCEPT_PHRASING")
        if child == "wbr":
            return ("ACCEPT", "ACCEPT_PHRASING")
        return ("ACCEPT", "ACCEPT_HIDDEN")  # meta, link, base, etc.

    # Table
    if child == "table":
        return ("ACCEPT", "ACCEPT_TABLE_IN_FLOW")

    # Form controls
    if child in {"input", "select", "textarea", "button", "meter", "progress", "datalist"}:
        return ("ACCEPT", "ACCEPT_REPLACED")

    # Lists
    if child in {"ul", "ol", "menu", "dl"}:
        return ("ACCEPT", "ACCEPT_BLOCK_IN_FLOW")

    # Headings
    if child in HEADINGS:
        return ("ACCEPT", "ACCEPT_BLOCK_IN_FLOW")

    # Block-level flow elements
    if child in {
        "div", "article", "section", "nav", "aside", "main", "search",
        "blockquote", "dialog", "figure", "figcaption", "details", "summary",
        "address", "hgroup", "header", "footer", "form", "fieldset", "p", "pre",
    }:
        return ("ACCEPT", "ACCEPT_BLOCK_IN_FLOW")

    # List items (orphaned but tolerated)
    if child in {"li", "dt", "dd"}:
        return ("ACCEPT", "ACCEPT_BLOCK_IN_FLOW")

    # Table parts (orphaned — render as block/inline outside table)
    if child in {"caption", "colgroup", "col", "thead", "tbody", "tfoot", "tr", "td", "th"}:
        return ("ACCEPT", "ACCEPT")

    # Inline/phrasing elements
    if child in PHRASING:
        return ("ACCEPT", "ACCEPT_INLINE_IN_FLOW")

    # Fallback
    return ("ACCEPT", "ACCEPT")


def generate():
    lines = []
    lines.append("# HTML Element Nesting Pairs — Full Exhaustive List")
    lines.append("")
    lines.append("Every individual parent → child pair on its own line, with UA default CSS.")
    lines.append(f"**{len(ALL_ELEMENTS)} elements × {len(ALL_ELEMENTS)} elements = {len(ALL_ELEMENTS)**2} pairs.**")
    lines.append("")
    lines.append("Format per entry:")
    lines.append("```")
    lines.append("N. `<parent> → <child>` = **ACTION** — behavior note")
    lines.append("   parent CSS: {ua defaults}  |  child CSS: {ua defaults}  |  context: {formatting context}")
    lines.append("```")
    lines.append("")

    # Stats
    action_counts = {}
    pair_num = 0

    for parent in ALL_ELEMENTS:
        rule = get_parent_rule(parent)
        p_css = css_str(parent)
        p_ctx = parent_context_str(parent)
        lines.append("---")
        lines.append("")
        lines.append(f"## `<{parent}>` (rule: {rule})")
        lines.append(f"UA default: `{p_css}`")
        lines.append(f"Children context: {p_ctx}")
        lines.append("")

        for child in ALL_ELEMENTS:
            pair_num += 1
            action, behavior = get_action(parent, child)
            action_counts[action] = action_counts.get(action, 0) + 1
            desc = BEHAVIOR.get(behavior, behavior)
            c_css = css_str(child)
            lines.append(f"{pair_num}. `<{parent}> → <{child}>` = **{action}** — {desc}")
            lines.append(f"   parent CSS: `{p_css}` | child CSS: `{c_css}` | context: {p_ctx}")

        lines.append("")

    # Summary
    lines.append("---")
    lines.append("")
    lines.append("## Summary")
    lines.append("")
    lines.append(f"**Total pairs listed:** {pair_num}")
    lines.append("")
    lines.append("| Action | Count | % |")
    lines.append("|--------|-------|---|")
    total = sum(action_counts.values())
    for action, count in sorted(action_counts.items(), key=lambda x: -x[1]):
        pct = count / total * 100
        lines.append(f"| {action} | {count} | {pct:.1f}% |")
    lines.append(f"| **Total** | **{total}** | **100%** |")
    lines.append("")

    return "\n".join(lines)


if __name__ == "__main__":
    output = generate()
    with open("/home/user/Browser/catalog/nesting_pairs_full.md", "w") as f:
        f.write(output)
    print(f"Generated {output.count(chr(10))+1} lines")
    print(f"Pairs: {len(ALL_ELEMENTS) * len(ALL_ELEMENTS)}")

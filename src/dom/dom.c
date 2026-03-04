/*
 * Pane — DOM Implementation
 */

#include "dom.h"
#include <string.h>
#include <stdlib.h>

/* ── Tag name table ─────────────────────────────────────────────────── */

static const char *tag_names[TAG__COUNT] = {
    [TAG_UNKNOWN]    = "",
    [TAG_HTML]       = "html",     [TAG_HEAD]       = "head",
    [TAG_BODY]       = "body",     [TAG_TITLE]      = "title",
    [TAG_BASE]       = "base",     [TAG_LINK]       = "link",
    [TAG_META]       = "meta",     [TAG_STYLE]      = "style",
    [TAG_SCRIPT]     = "script",   [TAG_NOSCRIPT]   = "noscript",
    [TAG_ARTICLE]    = "article",  [TAG_SECTION]    = "section",
    [TAG_NAV]        = "nav",      [TAG_ASIDE]      = "aside",
    [TAG_H1]         = "h1",       [TAG_H2]         = "h2",
    [TAG_H3]         = "h3",       [TAG_H4]         = "h4",
    [TAG_H5]         = "h5",       [TAG_H6]         = "h6",
    [TAG_HGROUP]     = "hgroup",   [TAG_HEADER]     = "header",
    [TAG_FOOTER]     = "footer",   [TAG_ADDRESS]    = "address",
    [TAG_MAIN]       = "main",     [TAG_SEARCH]     = "search",
    [TAG_P]          = "p",        [TAG_HR]         = "hr",
    [TAG_PRE]        = "pre",      [TAG_BLOCKQUOTE] = "blockquote",
    [TAG_OL]         = "ol",       [TAG_UL]         = "ul",
    [TAG_MENU]       = "menu",     [TAG_LI]         = "li",
    [TAG_DL]         = "dl",       [TAG_DT]         = "dt",
    [TAG_DD]         = "dd",       [TAG_FIGURE]     = "figure",
    [TAG_FIGCAPTION] = "figcaption", [TAG_DIV]      = "div",
    [TAG_A]          = "a",        [TAG_EM]         = "em",
    [TAG_STRONG]     = "strong",   [TAG_SMALL]      = "small",
    [TAG_S]          = "s",        [TAG_CITE]       = "cite",
    [TAG_Q]          = "q",        [TAG_DFN]        = "dfn",
    [TAG_ABBR]       = "abbr",     [TAG_RUBY]       = "ruby",
    [TAG_RT]         = "rt",       [TAG_RP]         = "rp",
    [TAG_RB]         = "rb",       [TAG_RTC]        = "rtc",
    [TAG_DATA]       = "data",     [TAG_TIME]       = "time",
    [TAG_CODE]       = "code",     [TAG_VAR]        = "var",
    [TAG_SAMP]       = "samp",     [TAG_KBD]        = "kbd",
    [TAG_SUB]        = "sub",      [TAG_SUP]        = "sup",
    [TAG_I]          = "i",        [TAG_B]          = "b",
    [TAG_U]          = "u",        [TAG_MARK]       = "mark",
    [TAG_BDI]        = "bdi",      [TAG_BDO]        = "bdo",
    [TAG_SPAN]       = "span",     [TAG_BR]         = "br",
    [TAG_WBR]        = "wbr",      [TAG_INS]        = "ins",
    [TAG_DEL]        = "del",      [TAG_PICTURE]    = "picture",
    [TAG_SOURCE]     = "source",   [TAG_IMG]        = "img",
    [TAG_IFRAME]     = "iframe",   [TAG_EMBED]      = "embed",
    [TAG_OBJECT]     = "object",   [TAG_PARAM]      = "param",
    [TAG_VIDEO]      = "video",    [TAG_AUDIO]      = "audio",
    [TAG_TRACK]      = "track",    [TAG_MAP]        = "map",
    [TAG_AREA]       = "area",     [TAG_PORTAL]     = "portal",
    [TAG_SVG]        = "svg",      [TAG_MATH]       = "math",
    [TAG_TABLE]      = "table",    [TAG_CAPTION]    = "caption",
    [TAG_COLGROUP]   = "colgroup", [TAG_COL]        = "col",
    [TAG_THEAD]      = "thead",    [TAG_TBODY]      = "tbody",
    [TAG_TFOOT]      = "tfoot",    [TAG_TR]         = "tr",
    [TAG_TD]         = "td",       [TAG_TH]         = "th",
    [TAG_FORM]       = "form",     [TAG_LABEL]      = "label",
    [TAG_INPUT]      = "input",    [TAG_BUTTON]     = "button",
    [TAG_SELECT]     = "select",   [TAG_DATALIST]   = "datalist",
    [TAG_OPTGROUP]   = "optgroup", [TAG_OPTION]     = "option",
    [TAG_TEXTAREA]   = "textarea", [TAG_OUTPUT]     = "output",
    [TAG_PROGRESS]   = "progress", [TAG_METER]      = "meter",
    [TAG_FIELDSET]   = "fieldset", [TAG_LEGEND]     = "legend",
    [TAG_DETAILS]    = "details",  [TAG_SUMMARY]    = "summary",
    [TAG_DIALOG]     = "dialog",   [TAG_SLOT]       = "slot",
    [TAG_TEMPLATE]   = "template", [TAG_CANVAS]     = "canvas",
    [TAG_APPLET]     = "applet",   [TAG_MARQUEE]    = "marquee",
    [TAG_CENTER]     = "center",   [TAG_FONT]       = "font",
    [TAG_BIG]        = "big",      [TAG_STRIKE]     = "strike",
    [TAG_TT]         = "tt",       [TAG_NOBR]       = "nobr",
    [TAG_DIR]        = "dir",      [TAG_LISTING]    = "listing",
    [TAG_NOFRAMES]   = "noframes", [TAG_NOEMBED]    = "noembed",
    [TAG_FRAMESET]   = "frameset", [TAG_FRAME]      = "frame",
};

HtmlTag html_tag_from_name(const char *name, size_t len)
{
    /* Linear scan is fine for ~130 tags — called once per element during parse.
     * Can be replaced with a perfect hash if profiling shows it matters. */
    for (int i = 1; i < TAG__COUNT; i++) {
        const char *t = tag_names[i];
        if (strlen(t) == len && memcmp(t, name, len) == 0)
            return (HtmlTag)i;
    }
    return TAG_UNKNOWN;
}

const char *html_tag_to_name(HtmlTag tag)
{
    if (tag >= 0 && tag < TAG__COUNT)
        return tag_names[tag];
    return "";
}

/* ── Document ───────────────────────────────────────────────────────── */

Document *doc_create(void)
{
    Document *doc = calloc(1, sizeof(Document));
    arena_init(&doc->arena, 0);
    strintern_init(&doc->strings);

    doc->root = arena_calloc(&doc->arena, 1, sizeof(DomNode));
    doc->root->type = PANE_NODE_DOCUMENT;
    return doc;
}

void doc_destroy(Document *doc)
{
    if (!doc) return;
    strintern_destroy(&doc->strings);
    arena_destroy(&doc->arena);
    free(doc);
}

/* ── Node Creation ──────────────────────────────────────────────────── */

DomNode *doc_create_element(Document *doc, HtmlTag tag, const char *tag_name)
{
    DomNode *n = arena_calloc(&doc->arena, 1, sizeof(DomNode));
    n->type = PANE_NODE_ELEMENT;
    n->elem.tag = tag;
    n->elem.tag_name = strintern_cstr(&doc->strings,
                                       tag_name ? tag_name : html_tag_to_name(tag));
    return n;
}

DomNode *doc_create_text(Document *doc, const char *data, size_t len)
{
    DomNode *n = arena_calloc(&doc->arena, 1, sizeof(DomNode));
    n->type = PANE_NODE_TEXT;
    n->text.data = arena_strndup(&doc->arena, data, len);
    n->text.len = len;
    return n;
}

DomNode *doc_create_comment(Document *doc, const char *data, size_t len)
{
    DomNode *n = arena_calloc(&doc->arena, 1, sizeof(DomNode));
    n->type = PANE_NODE_COMMENT;
    n->text.data = arena_strndup(&doc->arena, data, len);
    n->text.len = len;
    return n;
}

DomNode *doc_create_doctype(Document *doc, const char *name)
{
    DomNode *n = arena_calloc(&doc->arena, 1, sizeof(DomNode));
    n->type = PANE_NODE_DOCTYPE;
    n->doctype.name = arena_strdup(&doc->arena, name ? name : "html");
    return n;
}

/* ── Tree Manipulation ──────────────────────────────────────────────── */

void dom_append_child(DomNode *parent, DomNode *child)
{
    child->parent = parent;
    child->next_sibling = NULL;
    child->prev_sibling = parent->last_child;

    if (parent->last_child)
        parent->last_child->next_sibling = child;
    else
        parent->first_child = child;

    parent->last_child = child;
    parent->child_count++;
}

void dom_insert_before(DomNode *parent, DomNode *child, DomNode *ref)
{
    if (!ref) {
        dom_append_child(parent, child);
        return;
    }

    child->parent = parent;
    child->next_sibling = ref;
    child->prev_sibling = ref->prev_sibling;

    if (ref->prev_sibling)
        ref->prev_sibling->next_sibling = child;
    else
        parent->first_child = child;

    ref->prev_sibling = child;
    parent->child_count++;
}

void dom_remove_child(DomNode *parent, DomNode *child)
{
    if (child->prev_sibling)
        child->prev_sibling->next_sibling = child->next_sibling;
    else
        parent->first_child = child->next_sibling;

    if (child->next_sibling)
        child->next_sibling->prev_sibling = child->prev_sibling;
    else
        parent->last_child = child->prev_sibling;

    child->parent = NULL;
    child->next_sibling = NULL;
    child->prev_sibling = NULL;
    parent->child_count--;
}

/* ── Attributes ─────────────────────────────────────────────────────── */

void elem_set_attr(Document *doc, DomNode *elem,
                   const char *name, const char *value)
{
    if (elem->type != PANE_NODE_ELEMENT) return;

    const char *iname = strintern_cstr(&doc->strings, name);
    const char *ival  = arena_strdup(&doc->arena, value);

    /* Update existing. */
    for (uint16_t i = 0; i < elem->elem.attr_count; i++) {
        if (elem->elem.attrs[i].name == iname) {
            elem->elem.attrs[i].value = ival;
            goto update_shortcuts;
        }
    }

    /* Grow if needed. */
    if (elem->elem.attr_count >= elem->elem.attr_cap) {
        uint16_t new_cap = elem->elem.attr_cap ? elem->elem.attr_cap * 2 : 4;
        Attr *new_attrs = arena_alloc(&doc->arena, new_cap * sizeof(Attr), 8);
        if (elem->elem.attr_count > 0)
            memcpy(new_attrs, elem->elem.attrs,
                   elem->elem.attr_count * sizeof(Attr));
        elem->elem.attrs = new_attrs;
        elem->elem.attr_cap = new_cap;
    }

    elem->elem.attrs[elem->elem.attr_count++] = (Attr){ iname, ival };

update_shortcuts:
    if (strcmp(name, "id") == 0) elem->elem.id = ival;
    if (strcmp(name, "class") == 0) elem->elem.class_str = ival;
}

const char *elem_get_attr(const DomNode *elem, const char *name)
{
    if (elem->type != PANE_NODE_ELEMENT) return NULL;
    for (uint16_t i = 0; i < elem->elem.attr_count; i++) {
        if (strcmp(elem->elem.attrs[i].name, name) == 0)
            return elem->elem.attrs[i].value;
    }
    return NULL;
}

bool elem_has_attr(const DomNode *elem, const char *name)
{
    return elem_get_attr(elem, name) != NULL;
}

/* ── Tree Walking ───────────────────────────────────────────────────── */

DomNode *dom_next_in_tree(const DomNode *node, const DomNode *root)
{
    if (node->first_child)
        return node->first_child;
    while (node != root) {
        if (node->next_sibling)
            return node->next_sibling;
        node = node->parent;
    }
    return NULL;
}

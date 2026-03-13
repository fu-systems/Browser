/*
 * Pane — DOM Tree
 *
 * Minimal DOM representation. Nodes form a tree via first_child/next_sibling
 * pointers (child list is a linked list). All nodes are arena-allocated.
 */

#ifndef PANE_DOM_H
#define PANE_DOM_H

#include "../util/arena.h"
#include "../util/str.h"
#include "../util/vec.h"
#include <stdint.h>

/* ── Node Types ─────────────────────────────────────────────────────── */

typedef enum {
    PANE_NODE_DOCUMENT,
    PANE_NODE_DOCTYPE,
    PANE_NODE_ELEMENT,
    PANE_NODE_TEXT,
    PANE_NODE_COMMENT,
} NodeType;

/* ── HTML Tag IDs ───────────────────────────────────────────────────── */
/* Interned tag names for fast comparison (matched by pointer equality). */

typedef enum {
    TAG_UNKNOWN = 0,
    /* Root */
    TAG_HTML, TAG_HEAD, TAG_BODY,
    /* Metadata */
    TAG_TITLE, TAG_BASE, TAG_LINK, TAG_META, TAG_STYLE, TAG_SCRIPT, TAG_NOSCRIPT,
    /* Sections */
    TAG_ARTICLE, TAG_SECTION, TAG_NAV, TAG_ASIDE, TAG_H1, TAG_H2, TAG_H3,
    TAG_H4, TAG_H5, TAG_H6, TAG_HGROUP, TAG_HEADER, TAG_FOOTER, TAG_ADDRESS,
    TAG_MAIN, TAG_SEARCH,
    /* Grouping */
    TAG_P, TAG_HR, TAG_PRE, TAG_BLOCKQUOTE, TAG_OL, TAG_UL, TAG_MENU, TAG_LI,
    TAG_DL, TAG_DT, TAG_DD, TAG_FIGURE, TAG_FIGCAPTION, TAG_DIV,
    /* Text-level */
    TAG_A, TAG_EM, TAG_STRONG, TAG_SMALL, TAG_S, TAG_CITE, TAG_Q, TAG_DFN,
    TAG_ABBR, TAG_RUBY, TAG_RT, TAG_RP, TAG_RB, TAG_RTC,
    TAG_DATA, TAG_TIME, TAG_CODE, TAG_VAR, TAG_SAMP, TAG_KBD,
    TAG_SUB, TAG_SUP, TAG_I, TAG_B, TAG_U, TAG_MARK, TAG_BDI, TAG_BDO,
    TAG_SPAN, TAG_BR, TAG_WBR,
    /* Edits */
    TAG_INS, TAG_DEL,
    /* Embedded */
    TAG_PICTURE, TAG_SOURCE, TAG_IMG, TAG_IFRAME, TAG_EMBED, TAG_OBJECT,
    TAG_PARAM, TAG_VIDEO, TAG_AUDIO, TAG_TRACK, TAG_MAP, TAG_AREA, TAG_PORTAL,
    /* SVG/MathML */
    TAG_SVG, TAG_MATH,
    /* Table */
    TAG_TABLE, TAG_CAPTION, TAG_COLGROUP, TAG_COL, TAG_THEAD, TAG_TBODY,
    TAG_TFOOT, TAG_TR, TAG_TD, TAG_TH,
    /* Forms */
    TAG_FORM, TAG_LABEL, TAG_INPUT, TAG_BUTTON, TAG_SELECT, TAG_DATALIST,
    TAG_OPTGROUP, TAG_OPTION, TAG_TEXTAREA, TAG_OUTPUT, TAG_PROGRESS,
    TAG_METER, TAG_FIELDSET, TAG_LEGEND,
    /* Interactive */
    TAG_DETAILS, TAG_SUMMARY, TAG_DIALOG, TAG_SLOT,
    /* Scripting */
    TAG_TEMPLATE, TAG_CANVAS,
    /* Obsolete (still parsed) */
    TAG_APPLET, TAG_MARQUEE, TAG_CENTER, TAG_FONT, TAG_BIG, TAG_STRIKE,
    TAG_TT, TAG_NOBR, TAG_DIR, TAG_LISTING, TAG_NOFRAMES, TAG_NOEMBED,
    TAG_FRAMESET, TAG_FRAME,
    TAG__COUNT
} HtmlTag;

/* ── Attribute ──────────────────────────────────────────────────────── */

typedef struct {
    const char *name;   /* interned */
    const char *value;  /* arena-allocated */
} Attr;

/* ── DOM Node ───────────────────────────────────────────────────────── */

typedef struct DomNode DomNode;
struct DomNode {
    NodeType     type;
    DomNode     *parent;
    DomNode     *first_child;
    DomNode     *last_child;
    DomNode     *next_sibling;
    DomNode     *prev_sibling;
    uint32_t     child_count;

    union {
        struct {               /* PANE_NODE_ELEMENT */
            HtmlTag      tag;
            const char  *tag_name;   /* interned, lowercase */
            Attr        *attrs;
            uint16_t     attr_count;
            uint16_t     attr_cap;
            const char  *id;         /* shortcut to id attr value */
            const char  *class_str;  /* shortcut to class attr value */
        } elem;
        struct {               /* PANE_NODE_TEXT / PANE_NODE_COMMENT */
            char   *data;
            size_t  len;
        } text;
        struct {               /* PANE_NODE_DOCTYPE */
            const char *name;
        } doctype;
    };

    /* Style/layout data attached later. */
    void *computed_style;
    void *layout_box;
};

/* ── Document ───────────────────────────────────────────────────────── */

typedef struct {
    Arena      arena;
    StrIntern  strings;
    DomNode   *root;          /* PANE_NODE_DOCUMENT node */
    DomNode   *html;          /* <html> element */
    DomNode   *head;          /* <head> element */
    DomNode   *body;          /* <body> element */
} Document;

/* ── API ────────────────────────────────────────────────────────────── */

/* Create a new empty document. */
Document *doc_create(void);

/* Destroy a document and all its nodes. */
void doc_destroy(Document *doc);

/* Create nodes (all arena-allocated in the document). */
DomNode *doc_create_element(Document *doc, HtmlTag tag, const char *tag_name);
DomNode *doc_create_text(Document *doc, const char *data, size_t len);
DomNode *doc_create_comment(Document *doc, const char *data, size_t len);
DomNode *doc_create_doctype(Document *doc, const char *name);

/* Tree manipulation. */
void dom_append_child(DomNode *parent, DomNode *child);
void dom_insert_before(DomNode *parent, DomNode *child, DomNode *ref);
void dom_remove_child(DomNode *parent, DomNode *child);

/* Attribute manipulation. */
void elem_set_attr(Document *doc, DomNode *elem,
                   const char *name, const char *value);
const char *elem_get_attr(const DomNode *elem, const char *name);
bool elem_has_attr(const DomNode *elem, const char *name);

/* Tag name lookup. */
HtmlTag html_tag_from_name(const char *name, size_t len);
const char *html_tag_to_name(HtmlTag tag);

/* Tree walking. */
DomNode *dom_next_in_tree(const DomNode *node, const DomNode *root);

#endif /* PANE_DOM_H */

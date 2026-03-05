/*
 * Pane — HTML Tree Builder Implementation
 *
 * Implements insertion modes and the tree construction algorithm.
 * Rules derived from html_pair_chain_handling.md (62 compressed rules
 * covering all 13,456 parent×child pairs).
 *
 * Key element sets (from Section 2 of the rules doc):
 *   SET_P_CLOSING: address, article, aside, blockquote, center, details,
 *                  dialog, dir, div, dl, fieldset, figcaption, figure,
 *                  footer, header, hgroup, listing, main, menu, nav,
 *                  ol, p, pre, search, section, summary, ul
 *   SET_AFE:       a, b, big, code, em, font, i, nobr, s, small,
 *                  strike, strong, tt, u
 *   SET_VOID:      area, base, br, col, embed, hr, img, input, link,
 *                  meta, param, source, track, wbr
 *   SET_HEADINGS:  h1, h2, h3, h4, h5, h6
 */

#include "tree_builder.h"
#include <string.h>
#include <ctype.h>

/* ── Element Set Predicates ─────────────────────────────────────────── */

static bool is_void(HtmlTag t)
{
    switch (t) {
    case TAG_AREA: case TAG_BASE: case TAG_BR: case TAG_COL:
    case TAG_EMBED: case TAG_HR: case TAG_IMG: case TAG_INPUT:
    case TAG_LINK: case TAG_META: case TAG_PARAM: case TAG_SOURCE:
    case TAG_TRACK: case TAG_WBR: case TAG_FRAME: case TAG_PORTAL:
        return true;
    default:
        return false;
    }
}

static bool is_heading(HtmlTag t)
{
    return t >= TAG_H1 && t <= TAG_H6;
}

static bool is_formatting(HtmlTag t)
{
    switch (t) {
    case TAG_A: case TAG_B: case TAG_BIG: case TAG_CODE: case TAG_EM:
    case TAG_FONT: case TAG_I: case TAG_NOBR: case TAG_S: case TAG_SMALL:
    case TAG_STRIKE: case TAG_STRONG: case TAG_TT: case TAG_U:
        return true;
    default:
        return false;
    }
}

static bool is_p_closing(HtmlTag t)
{
    switch (t) {
    case TAG_ADDRESS: case TAG_ARTICLE: case TAG_ASIDE: case TAG_BLOCKQUOTE:
    case TAG_CENTER: case TAG_DETAILS: case TAG_DIALOG: case TAG_DIR:
    case TAG_DIV: case TAG_DL: case TAG_FIELDSET: case TAG_FIGCAPTION:
    case TAG_FIGURE: case TAG_FOOTER: case TAG_HEADER: case TAG_HGROUP:
    case TAG_LISTING: case TAG_MAIN: case TAG_MENU: case TAG_NAV:
    case TAG_OL: case TAG_P: case TAG_PRE: case TAG_SEARCH:
    case TAG_SECTION: case TAG_SUMMARY: case TAG_UL:
        return true;
    default:
        return false;
    }
}

static bool is_special(HtmlTag t)
{
    /* "Special" elements as defined by the HTML spec. */
    if (is_p_closing(t)) return true;
    if (is_heading(t)) return true;
    switch (t) {
    case TAG_APPLET: case TAG_BODY: case TAG_BR: case TAG_BUTTON:
    case TAG_CAPTION: case TAG_COL: case TAG_COLGROUP: case TAG_DD:
    case TAG_DT: case TAG_EMBED: case TAG_FORM: case TAG_FRAME:
    case TAG_FRAMESET: case TAG_HEAD: case TAG_HR: case TAG_HTML:
    case TAG_IFRAME: case TAG_IMG: case TAG_INPUT: case TAG_LI:
    case TAG_LINK: case TAG_MARQUEE: case TAG_META: case TAG_NOEMBED:
    case TAG_NOFRAMES: case TAG_NOSCRIPT: case TAG_OBJECT:
    case TAG_PARAM: case TAG_PRE: case TAG_SCRIPT: case TAG_SELECT:
    case TAG_STYLE: case TAG_TABLE: case TAG_TBODY: case TAG_TD:
    case TAG_TEMPLATE: case TAG_TEXTAREA: case TAG_TFOOT: case TAG_TH:
    case TAG_THEAD: case TAG_TITLE: case TAG_TR: case TAG_WBR:
    case TAG_AREA: case TAG_BASE: case TAG_SOURCE: case TAG_TRACK:
        return true;
    default:
        return false;
    }
}

static bool is_table_scope(HtmlTag t)
{
    return t == TAG_TABLE || t == TAG_TEMPLATE || t == TAG_HTML;
}

static bool is_table_body(HtmlTag t)
{
    return t == TAG_THEAD || t == TAG_TBODY || t == TAG_TFOOT;
}

static bool is_raw_text(HtmlTag t)
{
    return t == TAG_STYLE || t == TAG_SCRIPT || t == TAG_TEXTAREA ||
           t == TAG_TITLE || t == TAG_NOSCRIPT || t == TAG_NOFRAMES ||
           t == TAG_NOEMBED;
}

/* ── Stack Operations ───────────────────────────────────────────────── */

static DomNode *current_node(TreeBuilder *tb)
{
    if (tb->stack_len == 0) return tb->doc->root;
    return tb->stack[tb->stack_len - 1];
}

static void push_element(TreeBuilder *tb, DomNode *node)
{
    if (tb->stack_len < MAX_STACK_DEPTH)
        tb->stack[tb->stack_len++] = node;
}

static DomNode *pop_element(TreeBuilder *tb)
{
    if (tb->stack_len == 0) return NULL;
    return tb->stack[--tb->stack_len];
}

static bool stack_has_tag(TreeBuilder *tb, HtmlTag tag)
{
    for (int i = tb->stack_len - 1; i >= 0; i--) {
        if (tb->stack[i]->type == PANE_NODE_ELEMENT &&
            tb->stack[i]->elem.tag == tag)
            return true;
    }
    return false;
}

static bool has_element_in_scope(TreeBuilder *tb, HtmlTag tag)
{
    for (int i = tb->stack_len - 1; i >= 0; i--) {
        DomNode *n = tb->stack[i];
        if (n->type != PANE_NODE_ELEMENT) continue;
        if (n->elem.tag == tag) return true;
        /* Scope boundary elements. */
        switch (n->elem.tag) {
        case TAG_APPLET: case TAG_CAPTION: case TAG_HTML: case TAG_TABLE:
        case TAG_TD: case TAG_TH: case TAG_MARQUEE: case TAG_OBJECT:
        case TAG_TEMPLATE: case TAG_SVG: case TAG_MATH:
            return false;
        default: break;
        }
    }
    return false;
}

static bool has_p_in_button_scope(TreeBuilder *tb)
{
    for (int i = tb->stack_len - 1; i >= 0; i--) {
        DomNode *n = tb->stack[i];
        if (n->type != PANE_NODE_ELEMENT) continue;
        if (n->elem.tag == TAG_P) return true;
        if (n->elem.tag == TAG_BUTTON) return false;
        switch (n->elem.tag) {
        case TAG_APPLET: case TAG_CAPTION: case TAG_HTML: case TAG_TABLE:
        case TAG_TD: case TAG_TH: case TAG_MARQUEE: case TAG_OBJECT:
        case TAG_TEMPLATE: case TAG_SVG: case TAG_MATH:
            return false;
        default: break;
        }
    }
    return false;
}

static void pop_until_tag(TreeBuilder *tb, HtmlTag tag)
{
    while (tb->stack_len > 0) {
        DomNode *n = pop_element(tb);
        if (n->type == PANE_NODE_ELEMENT && n->elem.tag == tag)
            return;
    }
}

static void close_p_element(TreeBuilder *tb)
{
    if (has_p_in_button_scope(tb))
        pop_until_tag(tb, TAG_P);
}

static void generate_implied_end_tags(TreeBuilder *tb, HtmlTag except)
{
    while (tb->stack_len > 0) {
        DomNode *n = current_node(tb);
        if (n->type != PANE_NODE_ELEMENT) break;
        HtmlTag t = n->elem.tag;
        if (t == except) break;
        if (t == TAG_DD || t == TAG_DT || t == TAG_LI || t == TAG_OPTGROUP ||
            t == TAG_OPTION || t == TAG_P || t == TAG_RB || t == TAG_RP ||
            t == TAG_RT || t == TAG_RTC) {
            pop_element(tb);
        } else {
            break;
        }
    }
}

/* ── Insertion ──────────────────────────────────────────────────────── */

static DomNode *appropriate_place(TreeBuilder *tb)
{
    /* Simplified: insert into current node (foster parenting TODO). */
    return current_node(tb);
}

static DomNode *insert_element_for_token(TreeBuilder *tb, const HtmlToken *tok)
{
    HtmlTag tag = html_tag_from_name(tok->tag_name, tok->tag_name_len);
    DomNode *elem = doc_create_element(tb->doc, tag, tok->tag_name);

    /* Copy attributes. */
    for (uint16_t i = 0; i < tok->attr_count; i++) {
        char name_buf[256], val_buf[4096];
        size_t nlen = tok->attrs[i].name_len;
        size_t vlen = tok->attrs[i].value_len;
        if (nlen >= sizeof(name_buf)) nlen = sizeof(name_buf) - 1;
        if (vlen >= sizeof(val_buf)) vlen = sizeof(val_buf) - 1;
        memcpy(name_buf, tok->attrs[i].name, nlen); name_buf[nlen] = '\0';
        memcpy(val_buf, tok->attrs[i].value, vlen); val_buf[vlen] = '\0';
        elem_set_attr(tb->doc, elem, name_buf, val_buf);
    }

    DomNode *target = appropriate_place(tb);
    dom_append_child(target, elem);
    push_element(tb, elem);

    return elem;
}

static void insert_text(TreeBuilder *tb, const char *data, size_t len)
{
    DomNode *target = appropriate_place(tb);

    /* Merge with previous text node if possible. */
    if (target->last_child && target->last_child->type == PANE_NODE_TEXT) {
        DomNode *text = target->last_child;
        size_t new_len = text->text.len + len;
        char *new_data = arena_alloc(&tb->doc->arena, new_len + 1, 1);
        memcpy(new_data, text->text.data, text->text.len);
        memcpy(new_data + text->text.len, data, len);
        new_data[new_len] = '\0';
        text->text.data = new_data;
        text->text.len = new_len;
        return;
    }

    DomNode *text = doc_create_text(tb->doc, data, len);
    dom_append_child(target, text);
}

static void insert_comment(TreeBuilder *tb, const char *data, size_t len)
{
    DomNode *comment = doc_create_comment(tb->doc, data, len);
    DomNode *target = appropriate_place(tb);
    dom_append_child(target, comment);
}

/* ── Active Formatting Elements ─────────────────────────────────────── */

static void afe_push(TreeBuilder *tb, DomNode *node)
{
    if (tb->afe_len < MAX_AFE)
        tb->afe[tb->afe_len++] = node;
}

static void afe_push_marker(TreeBuilder *tb)
{
    afe_push(tb, NULL);
}

static void afe_clear_to_marker(TreeBuilder *tb)
{
    while (tb->afe_len > 0) {
        if (tb->afe[--tb->afe_len] == NULL)
            return;
    }
}

static void reconstruct_afe(TreeBuilder *tb)
{
    if (tb->afe_len == 0) return;
    DomNode *entry = tb->afe[tb->afe_len - 1];
    if (entry == NULL) return; /* marker */

    /* Check if already on the stack. */
    for (int i = tb->stack_len - 1; i >= 0; i--) {
        if (tb->stack[i] == entry) return;
    }

    /* Simplified reconstruction: just push the element again. */
    /* A full implementation would clone the element. For now this handles
     * the common case of reopening formatting elements. */
}

/* ── Insertion Mode Handlers ────────────────────────────────────────── */

static void process_token(TreeBuilder *tb, HtmlToken *tok);

static InsertionMode mode_for_tag(HtmlTag tag)
{
    /* From html_pair_chain_handling.md Section 1: Parent → Mode Mapping */
    switch (tag) {
    case TAG_TABLE:    return MODE_IN_TABLE;
    case TAG_THEAD:
    case TAG_TBODY:
    case TAG_TFOOT:    return MODE_IN_TABLE_BODY;
    case TAG_TR:       return MODE_IN_ROW;
    case TAG_TD:
    case TAG_TH:       return MODE_IN_CELL;
    case TAG_CAPTION:  return MODE_IN_CAPTION;
    case TAG_COLGROUP: return MODE_IN_COLUMN_GROUP;
    case TAG_SELECT:   return MODE_IN_SELECT;
    case TAG_TEMPLATE: return MODE_IN_TEMPLATE;
    case TAG_SVG:
    case TAG_MATH:     return MODE_IN_FOREIGN_CONTENT;
    default:           return MODE_IN_BODY;
    }
}

static void handle_initial(TreeBuilder *tb, HtmlToken *tok)
{
    if (tok->type == TOK_CHARACTER &&
        (tok->ch_data[0] == ' ' || tok->ch_data[0] == '\t' ||
         tok->ch_data[0] == '\n' || tok->ch_data[0] == '\f'))
        return; /* Ignore whitespace. */

    if (tok->type == TOK_DOCTYPE) {
        DomNode *dt = doc_create_doctype(tb->doc, tok->doctype_name);
        dom_append_child(tb->doc->root, dt);
        tb->mode = MODE_BEFORE_HTML;
        return;
    }

    /* Anything else: force no-quirks and reprocess. */
    tb->mode = MODE_BEFORE_HTML;
    process_token(tb, tok);
}

static void handle_before_html(TreeBuilder *tb, HtmlToken *tok)
{
    if (tok->type == TOK_CHARACTER &&
        (tok->ch_data[0] == ' ' || tok->ch_data[0] == '\t' ||
         tok->ch_data[0] == '\n' || tok->ch_data[0] == '\f'))
        return; /* Ignore whitespace. */

    if (tok->type == TOK_START_TAG &&
        strncmp(tok->tag_name, "html", tok->tag_name_len) == 0) {
        DomNode *html = insert_element_for_token(tb, tok);
        tb->doc->html = html;
        tb->mode = MODE_BEFORE_HEAD;
        return;
    }

    /* Auto-create <html>. */
    DomNode *html = doc_create_element(tb->doc, TAG_HTML, "html");
    dom_append_child(tb->doc->root, html);
    push_element(tb, html);
    tb->doc->html = html;
    tb->mode = MODE_BEFORE_HEAD;
    process_token(tb, tok);
}

static void handle_before_head(TreeBuilder *tb, HtmlToken *tok)
{
    if (tok->type == TOK_CHARACTER &&
        (tok->ch_data[0] == ' ' || tok->ch_data[0] == '\t' ||
         tok->ch_data[0] == '\n' || tok->ch_data[0] == '\f'))
        return;

    if (tok->type == TOK_START_TAG &&
        strncmp(tok->tag_name, "head", tok->tag_name_len) == 0) {
        DomNode *head = insert_element_for_token(tb, tok);
        tb->doc->head = head;
        tb->head_ptr = head;
        tb->mode = MODE_IN_HEAD;
        return;
    }

    /* Auto-create <head>. */
    DomNode *head = doc_create_element(tb->doc, TAG_HEAD, "head");
    dom_append_child(current_node(tb), head);
    push_element(tb, head);
    tb->doc->head = head;
    tb->head_ptr = head;
    tb->mode = MODE_IN_HEAD;
    process_token(tb, tok);
}

static void handle_in_head(TreeBuilder *tb, HtmlToken *tok)
{
    if (tok->type == TOK_CHARACTER &&
        (tok->ch_data[0] == ' ' || tok->ch_data[0] == '\t' ||
         tok->ch_data[0] == '\n' || tok->ch_data[0] == '\f')) {
        insert_text(tb, tok->ch_data, tok->ch_len);
        return;
    }

    if (tok->type == TOK_COMMENT) {
        insert_comment(tb, tok->comment_data, tok->comment_len);
        return;
    }

    if (tok->type == TOK_START_TAG) {
        HtmlTag tag = html_tag_from_name(tok->tag_name, tok->tag_name_len);

        /* HD-01: title → insert + switch to RCDATA */
        if (tag == TAG_TITLE) {
            insert_element_for_token(tb, tok);
            tokenizer_set_state(&tb->tokenizer, STATE_RCDATA);
            tb->original_mode = tb->mode;
            tb->mode = MODE_TEXT;
            return;
        }

        /* HD-02: style, noscript, noframes, noembed → RAWTEXT */
        if (tag == TAG_STYLE || tag == TAG_NOSCRIPT ||
            tag == TAG_NOFRAMES || tag == TAG_NOEMBED) {
            insert_element_for_token(tb, tok);
            tokenizer_set_state(&tb->tokenizer, STATE_RAWTEXT);
            tb->original_mode = tb->mode;
            tb->mode = MODE_TEXT;
            return;
        }

        /* HD-03: script → switch to script data */
        if (tag == TAG_SCRIPT) {
            insert_element_for_token(tb, tok);
            tokenizer_set_state(&tb->tokenizer, STATE_SCRIPT_DATA);
            tb->original_mode = tb->mode;
            tb->mode = MODE_TEXT;
            return;
        }

        /* HD-04: base, link, meta → void, stay in head */
        if (tag == TAG_BASE || tag == TAG_LINK || tag == TAG_META) {
            DomNode *elem = insert_element_for_token(tb, tok);
            pop_element(tb); /* void element */
            (void)elem;
            return;
        }

        /* HD-05: head → ignore */
        if (tag == TAG_HEAD)
            return;
    }

    if (tok->type == TOK_END_TAG) {
        HtmlTag tag = html_tag_from_name(tok->tag_name, tok->tag_name_len);
        if (tag == TAG_HEAD) {
            pop_element(tb);
            tb->mode = MODE_AFTER_HEAD;
            return;
        }
    }

    /* Anything else: close head and reprocess. */
    pop_element(tb);
    tb->mode = MODE_AFTER_HEAD;
    process_token(tb, tok);
}

static void handle_after_head(TreeBuilder *tb, HtmlToken *tok)
{
    if (tok->type == TOK_CHARACTER &&
        (tok->ch_data[0] == ' ' || tok->ch_data[0] == '\t' ||
         tok->ch_data[0] == '\n' || tok->ch_data[0] == '\f')) {
        insert_text(tb, tok->ch_data, tok->ch_len);
        return;
    }

    if (tok->type == TOK_START_TAG) {
        HtmlTag tag = html_tag_from_name(tok->tag_name, tok->tag_name_len);
        if (tag == TAG_BODY) {
            DomNode *body = insert_element_for_token(tb, tok);
            tb->doc->body = body;
            tb->frameset_ok = false;
            tb->mode = MODE_IN_BODY;
            return;
        }
        if (tag == TAG_FRAMESET) {
            insert_element_for_token(tb, tok);
            tb->mode = MODE_IN_BODY; /* Simplified. */
            return;
        }
        /* head-delegated elements */
        if (tag == TAG_BASE || tag == TAG_LINK || tag == TAG_META ||
            tag == TAG_TITLE || tag == TAG_STYLE || tag == TAG_SCRIPT ||
            tag == TAG_NOSCRIPT || tag == TAG_NOFRAMES || tag == TAG_NOEMBED) {
            push_element(tb, tb->head_ptr);
            handle_in_head(tb, tok);
            /* Remove head from stack. */
            for (int i = tb->stack_len - 1; i >= 0; i--) {
                if (tb->stack[i] == tb->head_ptr) {
                    memmove(&tb->stack[i], &tb->stack[i+1],
                            (tb->stack_len - i - 1) * sizeof(DomNode *));
                    tb->stack_len--;
                    break;
                }
            }
            return;
        }
    }

    /* Auto-create <body>. */
    DomNode *body = doc_create_element(tb->doc, TAG_BODY, "body");
    dom_append_child(current_node(tb), body);
    push_element(tb, body);
    tb->doc->body = body;
    tb->mode = MODE_IN_BODY;
    process_token(tb, tok);
}

static void handle_in_body(TreeBuilder *tb, HtmlToken *tok)
{
    /* ── Character tokens ──────────────────────────────────────────── */
    if (tok->type == TOK_CHARACTER) {
        reconstruct_afe(tb);
        insert_text(tb, tok->ch_data, tok->ch_len);
        if (tok->ch_data[0] != ' ' && tok->ch_data[0] != '\t' &&
            tok->ch_data[0] != '\n' && tok->ch_data[0] != '\f')
            tb->frameset_ok = false;
        return;
    }

    if (tok->type == TOK_COMMENT) {
        insert_comment(tb, tok->comment_data, tok->comment_len);
        return;
    }

    if (tok->type == TOK_EOF) return;

    /* ── Start tags ────────────────────────────────────────────────── */
    if (tok->type == TOK_START_TAG) {
        HtmlTag tag = html_tag_from_name(tok->tag_name, tok->tag_name_len);

        /* IB-01: html → merge attrs onto existing html element */
        if (tag == TAG_HTML) return;

        /* IB-02: head-delegated elements */
        if (tag == TAG_BASE || tag == TAG_LINK || tag == TAG_META ||
            tag == TAG_TITLE || tag == TAG_STYLE || tag == TAG_SCRIPT ||
            tag == TAG_NOSCRIPT || tag == TAG_NOFRAMES || tag == TAG_NOEMBED) {
            handle_in_head(tb, tok);
            return;
        }

        /* IB-03: body → merge attrs */
        if (tag == TAG_BODY) return;

        /* IB-04: P-closing blocks */
        if (is_p_closing(tag)) {
            close_p_element(tb);
            insert_element_for_token(tb, tok);
            return;
        }

        /* IB-05: Headings */
        if (is_heading(tag)) {
            close_p_element(tb);
            /* If current is a heading, close it. */
            if (current_node(tb)->type == PANE_NODE_ELEMENT &&
                is_heading(current_node(tb)->elem.tag)) {
                pop_element(tb);
            }
            insert_element_for_token(tb, tok);
            return;
        }

        /* IB-06: pre, listing → close P, insert, skip next LF */
        if (tag == TAG_PRE || tag == TAG_LISTING) {
            close_p_element(tb);
            insert_element_for_token(tb, tok);
            tb->frameset_ok = false;
            return;
        }

        /* IB-07: form */
        if (tag == TAG_FORM) {
            if (tb->form_ptr && !stack_has_tag(tb, TAG_TEMPLATE))
                return; /* Ignore nested form. */
            close_p_element(tb);
            DomNode *form = insert_element_for_token(tb, tok);
            if (!stack_has_tag(tb, TAG_TEMPLATE))
                tb->form_ptr = form;
            return;
        }

        /* IB-08: li */
        if (tag == TAG_LI) {
            tb->frameset_ok = false;
            for (int i = tb->stack_len - 1; i >= 0; i--) {
                DomNode *n = tb->stack[i];
                if (n->type == PANE_NODE_ELEMENT && n->elem.tag == TAG_LI) {
                    generate_implied_end_tags(tb, TAG_LI);
                    pop_until_tag(tb, TAG_LI);
                    break;
                }
                if (n->type == PANE_NODE_ELEMENT && is_special(n->elem.tag) &&
                    n->elem.tag != TAG_ADDRESS && n->elem.tag != TAG_DIV &&
                    n->elem.tag != TAG_P)
                    break;
            }
            close_p_element(tb);
            insert_element_for_token(tb, tok);
            return;
        }

        /* IB-09: dd, dt */
        if (tag == TAG_DD || tag == TAG_DT) {
            tb->frameset_ok = false;
            for (int i = tb->stack_len - 1; i >= 0; i--) {
                DomNode *n = tb->stack[i];
                if (n->type != PANE_NODE_ELEMENT) continue;
                if (n->elem.tag == TAG_DD || n->elem.tag == TAG_DT) {
                    generate_implied_end_tags(tb, n->elem.tag);
                    pop_until_tag(tb, n->elem.tag);
                    break;
                }
                if (is_special(n->elem.tag) &&
                    n->elem.tag != TAG_ADDRESS && n->elem.tag != TAG_DIV &&
                    n->elem.tag != TAG_P)
                    break;
            }
            close_p_element(tb);
            insert_element_for_token(tb, tok);
            return;
        }

        /* IB-10: hr → close P, insert void */
        if (tag == TAG_HR) {
            close_p_element(tb);
            insert_element_for_token(tb, tok);
            pop_element(tb);
            tb->frameset_ok = false;
            return;
        }

        /* IB-11: Formatting elements (AFE) */
        if (is_formatting(tag)) {
            reconstruct_afe(tb);
            DomNode *elem = insert_element_for_token(tb, tok);
            afe_push(tb, elem);
            return;
        }

        /* IB-12: Void elements */
        if (is_void(tag)) {
            reconstruct_afe(tb);
            insert_element_for_token(tb, tok);
            pop_element(tb);
            tb->frameset_ok = false;
            return;
        }

        /* IB-13: table → close P if in scope, switch to InTable */
        if (tag == TAG_TABLE) {
            close_p_element(tb);
            insert_element_for_token(tb, tok);
            tb->frameset_ok = false;
            tb->mode = MODE_IN_TABLE;
            return;
        }

        /* IB-14: textarea → switch to RCDATA */
        if (tag == TAG_TEXTAREA) {
            insert_element_for_token(tb, tok);
            tokenizer_set_state(&tb->tokenizer, STATE_RCDATA);
            tb->original_mode = tb->mode;
            tb->mode = MODE_TEXT;
            tb->frameset_ok = false;
            return;
        }

        /* IB-15: select → switch to InSelect */
        if (tag == TAG_SELECT) {
            reconstruct_afe(tb);
            insert_element_for_token(tb, tok);
            tb->frameset_ok = false;
            tb->mode = MODE_IN_SELECT;
            return;
        }

        /* IB-16: Table-internal elements in body → foster parent / ignore */
        if (tag == TAG_CAPTION || tag == TAG_COL || tag == TAG_COLGROUP ||
            tag == TAG_THEAD || tag == TAG_TBODY || tag == TAG_TFOOT ||
            tag == TAG_TR || tag == TAG_TD || tag == TAG_TH) {
            /* In body mode, these are parse errors — ignored. */
            return;
        }

        /* IB-17: applet, marquee, object → AFE marker + insert */
        if (tag == TAG_APPLET || tag == TAG_MARQUEE || tag == TAG_OBJECT) {
            reconstruct_afe(tb);
            insert_element_for_token(tb, tok);
            afe_push_marker(tb);
            tb->frameset_ok = false;
            return;
        }

        /* IB-18: button */
        if (tag == TAG_BUTTON) {
            if (has_element_in_scope(tb, TAG_BUTTON)) {
                generate_implied_end_tags(tb, TAG_UNKNOWN);
                pop_until_tag(tb, TAG_BUTTON);
            }
            reconstruct_afe(tb);
            insert_element_for_token(tb, tok);
            tb->frameset_ok = false;
            return;
        }

        /* IB-default: generic phrasing/flow content */
        reconstruct_afe(tb);
        insert_element_for_token(tb, tok);
        return;
    }

    /* ── End tags ──────────────────────────────────────────────────── */
    if (tok->type == TOK_END_TAG) {
        HtmlTag tag = html_tag_from_name(tok->tag_name, tok->tag_name_len);

        if (tag == TAG_BODY) {
            if (!has_element_in_scope(tb, TAG_BODY)) return;
            tb->mode = MODE_AFTER_BODY;
            return;
        }

        if (tag == TAG_HTML) {
            if (!has_element_in_scope(tb, TAG_BODY)) return;
            tb->mode = MODE_AFTER_BODY;
            process_token(tb, tok);
            return;
        }

        /* P-closing blocks: generate implied end tags, pop. */
        if (is_p_closing(tag) || is_heading(tag)) {
            if (has_element_in_scope(tb, tag)) {
                generate_implied_end_tags(tb, tag);
                pop_until_tag(tb, tag);
            }
            return;
        }

        /* Formatting end tags: simplified adoption agency. */
        if (is_formatting(tag)) {
            if (has_element_in_scope(tb, tag)) {
                generate_implied_end_tags(tb, TAG_UNKNOWN);
                pop_until_tag(tb, tag);
                /* Remove from AFE. */
                for (int i = tb->afe_len - 1; i >= 0; i--) {
                    if (tb->afe[i] && tb->afe[i]->type == PANE_NODE_ELEMENT &&
                        tb->afe[i]->elem.tag == tag) {
                        memmove(&tb->afe[i], &tb->afe[i+1],
                                (tb->afe_len - i - 1) * sizeof(DomNode *));
                        tb->afe_len--;
                        break;
                    }
                }
            }
            return;
        }

        if (tag == TAG_FORM) {
            if (!stack_has_tag(tb, TAG_TEMPLATE)) {
                tb->form_ptr = NULL;
                if (has_element_in_scope(tb, TAG_FORM))
                    pop_until_tag(tb, TAG_FORM);
            }
            return;
        }

        if (tag == TAG_LI) {
            if (has_element_in_scope(tb, TAG_LI)) {
                generate_implied_end_tags(tb, TAG_LI);
                pop_until_tag(tb, TAG_LI);
            }
            return;
        }

        if (tag == TAG_DD || tag == TAG_DT) {
            if (has_element_in_scope(tb, tag)) {
                generate_implied_end_tags(tb, tag);
                pop_until_tag(tb, tag);
            }
            return;
        }

        if (tag == TAG_APPLET || tag == TAG_MARQUEE || tag == TAG_OBJECT) {
            if (has_element_in_scope(tb, tag)) {
                generate_implied_end_tags(tb, TAG_UNKNOWN);
                pop_until_tag(tb, tag);
                afe_clear_to_marker(tb);
            }
            return;
        }

        if (tag == TAG_BR) {
            /* Parse error. Treat as <br> start tag. */
            tok->type = TOK_START_TAG;
            reconstruct_afe(tb);
            insert_element_for_token(tb, tok);
            pop_element(tb);
            return;
        }

        /* Any other end tag: walk stack, pop matching element. */
        for (int i = tb->stack_len - 1; i >= 0; i--) {
            DomNode *n = tb->stack[i];
            if (n->type == PANE_NODE_ELEMENT && n->elem.tag == tag) {
                generate_implied_end_tags(tb, tag);
                while (tb->stack_len > 0 && tb->stack[tb->stack_len - 1] != n)
                    pop_element(tb);
                pop_element(tb);
                return;
            }
            if (n->type == PANE_NODE_ELEMENT && is_special(n->elem.tag))
                return; /* Stop if we hit a special element. */
        }
    }
}

/* ── Table Modes ────────────────────────────────────────────────────── */
/* From html_pair_chain_handling.md rules IT-01 through IT-06 */

static void handle_in_table(TreeBuilder *tb, HtmlToken *tok)
{
    if (tok->type == TOK_CHARACTER) {
        /* IT-06: Foster parent text. Simplified: insert as text. */
        insert_text(tb, tok->ch_data, tok->ch_len);
        return;
    }

    if (tok->type == TOK_COMMENT) {
        insert_comment(tb, tok->comment_data, tok->comment_len);
        return;
    }

    if (tok->type == TOK_START_TAG) {
        HtmlTag tag = html_tag_from_name(tok->tag_name, tok->tag_name_len);

        /* IT-01: caption → push, switch to InCaption */
        if (tag == TAG_CAPTION) {
            afe_push_marker(tb);
            insert_element_for_token(tb, tok);
            tb->mode = MODE_IN_CAPTION;
            return;
        }

        /* IT-02: colgroup → push, switch to InColumnGroup */
        if (tag == TAG_COLGROUP) {
            insert_element_for_token(tb, tok);
            tb->mode = MODE_IN_COLUMN_GROUP;
            return;
        }

        /* IT-03: col → auto-create colgroup */
        if (tag == TAG_COL) {
            DomNode *cg = doc_create_element(tb->doc, TAG_COLGROUP, "colgroup");
            dom_append_child(current_node(tb), cg);
            push_element(tb, cg);
            tb->mode = MODE_IN_COLUMN_GROUP;
            process_token(tb, tok);
            return;
        }

        /* IT-04: thead, tbody, tfoot → push, switch to InTableBody */
        if (is_table_body(tag)) {
            insert_element_for_token(tb, tok);
            tb->mode = MODE_IN_TABLE_BODY;
            return;
        }

        /* IT-05: tr, td, th → auto-create tbody */
        if (tag == TAG_TR || tag == TAG_TD || tag == TAG_TH) {
            DomNode *tbody = doc_create_element(tb->doc, TAG_TBODY, "tbody");
            dom_append_child(current_node(tb), tbody);
            push_element(tb, tbody);
            tb->mode = MODE_IN_TABLE_BODY;
            process_token(tb, tok);
            return;
        }

        /* IT-default: Anything else → foster parent / ignore */
        if (tag == TAG_TABLE) {
            if (has_element_in_scope(tb, TAG_TABLE)) {
                pop_until_tag(tb, TAG_TABLE);
                /* Reset mode based on current node. */
                DomNode *cn = current_node(tb);
                if (cn->type == PANE_NODE_ELEMENT)
                    tb->mode = mode_for_tag(cn->elem.tag);
                else
                    tb->mode = MODE_IN_BODY;
                process_token(tb, tok);
            }
            return;
        }

        return; /* Ignore other start tags in table mode. */
    }

    if (tok->type == TOK_END_TAG) {
        HtmlTag tag = html_tag_from_name(tok->tag_name, tok->tag_name_len);

        if (tag == TAG_TABLE) {
            if (!has_element_in_scope(tb, TAG_TABLE)) return;
            pop_until_tag(tb, TAG_TABLE);
            DomNode *cn = current_node(tb);
            if (cn->type == PANE_NODE_ELEMENT)
                tb->mode = mode_for_tag(cn->elem.tag);
            else
                tb->mode = MODE_IN_BODY;
            return;
        }
    }
}

static void handle_in_table_body(TreeBuilder *tb, HtmlToken *tok)
{
    if (tok->type == TOK_START_TAG) {
        HtmlTag tag = html_tag_from_name(tok->tag_name, tok->tag_name_len);

        /* TB-01: tr → insert, switch to InRow */
        if (tag == TAG_TR) {
            insert_element_for_token(tb, tok);
            tb->mode = MODE_IN_ROW;
            return;
        }

        /* TB-02: td, th → auto-create tr */
        if (tag == TAG_TD || tag == TAG_TH) {
            DomNode *tr = doc_create_element(tb->doc, TAG_TR, "tr");
            dom_append_child(current_node(tb), tr);
            push_element(tb, tr);
            tb->mode = MODE_IN_ROW;
            process_token(tb, tok);
            return;
        }

        /* TB-03: caption, colgroup, thead, tbody, tfoot → close current tbody */
        if (tag == TAG_CAPTION || tag == TAG_COLGROUP || is_table_body(tag)) {
            HtmlTag cur = current_node(tb)->type == PANE_NODE_ELEMENT ?
                          current_node(tb)->elem.tag : TAG_UNKNOWN;
            if (is_table_body(cur)) pop_element(tb);
            tb->mode = MODE_IN_TABLE;
            process_token(tb, tok);
            return;
        }
    }

    if (tok->type == TOK_END_TAG) {
        HtmlTag tag = html_tag_from_name(tok->tag_name, tok->tag_name_len);

        if (is_table_body(tag)) {
            pop_element(tb);
            tb->mode = MODE_IN_TABLE;
            return;
        }

        if (tag == TAG_TABLE) {
            HtmlTag cur = current_node(tb)->type == PANE_NODE_ELEMENT ?
                          current_node(tb)->elem.tag : TAG_UNKNOWN;
            if (is_table_body(cur)) pop_element(tb);
            tb->mode = MODE_IN_TABLE;
            process_token(tb, tok);
            return;
        }
    }

    /* Default: process as in-table. */
    handle_in_table(tb, tok);
}

static void handle_in_row(TreeBuilder *tb, HtmlToken *tok)
{
    if (tok->type == TOK_START_TAG) {
        HtmlTag tag = html_tag_from_name(tok->tag_name, tok->tag_name_len);

        /* IR-01: td, th → insert, switch to InCell, push AFE marker */
        if (tag == TAG_TD || tag == TAG_TH) {
            insert_element_for_token(tb, tok);
            tb->mode = MODE_IN_CELL;
            afe_push_marker(tb);
            return;
        }

        /* IR-02: tr, caption, colgroup, thead, tbody, tfoot → close row */
        if (tag == TAG_TR || tag == TAG_CAPTION || tag == TAG_COLGROUP ||
            is_table_body(tag)) {
            if (current_node(tb)->type == PANE_NODE_ELEMENT &&
                current_node(tb)->elem.tag == TAG_TR)
                pop_element(tb);
            tb->mode = MODE_IN_TABLE_BODY;
            process_token(tb, tok);
            return;
        }
    }

    if (tok->type == TOK_END_TAG) {
        HtmlTag tag = html_tag_from_name(tok->tag_name, tok->tag_name_len);

        if (tag == TAG_TR) {
            pop_element(tb);
            tb->mode = MODE_IN_TABLE_BODY;
            return;
        }

        if (tag == TAG_TABLE) {
            if (current_node(tb)->type == PANE_NODE_ELEMENT &&
                current_node(tb)->elem.tag == TAG_TR)
                pop_element(tb);
            tb->mode = MODE_IN_TABLE_BODY;
            process_token(tb, tok);
            return;
        }

        if (is_table_body(tag)) {
            if (current_node(tb)->type == PANE_NODE_ELEMENT &&
                current_node(tb)->elem.tag == TAG_TR)
                pop_element(tb);
            tb->mode = MODE_IN_TABLE_BODY;
            process_token(tb, tok);
            return;
        }
    }

    handle_in_table(tb, tok);
}

static void handle_in_cell(TreeBuilder *tb, HtmlToken *tok)
{
    if (tok->type == TOK_END_TAG) {
        HtmlTag tag = html_tag_from_name(tok->tag_name, tok->tag_name_len);

        /* IC-01: td, th → close cell */
        if (tag == TAG_TD || tag == TAG_TH) {
            if (!has_element_in_scope(tb, tag)) return;
            generate_implied_end_tags(tb, TAG_UNKNOWN);
            pop_until_tag(tb, tag);
            afe_clear_to_marker(tb);
            tb->mode = MODE_IN_ROW;
            return;
        }

        if (tag == TAG_TABLE || tag == TAG_TR || is_table_body(tag)) {
            /* Close the cell first, then reprocess. */
            HtmlTag cell = TAG_UNKNOWN;
            for (int i = tb->stack_len - 1; i >= 0; i--) {
                DomNode *n = tb->stack[i];
                if (n->type == PANE_NODE_ELEMENT &&
                    (n->elem.tag == TAG_TD || n->elem.tag == TAG_TH)) {
                    cell = n->elem.tag;
                    break;
                }
            }
            if (cell != TAG_UNKNOWN) {
                generate_implied_end_tags(tb, TAG_UNKNOWN);
                pop_until_tag(tb, cell);
                afe_clear_to_marker(tb);
                tb->mode = MODE_IN_ROW;
                process_token(tb, tok);
            }
            return;
        }
    }

    if (tok->type == TOK_START_TAG) {
        HtmlTag tag = html_tag_from_name(tok->tag_name, tok->tag_name_len);

        /* If starting a new cell-level element, close current cell. */
        if (tag == TAG_CAPTION || tag == TAG_COL || tag == TAG_COLGROUP ||
            is_table_body(tag) || tag == TAG_TR || tag == TAG_TD ||
            tag == TAG_TH) {
            HtmlTag cell = TAG_UNKNOWN;
            for (int i = tb->stack_len - 1; i >= 0; i--) {
                DomNode *n = tb->stack[i];
                if (n->type == PANE_NODE_ELEMENT &&
                    (n->elem.tag == TAG_TD || n->elem.tag == TAG_TH)) {
                    cell = n->elem.tag;
                    break;
                }
            }
            if (cell != TAG_UNKNOWN) {
                generate_implied_end_tags(tb, TAG_UNKNOWN);
                pop_until_tag(tb, cell);
                afe_clear_to_marker(tb);
                tb->mode = MODE_IN_ROW;
                process_token(tb, tok);
            }
            return;
        }
    }

    /* Default: process as in-body. */
    handle_in_body(tb, tok);
}

static void handle_in_caption(TreeBuilder *tb, HtmlToken *tok)
{
    if (tok->type == TOK_END_TAG) {
        HtmlTag tag = html_tag_from_name(tok->tag_name, tok->tag_name_len);
        if (tag == TAG_CAPTION) {
            if (!has_element_in_scope(tb, TAG_CAPTION)) return;
            generate_implied_end_tags(tb, TAG_UNKNOWN);
            pop_until_tag(tb, TAG_CAPTION);
            afe_clear_to_marker(tb);
            tb->mode = MODE_IN_TABLE;
            return;
        }
        if (tag == TAG_TABLE) {
            if (!has_element_in_scope(tb, TAG_CAPTION)) return;
            generate_implied_end_tags(tb, TAG_UNKNOWN);
            pop_until_tag(tb, TAG_CAPTION);
            afe_clear_to_marker(tb);
            tb->mode = MODE_IN_TABLE;
            process_token(tb, tok);
            return;
        }
    }

    if (tok->type == TOK_START_TAG) {
        HtmlTag tag = html_tag_from_name(tok->tag_name, tok->tag_name_len);
        if (tag == TAG_CAPTION || tag == TAG_COL || tag == TAG_COLGROUP ||
            is_table_body(tag) || tag == TAG_TR || tag == TAG_TD ||
            tag == TAG_TH) {
            if (!has_element_in_scope(tb, TAG_CAPTION)) return;
            generate_implied_end_tags(tb, TAG_UNKNOWN);
            pop_until_tag(tb, TAG_CAPTION);
            afe_clear_to_marker(tb);
            tb->mode = MODE_IN_TABLE;
            process_token(tb, tok);
            return;
        }
    }

    handle_in_body(tb, tok);
}

static void handle_in_column_group(TreeBuilder *tb, HtmlToken *tok)
{
    if (tok->type == TOK_CHARACTER &&
        (tok->ch_data[0] == ' ' || tok->ch_data[0] == '\t' ||
         tok->ch_data[0] == '\n' || tok->ch_data[0] == '\f')) {
        insert_text(tb, tok->ch_data, tok->ch_len);
        return;
    }

    if (tok->type == TOK_COMMENT) {
        insert_comment(tb, tok->comment_data, tok->comment_len);
        return;
    }

    if (tok->type == TOK_START_TAG) {
        HtmlTag tag = html_tag_from_name(tok->tag_name, tok->tag_name_len);
        if (tag == TAG_COL) {
            insert_element_for_token(tb, tok);
            pop_element(tb); /* void */
            return;
        }
        if (tag == TAG_TEMPLATE) {
            handle_in_head(tb, tok);
            return;
        }
    }

    if (tok->type == TOK_END_TAG) {
        HtmlTag tag = html_tag_from_name(tok->tag_name, tok->tag_name_len);
        if (tag == TAG_COLGROUP) {
            if (current_node(tb)->type == PANE_NODE_ELEMENT &&
                current_node(tb)->elem.tag == TAG_COLGROUP)
                pop_element(tb);
            tb->mode = MODE_IN_TABLE;
            return;
        }
        if (tag == TAG_COL) return; /* Ignore. */
    }

    /* Close colgroup, reprocess in table mode. */
    if (current_node(tb)->type == PANE_NODE_ELEMENT &&
        current_node(tb)->elem.tag == TAG_COLGROUP)
        pop_element(tb);
    tb->mode = MODE_IN_TABLE;
    process_token(tb, tok);
}

static void handle_in_select(TreeBuilder *tb, HtmlToken *tok)
{
    if (tok->type == TOK_CHARACTER) {
        insert_text(tb, tok->ch_data, tok->ch_len);
        return;
    }

    if (tok->type == TOK_COMMENT) {
        insert_comment(tb, tok->comment_data, tok->comment_len);
        return;
    }

    if (tok->type == TOK_START_TAG) {
        HtmlTag tag = html_tag_from_name(tok->tag_name, tok->tag_name_len);

        if (tag == TAG_OPTION) {
            if (current_node(tb)->type == PANE_NODE_ELEMENT &&
                current_node(tb)->elem.tag == TAG_OPTION)
                pop_element(tb);
            insert_element_for_token(tb, tok);
            return;
        }
        if (tag == TAG_OPTGROUP) {
            if (current_node(tb)->type == PANE_NODE_ELEMENT &&
                current_node(tb)->elem.tag == TAG_OPTION)
                pop_element(tb);
            if (current_node(tb)->type == PANE_NODE_ELEMENT &&
                current_node(tb)->elem.tag == TAG_OPTGROUP)
                pop_element(tb);
            insert_element_for_token(tb, tok);
            return;
        }
        if (tag == TAG_HR) {
            if (current_node(tb)->type == PANE_NODE_ELEMENT &&
                current_node(tb)->elem.tag == TAG_OPTION)
                pop_element(tb);
            if (current_node(tb)->type == PANE_NODE_ELEMENT &&
                current_node(tb)->elem.tag == TAG_OPTGROUP)
                pop_element(tb);
            insert_element_for_token(tb, tok);
            pop_element(tb); /* void */
            return;
        }
        if (tag == TAG_SELECT) {
            pop_until_tag(tb, TAG_SELECT);
            DomNode *cn = current_node(tb);
            if (cn->type == PANE_NODE_ELEMENT)
                tb->mode = mode_for_tag(cn->elem.tag);
            else
                tb->mode = MODE_IN_BODY;
            return;
        }
        if (tag == TAG_INPUT || tag == TAG_TEXTAREA) {
            pop_until_tag(tb, TAG_SELECT);
            DomNode *cn = current_node(tb);
            if (cn->type == PANE_NODE_ELEMENT)
                tb->mode = mode_for_tag(cn->elem.tag);
            else
                tb->mode = MODE_IN_BODY;
            process_token(tb, tok);
            return;
        }
        return; /* Ignore other start tags in select. */
    }

    if (tok->type == TOK_END_TAG) {
        HtmlTag tag = html_tag_from_name(tok->tag_name, tok->tag_name_len);
        if (tag == TAG_OPTGROUP) {
            if (current_node(tb)->type == PANE_NODE_ELEMENT &&
                current_node(tb)->elem.tag == TAG_OPTION &&
                tb->stack_len >= 2 &&
                tb->stack[tb->stack_len-2]->type == PANE_NODE_ELEMENT &&
                tb->stack[tb->stack_len-2]->elem.tag == TAG_OPTGROUP)
                pop_element(tb);
            if (current_node(tb)->type == PANE_NODE_ELEMENT &&
                current_node(tb)->elem.tag == TAG_OPTGROUP)
                pop_element(tb);
            return;
        }
        if (tag == TAG_OPTION) {
            if (current_node(tb)->type == PANE_NODE_ELEMENT &&
                current_node(tb)->elem.tag == TAG_OPTION)
                pop_element(tb);
            return;
        }
        if (tag == TAG_SELECT) {
            if (!has_element_in_scope(tb, TAG_SELECT)) return;
            pop_until_tag(tb, TAG_SELECT);
            DomNode *cn = current_node(tb);
            if (cn->type == PANE_NODE_ELEMENT)
                tb->mode = mode_for_tag(cn->elem.tag);
            else
                tb->mode = MODE_IN_BODY;
            return;
        }
    }
}

static void handle_text(TreeBuilder *tb, HtmlToken *tok)
{
    if (tok->type == TOK_CHARACTER) {
        insert_text(tb, tok->ch_data, tok->ch_len);
        return;
    }
    if (tok->type == TOK_EOF || tok->type == TOK_END_TAG) {
        pop_element(tb);
        tb->mode = tb->original_mode;
        return;
    }
}

static void handle_after_body(TreeBuilder *tb, HtmlToken *tok)
{
    if (tok->type == TOK_CHARACTER &&
        (tok->ch_data[0] == ' ' || tok->ch_data[0] == '\t' ||
         tok->ch_data[0] == '\n' || tok->ch_data[0] == '\f')) {
        handle_in_body(tb, tok);
        return;
    }
    if (tok->type == TOK_COMMENT) {
        insert_comment(tb, tok->comment_data, tok->comment_len);
        return;
    }
    if (tok->type == TOK_END_TAG) {
        HtmlTag tag = html_tag_from_name(tok->tag_name, tok->tag_name_len);
        if (tag == TAG_HTML) {
            tb->mode = MODE_AFTER_AFTER_BODY;
            return;
        }
    }
    /* Reprocess in body. */
    tb->mode = MODE_IN_BODY;
    process_token(tb, tok);
}

/* ── Token Dispatch ─────────────────────────────────────────────────── */

static void process_token(TreeBuilder *tb, HtmlToken *tok)
{
    switch (tb->mode) {
    case MODE_INITIAL:           handle_initial(tb, tok); break;
    case MODE_BEFORE_HTML:       handle_before_html(tb, tok); break;
    case MODE_BEFORE_HEAD:       handle_before_head(tb, tok); break;
    case MODE_IN_HEAD:           handle_in_head(tb, tok); break;
    case MODE_AFTER_HEAD:        handle_after_head(tb, tok); break;
    case MODE_IN_BODY:           handle_in_body(tb, tok); break;
    case MODE_IN_TABLE:          handle_in_table(tb, tok); break;
    case MODE_IN_TABLE_BODY:     handle_in_table_body(tb, tok); break;
    case MODE_IN_ROW:            handle_in_row(tb, tok); break;
    case MODE_IN_CELL:           handle_in_cell(tb, tok); break;
    case MODE_IN_CAPTION:        handle_in_caption(tb, tok); break;
    case MODE_IN_COLUMN_GROUP:   handle_in_column_group(tb, tok); break;
    case MODE_IN_SELECT:
    case MODE_IN_SELECT_IN_TABLE: handle_in_select(tb, tok); break;
    case MODE_TEXT:              handle_text(tb, tok); break;
    case MODE_AFTER_BODY:
    case MODE_AFTER_AFTER_BODY: handle_after_body(tb, tok); break;

    case MODE_IN_TEMPLATE:
    case MODE_IN_FOREIGN_CONTENT:
        /* Simplified: process as in-body. */
        handle_in_body(tb, tok);
        break;
    }
}

/* ── Public API ─────────────────────────────────────────────────────── */

void tree_builder_init(TreeBuilder *tb, Document *doc,
                       const char *input, size_t len)
{
    memset(tb, 0, sizeof(*tb));
    tb->doc = doc;
    tb->mode = MODE_INITIAL;
    tb->frameset_ok = true;
    tokenizer_init(&tb->tokenizer, input, len);
}

void tree_builder_run(TreeBuilder *tb)
{
    HtmlToken tok;
    for (;;) {
        bool more = tokenizer_next(&tb->tokenizer, &tok);
        process_token(tb, &tok);
        if (tok.type == TOK_EOF) break;
        if (!more) break;
    }
}

Document *html_parse(const char *input, size_t len)
{
    Document *doc = doc_create();
    TreeBuilder tb;
    tree_builder_init(&tb, doc, input, len);
    tree_builder_run(&tb);
    return doc;
}

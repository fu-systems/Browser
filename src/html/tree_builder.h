/*
 * Pane — HTML Tree Builder
 *
 * Constructs a DOM tree from HTML tokens using insertion modes.
 * Implements the 62 compressed rules from html_pair_chain_handling.md,
 * derived from the HTML5 spec's tree construction algorithm.
 */

#ifndef PANE_HTML_TREE_BUILDER_H
#define PANE_HTML_TREE_BUILDER_H

#include "tokenizer.h"
#include "../dom/dom.h"

/* ── Insertion Modes ────────────────────────────────────────────────── */
/* From html_pair_chain_handling.md Section 1: Parent → Mode Mapping */

typedef enum {
    MODE_INITIAL,
    MODE_BEFORE_HTML,
    MODE_BEFORE_HEAD,
    MODE_IN_HEAD,
    MODE_AFTER_HEAD,
    MODE_IN_BODY,
    MODE_IN_TABLE,
    MODE_IN_TABLE_BODY,
    MODE_IN_ROW,
    MODE_IN_CELL,
    MODE_IN_CAPTION,
    MODE_IN_COLUMN_GROUP,
    MODE_IN_SELECT,
    MODE_IN_SELECT_IN_TABLE,
    MODE_IN_TEMPLATE,
    MODE_IN_FOREIGN_CONTENT,
    MODE_TEXT,
    MODE_AFTER_BODY,
    MODE_AFTER_AFTER_BODY,
} InsertionMode;

/* ── Open Elements Stack ────────────────────────────────────────────── */

#define MAX_STACK_DEPTH 512

/* ── Active Formatting Elements ─────────────────────────────────────── */

#define AFE_MARKER NULL  /* Scope marker */
#define MAX_AFE 128

/* ── Tree Builder ───────────────────────────────────────────────────── */

typedef struct {
    Document      *doc;
    HtmlTokenizer  tokenizer;
    InsertionMode  mode;
    InsertionMode  original_mode;

    /* Open elements stack. */
    DomNode       *stack[MAX_STACK_DEPTH];
    int            stack_len;

    /* Active formatting elements list. */
    DomNode       *afe[MAX_AFE];
    int            afe_len;

    /* Head/body/form element pointers. */
    DomNode       *head_ptr;
    DomNode       *form_ptr;
    bool           frameset_ok;
    bool           scripting;    /* always false in Pane — no JS */

    /* Foster parenting flag. */
    bool           foster_parenting;

    /* Template insertion mode stack. */
    InsertionMode  template_modes[64];
    int            template_mode_len;
} TreeBuilder;

/* Parse an HTML string into a Document. This is the main entry point. */
Document *html_parse(const char *input, size_t len);

/* Lower-level API. */
void tree_builder_init(TreeBuilder *tb, Document *doc,
                       const char *input, size_t len);
void tree_builder_run(TreeBuilder *tb);

#endif /* PANE_HTML_TREE_BUILDER_H */

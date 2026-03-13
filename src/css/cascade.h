/*
 * Pane — CSS Cascade
 *
 * Resolves which declarations apply to each element, considering
 * selector matching, specificity, origin, importance, and order.
 */

#ifndef PANE_CSS_CASCADE_H
#define PANE_CSS_CASCADE_H

#include "css_parser.h"
#include "selectors.h"
#include "../dom/dom.h"
#include "../util/arena.h"

/* ── Matched Declaration ───────────────────────────────────────────── */

typedef struct {
    CssPropId    property;
    CssValue     value;
    Specificity  specificity;
    int          source_order;
    bool         important;
} MatchedDecl;

/* ── Cascade Result ────────────────────────────────────────────────── */

typedef struct {
    MatchedDecl *decls;
    int          count;
    int          cap;
} CascadeResult;

/* ── API ────────────────────────────────────────────────────────────── */

/* Collect all declarations matching an element from a stylesheet. */
void cascade_collect(const Stylesheet *ss, const DomNode *elem,
                     CascadeResult *result, Arena *arena);

/* Add inline style declarations (highest specificity). */
void cascade_add_inline(const DeclBlock *db, CascadeResult *result,
                        Arena *arena);

/* Sort and resolve: for each property, keep the winning declaration. */
void cascade_resolve(CascadeResult *result);

/* Lookup the winning value for a property in a resolved cascade. */
const CssValue *cascade_get(const CascadeResult *result, CssPropId prop);

#endif /* PANE_CSS_CASCADE_H */

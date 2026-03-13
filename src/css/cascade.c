/*
 * Pane — CSS Cascade Implementation
 */

#include "cascade.h"
#include <string.h>
#include <stdlib.h>

/* ── Helpers ────────────────────────────────────────────────────────── */

static void result_push(CascadeResult *r, MatchedDecl md, Arena *arena)
{
    if (r->count >= r->cap) {
        int new_cap = r->cap ? r->cap * 2 : 32;
        MatchedDecl *new_decls = arena_alloc(arena, new_cap * sizeof(MatchedDecl), 8);
        if (r->count > 0)
            memcpy(new_decls, r->decls, r->count * sizeof(MatchedDecl));
        r->decls = new_decls;
        r->cap = new_cap;
    }
    r->decls[r->count++] = md;
}

/* ── Collection ─────────────────────────────────────────────────────── */

void cascade_collect(const Stylesheet *ss, const DomNode *elem,
                     CascadeResult *result, Arena *arena)
{
    for (int ri = 0; ri < ss->count; ri++) {
        const CssRule *rule = &ss->rules[ri];

        if (!selector_list_matches(&rule->selectors, elem))
            continue;

        /* Find the highest specificity among matching selectors. */
        Specificity best = {0, 0, 0};
        for (int si = 0; si < rule->selectors.count; si++) {
            if (selector_matches(&rule->selectors.selectors[si], elem)) {
                Specificity sp = selector_specificity(&rule->selectors.selectors[si]);
                if (specificity_cmp(sp, best) > 0)
                    best = sp;
            }
        }

        /* Add each declaration. */
        for (int di = 0; di < rule->declarations.count; di++) {
            const CssDeclaration *d = &rule->declarations.decls[di];
            MatchedDecl md = {
                .property     = d->property,
                .value        = d->value,
                .specificity  = best,
                .source_order = ri,
                .important    = d->important,
            };
            result_push(result, md, arena);
        }
    }
}

void cascade_add_inline(const DeclBlock *db, CascadeResult *result,
                        Arena *arena)
{
    /* Inline styles have specificity (1,0,0,0) — we represent as a=1000. */
    Specificity inline_spec = {1000, 0, 0};

    for (int i = 0; i < db->count; i++) {
        MatchedDecl md = {
            .property     = db->decls[i].property,
            .value        = db->decls[i].value,
            .specificity  = inline_spec,
            .source_order = 1000000, /* after all stylesheet rules */
            .important    = db->decls[i].important,
        };
        result_push(result, md, arena);
    }
}

/* ── Sort comparison ────────────────────────────────────────────────── */

static int matched_decl_cmp(const void *a, const void *b)
{
    const MatchedDecl *ma = a;
    const MatchedDecl *mb = b;

    /* 1. Important > normal. */
    if (ma->important != mb->important)
        return ma->important ? 1 : -1;

    /* 2. Higher specificity wins. */
    int sc = specificity_cmp(ma->specificity, mb->specificity);
    if (sc != 0) return sc;

    /* 3. Later source order wins. */
    return ma->source_order - mb->source_order;
}

/* ── Resolution ─────────────────────────────────────────────────────── */

void cascade_resolve(CascadeResult *result)
{
    if (result->count <= 1) return;

    /* Sort so that for each property, the winning value is last. */
    qsort(result->decls, result->count, sizeof(MatchedDecl), matched_decl_cmp);

    /* Deduplicate: keep only the last (winning) entry per property. */
    int write = 0;
    for (int i = 0; i < result->count; i++) {
        /* Check if a later entry has the same property. */
        bool dominated = false;
        for (int j = i + 1; j < result->count; j++) {
            if (result->decls[j].property == result->decls[i].property) {
                dominated = true;
                break;
            }
        }
        if (!dominated) {
            result->decls[write++] = result->decls[i];
        }
    }
    result->count = write;
}

const CssValue *cascade_get(const CascadeResult *result, CssPropId prop)
{
    for (int i = 0; i < result->count; i++) {
        if (result->decls[i].property == prop)
            return &result->decls[i].value;
    }
    return NULL;
}

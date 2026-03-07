/*
 * Pane — CSS Selector Matching Implementation
 */

#include "selectors.h"
#include "css_tokenizer.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* ── Specificity ────────────────────────────────────────────────────── */

int specificity_cmp(Specificity a, Specificity b)
{
    if (a.a != b.a) return a.a - b.a;
    if (a.b != b.b) return a.b - b.b;
    return a.c - b.c;
}

Specificity selector_specificity(const Selector *sel)
{
    Specificity s = {0, 0, 0};
    for (int i = 0; i < sel->count; i++) {
        const CompoundSelector *cs = &sel->compounds[i];
        for (int j = 0; j < cs->part_count; j++) {
            switch (cs->parts[j].type) {
            case SEL_ID:            s.a++; break;
            case SEL_CLASS:
            case SEL_ATTR_EXISTS:
            case SEL_ATTR_EQUALS:
            case SEL_ATTR_CONTAINS:
            case SEL_ATTR_STARTS:
            case SEL_PSEUDO_CLASS:  s.b++; break;
            case SEL_TYPE:
            case SEL_PSEUDO_ELEM:   s.c++; break;
            case SEL_UNIVERSAL:     break;
            }
        }
    }
    return s;
}

/* ── Compound Matching ──────────────────────────────────────────────── */

static bool has_class(const DomNode *elem, const char *cls)
{
    if (elem->type != PANE_NODE_ELEMENT || !elem->elem.class_str) return false;
    const char *s = elem->elem.class_str;
    size_t cls_len = strlen(cls);
    while (*s) {
        while (*s == ' ') s++;
        const char *word = s;
        while (*s && *s != ' ') s++;
        size_t wlen = s - word;
        if (wlen == cls_len && strncmp(word, cls, cls_len) == 0) return true;
    }
    return false;
}

static bool match_compound(const CompoundSelector *cs, const DomNode *elem)
{
    if (elem->type != PANE_NODE_ELEMENT) return false;

    for (int i = 0; i < cs->part_count; i++) {
        const SelectorPart *p = &cs->parts[i];
        switch (p->type) {
        case SEL_UNIVERSAL:
            break;
        case SEL_TYPE:
            if (strcmp(elem->elem.tag_name, p->name) != 0) return false;
            break;
        case SEL_CLASS:
            if (!has_class(elem, p->name)) return false;
            break;
        case SEL_ID:
            if (!elem->elem.id || strcmp(elem->elem.id, p->name) != 0)
                return false;
            break;
        case SEL_ATTR_EXISTS:
            if (!elem_has_attr(elem, p->name)) return false;
            break;
        case SEL_ATTR_EQUALS: {
            const char *val = elem_get_attr(elem, p->name);
            if (!val || strcmp(val, p->value) != 0) return false;
            break;
        }
        case SEL_ATTR_CONTAINS: {
            const char *val = elem_get_attr(elem, p->name);
            if (!val || !strstr(val, p->value)) return false;
            break;
        }
        case SEL_ATTR_STARTS: {
            const char *val = elem_get_attr(elem, p->name);
            if (!val || strncmp(val, p->value, strlen(p->value)) != 0)
                return false;
            break;
        }
        case SEL_PSEUDO_CLASS:
            if (strcmp(p->name, "first-child") == 0) {
                if (!elem->parent) return false;
                /* Find first element child. */
                const DomNode *fc = elem->parent->first_child;
                while (fc && fc->type != PANE_NODE_ELEMENT) fc = fc->next_sibling;
                if (fc != elem) return false;
            } else if (strcmp(p->name, "last-child") == 0) {
                if (!elem->parent) return false;
                const DomNode *lc = elem->parent->last_child;
                while (lc && lc->type != PANE_NODE_ELEMENT) lc = lc->prev_sibling;
                if (lc != elem) return false;
            } else if (strcmp(p->name, "root") == 0) {
                if (elem->elem.tag != TAG_HTML) return false;
            } else if (strcmp(p->name, "first-of-type") == 0) {
                if (!elem->parent) return false;
                HtmlTag tag = elem->elem.tag;
                for (const DomNode *s = elem->parent->first_child; s; s = s->next_sibling) {
                    if (s->type == PANE_NODE_ELEMENT && s->elem.tag == tag) {
                        if (s != elem) return false;
                        break;
                    }
                }
            } else if (strcmp(p->name, "last-of-type") == 0) {
                if (!elem->parent) return false;
                HtmlTag tag = elem->elem.tag;
                for (const DomNode *s = elem->parent->last_child; s; s = s->prev_sibling) {
                    if (s->type == PANE_NODE_ELEMENT && s->elem.tag == tag) {
                        if (s != elem) return false;
                        break;
                    }
                }
            } else if (strcmp(p->name, "only-child") == 0) {
                if (!elem->parent) return false;
                int count = 0;
                for (const DomNode *s = elem->parent->first_child; s; s = s->next_sibling) {
                    if (s->type == PANE_NODE_ELEMENT) count++;
                    if (count > 1) break;
                }
                if (count != 1) return false;
            } else if (strcmp(p->name, "empty") == 0) {
                if (elem->first_child) return false;
            } else if (strcmp(p->name, "link") == 0) {
                if (elem->elem.tag != TAG_A) return false;
            } else if (strcmp(p->name, "any-link") == 0) {
                if (elem->elem.tag != TAG_A && elem->elem.tag != TAG_AREA)
                    return false;
            } else if (strcmp(p->name, "enabled") == 0) {
                if (elem->elem.tag != TAG_INPUT && elem->elem.tag != TAG_BUTTON &&
                    elem->elem.tag != TAG_SELECT && elem->elem.tag != TAG_TEXTAREA)
                    return false;
                /* Check for disabled attribute. */
                const char *disabled = elem_get_attr(elem, "disabled");
                if (disabled) return false;
            } else if (strcmp(p->name, "disabled") == 0) {
                const char *disabled = elem_get_attr(elem, "disabled");
                if (!disabled) return false;
            } else if (strcmp(p->name, "checked") == 0) {
                const char *checked = elem_get_attr(elem, "checked");
                if (!checked) return false;
            } else if (strcmp(p->name, "hover") == 0 ||
                       strcmp(p->name, "focus") == 0 ||
                       strcmp(p->name, "active") == 0 ||
                       strcmp(p->name, "visited") == 0 ||
                       strcmp(p->name, "focus-within") == 0 ||
                       strcmp(p->name, "focus-visible") == 0) {
                /* Interactive states: no hover/focus support yet.
                 * Return false so these selectors don't match (they add hover effects). */
                return false;
            } else if (strncmp(p->name, "nth-child(", 10) == 0) {
                /* Simplified: support nth-child(odd), nth-child(even), nth-child(N). */
                if (!elem->parent) return false;
                int n = 0;
                for (const DomNode *s = elem->parent->first_child; s; s = s->next_sibling) {
                    if (s->type == PANE_NODE_ELEMENT) n++;
                    if (s == elem) break;
                }
                const char *arg = p->name + 10;
                if (strncmp(arg, "odd", 3) == 0) {
                    if (n % 2 == 0) return false;
                } else if (strncmp(arg, "even", 4) == 0) {
                    if (n % 2 != 0) return false;
                } else {
                    int target = atoi(arg);
                    if (target > 0 && n != target) return false;
                }
            } else if (strcmp(p->name, "not") == 0) {
                /* :not() with simple argument stored in p->value. */
                /* For now, just accept — :not() matching is complex. */
            } else {
                /* Unknown pseudo-class: don't reject the entire selector.
                 * Many pseudo-classes are harmless (e.g., ::-webkit-*). */
                return false;
            }
            break;
        case SEL_PSEUDO_ELEM:
            /* Pseudo-elements don't match for selector matching purposes. */
            return false;
        }
    }
    return true;
}

/* ── Complex Selector Matching ──────────────────────────────────────── */

bool selector_matches(const Selector *sel, const DomNode *elem)
{
    if (sel->count == 0) return false;

    /* Start from the rightmost compound selector (subject). */
    int idx = sel->count - 1;
    if (!match_compound(&sel->compounds[idx], elem)) return false;

    const DomNode *node = elem;
    for (idx = sel->count - 2; idx >= 0; idx--) {
        Combinator comb = sel->combinators[idx];
        bool found = false;

        switch (comb) {
        case COMB_DESCENDANT:
            node = node->parent;
            while (node) {
                if (node->type == PANE_NODE_ELEMENT &&
                    match_compound(&sel->compounds[idx], node)) {
                    found = true;
                    break;
                }
                node = node->parent;
            }
            break;

        case COMB_CHILD:
            node = node->parent;
            if (node && node->type == PANE_NODE_ELEMENT &&
                match_compound(&sel->compounds[idx], node))
                found = true;
            break;

        case COMB_NEXT_SIBLING:
            node = node->prev_sibling;
            /* Skip non-element siblings. */
            while (node && node->type != PANE_NODE_ELEMENT)
                node = node->prev_sibling;
            if (node && match_compound(&sel->compounds[idx], node))
                found = true;
            break;

        case COMB_SUBSEQUENT:
            node = node->prev_sibling;
            while (node) {
                if (node->type == PANE_NODE_ELEMENT &&
                    match_compound(&sel->compounds[idx], node)) {
                    found = true;
                    break;
                }
                node = node->prev_sibling;
            }
            break;

        case COMB_NONE:
            found = true;
            break;
        }

        if (!found) return false;
    }
    return true;
}

bool selector_list_matches(const SelectorList *list, const DomNode *elem)
{
    for (int i = 0; i < list->count; i++) {
        if (selector_matches(&list->selectors[i], elem))
            return true;
    }
    return false;
}

/* ── Selector Parsing ───────────────────────────────────────────────── */

/* Grow a SelectorList's capacity. */
static void sellist_grow(SelectorList *sl, Arena *arena)
{
    int new_cap = sl->cap * 2;
    Selector *new_sels = arena_alloc(arena, new_cap * sizeof(Selector), 8);
    memcpy(new_sels, sl->selectors, sl->count * sizeof(Selector));
    sl->selectors = new_sels;
    sl->cap = new_cap;
}

/* Grow a Selector's compound capacity. */
static void sel_grow(Selector *sel, Arena *arena)
{
    int new_cap = sel->cap * 2;
    CompoundSelector *new_cs = arena_alloc(arena, new_cap * sizeof(CompoundSelector), 8);
    memcpy(new_cs, sel->compounds, sel->count * sizeof(CompoundSelector));
    sel->compounds = new_cs;
    Combinator *new_comb = arena_alloc(arena, new_cap * sizeof(Combinator), 4);
    if (sel->count > 0)
        memcpy(new_comb, sel->combinators, sel->count * sizeof(Combinator));
    sel->combinators = new_comb;
    sel->cap = new_cap;
}

bool selector_parse(const char *input, size_t len, Arena *arena,
                    SelectorList *out)
{
    out->count = 0;
    out->cap = 8;
    out->selectors = arena_alloc(arena, out->cap * sizeof(Selector), 8);

    CssTokenizer t;
    css_tokenizer_init(&t, input, len);

    Selector *cur_sel = &out->selectors[0];
    cur_sel->count = 0;
    cur_sel->cap = 4;
    cur_sel->compounds = arena_alloc(arena, cur_sel->cap * sizeof(CompoundSelector), 8);
    cur_sel->combinators = arena_alloc(arena, cur_sel->cap * sizeof(Combinator), 4);

    CompoundSelector *cur_cs = &cur_sel->compounds[0];
    memset(cur_cs, 0, sizeof(*cur_cs));
    cur_cs->part_cap = 4;
    cur_cs->parts = arena_alloc(arena, cur_cs->part_cap * sizeof(SelectorPart), 8);

    bool in_compound = false;

    CssToken tok;
    while (css_tokenizer_next(&t, &tok)) {
        if (tok.type == CSSTOK_EOF) break;

        if (tok.type == CSSTOK_WHITESPACE) {
            if (in_compound) {
                /* Whitespace = descendant combinator candidate. */
                CssToken peek;
                css_tokenizer_peek(&t, &peek);
                if (peek.type != CSSTOK_EOF && peek.type != CSSTOK_COMMA &&
                    peek.type != CSSTOK_LBRACE &&
                    !(peek.type == CSSTOK_DELIM &&
                      (peek.delim == '>' || peek.delim == '+' || peek.delim == '~'))) {
                    /* Finalize current compound, start new with descendant combinator. */
                    cur_sel->count++;
                    if (cur_sel->count >= cur_sel->cap) {
                        sel_grow(cur_sel, arena);
                    }
                    cur_sel->combinators[cur_sel->count - 1] = COMB_DESCENDANT;
                    cur_cs = &cur_sel->compounds[cur_sel->count];
                    memset(cur_cs, 0, sizeof(*cur_cs));
                    cur_cs->part_cap = 4;
                    cur_cs->parts = arena_alloc(arena, 4 * sizeof(SelectorPart), 8);
                    in_compound = false;
                }
            }
            continue;
        }

        if (tok.type == CSSTOK_COMMA) {
            /* Finalize current selector, start new. */
            if (in_compound) cur_sel->count++;
            out->count++;
            if (out->count >= out->cap) {
                sellist_grow(out, arena);
            }
            cur_sel = &out->selectors[out->count];
            cur_sel->count = 0;
            cur_sel->cap = 4;
            cur_sel->compounds = arena_alloc(arena, 4 * sizeof(CompoundSelector), 8);
            cur_sel->combinators = arena_alloc(arena, 4 * sizeof(Combinator), 4);
            cur_cs = &cur_sel->compounds[0];
            memset(cur_cs, 0, sizeof(*cur_cs));
            cur_cs->part_cap = 4;
            cur_cs->parts = arena_alloc(arena, 4 * sizeof(SelectorPart), 8);
            in_compound = false;
            continue;
        }

        if (tok.type == CSSTOK_DELIM) {
            Combinator comb = COMB_NONE;
            if (tok.delim == '>') comb = COMB_CHILD;
            else if (tok.delim == '+') comb = COMB_NEXT_SIBLING;
            else if (tok.delim == '~') comb = COMB_SUBSEQUENT;
            else if (tok.delim == '*') {
                /* Universal selector. */
                if (cur_cs->part_count < cur_cs->part_cap) {
                    cur_cs->parts[cur_cs->part_count++] = (SelectorPart){
                        .type = SEL_UNIVERSAL
                    };
                }
                in_compound = true;
                continue;
            }
            else if (tok.delim == '.') {
                /* Class selector. */
                if (css_tokenizer_next(&t, &tok) && tok.type == CSSTOK_IDENT) {
                    if (cur_cs->part_count < cur_cs->part_cap) {
                        cur_cs->parts[cur_cs->part_count++] = (SelectorPart){
                            .type = SEL_CLASS,
                            .name = arena_strndup(arena, tok.start, tok.len),
                        };
                    }
                    in_compound = true;
                }
                continue;
            }

            if (comb != COMB_NONE && in_compound) {
                cur_sel->count++;
                if (cur_sel->count >= cur_sel->cap) {
                    sel_grow(cur_sel, arena);
                }
                cur_sel->combinators[cur_sel->count - 1] = comb;
                cur_cs = &cur_sel->compounds[cur_sel->count];
                memset(cur_cs, 0, sizeof(*cur_cs));
                cur_cs->part_cap = 4;
                cur_cs->parts = arena_alloc(arena, 4 * sizeof(SelectorPart), 8);
                in_compound = false;
            }
            continue;
        }

        if (tok.type == CSSTOK_IDENT) {
            /* Type selector. */
            char *name = arena_strndup(arena, tok.start, tok.len);
            str_ascii_lower(name, tok.len);
            if (cur_cs->part_count < cur_cs->part_cap) {
                cur_cs->parts[cur_cs->part_count++] = (SelectorPart){
                    .type = SEL_TYPE, .name = name,
                };
            }
            in_compound = true;
            continue;
        }

        if (tok.type == CSSTOK_HASH) {
            if (cur_cs->part_count < cur_cs->part_cap) {
                cur_cs->parts[cur_cs->part_count++] = (SelectorPart){
                    .type = SEL_ID,
                    .name = arena_strndup(arena, tok.start, tok.len),
                };
            }
            in_compound = true;
            continue;
        }

        if (tok.type == CSSTOK_COLON) {
            /* Pseudo-class or pseudo-element. */
            CssToken next;
            if (!css_tokenizer_next(&t, &next)) break;

            SelectorPartType ptype = SEL_PSEUDO_CLASS;
            if (next.type == CSSTOK_COLON) {
                ptype = SEL_PSEUDO_ELEM;
                if (!css_tokenizer_next(&t, &next)) break;
            }

            if (next.type == CSSTOK_IDENT || next.type == CSSTOK_FUNCTION) {
                if (cur_cs->part_count < cur_cs->part_cap) {
                    cur_cs->parts[cur_cs->part_count++] = (SelectorPart){
                        .type = ptype,
                        .name = arena_strndup(arena, next.start, next.len),
                    };
                }
                /* For function pseudo-classes, include arguments in the name. */
                if (next.type == CSSTOK_FUNCTION) {
                    /* Collect "name(args)" into the name field. */
                    const char *func_start = next.start;
                    int depth = 1;
                    const char *arg_end = func_start + next.len;
                    while (depth > 0 && css_tokenizer_next(&t, &next)) {
                        if (next.type == CSSTOK_LPAREN || next.type == CSSTOK_FUNCTION)
                            depth++;
                        if (next.type == CSSTOK_RPAREN) { depth--; arg_end = next.start + next.len; }
                        if (next.type == CSSTOK_EOF) break;
                    }
                    /* Overwrite name with full "name(args)" string. */
                    size_t full_len = (size_t)(arg_end - func_start);
                    if (full_len > 0 && cur_cs->part_count > 0) {
                        cur_cs->parts[cur_cs->part_count - 1].name =
                            arena_strndup(arena, func_start, full_len);
                    }
                }
            }
            in_compound = true;
            continue;
        }

        if (tok.type == CSSTOK_LBRACKET) {
            /* Attribute selector. */
            CssToken attr_tok;
            if (!css_tokenizer_next(&t, &attr_tok)) break;
            if (attr_tok.type != CSSTOK_IDENT) {
                /* Skip to ]. */
                while (css_tokenizer_next(&t, &attr_tok) &&
                       attr_tok.type != CSSTOK_RBRACKET &&
                       attr_tok.type != CSSTOK_EOF) {}
                continue;
            }

            const char *attr_name = arena_strndup(arena, attr_tok.start, attr_tok.len);

            CssToken op_tok;
            if (!css_tokenizer_next(&t, &op_tok)) break;

            if (op_tok.type == CSSTOK_RBRACKET) {
                if (cur_cs->part_count < cur_cs->part_cap) {
                    cur_cs->parts[cur_cs->part_count++] = (SelectorPart){
                        .type = SEL_ATTR_EXISTS, .name = attr_name,
                    };
                }
                in_compound = true;
                continue;
            }

            /* Skip to value and closing bracket. */
            SelectorPartType stype = SEL_ATTR_EQUALS;
            if (op_tok.type == CSSTOK_DELIM) {
                if (op_tok.delim == '~') stype = SEL_ATTR_CONTAINS;
                if (op_tok.delim == '^') stype = SEL_ATTR_STARTS;
                css_tokenizer_next(&t, &op_tok); /* consume '=' */
            }

            /* Skip whitespace. */
            CssToken val_tok;
            do { css_tokenizer_next(&t, &val_tok); }
            while (val_tok.type == CSSTOK_WHITESPACE);

            const char *val = "";
            if (val_tok.type == CSSTOK_IDENT || val_tok.type == CSSTOK_STRING)
                val = arena_strndup(arena, val_tok.start, val_tok.len);

            /* Skip to ]. */
            while (css_tokenizer_next(&t, &val_tok) &&
                   val_tok.type != CSSTOK_RBRACKET &&
                   val_tok.type != CSSTOK_EOF) {}

            if (cur_cs->part_count < cur_cs->part_cap) {
                cur_cs->parts[cur_cs->part_count++] = (SelectorPart){
                    .type = stype, .name = attr_name, .value = val,
                };
            }
            in_compound = true;
            continue;
        }
    }

    /* Finalize last compound and selector. */
    if (in_compound) cur_sel->count++;
    if (cur_sel->count > 0 || in_compound) out->count++;

    return out->count > 0;
}

void selector_list_free(SelectorList *list)
{
    /* No-op when arena-allocated. */
    (void)list;
}

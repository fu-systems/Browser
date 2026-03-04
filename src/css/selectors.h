/*
 * Pane — CSS Selector Matching
 *
 * Represents and matches CSS selectors against DOM elements.
 */

#ifndef PANE_CSS_SELECTORS_H
#define PANE_CSS_SELECTORS_H

#include "../dom/dom.h"
#include <stdbool.h>

/* ── Selector Components ────────────────────────────────────────────── */

typedef enum {
    SEL_TYPE,          /* element type: div, p, etc. */
    SEL_UNIVERSAL,     /* * */
    SEL_CLASS,         /* .foo */
    SEL_ID,            /* #foo */
    SEL_ATTR_EXISTS,   /* [attr] */
    SEL_ATTR_EQUALS,   /* [attr=val] */
    SEL_ATTR_CONTAINS, /* [attr~=val] */
    SEL_ATTR_STARTS,   /* [attr^=val] */
    SEL_PSEUDO_CLASS,  /* :hover, :first-child, etc. */
    SEL_PSEUDO_ELEM,   /* ::before, ::after */
} SelectorPartType;

typedef struct {
    SelectorPartType type;
    const char      *name;   /* tag name, class, id, attr name, pseudo name */
    const char      *value;  /* for attr selectors */
} SelectorPart;

/* ── Combinators ────────────────────────────────────────────────────── */

typedef enum {
    COMB_NONE,         /* no combinator (compound selector) */
    COMB_DESCENDANT,   /* space */
    COMB_CHILD,        /* > */
    COMB_NEXT_SIBLING, /* + */
    COMB_SUBSEQUENT,   /* ~ */
} Combinator;

/* ── Selector ───────────────────────────────────────────────────────── */

/* A compound selector is a sequence of simple selectors without combinators. */
typedef struct {
    SelectorPart *parts;
    int           part_count;
    int           part_cap;
} CompoundSelector;

/* A complex selector is a chain of compound selectors with combinators. */
typedef struct {
    CompoundSelector *compounds;
    Combinator       *combinators;  /* between compounds[i] and compounds[i+1] */
    int               count;
    int               cap;
} Selector;

/* A selector list (comma-separated selectors). */
typedef struct {
    Selector *selectors;
    int       count;
    int       cap;
} SelectorList;

/* ── Specificity ────────────────────────────────────────────────────── */

typedef struct {
    uint16_t a;  /* ID selectors */
    uint16_t b;  /* class, attribute, pseudo-class selectors */
    uint16_t c;  /* type, pseudo-element selectors */
} Specificity;

/* Compare specificities. Returns <0, 0, >0. */
int specificity_cmp(Specificity a, Specificity b);

/* Compute specificity for a selector. */
Specificity selector_specificity(const Selector *sel);

/* ── Matching ───────────────────────────────────────────────────────── */

/* Test if a selector matches an element. */
bool selector_matches(const Selector *sel, const DomNode *elem);

/* Test if a selector list matches an element. */
bool selector_list_matches(const SelectorList *list, const DomNode *elem);

/* ── Parsing ────────────────────────────────────────────────────────── */

/* Parse a selector string. Uses arena for allocations. */
bool selector_parse(const char *input, size_t len, Arena *arena,
                    SelectorList *out);

/* Free a selector list (only if heap-allocated parts). */
void selector_list_free(SelectorList *list);

#endif /* PANE_CSS_SELECTORS_H */

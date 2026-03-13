/*
 * Pane — Edge-Case Decision Engine
 *
 * Stage 7: Systematic detection and classification of rendering edge cases.
 *
 * Taxonomy of edge-case origins:
 *   EC_PAIRWISE        — Missing pairwise context transition rule
 *   EC_CHAIN           — Chain-sensitive interaction (needs ancestor lookup)
 *   EC_SPEC_AMBIGUITY  — CSS/HTML spec underspecification or conflict
 *   EC_MARKUP_QUIRKS   — Real-world malformed HTML patterns
 *   EC_SHORTHAND       — CSS shorthand expansion gap
 *   EC_PARSER          — HTML/CSS parser limitation
 *   EC_VALUE_RESOLVE   — CSS value resolution (units, percentages, calc)
 *   EC_DISPLAY_COERCE  — Display type coercion (blockification, etc.)
 *
 * Resolution strategies (per category):
 *   PAIRWISE        → Add field to PairwiseContext, add transition rule
 *   CHAIN           → Enrich context with summary field or flag
 *   SPEC_AMBIGUITY  → Document decision, follow majority browser behavior
 *   MARKUP_QUIRKS   → Handle in tree builder's adoption agency / foster parenting
 *   SHORTHAND       → Expand in CSS parser's try_expand_shorthand()
 *   PARSER          → Fix tokenizer/tree builder state machine
 *   VALUE_RESOLVE   → Add resolution path in computed_style_resolve()
 *   DISPLAY_COERCE  → Add rule in blockification fixup
 */

#ifndef PANE_EDGE_CASES_H
#define PANE_EDGE_CASES_H

#include <stdbool.h>
#include <stddef.h>

/* ── Edge-Case Category ────────────────────────────────────────────── */

typedef enum {
    EC_NONE = 0,
    EC_PAIRWISE,          /* Missing pairwise context transition */
    EC_CHAIN,             /* Chain-sensitive (ancestor-dependent) */
    EC_SPEC_AMBIGUITY,    /* Spec ambiguity or underspecification */
    EC_MARKUP_QUIRKS,     /* Malformed real-world HTML */
    EC_SHORTHAND,         /* CSS shorthand expansion gap */
    EC_PARSER,            /* HTML/CSS parser limitation */
    EC_VALUE_RESOLVE,     /* CSS value resolution gap */
    EC_DISPLAY_COERCE,    /* Display type coercion edge case */
    EC_CATEGORY_COUNT
} EdgeCaseCategory;

/* ── Severity ──────────────────────────────────────────────────────── */

typedef enum {
    EC_SEV_LOW,           /* Cosmetic difference only */
    EC_SEV_MEDIUM,        /* Visible layout difference */
    EC_SEV_HIGH,          /* Major layout breakage */
    EC_SEV_CRITICAL,      /* Content invisible or inaccessible */
} EdgeCaseSeverity;

/* ── Resolution Strategy ───────────────────────────────────────────── */

typedef enum {
    EC_RES_CONTEXT_FIELD,   /* Add a context field + transition rule */
    EC_RES_PARSER_FIX,      /* Fix in tokenizer or tree builder */
    EC_RES_SHORTHAND_EXP,   /* Add shorthand expansion rule */
    EC_RES_VALUE_HANDLER,   /* Add value resolution path */
    EC_RES_SPEC_DECISION,   /* Document principled decision */
    EC_RES_COERCION_RULE,   /* Add display coercion rule */
    EC_RES_QUIRKS_HANDLER,  /* Add quirks-mode handling */
    EC_RES_NOT_APPLICABLE,  /* Outside current scope */
} ResolutionStrategy;

/* ── Edge Case Record ──────────────────────────────────────────────── */

typedef struct {
    const char       *id;           /* Unique identifier (e.g., "EC-001") */
    const char       *description;  /* Human-readable description */
    EdgeCaseCategory  category;
    EdgeCaseSeverity  severity;
    ResolutionStrategy resolution;
    const char       *affected_property; /* CSS property or HTML element */
    const char       *test_case;    /* Reproducing HTML snippet */
    bool              resolved;     /* Has this been fixed? */
    const char       *resolution_note; /* How it was resolved */
} EdgeCaseRecord;

/* ── Detection Functions ───────────────────────────────────────────── */

/* Detect if a rendering failure is a pairwise context gap.
 * A pairwise gap means the parent context doesn't carry enough information
 * for the child to lay itself out correctly. */
bool ec_detect_pairwise_gap(const char *parent_display, const char *child_display,
                            const char *failing_property);

/* Detect if a failure requires chain-sensitive ancestor lookup.
 * Chain-sensitive means the child needs information from a grandparent
 * or deeper ancestor, not just the immediate parent. */
bool ec_detect_chain_sensitive(const char *failing_property,
                               bool needs_ancestor_position,
                               bool needs_ancestor_size);

/* Detect if a CSS shorthand property wasn't expanded. */
bool ec_detect_shorthand_gap(const char *property_name);

/* Detect if a parsing issue caused the failure. */
bool ec_detect_parser_issue(const char *html_fragment,
                            bool style_applied,
                            bool element_found);

/* Classify a rendering failure into a category. */
EdgeCaseCategory ec_classify_failure(bool element_found,
                                     bool style_applied,
                                     bool position_correct,
                                     bool size_correct,
                                     const char *parent_display,
                                     const char *child_display);

/* ── Registry ──────────────────────────────────────────────────────── */

/* Get the full catalog of known edge cases. */
const EdgeCaseRecord *ec_get_catalog(int *count);

/* Get category name string. */
const char *ec_category_name(EdgeCaseCategory cat);

/* Get severity name string. */
const char *ec_severity_name(EdgeCaseSeverity sev);

/* Get resolution strategy name. */
const char *ec_resolution_name(ResolutionStrategy res);

/* ── Statistics ─────────────────────────────────────────────────────── */

typedef struct {
    int total_cases;
    int resolved_cases;
    int by_category[EC_CATEGORY_COUNT];
    int by_severity[4]; /* LOW, MEDIUM, HIGH, CRITICAL */
    int pairwise_clean;     /* Cases handled by pairwise model */
    int chain_sensitive;    /* Cases requiring ancestor info */
    int irreducible;        /* Genuine architectural limitations */
} EdgeCaseStats;

/* Compute statistics from the catalog. */
EdgeCaseStats ec_compute_stats(void);

#endif /* PANE_EDGE_CASES_H */

/*
 * Pane — Edge-Case Decision Engine Implementation
 *
 * Stage 7: Catalogs all known edge cases, provides detection heuristics,
 * and defines resolution strategies that avoid ad-hoc patches.
 *
 * Methodology:
 *   1. Each edge case is discovered through test failures (Stage 4/6)
 *   2. Classified into a taxonomy category
 *   3. Assigned a principled resolution strategy
 *   4. Tracked through resolution
 *
 * The pairwise context model handles the vast majority of layout interactions.
 * This catalog documents the exceptions and boundary conditions.
 */

#include "edge_cases.h"
#include <string.h>

/* ── Name Tables ───────────────────────────────────────────────────── */

static const char *category_names[] = {
    [EC_NONE]            = "none",
    [EC_PAIRWISE]        = "pairwise-gap",
    [EC_CHAIN]           = "chain-sensitive",
    [EC_SPEC_AMBIGUITY]  = "spec-ambiguity",
    [EC_MARKUP_QUIRKS]   = "markup-quirks",
    [EC_SHORTHAND]       = "shorthand-gap",
    [EC_PARSER]          = "parser-issue",
    [EC_VALUE_RESOLVE]   = "value-resolution",
    [EC_DISPLAY_COERCE]  = "display-coercion",
};

static const char *severity_names[] = {
    [EC_SEV_LOW]      = "low",
    [EC_SEV_MEDIUM]   = "medium",
    [EC_SEV_HIGH]     = "high",
    [EC_SEV_CRITICAL] = "critical",
};

static const char *resolution_names[] = {
    [EC_RES_CONTEXT_FIELD]  = "context-field",
    [EC_RES_PARSER_FIX]     = "parser-fix",
    [EC_RES_SHORTHAND_EXP]  = "shorthand-expansion",
    [EC_RES_VALUE_HANDLER]  = "value-handler",
    [EC_RES_SPEC_DECISION]  = "spec-decision",
    [EC_RES_COERCION_RULE]  = "coercion-rule",
    [EC_RES_QUIRKS_HANDLER] = "quirks-handler",
    [EC_RES_NOT_APPLICABLE] = "not-applicable",
};

const char *ec_category_name(EdgeCaseCategory cat) {
    if (cat >= 0 && cat < EC_CATEGORY_COUNT)
        return category_names[cat];
    return "unknown";
}

const char *ec_severity_name(EdgeCaseSeverity sev) {
    if (sev >= EC_SEV_LOW && sev <= EC_SEV_CRITICAL)
        return severity_names[sev];
    return "unknown";
}

const char *ec_resolution_name(ResolutionStrategy res) {
    if (res >= EC_RES_CONTEXT_FIELD && res <= EC_RES_NOT_APPLICABLE)
        return resolution_names[res];
    return "unknown";
}

/* ── Edge Case Catalog ─────────────────────────────────────────────── */
/*
 * This catalog records every edge case discovered during development,
 * organized by discovery stage. Each entry documents:
 *   - What went wrong
 *   - Why (root cause category)
 *   - How it was (or should be) resolved
 *   - A minimal reproducing test case
 */

static const EdgeCaseRecord catalog[] = {
    /* ── Stage 4 Discoveries (Layout Test Generation) ──────────────── */

    {
        .id = "EC-001",
        .description = "HTML tokenizer multi-attribute corruption: second attribute "
                       "overwrites first because all TokenAttr.name pointers share "
                       "the same buffer (t->attr_name_buf)",
        .category = EC_PARSER,
        .severity = EC_SEV_CRITICAL,
        .resolution = EC_RES_PARSER_FIX,
        .affected_property = "HTML attributes (id, class, style)",
        .test_case = "<div id=\"parent\" style=\"width:200px\">",
        .resolved = true,
        .resolution_note = "Added per-token attr_data[8192] storage pool; each "
                           "attribute copies name/value into pool (tokenizer.c/h)",
    },
    {
        .id = "EC-002",
        .description = "Margin collapsing double-counts margin.top: cursor_y += collapsed "
                       "then rect.y = cursor_y + margin.top adds it again",
        .category = EC_PAIRWISE,
        .severity = EC_SEV_HIGH,
        .resolution = EC_RES_CONTEXT_FIELD,
        .affected_property = "margin-top, margin-bottom",
        .test_case = "<div style=\"margin:30px 0\"></div><div style=\"margin:20px 0\"></div>",
        .resolved = true,
        .resolution_note = "Fixed block.c: rect.y = cursor_y + collapsed + border.top + "
                           "padding.top (removed duplicate margin.top addition)",
    },
    {
        .id = "EC-003",
        .description = "Flex column passes content_h as width to child layout, "
                       "giving children height-sized widths",
        .category = EC_PAIRWISE,
        .severity = EC_SEV_HIGH,
        .resolution = EC_RES_CONTEXT_FIELD,
        .affected_property = "flex-direction: column",
        .test_case = "<div style=\"display:flex;flex-direction:column;width:300px\">"
                     "<div style=\"width:100px;height:50px\"></div></div>",
        .resolved = true,
        .resolution_note = "Fixed flex.c: always pass content_w as containing width "
                           "for child layout regardless of flex direction",
    },
    {
        .id = "EC-004",
        .description = "Flex auto-height column shrinks items to zero: main_size=0 "
                       "when container height is auto, causing negative free space",
        .category = EC_PAIRWISE,
        .severity = EC_SEV_HIGH,
        .resolution = EC_RES_CONTEXT_FIELD,
        .affected_property = "flex-direction: column; height: auto",
        .test_case = "<div style=\"display:flex;flex-direction:column\">"
                     "<div style=\"height:50px\"></div></div>",
        .resolved = true,
        .resolution_note = "Fixed flex.c: set main_size = total_main for column flex "
                           "with auto height (container grows to fit, no shrinking)",
    },
    {
        .id = "EC-005",
        .description = "Grid layout calls layout_block() for flex children, "
                       "bypassing flex layout entirely",
        .category = EC_PAIRWISE,
        .severity = EC_SEV_HIGH,
        .resolution = EC_RES_CONTEXT_FIELD,
        .affected_property = "display: flex (inside grid)",
        .test_case = "<div style=\"display:grid\"><div style=\"display:flex\">"
                     "<div style=\"width:50px\"></div></div></div>",
        .resolved = true,
        .resolution_note = "Added BOX_FLEX type check in both grid layout dispatch "
                           "sites to call layout_flex() for flex children",
    },

    /* ── Stage 6 Discoveries (Real-World Dataset Analysis) ─────────── */

    {
        .id = "EC-006",
        .description = "CSS shorthand properties (margin, padding, border) stored "
                       "as shorthand IDs but never expanded to longhands; computed "
                       "style resolver only handles longhands",
        .category = EC_SHORTHAND,
        .severity = EC_SEV_CRITICAL,
        .resolution = EC_RES_SHORTHAND_EXP,
        .affected_property = "margin, padding, border, border-width/style/color, "
                             "overflow, flex, flex-flow, gap",
        .test_case = "* { margin: 0; padding: 0; } .box { border: 1px solid #ccc; }",
        .resolved = true,
        .resolution_note = "Added try_expand_shorthand() in css_parser.c: expands "
                           "shorthands at parse time to longhands. Handles 1-4 value "
                           "box syntax, compound border, flex, overflow, gap.",
    },
    {
        .id = "EC-007",
        .description = "Tree builder MODE_BEFORE_HTML doesn't skip whitespace: "
                       "newline between <!DOCTYPE html> and <html> causes premature "
                       "auto-creation of <html>, breaking style application",
        .category = EC_PARSER,
        .severity = EC_SEV_CRITICAL,
        .resolution = EC_RES_PARSER_FIX,
        .affected_property = "All CSS (style tag not processed)",
        .test_case = "<!DOCTYPE html>\\n<html>\\n<head>\\n<style>...",
        .resolved = true,
        .resolution_note = "Added whitespace character handling to handle_before_html() "
                           "in tree_builder.c",
    },

    /* ── Known Unresolved Edge Cases ───────────────────────────────── */

    {
        .id = "EC-008",
        .description = "Multi-value margin/padding shorthand (margin: 10px 20px 30px) "
                       "only parses first token in css_parse_value(); multi-value "
                       "parsing now handled by parse_box_values() in shorthand expansion",
        .category = EC_SHORTHAND,
        .severity = EC_SEV_MEDIUM,
        .resolution = EC_RES_SHORTHAND_EXP,
        .affected_property = "margin, padding (multi-value)",
        .test_case = ".box { margin: 10px 20px 30px 40px; }",
        .resolved = true,
        .resolution_note = "parse_box_values() tokenizes up to 4 values; "
                           "box_sides_from_values() applies CSS 1-4 value logic",
    },
    {
        .id = "EC-009",
        .description = "Percentage height resolution requires definite containing "
                       "block height; auto-height parents make percentage heights "
                       "resolve to auto per spec",
        .category = EC_SPEC_AMBIGUITY,
        .severity = EC_SEV_MEDIUM,
        .resolution = EC_RES_SPEC_DECISION,
        .affected_property = "height (percentage)",
        .test_case = "<div><div style=\"height:50%\">...</div></div>",
        .resolved = false,
        .resolution_note = "Per CSS2.1 §10.5: percentage height with auto parent "
                           "resolves to auto. Context field available_height=-1 "
                           "handles this but some real pages expect the percentage "
                           "to work anyway.",
    },
    {
        .id = "EC-010",
        .description = "Positioned elements (absolute/fixed) should be laid out "
                       "relative to their containing block, not in normal flow. "
                       "Currently treated as normal flow elements.",
        .category = EC_PAIRWISE,
        .severity = EC_SEV_HIGH,
        .resolution = EC_RES_CONTEXT_FIELD,
        .affected_property = "position: absolute/fixed",
        .test_case = "<div style=\"position:relative\"><div style=\"position:absolute;"
                     "top:0;right:0\">...</div></div>",
        .resolved = false,
        .resolution_note = "Requires: (1) removing positioned children from normal flow, "
                           "(2) using positioned_containing_block from context, "
                           "(3) resolving top/right/bottom/left offsets",
    },
    {
        .id = "EC-011",
        .description = "Float layout not implemented: float:left/right elements are "
                       "blockified but remain in normal flow instead of being "
                       "pulled to the side with text wrap",
        .category = EC_PAIRWISE,
        .severity = EC_SEV_HIGH,
        .resolution = EC_RES_CONTEXT_FIELD,
        .affected_property = "float: left/right",
        .test_case = "<img style=\"float:left;width:100px\"><p>Text wraps around...</p>",
        .resolved = false,
        .resolution_note = "Requires float placement algorithm in block layout: "
                           "accumulate float rects, adjust line boxes to avoid floats, "
                           "add clear handling",
    },
    {
        .id = "EC-012",
        .description = "Inline layout produces single-line text boxes only; "
                       "line breaking and word wrapping not implemented",
        .category = EC_PAIRWISE,
        .severity = EC_SEV_MEDIUM,
        .resolution = EC_RES_CONTEXT_FIELD,
        .affected_property = "inline text, word-wrap, overflow-wrap",
        .test_case = "<p style=\"width:100px\">This is a long paragraph that should "
                     "wrap to multiple lines.</p>",
        .resolved = false,
        .resolution_note = "Requires line-breaking algorithm: measure words against "
                           "available width, break into line boxes, handle white-space "
                           "modes (normal, nowrap, pre, pre-wrap)",
    },
    {
        .id = "EC-013",
        .description = "Stacking context ordering not implemented: z-index and "
                       "painting order follow DOM order instead of stacking contexts",
        .category = EC_CHAIN,
        .severity = EC_SEV_MEDIUM,
        .resolution = EC_RES_CONTEXT_FIELD,
        .affected_property = "z-index, stacking context",
        .test_case = "<div style=\"position:relative;z-index:2\">Behind</div>"
                     "<div style=\"position:relative;z-index:1\">In Front</div>",
        .resolved = false,
        .resolution_note = "The stacking_context field is tracked in PairwiseContext "
                           "but not used during painting. Paint phase needs to sort "
                           "by stacking context + z-index.",
    },
    {
        .id = "EC-014",
        .description = "CSS selector combinators (descendant, child, sibling) beyond "
                       "simple class/ID/tag matching have limited support",
        .category = EC_CHAIN,
        .severity = EC_SEV_MEDIUM,
        .resolution = EC_RES_PARSER_FIX,
        .affected_property = "CSS selectors (A B, A > B, A + B, A ~ B)",
        .test_case = ".nav > li { margin: 0; } .card + .card { margin-top: 10px; }",
        .resolved = false,
        .resolution_note = "Selector matching currently supports class, ID, tag, and "
                           "universal selectors. Combinators require parent/sibling "
                           "traversal which is inherently chain-sensitive but bounded "
                           "(only needs immediate context, not full ancestor chain).",
    },
    {
        .id = "EC-015",
        .description = "display:contents should make element invisible to layout "
                       "but promote its children; currently creates anonymous wrapper",
        .category = EC_DISPLAY_COERCE,
        .severity = EC_SEV_LOW,
        .resolution = EC_RES_COERCION_RULE,
        .affected_property = "display: contents",
        .test_case = "<div style=\"display:flex\"><div style=\"display:contents\">"
                     "<div>A</div><div>B</div></div></div>",
        .resolved = true,
        .resolution_note = "Handled in layout.c build_layout_box(): creates anonymous "
                           "wrapper with zeroed margins/padding/border and promotes "
                           "children. Mostly correct for block flow; flex/grid promotion "
                           "may need refinement.",
    },
    {
        .id = "EC-016",
        .description = "CSS calc() expressions not evaluated; stored as keyword "
                       "and resolve to 0",
        .category = EC_VALUE_RESOLVE,
        .severity = EC_SEV_MEDIUM,
        .resolution = EC_RES_VALUE_HANDLER,
        .affected_property = "calc() in width, height, margin, padding, etc.",
        .test_case = ".box { width: calc(100% - 40px); }",
        .resolved = false,
        .resolution_note = "Requires: (1) parsing calc() expression tree in css_parser.c, "
                           "(2) evaluating with context (containing block size, font size) "
                           "in resolve_length(). VAL_FUNCTION type exists but is unused.",
    },
    {
        .id = "EC-017",
        .description = "CSS custom properties (--var) and var() not supported",
        .category = EC_VALUE_RESOLVE,
        .severity = EC_SEV_MEDIUM,
        .resolution = EC_RES_VALUE_HANDLER,
        .affected_property = "var(), custom properties",
        .test_case = ":root { --main-color: #333; } .text { color: var(--main-color); }",
        .resolved = false,
        .resolution_note = "Requires: (1) storing custom properties during cascade, "
                           "(2) resolving var() references during computed style. "
                           "Custom properties are inherited by default.",
    },
    {
        .id = "EC-018",
        .description = "Flex wrap (flex-wrap: wrap) not implemented: items overflow "
                       "instead of wrapping to new line",
        .category = EC_PAIRWISE,
        .severity = EC_SEV_MEDIUM,
        .resolution = EC_RES_CONTEXT_FIELD,
        .affected_property = "flex-wrap: wrap/wrap-reverse",
        .test_case = "<div style=\"display:flex;flex-wrap:wrap;width:300px\">"
                     "<div style=\"width:200px\"></div>"
                     "<div style=\"width:200px\"></div></div>",
        .resolved = false,
        .resolution_note = "flex.c tracks flex_wrap in style but doesn't implement "
                           "multi-line layout. Needs: (1) detect when items exceed "
                           "main size, (2) break into flex lines, (3) distribute cross "
                           "space between lines.",
    },
};

static const int catalog_count = sizeof(catalog) / sizeof(catalog[0]);

/* ── Public API ────────────────────────────────────────────────────── */

const EdgeCaseRecord *ec_get_catalog(int *count) {
    if (count) *count = catalog_count;
    return catalog;
}

/* ── Detection Heuristics ──────────────────────────────────────────── */

bool ec_detect_pairwise_gap(const char *parent_display, const char *child_display,
                            const char *failing_property)
{
    /* A pairwise gap is indicated when the parent context type should
     * influence the child layout but doesn't carry the needed info. */

    /* Flex/grid child in block parent → missing FC transition */
    if (parent_display && child_display) {
        if (strcmp(parent_display, "block") == 0 &&
            (strcmp(child_display, "flex") == 0 ||
             strcmp(child_display, "grid") == 0))
            return true;
    }

    /* Margin collapsing between siblings → needs prev_margin in context */
    if (failing_property && strcmp(failing_property, "margin") == 0)
        return true;

    return false;
}

bool ec_detect_chain_sensitive(const char *failing_property,
                               bool needs_ancestor_position,
                               bool needs_ancestor_size)
{
    /* Chain sensitivity: needs information beyond immediate parent */
    if (needs_ancestor_position || needs_ancestor_size)
        return true;

    /* z-index stacking order requires full stacking context chain */
    if (failing_property && strcmp(failing_property, "z-index") == 0)
        return true;

    /* Selector combinators need sibling/ancestor traversal */
    if (failing_property && strstr(failing_property, "selector"))
        return true;

    return false;
}

bool ec_detect_shorthand_gap(const char *property_name)
{
    if (!property_name) return false;

    /* These are the shorthands we now handle */
    static const char *handled[] = {
        "margin", "padding", "border", "border-width", "border-style",
        "border-color", "border-top", "border-right", "border-bottom",
        "border-left", "overflow", "flex", "flex-flow", "gap", NULL
    };

    for (int i = 0; handled[i]; i++) {
        if (strcmp(property_name, handled[i]) == 0)
            return false; /* handled */
    }

    /* These shorthands are NOT yet handled */
    static const char *unhandled[] = {
        "background", "font", "list-style", "transition", "animation",
        "grid", "grid-template", "grid-area", "place-content",
        "place-items", "place-self", "columns", "outline",
        "text-decoration", NULL
    };

    for (int i = 0; unhandled[i]; i++) {
        if (strcmp(property_name, unhandled[i]) == 0)
            return true;
    }

    return false;
}

bool ec_detect_parser_issue(const char *html_fragment,
                            bool style_applied,
                            bool element_found)
{
    if (!element_found) return true;  /* DOM construction failed */
    if (!style_applied) return true;  /* Style processing failed */
    return false;
}

EdgeCaseCategory ec_classify_failure(bool element_found,
                                     bool style_applied,
                                     bool position_correct,
                                     bool size_correct,
                                     const char *parent_display,
                                     const char *child_display)
{
    /* Parser issue: element not in DOM or styles not applied */
    if (!element_found || !style_applied)
        return EC_PARSER;

    /* Size wrong but position right → likely value resolution or shorthand */
    if (position_correct && !size_correct)
        return EC_VALUE_RESOLVE;

    /* Position wrong → likely pairwise context issue */
    if (!position_correct) {
        /* Check if it's a known pairwise gap */
        if (parent_display && child_display) {
            /* Cross-context transitions */
            if (strcmp(parent_display, "flex") != 0 &&
                strcmp(parent_display, "grid") != 0 &&
                strcmp(child_display, "flex") == 0)
                return EC_PAIRWISE;
        }
        return EC_PAIRWISE;
    }

    return EC_NONE;
}

/* ── Statistics ─────────────────────────────────────────────────────── */

EdgeCaseStats ec_compute_stats(void)
{
    EdgeCaseStats stats = {0};
    stats.total_cases = catalog_count;

    for (int i = 0; i < catalog_count; i++) {
        const EdgeCaseRecord *r = &catalog[i];

        if (r->resolved)
            stats.resolved_cases++;

        if (r->category >= 0 && r->category < EC_CATEGORY_COUNT)
            stats.by_category[r->category]++;

        if (r->severity >= EC_SEV_LOW && r->severity <= EC_SEV_CRITICAL)
            stats.by_severity[r->severity]++;

        /* Classify resolution model */
        if (r->category == EC_CHAIN)
            stats.chain_sensitive++;
        else if (r->resolved || r->category == EC_PAIRWISE ||
                 r->category == EC_SHORTHAND || r->category == EC_PARSER)
            stats.pairwise_clean++;
    }

    /* Irreducible = total - pairwise_clean - chain_sensitive */
    stats.irreducible = stats.total_cases - stats.pairwise_clean - stats.chain_sensitive;
    if (stats.irreducible < 0) stats.irreducible = 0;

    return stats;
}

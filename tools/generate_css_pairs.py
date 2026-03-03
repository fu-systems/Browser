#!/usr/bin/env python3
"""
Generate the exhaustive CSS property × property interaction pair list.
For every pair of CSS properties, describes how they interact when both
are set on the same element — for use in a C-based renderer.
"""

# ═══════════════════════════════════════════════════════════════════════
# COMPLETE CSS PROPERTY LIST — organized by spec module
# ═══════════════════════════════════════════════════════════════════════

ALL_CSS_PROPERTIES = [
    # ─── Display & Box Generation (CSS Display 3) ───
    "display",
    "visibility",
    "opacity",

    # ─── Positioning (CSS Position 3) ───
    "position",
    "top", "right", "bottom", "left",
    "inset",
    "inset-block", "inset-block-start", "inset-block-end",
    "inset-inline", "inset-inline-start", "inset-inline-end",
    "z-index",
    "float", "clear",

    # ─── Box Model — Margin (CSS Box 4) ───
    "margin",
    "margin-top", "margin-right", "margin-bottom", "margin-left",
    "margin-block", "margin-block-start", "margin-block-end",
    "margin-inline", "margin-inline-start", "margin-inline-end",

    # ─── Box Model — Padding ───
    "padding",
    "padding-top", "padding-right", "padding-bottom", "padding-left",
    "padding-block", "padding-block-start", "padding-block-end",
    "padding-inline", "padding-inline-start", "padding-inline-end",

    # ─── Box Model — Sizing (CSS Sizing 3/4) ───
    "width", "height",
    "min-width", "min-height",
    "max-width", "max-height",
    "inline-size", "block-size",
    "min-inline-size", "min-block-size",
    "max-inline-size", "max-block-size",
    "box-sizing",
    "aspect-ratio",

    # ─── Border (CSS Backgrounds & Borders 3) ───
    "border",
    "border-width", "border-style", "border-color",
    "border-top", "border-right", "border-bottom", "border-left",
    "border-top-width", "border-right-width", "border-bottom-width", "border-left-width",
    "border-top-style", "border-right-style", "border-bottom-style", "border-left-style",
    "border-top-color", "border-right-color", "border-bottom-color", "border-left-color",
    "border-block", "border-block-start", "border-block-end",
    "border-inline", "border-inline-start", "border-inline-end",
    "border-block-width", "border-block-start-width", "border-block-end-width",
    "border-inline-width", "border-inline-start-width", "border-inline-end-width",
    "border-block-style", "border-block-start-style", "border-block-end-style",
    "border-inline-style", "border-inline-start-style", "border-inline-end-style",
    "border-block-color", "border-block-start-color", "border-block-end-color",
    "border-inline-color", "border-inline-start-color", "border-inline-end-color",
    "border-radius",
    "border-top-left-radius", "border-top-right-radius",
    "border-bottom-left-radius", "border-bottom-right-radius",
    "border-start-start-radius", "border-start-end-radius",
    "border-end-start-radius", "border-end-end-radius",
    "border-image",
    "border-image-source", "border-image-slice", "border-image-width",
    "border-image-outset", "border-image-repeat",

    # ─── Background (CSS Backgrounds & Borders 3) ───
    "background",
    "background-color", "background-image", "background-position",
    "background-position-x", "background-position-y",
    "background-size", "background-repeat",
    "background-origin", "background-clip", "background-attachment",
    "background-blend-mode",

    # ─── Overflow (CSS Overflow 3) ───
    "overflow",
    "overflow-x", "overflow-y",
    "overflow-block", "overflow-inline",
    "overflow-clip-margin",
    "overflow-anchor",
    "text-overflow",
    "overflow-wrap",

    # ─── Flexbox (CSS Flexbox 1) ───
    "flex",
    "flex-grow", "flex-shrink", "flex-basis",
    "flex-direction", "flex-wrap", "flex-flow",
    "justify-content",
    "align-items", "align-self", "align-content",
    "order",
    "gap", "row-gap", "column-gap",

    # ─── Grid (CSS Grid 1/2) ───
    "grid",
    "grid-template", "grid-template-rows", "grid-template-columns", "grid-template-areas",
    "grid-auto-rows", "grid-auto-columns", "grid-auto-flow",
    "grid-row", "grid-row-start", "grid-row-end",
    "grid-column", "grid-column-start", "grid-column-end",
    "grid-area",
    "place-content", "place-items", "place-self",
    "justify-items", "justify-self",

    # ─── Table (CSS Tables 3) ───
    "border-collapse", "border-spacing",
    "table-layout", "caption-side", "empty-cells",
    "vertical-align",

    # ─── Multi-column (CSS Multicol 1) ───
    "columns", "column-count", "column-width",
    "column-rule", "column-rule-width", "column-rule-style", "column-rule-color",
    "column-fill", "column-span",

    # ─── Font (CSS Fonts 4) ───
    "font",
    "font-family", "font-size", "font-style", "font-weight",
    "font-variant", "font-stretch",
    "font-size-adjust",
    "font-feature-settings", "font-kerning",
    "font-language-override",
    "font-optical-sizing",
    "font-synthesis", "font-synthesis-weight", "font-synthesis-style", "font-synthesis-small-caps",
    "font-variant-alternates", "font-variant-caps",
    "font-variant-east-asian", "font-variant-ligatures",
    "font-variant-numeric", "font-variant-position",
    "font-variation-settings",
    "font-palette",

    # ─── Text (CSS Text 3/4) ───
    "text-align", "text-align-last",
    "text-indent",
    "text-transform",
    "text-decoration",
    "text-decoration-line", "text-decoration-style",
    "text-decoration-color", "text-decoration-thickness",
    "text-underline-offset", "text-underline-position",
    "text-emphasis",
    "text-emphasis-style", "text-emphasis-color", "text-emphasis-position",
    "text-shadow",
    "text-combine-upright",
    "text-orientation",
    "text-rendering",
    "text-wrap",
    "text-wrap-mode", "text-wrap-style",
    "letter-spacing", "word-spacing",
    "line-height",
    "white-space", "white-space-collapse",
    "word-break", "line-break",
    "hyphens",
    "hyphenate-character", "hyphenate-limit-chars",
    "tab-size",
    "word-wrap",  # alias for overflow-wrap

    # ─── Color (CSS Color 4) ───
    "color",
    "color-scheme",
    "forced-color-adjust",
    "print-color-adjust",
    "accent-color",
    "caret-color",

    # ─── Lists (CSS Lists 3) ───
    "list-style",
    "list-style-type", "list-style-position", "list-style-image",
    "counter-reset", "counter-increment", "counter-set",
    "marker-side",

    # ─── Generated Content (CSS Content 3) ───
    "content",
    "quotes",

    # ─── Box Shadow / Outline (CSS UI 4 / Backgrounds) ───
    "box-shadow",
    "outline",
    "outline-width", "outline-style", "outline-color", "outline-offset",

    # ─── Transform (CSS Transforms 1/2) ───
    "transform",
    "transform-origin",
    "transform-style",
    "transform-box",
    "perspective",
    "perspective-origin",
    "backface-visibility",
    "rotate", "scale", "translate",

    # ─── Transitions (CSS Transitions 1) ───
    "transition",
    "transition-property", "transition-duration",
    "transition-timing-function", "transition-delay",
    "transition-behavior",

    # ─── Animations (CSS Animations 1/2) ───
    "animation",
    "animation-name", "animation-duration",
    "animation-timing-function", "animation-delay",
    "animation-iteration-count", "animation-direction",
    "animation-fill-mode", "animation-play-state",
    "animation-timeline",
    "animation-composition",
    "animation-range", "animation-range-start", "animation-range-end",

    # ─── Scroll (CSS Scroll Snap 1, Overscroll 1) ───
    "scroll-behavior",
    "scroll-snap-type", "scroll-snap-align", "scroll-snap-stop",
    "scroll-margin",
    "scroll-margin-top", "scroll-margin-right", "scroll-margin-bottom", "scroll-margin-left",
    "scroll-margin-block", "scroll-margin-block-start", "scroll-margin-block-end",
    "scroll-margin-inline", "scroll-margin-inline-start", "scroll-margin-inline-end",
    "scroll-padding",
    "scroll-padding-top", "scroll-padding-right", "scroll-padding-bottom", "scroll-padding-left",
    "scroll-padding-block", "scroll-padding-block-start", "scroll-padding-block-end",
    "scroll-padding-inline", "scroll-padding-inline-start", "scroll-padding-inline-end",
    "overscroll-behavior",
    "overscroll-behavior-x", "overscroll-behavior-y",
    "overscroll-behavior-block", "overscroll-behavior-inline",
    "scrollbar-width", "scrollbar-color", "scrollbar-gutter",

    # ─── Containment (CSS Contain 1/2/3) ───
    "contain",
    "container", "container-name", "container-type",
    "content-visibility",
    "contain-intrinsic-size",
    "contain-intrinsic-width", "contain-intrinsic-height",
    "contain-intrinsic-inline-size", "contain-intrinsic-block-size",

    # ─── Fragmentation (CSS Break 3) ───
    "break-before", "break-after", "break-inside",
    "orphans", "widows",
    "box-decoration-break",

    # ─── Writing Modes (CSS Writing Modes 4) ───
    "writing-mode",
    "direction",
    "unicode-bidi",

    # ─── Images & Replaced (CSS Images 3/4) ───
    "object-fit", "object-position",
    "image-rendering",
    "image-orientation",

    # ─── Masking & Clipping (CSS Masking 1) ───
    "clip-path",
    "clip-rule",
    "mask",
    "mask-image", "mask-mode", "mask-repeat",
    "mask-position", "mask-clip", "mask-origin",
    "mask-size", "mask-composite", "mask-type",
    "mask-border",
    "mask-border-source", "mask-border-slice", "mask-border-width",
    "mask-border-outset", "mask-border-repeat", "mask-border-mode",

    # ─── Filters & Compositing (CSS Filter Effects 1) ───
    "filter",
    "backdrop-filter",
    "mix-blend-mode",
    "isolation",

    # ─── Shapes (CSS Shapes 1) ───
    "shape-outside",
    "shape-image-threshold",
    "shape-margin",

    # ─── UI (CSS UI 4) ───
    "cursor",
    "pointer-events",
    "touch-action",
    "user-select",
    "resize",
    "appearance",
    "caret",

    # ─── Will-change (CSS Will Change 1) ───
    "will-change",

    # ─── Miscellaneous / All ───
    "all",

    # ─── Ruby (CSS Ruby 1) ───
    "ruby-position", "ruby-align",

    # ─── Motion Path (CSS Motion Path 1) ───
    "offset",
    "offset-path", "offset-distance", "offset-rotate",
    "offset-position", "offset-anchor",

    # ─── Logical Properties additional (CSS Logical 1) ───
    # Most are already listed under their physical equivalents above

    # ─── View Transitions (CSS View Transitions 1) ───
    "view-transition-name",
    "view-transition-class",

    # ─── Anchor Positioning (CSS Anchor Positioning 1) ───
    "anchor-name",
    "position-anchor",
    "position-area",
    "position-try-fallbacks", "position-try-order",
    "position-try",
    "position-visibility",

    # ─── Scroll-driven Animations ───
    "scroll-timeline",
    "scroll-timeline-name", "scroll-timeline-axis",
    "view-timeline",
    "view-timeline-name", "view-timeline-axis", "view-timeline-inset",
    "timeline-scope",

    # ─── Interactivity / Popover ───
    "field-sizing",

    # ─── Custom Properties ───
    # Not listed individually — handled by --* pattern in parsers
]

# Deduplicate and sort
ALL_CSS_PROPERTIES = sorted(set(ALL_CSS_PROPERTIES))

# ═══════════════════════════════════════════════════════════════════════
# PROPERTY METADATA
# ═══════════════════════════════════════════════════════════════════════

# Categories for grouping
PROPERTY_CATEGORY = {}
_categories = {
    "display": ["display"],
    "visibility": ["visibility", "opacity", "content-visibility"],
    "position": [
        "position", "top", "right", "bottom", "left",
        "inset", "inset-block", "inset-block-start", "inset-block-end",
        "inset-inline", "inset-inline-start", "inset-inline-end",
        "z-index",
    ],
    "float": ["float", "clear"],
    "margin": [
        "margin", "margin-top", "margin-right", "margin-bottom", "margin-left",
        "margin-block", "margin-block-start", "margin-block-end",
        "margin-inline", "margin-inline-start", "margin-inline-end",
    ],
    "padding": [
        "padding", "padding-top", "padding-right", "padding-bottom", "padding-left",
        "padding-block", "padding-block-start", "padding-block-end",
        "padding-inline", "padding-inline-start", "padding-inline-end",
    ],
    "sizing": [
        "width", "height", "min-width", "min-height", "max-width", "max-height",
        "inline-size", "block-size", "min-inline-size", "min-block-size",
        "max-inline-size", "max-block-size", "box-sizing", "aspect-ratio",
    ],
    "border": [
        "border", "border-width", "border-style", "border-color",
        "border-top", "border-right", "border-bottom", "border-left",
        "border-top-width", "border-right-width", "border-bottom-width", "border-left-width",
        "border-top-style", "border-right-style", "border-bottom-style", "border-left-style",
        "border-top-color", "border-right-color", "border-bottom-color", "border-left-color",
        "border-block", "border-block-start", "border-block-end",
        "border-inline", "border-inline-start", "border-inline-end",
        "border-block-width", "border-block-start-width", "border-block-end-width",
        "border-inline-width", "border-inline-start-width", "border-inline-end-width",
        "border-block-style", "border-block-start-style", "border-block-end-style",
        "border-inline-style", "border-inline-start-style", "border-inline-end-style",
        "border-block-color", "border-block-start-color", "border-block-end-color",
        "border-inline-color", "border-inline-start-color", "border-inline-end-color",
        "border-radius", "border-top-left-radius", "border-top-right-radius",
        "border-bottom-left-radius", "border-bottom-right-radius",
        "border-start-start-radius", "border-start-end-radius",
        "border-end-start-radius", "border-end-end-radius",
        "border-image", "border-image-source", "border-image-slice",
        "border-image-width", "border-image-outset", "border-image-repeat",
        "border-collapse", "border-spacing",
    ],
    "background": [
        "background", "background-color", "background-image", "background-position",
        "background-position-x", "background-position-y",
        "background-size", "background-repeat",
        "background-origin", "background-clip", "background-attachment",
        "background-blend-mode",
    ],
    "overflow": [
        "overflow", "overflow-x", "overflow-y",
        "overflow-block", "overflow-inline",
        "overflow-clip-margin", "overflow-anchor",
        "text-overflow",
    ],
    "flex": [
        "flex", "flex-grow", "flex-shrink", "flex-basis",
        "flex-direction", "flex-wrap", "flex-flow",
    ],
    "flex-align": [
        "justify-content", "align-items", "align-self", "align-content",
        "order",
    ],
    "gap": ["gap", "row-gap", "column-gap"],
    "grid": [
        "grid", "grid-template", "grid-template-rows", "grid-template-columns",
        "grid-template-areas", "grid-auto-rows", "grid-auto-columns", "grid-auto-flow",
        "grid-row", "grid-row-start", "grid-row-end",
        "grid-column", "grid-column-start", "grid-column-end", "grid-area",
    ],
    "grid-align": [
        "place-content", "place-items", "place-self",
        "justify-items", "justify-self",
    ],
    "table": ["table-layout", "caption-side", "empty-cells", "vertical-align"],
    "multicol": [
        "columns", "column-count", "column-width",
        "column-rule", "column-rule-width", "column-rule-style", "column-rule-color",
        "column-fill", "column-span",
    ],
    "font": [
        "font", "font-family", "font-size", "font-style", "font-weight",
        "font-variant", "font-stretch", "font-size-adjust",
        "font-feature-settings", "font-kerning", "font-language-override",
        "font-optical-sizing",
        "font-synthesis", "font-synthesis-weight", "font-synthesis-style", "font-synthesis-small-caps",
        "font-variant-alternates", "font-variant-caps",
        "font-variant-east-asian", "font-variant-ligatures",
        "font-variant-numeric", "font-variant-position",
        "font-variation-settings", "font-palette",
    ],
    "text": [
        "text-align", "text-align-last", "text-indent",
        "text-transform",
        "text-rendering",
        "text-wrap", "text-wrap-mode", "text-wrap-style",
        "letter-spacing", "word-spacing", "line-height",
        "white-space", "white-space-collapse",
        "word-break", "line-break", "hyphens",
        "hyphenate-character", "hyphenate-limit-chars",
        "tab-size", "overflow-wrap", "word-wrap",
    ],
    "text-decoration": [
        "text-decoration", "text-decoration-line", "text-decoration-style",
        "text-decoration-color", "text-decoration-thickness",
        "text-underline-offset", "text-underline-position",
        "text-emphasis", "text-emphasis-style", "text-emphasis-color", "text-emphasis-position",
        "text-shadow",
    ],
    "text-cjk": ["text-combine-upright", "text-orientation"],
    "color": [
        "color", "color-scheme", "forced-color-adjust",
        "print-color-adjust", "accent-color", "caret-color",
    ],
    "list": [
        "list-style", "list-style-type", "list-style-position", "list-style-image",
        "counter-reset", "counter-increment", "counter-set", "marker-side",
    ],
    "content": ["content", "quotes"],
    "outline": [
        "outline", "outline-width", "outline-style", "outline-color", "outline-offset",
        "box-shadow",
    ],
    "transform": [
        "transform", "transform-origin", "transform-style", "transform-box",
        "perspective", "perspective-origin", "backface-visibility",
        "rotate", "scale", "translate",
    ],
    "transition": [
        "transition", "transition-property", "transition-duration",
        "transition-timing-function", "transition-delay", "transition-behavior",
    ],
    "animation": [
        "animation", "animation-name", "animation-duration",
        "animation-timing-function", "animation-delay",
        "animation-iteration-count", "animation-direction",
        "animation-fill-mode", "animation-play-state",
        "animation-timeline", "animation-composition",
        "animation-range", "animation-range-start", "animation-range-end",
    ],
    "scroll-snap": [
        "scroll-behavior", "scroll-snap-type", "scroll-snap-align", "scroll-snap-stop",
    ],
    "scroll-margin": [
        "scroll-margin",
        "scroll-margin-top", "scroll-margin-right", "scroll-margin-bottom", "scroll-margin-left",
        "scroll-margin-block", "scroll-margin-block-start", "scroll-margin-block-end",
        "scroll-margin-inline", "scroll-margin-inline-start", "scroll-margin-inline-end",
    ],
    "scroll-padding": [
        "scroll-padding",
        "scroll-padding-top", "scroll-padding-right", "scroll-padding-bottom", "scroll-padding-left",
        "scroll-padding-block", "scroll-padding-block-start", "scroll-padding-block-end",
        "scroll-padding-inline", "scroll-padding-inline-start", "scroll-padding-inline-end",
    ],
    "overscroll": [
        "overscroll-behavior", "overscroll-behavior-x", "overscroll-behavior-y",
        "overscroll-behavior-block", "overscroll-behavior-inline",
    ],
    "scrollbar": ["scrollbar-width", "scrollbar-color", "scrollbar-gutter"],
    "contain": [
        "contain", "container", "container-name", "container-type",
        "content-visibility",
        "contain-intrinsic-size",
        "contain-intrinsic-width", "contain-intrinsic-height",
        "contain-intrinsic-inline-size", "contain-intrinsic-block-size",
    ],
    "break": [
        "break-before", "break-after", "break-inside",
        "orphans", "widows", "box-decoration-break",
    ],
    "writing-mode": ["writing-mode", "direction", "unicode-bidi"],
    "image": ["object-fit", "object-position", "image-rendering", "image-orientation"],
    "clip-mask": [
        "clip-path", "clip-rule",
        "mask", "mask-image", "mask-mode", "mask-repeat",
        "mask-position", "mask-clip", "mask-origin",
        "mask-size", "mask-composite", "mask-type",
        "mask-border", "mask-border-source", "mask-border-slice",
        "mask-border-width", "mask-border-outset", "mask-border-repeat", "mask-border-mode",
    ],
    "filter": ["filter", "backdrop-filter", "mix-blend-mode", "isolation"],
    "shape": ["shape-outside", "shape-image-threshold", "shape-margin"],
    "ui": [
        "cursor", "pointer-events", "touch-action",
        "user-select", "resize", "appearance", "caret",
    ],
    "will-change": ["will-change"],
    "all": ["all"],
    "ruby": ["ruby-position", "ruby-align"],
    "motion": [
        "offset", "offset-path", "offset-distance", "offset-rotate",
        "offset-position", "offset-anchor",
    ],
    "view-transition": ["view-transition-name", "view-transition-class"],
    "anchor": [
        "anchor-name", "position-anchor", "position-area",
        "position-try-fallbacks", "position-try-order", "position-try", "position-visibility",
    ],
    "scroll-timeline": [
        "scroll-timeline", "scroll-timeline-name", "scroll-timeline-axis",
        "view-timeline", "view-timeline-name", "view-timeline-axis", "view-timeline-inset",
        "timeline-scope",
    ],
    "field": ["field-sizing"],
}
for cat, props in _categories.items():
    for p in props:
        PROPERTY_CATEGORY[p] = cat

# ═══════════════════════════════════════════════════════════════════════
# SHORTHAND → LONGHAND RELATIONSHIPS
# ═══════════════════════════════════════════════════════════════════════

SHORTHAND_MAP = {
    "margin": ["margin-top", "margin-right", "margin-bottom", "margin-left"],
    "margin-block": ["margin-block-start", "margin-block-end"],
    "margin-inline": ["margin-inline-start", "margin-inline-end"],
    "padding": ["padding-top", "padding-right", "padding-bottom", "padding-left"],
    "padding-block": ["padding-block-start", "padding-block-end"],
    "padding-inline": ["padding-inline-start", "padding-inline-end"],
    "border": [
        "border-top-width", "border-right-width", "border-bottom-width", "border-left-width",
        "border-top-style", "border-right-style", "border-bottom-style", "border-left-style",
        "border-top-color", "border-right-color", "border-bottom-color", "border-left-color",
    ],
    "border-width": ["border-top-width", "border-right-width", "border-bottom-width", "border-left-width"],
    "border-style": ["border-top-style", "border-right-style", "border-bottom-style", "border-left-style"],
    "border-color": ["border-top-color", "border-right-color", "border-bottom-color", "border-left-color"],
    "border-top": ["border-top-width", "border-top-style", "border-top-color"],
    "border-right": ["border-right-width", "border-right-style", "border-right-color"],
    "border-bottom": ["border-bottom-width", "border-bottom-style", "border-bottom-color"],
    "border-left": ["border-left-width", "border-left-style", "border-left-color"],
    "border-block": [
        "border-block-start-width", "border-block-end-width",
        "border-block-start-style", "border-block-end-style",
        "border-block-start-color", "border-block-end-color",
    ],
    "border-block-start": ["border-block-start-width", "border-block-start-style", "border-block-start-color"],
    "border-block-end": ["border-block-end-width", "border-block-end-style", "border-block-end-color"],
    "border-inline": [
        "border-inline-start-width", "border-inline-end-width",
        "border-inline-start-style", "border-inline-end-style",
        "border-inline-start-color", "border-inline-end-color",
    ],
    "border-inline-start": ["border-inline-start-width", "border-inline-start-style", "border-inline-start-color"],
    "border-inline-end": ["border-inline-end-width", "border-inline-end-style", "border-inline-end-color"],
    "border-block-width": ["border-block-start-width", "border-block-end-width"],
    "border-block-style": ["border-block-start-style", "border-block-end-style"],
    "border-block-color": ["border-block-start-color", "border-block-end-color"],
    "border-inline-width": ["border-inline-start-width", "border-inline-end-width"],
    "border-inline-style": ["border-inline-start-style", "border-inline-end-style"],
    "border-inline-color": ["border-inline-start-color", "border-inline-end-color"],
    "border-radius": [
        "border-top-left-radius", "border-top-right-radius",
        "border-bottom-left-radius", "border-bottom-right-radius",
    ],
    "border-image": [
        "border-image-source", "border-image-slice", "border-image-width",
        "border-image-outset", "border-image-repeat",
    ],
    "background": [
        "background-color", "background-image", "background-position",
        "background-size", "background-repeat",
        "background-origin", "background-clip", "background-attachment",
    ],
    "font": ["font-family", "font-size", "font-style", "font-weight", "font-variant", "font-stretch", "line-height"],
    "font-synthesis": ["font-synthesis-weight", "font-synthesis-style", "font-synthesis-small-caps"],
    "font-variant": [
        "font-variant-ligatures", "font-variant-caps",
        "font-variant-numeric", "font-variant-east-asian",
        "font-variant-alternates", "font-variant-position",
    ],
    "text-decoration": ["text-decoration-line", "text-decoration-style", "text-decoration-color", "text-decoration-thickness"],
    "text-emphasis": ["text-emphasis-style", "text-emphasis-color"],
    "text-wrap": ["text-wrap-mode", "text-wrap-style"],
    "flex": ["flex-grow", "flex-shrink", "flex-basis"],
    "flex-flow": ["flex-direction", "flex-wrap"],
    "grid": [
        "grid-template-rows", "grid-template-columns", "grid-template-areas",
        "grid-auto-rows", "grid-auto-columns", "grid-auto-flow",
    ],
    "grid-template": ["grid-template-rows", "grid-template-columns", "grid-template-areas"],
    "grid-row": ["grid-row-start", "grid-row-end"],
    "grid-column": ["grid-column-start", "grid-column-end"],
    "grid-area": ["grid-row-start", "grid-row-end", "grid-column-start", "grid-column-end"],
    "place-content": ["align-content", "justify-content"],
    "place-items": ["align-items", "justify-items"],
    "place-self": ["align-self", "justify-self"],
    "gap": ["row-gap", "column-gap"],
    "overflow": ["overflow-x", "overflow-y"],
    "overscroll-behavior": ["overscroll-behavior-x", "overscroll-behavior-y"],
    "inset": ["top", "right", "bottom", "left"],
    "inset-block": ["inset-block-start", "inset-block-end"],
    "inset-inline": ["inset-inline-start", "inset-inline-end"],
    "outline": ["outline-width", "outline-style", "outline-color"],
    "columns": ["column-count", "column-width"],
    "column-rule": ["column-rule-width", "column-rule-style", "column-rule-color"],
    "list-style": ["list-style-type", "list-style-position", "list-style-image"],
    "transition": ["transition-property", "transition-duration", "transition-timing-function", "transition-delay"],
    "animation": [
        "animation-name", "animation-duration", "animation-timing-function",
        "animation-delay", "animation-iteration-count", "animation-direction",
        "animation-fill-mode", "animation-play-state",
    ],
    "offset": ["offset-path", "offset-distance", "offset-rotate", "offset-position", "offset-anchor"],
    "mask": ["mask-image", "mask-mode", "mask-repeat", "mask-position", "mask-clip", "mask-origin", "mask-size", "mask-composite"],
    "mask-border": ["mask-border-source", "mask-border-slice", "mask-border-width", "mask-border-outset", "mask-border-repeat", "mask-border-mode"],
    "scroll-margin": ["scroll-margin-top", "scroll-margin-right", "scroll-margin-bottom", "scroll-margin-left"],
    "scroll-margin-block": ["scroll-margin-block-start", "scroll-margin-block-end"],
    "scroll-margin-inline": ["scroll-margin-inline-start", "scroll-margin-inline-end"],
    "scroll-padding": ["scroll-padding-top", "scroll-padding-right", "scroll-padding-bottom", "scroll-padding-left"],
    "scroll-padding-block": ["scroll-padding-block-start", "scroll-padding-block-end"],
    "scroll-padding-inline": ["scroll-padding-inline-start", "scroll-padding-inline-end"],
    "container": ["container-name", "container-type"],
    "contain-intrinsic-size": ["contain-intrinsic-width", "contain-intrinsic-height"],
    "scroll-timeline": ["scroll-timeline-name", "scroll-timeline-axis"],
    "view-timeline": ["view-timeline-name", "view-timeline-axis"],
    "animation-range": ["animation-range-start", "animation-range-end"],
    "overflow-block": [],  # not a shorthand, maps to overflow-x/y based on writing-mode
    "overflow-inline": [],
    "position-try": ["position-try-fallbacks", "position-try-order"],
    "caret": ["caret-color"],  # caret shorthand
    "white-space": ["white-space-collapse", "text-wrap-mode"],
}

# Build reverse lookup: longhand → list of shorthands that set it
LONGHAND_TO_SHORTHANDS = {}
for shorthand, longhands in SHORTHAND_MAP.items():
    for lh in longhands:
        LONGHAND_TO_SHORTHANDS.setdefault(lh, []).append(shorthand)

# ═══════════════════════════════════════════════════════════════════════
# LOGICAL ↔ PHYSICAL PROPERTY MAPPINGS
# (which physical property a logical property maps to in default writing-mode)
# ═══════════════════════════════════════════════════════════════════════

LOGICAL_PHYSICAL_PAIRS = {
    "margin-block-start": "margin-top",
    "margin-block-end": "margin-bottom",
    "margin-inline-start": "margin-left",
    "margin-inline-end": "margin-right",
    "padding-block-start": "padding-top",
    "padding-block-end": "padding-bottom",
    "padding-inline-start": "padding-left",
    "padding-inline-end": "padding-right",
    "inset-block-start": "top",
    "inset-block-end": "bottom",
    "inset-inline-start": "left",
    "inset-inline-end": "right",
    "border-block-start-width": "border-top-width",
    "border-block-end-width": "border-bottom-width",
    "border-inline-start-width": "border-left-width",
    "border-inline-end-width": "border-right-width",
    "border-block-start-style": "border-top-style",
    "border-block-end-style": "border-bottom-style",
    "border-inline-start-style": "border-left-style",
    "border-inline-end-style": "border-right-style",
    "border-block-start-color": "border-top-color",
    "border-block-end-color": "border-bottom-color",
    "border-inline-start-color": "border-left-color",
    "border-inline-end-color": "border-right-color",
    "border-start-start-radius": "border-top-left-radius",
    "border-start-end-radius": "border-top-right-radius",
    "border-end-start-radius": "border-bottom-left-radius",
    "border-end-end-radius": "border-bottom-right-radius",
    "inline-size": "width",
    "block-size": "height",
    "min-inline-size": "min-width",
    "min-block-size": "min-height",
    "max-inline-size": "max-width",
    "max-block-size": "max-height",
    "overflow-block": "overflow-y",
    "overflow-inline": "overflow-x",
    "overscroll-behavior-block": "overscroll-behavior-y",
    "overscroll-behavior-inline": "overscroll-behavior-x",
    "contain-intrinsic-inline-size": "contain-intrinsic-width",
    "contain-intrinsic-block-size": "contain-intrinsic-height",
}
# Build reverse
PHYSICAL_TO_LOGICAL = {}
for logical, physical in LOGICAL_PHYSICAL_PAIRS.items():
    PHYSICAL_TO_LOGICAL[physical] = logical

# ═══════════════════════════════════════════════════════════════════════
# DISPLAY-DEPENDENT PROPERTIES
# Properties that only take effect under certain display values
# ═══════════════════════════════════════════════════════════════════════

FLEX_CONTAINER_PROPS = {
    "flex-direction", "flex-wrap", "flex-flow",
    "justify-content", "align-items", "align-content",
}
FLEX_ITEM_PROPS = {
    "flex", "flex-grow", "flex-shrink", "flex-basis",
    "align-self", "order",
}
GRID_CONTAINER_PROPS = {
    "grid", "grid-template", "grid-template-rows", "grid-template-columns",
    "grid-template-areas", "grid-auto-rows", "grid-auto-columns", "grid-auto-flow",
    "justify-items",
}
GRID_ITEM_PROPS = {
    "grid-row", "grid-row-start", "grid-row-end",
    "grid-column", "grid-column-start", "grid-column-end",
    "grid-area", "justify-self",
}
TABLE_ONLY_PROPS = {"border-collapse", "border-spacing", "table-layout", "caption-side", "empty-cells"}
LIST_ITEM_PROPS = {"list-style", "list-style-type", "list-style-position", "list-style-image", "marker-side"}
RUBY_PROPS = {"ruby-position", "ruby-align"}
MULTICOL_CONTAINER_PROPS = {"columns", "column-count", "column-width", "column-fill",
                            "column-rule", "column-rule-width", "column-rule-style", "column-rule-color"}

# ═══════════════════════════════════════════════════════════════════════
# SPEC-DEFINED OVERRIDE RULES
# When two properties interact, one may override/modify the other
# ═══════════════════════════════════════════════════════════════════════

# (A, B) → description of how A affects B
SPECIFIC_OVERRIDES = {
    ("position", "float"):
        "position:absolute/fixed computes float to 'none'. position:relative has no effect on float.",
    ("position", "display"):
        "position:absolute/fixed blockifies display (inline→block, inline-flex→flex, etc.). position:static/relative no effect.",
    ("float", "display"):
        "float (not none) blockifies display (inline→block, inline-table→table, etc.).",
    ("display", "float"):
        "display:flex/grid/contents makes float compute to 'none' on children. display:none makes float irrelevant.",
    ("display", "position"):
        "display:none makes position irrelevant (no box). display:contents removes box but children remain positioned normally.",
    ("display", "vertical-align"):
        "vertical-align only applies to inline-level and table-cell elements. Ignored on block-level.",
    ("display", "margin"):
        "Inline-level elements: vertical margins (top/bottom) have no effect. display:none: all margins irrelevant.",
    ("display", "padding"):
        "Inline-level elements: vertical padding doesn't affect line height or layout (but does render). display:none: irrelevant.",
    ("display", "width"):
        "Inline non-replaced elements: width has no effect. display:none: irrelevant.",
    ("display", "height"):
        "Inline non-replaced elements: height has no effect. display:none: irrelevant.",
    ("display", "overflow"):
        "overflow only applies to block containers and flex/grid containers. Ignored on inline-level.",
    ("overflow", "position"):
        "overflow:hidden/auto/scroll on ancestor doesn't clip position:fixed. overflow:hidden clips position:absolute only if ancestor is containing block.",
    ("overflow", "resize"):
        "resize only works when overflow is not 'visible'. overflow:visible makes resize have no effect.",
    ("position", "margin"):
        "position:absolute/fixed: margins don't collapse. position:static/relative: normal margin collapsing.",
    ("position", "z-index"):
        "z-index only applies to positioned elements (position not static) and flex/grid items.",
    ("display", "z-index"):
        "z-index applies to flex items and grid items regardless of position value.",
    ("float", "margin"):
        "Floated elements: margins never collapse with adjacent elements.",
    ("float", "clear"):
        "clear only has effect on block-level elements. Clears past floated siblings on specified side(s).",
    ("float", "width"):
        "Floated elements shrink-to-fit if width is auto. Width constrains float box.",
    ("display", "clear"):
        "clear only applies to block-level elements. Ignored on inline-level.",
    ("overflow", "text-overflow"):
        "text-overflow only works with overflow:hidden/clip on inline direction. Requires white-space:nowrap or overflow-wrap:normal typically.",
    ("white-space", "text-overflow"):
        "text-overflow typically requires white-space:nowrap to produce ellipsis. With wrapping, text-overflow rarely triggers.",
    ("white-space", "overflow-wrap"):
        "white-space:pre/nowrap prevents wrapping, making overflow-wrap irrelevant. white-space:normal allows overflow-wrap to break words.",
    ("white-space", "word-break"):
        "white-space:nowrap/pre prevents line breaks, making word-break settings irrelevant.",
    ("writing-mode", "direction"):
        "writing-mode sets block flow direction. direction sets inline base direction. Together they determine axis mapping for all logical properties.",
    ("writing-mode", "text-orientation"):
        "text-orientation only applies in vertical writing modes. Ignored in horizontal-tb.",
    ("writing-mode", "text-combine-upright"):
        "text-combine-upright only applies in vertical writing modes. Ignored in horizontal-tb.",
    ("position", "top"):
        "top/right/bottom/left only apply to positioned elements (position not static).",
    ("transform", "position"):
        "An element with transform creates a containing block for all positioned descendants (even position:fixed).",
    ("transform", "z-index"):
        "An element with transform (not none) establishes a stacking context, making z-index meaningful.",
    ("transform", "overflow"):
        "An element with transform establishes a containing block, affecting how overflow clips descendants.",
    ("opacity", "z-index"):
        "opacity < 1 establishes a stacking context, making z-index meaningful even without positioning.",
    ("filter", "z-index"):
        "filter (not none) establishes a stacking context.",
    ("filter", "position"):
        "filter on ancestor creates containing block for position:fixed descendants (breaking out of viewport).",
    ("backdrop-filter", "z-index"):
        "backdrop-filter (not none) establishes a stacking context.",
    ("mix-blend-mode", "isolation"):
        "isolation:isolate creates a stacking context, preventing mix-blend-mode from blending with elements behind the isolated group.",
    ("mix-blend-mode", "z-index"):
        "mix-blend-mode (not normal) establishes a stacking context.",
    ("isolation", "z-index"):
        "isolation:isolate establishes a stacking context.",
    ("will-change", "z-index"):
        "will-change with transform/opacity/filter establishes a stacking context.",
    ("will-change", "position"):
        "will-change:transform creates a containing block for positioned descendants.",
    ("will-change", "overflow"):
        "will-change:transform creates a containing block, affecting overflow clipping.",
    ("contain", "z-index"):
        "contain:layout/paint/strict/content establishes a stacking context.",
    ("contain", "position"):
        "contain:layout/paint creates a containing block for all positioned descendants.",
    ("contain", "overflow"):
        "contain:paint clips overflow (acts like overflow:clip).",
    ("contain", "counter-reset"):
        "contain:style scopes counters within the element.",
    ("contain", "float"):
        "contain:layout contains floats (clearfix behavior).",
    ("content-visibility", "contain"):
        "content-visibility:auto implies contain:layout style paint. content-visibility:hidden implies contain:size layout style paint.",
    ("display", "contain"):
        "display:none makes contain irrelevant. display:contents and contain are incompatible — contain forces a principal box.",
    ("display", "content-visibility"):
        "display:none makes content-visibility irrelevant. display:contents and content-visibility are incompatible.",
    ("border-collapse", "border-spacing"):
        "border-spacing only applies when border-collapse is 'separate'. Ignored with border-collapse:collapse.",
    ("border-collapse", "border-radius"):
        "border-radius on table cells is ignored when border-collapse:collapse.",
    ("position", "break-before"):
        "break-before/after/inside only applies to in-flow elements. Ignored on absolutely/fixed positioned elements.",
    ("float", "break-before"):
        "break-before/after/inside only applies to in-flow elements. Ignored on floats.",
    ("column-span", "column-fill"):
        "column-span:all causes element to span all columns, breaking the column layout at that point.",
    ("display", "column-span"):
        "column-span only applies to in-flow block-level elements inside a multi-column container.",
    ("aspect-ratio", "width"):
        "If both width and height are definite, aspect-ratio is ignored. aspect-ratio resolves the missing dimension.",
    ("aspect-ratio", "height"):
        "If both width and height are definite, aspect-ratio is ignored. aspect-ratio resolves the missing dimension.",
    ("min-width", "width"):
        "min-width wins over width when min-width > width. Effective width = max(min-width, min(max-width, width)).",
    ("max-width", "width"):
        "max-width wins over width when max-width < width. Effective width = max(min-width, min(max-width, width)).",
    ("min-width", "max-width"):
        "When min-width > max-width, min-width wins.",
    ("min-height", "height"):
        "min-height wins over height when min-height > height. Same resolution as width axis.",
    ("max-height", "height"):
        "max-height wins over height when max-height < height.",
    ("min-height", "max-height"):
        "When min-height > max-height, min-height wins.",
    ("box-sizing", "width"):
        "box-sizing:border-box makes width include padding+border. box-sizing:content-box (default) width is content only.",
    ("box-sizing", "height"):
        "box-sizing:border-box makes height include padding+border.",
    ("box-sizing", "padding"):
        "box-sizing:border-box subtracts padding from width/height to get content size.",
    ("box-sizing", "border"):
        "box-sizing:border-box subtracts border from width/height to get content size.",
    ("object-fit", "object-position"):
        "object-position positions content within the box as determined by object-fit. Only meaningful when object-fit creates extra space.",
    ("clip-path", "overflow"):
        "clip-path clips independently of overflow. Both can clip; the intersection is the visible area.",
    ("transform-style", "overflow"):
        "overflow other than visible forces transform-style:flat (breaking preserve-3d).",
    ("transform-style", "filter"):
        "filter (not none) forces transform-style:flat.",
    ("transform-style", "clip-path"):
        "clip-path forces transform-style:flat.",
    ("transform-style", "opacity"):
        "opacity < 1 forces transform-style:flat.",
    ("transform-style", "mix-blend-mode"):
        "mix-blend-mode (not normal) forces transform-style:flat.",
    ("transform-style", "isolation"):
        "isolation:isolate forces transform-style:flat.",
    ("transform-style", "contain"):
        "contain:paint forces transform-style:flat.",
    ("perspective", "transform"):
        "perspective on parent adds perspective projection to children's transforms.",
    ("transform", "backface-visibility"):
        "backface-visibility only meaningful when transform rotates element >90deg on X or Y axis.",
    ("shape-outside", "float"):
        "shape-outside only works on floated elements. Ignored without float.",
    ("shape-margin", "shape-outside"):
        "shape-margin expands the float area defined by shape-outside.",
    ("shape-image-threshold", "shape-outside"):
        "shape-image-threshold defines alpha threshold for shape-outside:url() images.",
    ("animation-name", "transition-property"):
        "Animations take priority over transitions. If both target the same property, the animation wins.",
    ("animation-fill-mode", "animation-play-state"):
        "animation-fill-mode:forwards + animation-play-state:paused keeps the element at the paused keyframe value.",
    ("scroll-snap-type", "scroll-snap-align"):
        "scroll-snap-align on children only takes effect when ancestor has scroll-snap-type set.",
    ("scroll-snap-type", "scroll-padding"):
        "scroll-padding on scroll container defines snap point alignment box. Only relevant with scroll-snap-type.",
    ("scroll-snap-type", "scroll-margin"):
        "scroll-margin on snap children adjusts their snap area. Only relevant with scroll-snap-type on container.",
    ("display", "gap"):
        "gap applies to flex, grid, and multi-column containers. Ignored on other display types.",
    ("display", "justify-content"):
        "justify-content applies to flex and grid containers. Ignored on block/inline.",
    ("display", "align-items"):
        "align-items applies to flex and grid containers. Ignored on block/inline.",
    ("display", "align-content"):
        "align-content applies to flex and grid containers (and block containers in some specs). Ignored on inline.",
    ("display", "order"):
        "order only applies to flex and grid items.",
    ("display", "flex-grow"):
        "flex-grow/shrink/basis only apply to flex items (children of display:flex/inline-flex).",
    ("display", "grid-row"):
        "grid-row/column placement only applies to grid items (children of display:grid/inline-grid).",
    ("position", "order"):
        "position:absolute/fixed removes element from flex/grid flow, making order irrelevant.",
    ("position", "flex-grow"):
        "position:absolute/fixed on flex item: removed from flow. flex-grow/shrink ignored.",
    ("position", "grid-row"):
        "position:absolute/fixed on grid item: removed from flow. grid-row/column behave differently (grid area still applies for positioning).",
    ("visibility", "pointer-events"):
        "visibility:hidden hides element but pointer-events:auto still captures events. pointer-events:none on visible element passes through events.",
    ("display", "pointer-events"):
        "display:none removes from rendering. pointer-events irrelevant.",
    ("resize", "overflow"):
        "resize only works when overflow is not 'visible' or 'clip'. overflow:visible → resize ignored.",
    ("text-decoration-line", "text-decoration-style"):
        "text-decoration-style only applies when text-decoration-line is not 'none'.",
    ("text-decoration-line", "text-decoration-color"):
        "text-decoration-color only applies when text-decoration-line is not 'none'.",
    ("text-decoration-line", "text-decoration-thickness"):
        "text-decoration-thickness only applies when text-decoration-line is not 'none'.",
    ("text-decoration-line", "text-underline-offset"):
        "text-underline-offset only applies when text-decoration-line includes 'underline'.",
    ("text-decoration-line", "text-underline-position"):
        "text-underline-position only applies when text-decoration-line includes 'underline'.",
    ("hyphens", "overflow-wrap"):
        "hyphens:auto and overflow-wrap:break-word both allow breaking. hyphens:auto adds hyphen characters; overflow-wrap just breaks.",
    ("container-type", "container-name"):
        "container-name only meaningful when container-type is not 'normal' (must be size or inline-size).",
    ("container-type", "contain"):
        "container-type:size implies contain:size layout style. container-type:inline-size implies contain:inline-size layout style.",
    ("anchor-name", "position-anchor"):
        "position-anchor references an anchor-name value to establish the anchor element.",
    ("position-anchor", "position-area"):
        "position-area positions element relative to the anchor established by position-anchor.",
    ("position", "position-anchor"):
        "position-anchor only applies to absolutely positioned elements.",
    ("position", "position-area"):
        "position-area only applies to absolutely positioned elements.",
}

# ═══════════════════════════════════════════════════════════════════════
# SAME-AXIS PROPERTY GROUPS (compete on same dimension)
# ═══════════════════════════════════════════════════════════════════════

SAME_AXIS_GROUPS = [
    {"width", "min-width", "max-width", "inline-size", "min-inline-size", "max-inline-size"},
    {"height", "min-height", "max-height", "block-size", "min-block-size", "max-block-size"},
    {"margin-top", "margin-block-start"},
    {"margin-bottom", "margin-block-end"},
    {"margin-left", "margin-inline-start"},
    {"margin-right", "margin-inline-end"},
    {"padding-top", "padding-block-start"},
    {"padding-bottom", "padding-block-end"},
    {"padding-left", "padding-inline-start"},
    {"padding-right", "padding-inline-end"},
    {"top", "inset-block-start"},
    {"bottom", "inset-block-end"},
    {"left", "inset-inline-start"},
    {"right", "inset-inline-end"},
    {"overflow-x", "overflow-inline"},
    {"overflow-y", "overflow-block"},
]

# Build lookup: prop → set of same-axis partners
_same_axis_lookup = {}
for group in SAME_AXIS_GROUPS:
    for p in group:
        _same_axis_lookup.setdefault(p, set()).update(group - {p})

# ═══════════════════════════════════════════════════════════════════════
# INHERITED vs NON-INHERITED
# ═══════════════════════════════════════════════════════════════════════

INHERITED_PROPERTIES = {
    # Text
    "color", "direction", "font", "font-family", "font-size", "font-style",
    "font-weight", "font-variant", "font-stretch", "font-size-adjust",
    "font-feature-settings", "font-kerning", "font-language-override",
    "font-optical-sizing", "font-synthesis", "font-synthesis-weight",
    "font-synthesis-style", "font-synthesis-small-caps",
    "font-variant-alternates", "font-variant-caps", "font-variant-east-asian",
    "font-variant-ligatures", "font-variant-numeric", "font-variant-position",
    "font-variation-settings", "font-palette",
    "text-align", "text-align-last", "text-indent", "text-transform",
    "text-rendering", "text-wrap", "text-wrap-mode", "text-wrap-style",
    "letter-spacing", "word-spacing", "line-height",
    "white-space", "white-space-collapse",
    "word-break", "line-break", "hyphens",
    "hyphenate-character", "hyphenate-limit-chars",
    "tab-size", "overflow-wrap", "word-wrap",
    "text-decoration-skip-ink",
    "text-emphasis-color", "text-emphasis-position", "text-emphasis-style",
    "text-combine-upright", "text-orientation",
    "text-underline-position",
    # Writing mode
    "writing-mode",
    # List
    "list-style", "list-style-type", "list-style-position", "list-style-image",
    # Table
    "border-collapse", "border-spacing", "caption-side", "empty-cells",
    # UI
    "cursor", "caret-color",
    # Visibility
    "visibility",
    # Misc
    "quotes", "orphans", "widows", "image-rendering", "image-orientation",
    "pointer-events", "user-select",
    "color-scheme", "forced-color-adjust", "print-color-adjust",
    "ruby-position", "ruby-align",
    "scrollbar-color", "scrollbar-width",
    "accent-color",
    # counter
    "marker-side",
}

# ═══════════════════════════════════════════════════════════════════════
# INTERACTION CLASSIFICATION ENGINE
# ═══════════════════════════════════════════════════════════════════════

def get_interaction(prop_a, prop_b):
    """
    Return (interaction_type, description) for the pair (prop_a, prop_b).
    """
    # Same property
    if prop_a == prop_b:
        return ("SELF", "Same property. Last declaration in cascade wins.")

    # Check specific known overrides (both directions)
    key_ab = (prop_a, prop_b)
    key_ba = (prop_b, prop_a)
    if key_ab in SPECIFIC_OVERRIDES:
        return ("INTERACTION", SPECIFIC_OVERRIDES[key_ab])
    if key_ba in SPECIFIC_OVERRIDES:
        return ("INTERACTION", SPECIFIC_OVERRIDES[key_ba])

    # Shorthand/longhand
    if prop_a in SHORTHAND_MAP and prop_b in SHORTHAND_MAP.get(prop_a, []):
        return ("SHORTHAND_SETS", f"`{prop_a}` is a shorthand that sets `{prop_b}`. Later declaration (shorthand or longhand) wins per cascade order.")
    if prop_b in SHORTHAND_MAP and prop_a in SHORTHAND_MAP.get(prop_b, []):
        return ("SHORTHAND_SETS", f"`{prop_b}` is a shorthand that sets `{prop_a}`. Later declaration (shorthand or longhand) wins per cascade order.")

    # Both longhands of the same shorthand
    sh_a = set(LONGHAND_TO_SHORTHANDS.get(prop_a, []))
    sh_b = set(LONGHAND_TO_SHORTHANDS.get(prop_b, []))
    common_sh = sh_a & sh_b
    if common_sh:
        sh_str = ", ".join(f"`{s}`" for s in sorted(common_sh))
        return ("SAME_SHORTHAND", f"Both are longhands of {sh_str}. Independent sub-properties — both apply simultaneously.")

    # Logical/physical pairs
    if prop_a in LOGICAL_PHYSICAL_PAIRS and LOGICAL_PHYSICAL_PAIRS[prop_a] == prop_b:
        return ("LOGICAL_PHYSICAL", f"`{prop_a}` (logical) maps to `{prop_b}` (physical) in default writing-mode (horizontal-tb, ltr). Both set same computed value — last in cascade wins.")
    if prop_b in LOGICAL_PHYSICAL_PAIRS and LOGICAL_PHYSICAL_PAIRS[prop_b] == prop_a:
        return ("LOGICAL_PHYSICAL", f"`{prop_b}` (logical) maps to `{prop_a}` (physical) in default writing-mode (horizontal-tb, ltr). Both set same computed value — last in cascade wins.")

    # Same-axis competition
    if prop_b in _same_axis_lookup.get(prop_a, set()):
        return ("SAME_AXIS", f"Both affect the same dimensional axis. In default writing-mode, logical and physical properties map to the same computed value — last in cascade wins.")

    # display-dependent checks
    cat_a = PROPERTY_CATEGORY.get(prop_a, "")
    cat_b = PROPERTY_CATEGORY.get(prop_b, "")

    # Flex container+item pairings
    if prop_a == "display" and prop_b in FLEX_CONTAINER_PROPS:
        return ("DISPLAY_DEPENDENT", f"`{prop_b}` only applies when display is flex/inline-flex. Ignored otherwise.")
    if prop_b == "display" and prop_a in FLEX_CONTAINER_PROPS:
        return ("DISPLAY_DEPENDENT", f"`{prop_a}` only applies when display is flex/inline-flex. Ignored otherwise.")
    if prop_a == "display" and prop_b in FLEX_ITEM_PROPS:
        return ("DISPLAY_DEPENDENT", f"`{prop_b}` only applies to flex items (children of display:flex/inline-flex parent).")
    if prop_b == "display" and prop_a in FLEX_ITEM_PROPS:
        return ("DISPLAY_DEPENDENT", f"`{prop_a}` only applies to flex items (children of display:flex/inline-flex parent).")

    # Grid
    if prop_a == "display" and prop_b in GRID_CONTAINER_PROPS:
        return ("DISPLAY_DEPENDENT", f"`{prop_b}` only applies when display is grid/inline-grid. Ignored otherwise.")
    if prop_b == "display" and prop_a in GRID_CONTAINER_PROPS:
        return ("DISPLAY_DEPENDENT", f"`{prop_a}` only applies when display is grid/inline-grid. Ignored otherwise.")
    if prop_a == "display" and prop_b in GRID_ITEM_PROPS:
        return ("DISPLAY_DEPENDENT", f"`{prop_b}` only applies to grid items (children of display:grid/inline-grid parent).")
    if prop_b == "display" and prop_a in GRID_ITEM_PROPS:
        return ("DISPLAY_DEPENDENT", f"`{prop_a}` only applies to grid items (children of display:grid/inline-grid parent).")

    # Table
    if prop_a == "display" and prop_b in TABLE_ONLY_PROPS:
        return ("DISPLAY_DEPENDENT", f"`{prop_b}` only applies to table elements (display:table or table-*).")
    if prop_b == "display" and prop_a in TABLE_ONLY_PROPS:
        return ("DISPLAY_DEPENDENT", f"`{prop_a}` only applies to table elements (display:table or table-*).")

    # List
    if prop_a == "display" and prop_b in LIST_ITEM_PROPS:
        return ("DISPLAY_DEPENDENT", f"`{prop_b}` only applies to display:list-item elements.")
    if prop_b == "display" and prop_a in LIST_ITEM_PROPS:
        return ("DISPLAY_DEPENDENT", f"`{prop_a}` only applies to display:list-item elements.")

    # Multicol
    if prop_a == "display" and prop_b in MULTICOL_CONTAINER_PROPS:
        return ("DISPLAY_DEPENDENT", f"`{prop_b}` establishes/configures multi-column layout on block containers.")
    if prop_b == "display" and prop_a in MULTICOL_CONTAINER_PROPS:
        return ("DISPLAY_DEPENDENT", f"`{prop_a}` establishes/configures multi-column layout on block containers.")

    # Ruby
    if prop_a == "display" and prop_b in RUBY_PROPS:
        return ("DISPLAY_DEPENDENT", f"`{prop_b}` only applies to ruby elements (display:ruby/ruby-text/etc).")
    if prop_b == "display" and prop_a in RUBY_PROPS:
        return ("DISPLAY_DEPENDENT", f"`{prop_a}` only applies to ruby elements (display:ruby/ruby-text/etc).")

    # Flex-container ↔ flex-item (parent→child relationship)
    if prop_a in FLEX_CONTAINER_PROPS and prop_b in FLEX_ITEM_PROPS:
        return ("PARENT_CHILD_CONTEXT", f"`{prop_a}` (flex container) defines context for `{prop_b}` (flex item on child). Container's value affects how item property is resolved.")
    if prop_b in FLEX_CONTAINER_PROPS and prop_a in FLEX_ITEM_PROPS:
        return ("PARENT_CHILD_CONTEXT", f"`{prop_b}` (flex container) defines context for `{prop_a}` (flex item on child). Container's value affects how item property is resolved.")

    # Grid container ↔ grid item
    if prop_a in GRID_CONTAINER_PROPS and prop_b in GRID_ITEM_PROPS:
        return ("PARENT_CHILD_CONTEXT", f"`{prop_a}` (grid container) defines context for `{prop_b}` (grid item on child).")
    if prop_b in GRID_CONTAINER_PROPS and prop_a in GRID_ITEM_PROPS:
        return ("PARENT_CHILD_CONTEXT", f"`{prop_b}` (grid container) defines context for `{prop_a}` (grid item on child).")

    # `all` resets everything
    if prop_a == "all" or prop_b == "all":
        other = prop_b if prop_a == "all" else prop_a
        if other in ("direction", "unicode-bidi"):
            return ("ALL_EXEMPT", f"`all` does NOT reset `{other}`. Exempt from the 'all' shorthand.")
        return ("ALL_RESETS", f"`all` is a shorthand that resets `{other}` (and all other properties except direction and unicode-bidi).")

    # word-wrap is alias for overflow-wrap
    if {prop_a, prop_b} == {"word-wrap", "overflow-wrap"}:
        return ("ALIAS", "word-wrap is a legacy alias for overflow-wrap. Same computed property.")

    # Same category — likely related but independent sub-properties
    if cat_a == cat_b and cat_a:
        # Within same module but not shorthand/longhand — generally independent
        return ("SAME_MODULE", f"Both in the {cat_a} module. Independent properties that may both contribute to the same visual feature.")

    # Inherited + non-inherited on same element: both just apply, no interaction
    # Cross-category — check for any remaining known relationships
    # writing-mode affects all logical properties
    if prop_a == "writing-mode" and prop_b in LOGICAL_PHYSICAL_PAIRS:
        return ("AXIS_REMAP", f"writing-mode changes which physical axis `{prop_b}` (logical) maps to.")
    if prop_b == "writing-mode" and prop_a in LOGICAL_PHYSICAL_PAIRS:
        return ("AXIS_REMAP", f"writing-mode changes which physical axis `{prop_a}` (logical) maps to.")

    # Default: independent
    return ("INDEPENDENT", "No interaction. Both properties apply independently.")


# ═══════════════════════════════════════════════════════════════════════
# OUTPUT GENERATION
# ═══════════════════════════════════════════════════════════════════════

def generate():
    props = ALL_CSS_PROPERTIES
    total = len(props) * len(props)

    lines = []
    lines.append("# CSS Property × Property Interaction Pairs — Full Exhaustive List")
    lines.append("")
    lines.append("Every individual CSS property pair on its own line.")
    lines.append(f"**{len(props)} properties × {len(props)} properties = {total} pairs.**")
    lines.append("")
    lines.append("For a C-based renderer: describes what happens when both properties")
    lines.append("are set on the same element (or parent/child for context-dependent props).")
    lines.append("")
    lines.append("Interaction types:")
    lines.append("- **SELF** — Same property paired with itself")
    lines.append("- **SHORTHAND_SETS** — One is a shorthand that sets the other")
    lines.append("- **SAME_SHORTHAND** — Both are longhands of a common shorthand")
    lines.append("- **LOGICAL_PHYSICAL** — Logical and physical mapping to same computed value")
    lines.append("- **SAME_AXIS** — Both affect the same dimensional axis")
    lines.append("- **INTERACTION** — Specific known override/conflict/dependency")
    lines.append("- **DISPLAY_DEPENDENT** — One only applies under certain display values")
    lines.append("- **PARENT_CHILD_CONTEXT** — One defines context on parent, other applies on child")
    lines.append("- **ALL_RESETS** — `all` shorthand resets the other property")
    lines.append("- **ALL_EXEMPT** — `all` does NOT reset the other property")
    lines.append("- **ALIAS** — One is a legacy alias for the other")
    lines.append("- **SAME_MODULE** — Same CSS module, both contribute to same visual feature")
    lines.append("- **AXIS_REMAP** — writing-mode changes which axis a logical property maps to")
    lines.append("- **INDEPENDENT** — No interaction, both apply independently")
    lines.append("")

    action_counts = {}
    pair_num = 0

    for prop_a in props:
        cat = PROPERTY_CATEGORY.get(prop_a, "uncategorized")
        lines.append("---")
        lines.append("")
        lines.append(f"## `{prop_a}` (category: {cat})")
        inh = "inherited" if prop_a in INHERITED_PROPERTIES else "non-inherited"
        lines.append(f"Type: {inh}")
        lines.append("")

        for prop_b in props:
            pair_num += 1
            interaction, desc = get_interaction(prop_a, prop_b)
            action_counts[interaction] = action_counts.get(interaction, 0) + 1
            lines.append(f"{pair_num}. `{prop_a}` × `{prop_b}` = **{interaction}** — {desc}")

        lines.append("")

    # Summary
    lines.append("---")
    lines.append("")
    lines.append("## Summary")
    lines.append("")
    lines.append(f"**Total pairs listed:** {pair_num}")
    lines.append(f"**Total properties:** {len(props)}")
    lines.append("")
    lines.append("| Interaction | Count | % |")
    lines.append("|-------------|-------|---|")
    for interaction, count in sorted(action_counts.items(), key=lambda x: -x[1]):
        pct = count / pair_num * 100
        lines.append(f"| {interaction} | {count} | {pct:.1f}% |")
    lines.append(f"| **Total** | **{pair_num}** | **100%** |")
    lines.append("")

    return "\n".join(lines)


if __name__ == "__main__":
    output = generate()
    with open("/home/user/Browser/catalog/css_property_pairs_full.md", "w") as f:
        f.write(output)
    prop_count = len(ALL_CSS_PROPERTIES)
    print(f"Properties: {prop_count}")
    print(f"Pairs: {prop_count * prop_count}")
    print(f"Lines: {output.count(chr(10)) + 1}")

#!/usr/bin/env python3
"""Generate the complete CSS properties catalog from spec data."""
import json

properties = []

def add(prop, module, value_type, initial, inherited, applies_to="all elements",
        fmt_ctx=False, stacking_ctx=False, containing_block=False, animation="by computed value type"):
    properties.append({
        "property": prop,
        "module": module,
        "value_type": value_type,
        "initial_value": initial,
        "inherited": inherited,
        "applies_to": applies_to,
        "creates_formatting_context": fmt_ctx,
        "creates_stacking_context": stacking_ctx,
        "creates_containing_block": containing_block,
        "animation_type": animation
    })

# === DISPLAY & BOX GENERATION ===
add("display", "CSS Display", "<display-outside> || <display-inside> | <display-listitem> | <display-internal> | <display-box> | <display-legacy>", "inline", False, "all elements", True, False, False, "discrete")
add("visibility", "CSS Display", "visible | hidden | collapse", "visible", True, "all elements", False, False, False, "discrete")
add("opacity", "CSS Color", "<number [0,1]>", "1", False, "all elements", False, True, False, "by computed value type")
add("content", "CSS Generated Content", "normal | none | [ <string> | <image> | <counter> | <quote> | <target> | <leader()> ]+", "normal", False, "::before, ::after, ::marker", False, False, False, "discrete")
add("quotes", "CSS Generated Content", "auto | none | [ <string> <string> ]+", "auto", True, "all elements", False, False, False, "discrete")
add("counter-reset", "CSS Lists", "[ <counter-name> <integer>? ]+ | none", "none", False, "all elements", False, False, False, "discrete")
add("counter-increment", "CSS Lists", "[ <counter-name> <integer>? ]+ | none", "none", False, "all elements", False, False, False, "discrete")
add("counter-set", "CSS Lists", "[ <counter-name> <integer>? ]+ | none", "none", False, "all elements", False, False, False, "discrete")

# === BOX MODEL ===
for side in ["", "-top", "-right", "-bottom", "-left"]:
    add(f"margin{side}", "CSS Box Model", "<length> | <percentage> | auto", "0", False,
        "all elements (not table-internal except table-caption)")

for side in ["", "-top", "-right", "-bottom", "-left"]:
    add(f"padding{side}", "CSS Box Model", "<length> | <percentage>", "0", False,
        "all elements (not table-row-group, table-header-group, table-footer-group, table-row, table-column-group, table-column)")

add("width", "CSS Box Sizing", "<length> | <percentage> | auto | min-content | max-content | fit-content(<length-percentage>)", "auto", False, "all elements except non-replaced inline")
add("height", "CSS Box Sizing", "<length> | <percentage> | auto | min-content | max-content | fit-content(<length-percentage>)", "auto", False, "all elements except non-replaced inline")
add("min-width", "CSS Box Sizing", "<length> | <percentage> | auto | min-content | max-content | fit-content(<length-percentage>)", "auto", False, "all elements except non-replaced inline")
add("min-height", "CSS Box Sizing", "<length> | <percentage> | auto | min-content | max-content | fit-content(<length-percentage>)", "auto", False, "all elements except non-replaced inline")
add("max-width", "CSS Box Sizing", "<length> | <percentage> | none | min-content | max-content | fit-content(<length-percentage>)", "none", False, "all elements except non-replaced inline")
add("max-height", "CSS Box Sizing", "<length> | <percentage> | none | min-content | max-content | fit-content(<length-percentage>)", "none", False, "all elements except non-replaced inline")
add("box-sizing", "CSS Box Sizing", "content-box | border-box", "content-box", False, "all elements that accept width or height", False, False, False, "discrete")

# Logical sizing
add("inline-size", "CSS Logical Properties", "<length> | <percentage> | auto | min-content | max-content | fit-content(<length-percentage>)", "auto", False, "same as width/height")
add("block-size", "CSS Logical Properties", "<length> | <percentage> | auto | min-content | max-content | fit-content(<length-percentage>)", "auto", False, "same as width/height")
add("min-inline-size", "CSS Logical Properties", "<length> | <percentage> | auto", "auto", False, "same as min-width")
add("min-block-size", "CSS Logical Properties", "<length> | <percentage> | auto", "auto", False, "same as min-height")
add("max-inline-size", "CSS Logical Properties", "<length> | <percentage> | none", "none", False, "same as max-width")
add("max-block-size", "CSS Logical Properties", "<length> | <percentage> | none", "none", False, "same as max-height")

# Logical margins
for prop in ["margin-block", "margin-block-start", "margin-block-end", "margin-inline", "margin-inline-start", "margin-inline-end"]:
    add(prop, "CSS Logical Properties", "<length> | <percentage> | auto", "0", False, "same as margin")

# Logical padding
for prop in ["padding-block", "padding-block-start", "padding-block-end", "padding-inline", "padding-inline-start", "padding-inline-end"]:
    add(prop, "CSS Logical Properties", "<length> | <percentage>", "0", False, "same as padding")

# === POSITIONING ===
add("position", "CSS Positioning", "static | relative | absolute | sticky | fixed", "static", False, "all elements", True, True, True, "discrete")
for prop in ["top", "right", "bottom", "left"]:
    add(prop, "CSS Positioning", "<length> | <percentage> | auto", "auto", False, "positioned elements")
add("inset", "CSS Logical Properties", "<length> | <percentage> | auto", "auto", False, "positioned elements")
for prop in ["inset-block", "inset-block-start", "inset-block-end", "inset-inline", "inset-inline-start", "inset-inline-end"]:
    add(prop, "CSS Logical Properties", "<length> | <percentage> | auto", "auto", False, "positioned elements")
add("z-index", "CSS Positioning", "auto | <integer>", "auto", False, "positioned elements", False, True, False)
add("float", "CSS Positioning", "none | left | right | inline-start | inline-end", "none", False, "all elements (not absolutely positioned)", True, False, False, "discrete")
add("clear", "CSS Positioning", "none | left | right | both | inline-start | inline-end", "none", False, "block-level elements", False, False, False, "discrete")

# === OVERFLOW & CLIPPING ===
add("overflow", "CSS Overflow", "[ visible | hidden | clip | scroll | auto ]{1,2}", "visible", False, "block containers, flex containers, grid containers", True, False, False, "discrete")
add("overflow-x", "CSS Overflow", "visible | hidden | clip | scroll | auto", "visible", False, "block containers, flex containers, grid containers", True, False, False, "discrete")
add("overflow-y", "CSS Overflow", "visible | hidden | clip | scroll | auto", "visible", False, "block containers, flex containers, grid containers", True, False, False, "discrete")
add("overflow-block", "CSS Overflow", "visible | hidden | clip | scroll | auto", "visible", False, "block containers", True, False, False, "discrete")
add("overflow-inline", "CSS Overflow", "visible | hidden | clip | scroll | auto", "visible", False, "block containers", True, False, False, "discrete")
add("overflow-wrap", "CSS Text", "normal | break-word | anywhere", "normal", True)
add("text-overflow", "CSS Overflow", "clip | ellipsis | <string>", "clip", False, "block containers", False, False, False, "discrete")
add("clip-path", "CSS Masking", "<clip-source> | <basic-shape> | <geometry-box> | none", "none", False, "all elements (SVG: container and graphics)")
add("clip", "CSS Masking", "<rect()> | auto", "auto", False, "absolutely positioned elements")

# === BACKGROUNDS & BORDERS ===
add("background", "CSS Backgrounds", "<bg-layer># , <final-bg-layer>", "see individual properties", False, "all elements", False, False, False, "see individual properties")
add("background-color", "CSS Backgrounds", "<color>", "transparent", False)
add("background-image", "CSS Backgrounds", "<bg-image>#", "none", False, "all elements", False, False, False, "discrete")
add("background-repeat", "CSS Backgrounds", "<repeat-style>#", "repeat", False, "all elements", False, False, False, "discrete")
add("background-position", "CSS Backgrounds", "<bg-position>#", "0% 0%", False)
add("background-size", "CSS Backgrounds", "<bg-size>#", "auto", False)
add("background-origin", "CSS Backgrounds", "<box>#", "padding-box", False, "all elements", False, False, False, "discrete")
add("background-clip", "CSS Backgrounds", "<box># | text", "border-box", False, "all elements", False, False, False, "discrete")
add("background-attachment", "CSS Backgrounds", "<attachment>#", "scroll", False, "all elements", False, False, False, "discrete")

add("border", "CSS Backgrounds", "<line-width> || <line-style> || <color>", "see individual properties", False, "all elements", False, False, False, "see individual properties")
for side in ["top", "right", "bottom", "left"]:
    add(f"border-{side}", "CSS Backgrounds", "<line-width> || <line-style> || <color>", "see individual properties", False, "all elements", False, False, False, "see individual properties")
    add(f"border-{side}-color", "CSS Backgrounds", "<color>", "currentcolor", False)
    add(f"border-{side}-style", "CSS Backgrounds", "<line-style>", "none", False, "all elements", False, False, False, "discrete")
    add(f"border-{side}-width", "CSS Backgrounds", "<line-width>", "medium", False)

# Logical borders
for side in ["block", "block-start", "block-end", "inline", "inline-start", "inline-end"]:
    add(f"border-{side}", "CSS Logical Properties", "<line-width> || <line-style> || <color>", "see individual properties", False, "all elements", False, False, False, "see individual properties")
    add(f"border-{side}-color", "CSS Logical Properties", "<color>", "currentcolor", False)
    add(f"border-{side}-style", "CSS Logical Properties", "<line-style>", "none", False, "all elements", False, False, False, "discrete")
    add(f"border-{side}-width", "CSS Logical Properties", "<line-width>", "medium", False)

add("border-color", "CSS Backgrounds", "<color>{1,4}", "currentcolor", False)
add("border-style", "CSS Backgrounds", "<line-style>{1,4}", "none", False, "all elements", False, False, False, "discrete")
add("border-width", "CSS Backgrounds", "<line-width>{1,4}", "medium", False)

add("border-radius", "CSS Backgrounds", "<length-percentage>{1,4} [ / <length-percentage>{1,4} ]?", "0", False, "all elements (not table-internal when border-collapse: collapse)")
for corner in ["top-left", "top-right", "bottom-right", "bottom-left"]:
    add(f"border-{corner}-radius", "CSS Backgrounds", "<length-percentage>{1,2}", "0", False)
for corner in ["start-start", "start-end", "end-start", "end-end"]:
    add(f"border-{corner}-radius", "CSS Logical Properties", "<length-percentage>{1,2}", "0", False)

add("border-image", "CSS Backgrounds", "<border-image-source> || <border-image-slice> [ / <border-image-width> | / <border-image-width>? / <border-image-outset> ]? || <border-image-repeat>", "see individual properties", False, "all elements", False, False, False, "see individual properties")
add("border-image-source", "CSS Backgrounds", "none | <image>", "none", False, "all elements", False, False, False, "discrete")
add("border-image-slice", "CSS Backgrounds", "<number-percentage>{1,4} && fill?", "100%", False)
add("border-image-width", "CSS Backgrounds", "[ <length-percentage> | <number> | auto ]{1,4}", "1", False)
add("border-image-outset", "CSS Backgrounds", "[ <length> | <number> ]{1,4}", "0", False)
add("border-image-repeat", "CSS Backgrounds", "[ stretch | repeat | round | space ]{1,2}", "stretch", False, "all elements", False, False, False, "discrete")

add("outline", "CSS UI", "<outline-color> || <outline-style> || <outline-width>", "see individual properties", False, "all elements", False, False, False, "see individual properties")
add("outline-color", "CSS UI", "<color> | invert", "invert", False)
add("outline-style", "CSS UI", "auto | <line-style>", "none", False, "all elements", False, False, False, "discrete")
add("outline-width", "CSS UI", "<line-width>", "medium", False)
add("outline-offset", "CSS UI", "<length>", "0", False)
add("box-shadow", "CSS Backgrounds", "none | <shadow>#", "none", False)
add("box-decoration-break", "CSS Fragmentation", "slice | clone", "slice", False, "all elements", False, False, False, "discrete")

# === COLORS ===
add("color", "CSS Color", "<color>", "canvastext", True)
add("accent-color", "CSS UI", "auto | <color>", "auto", True)
add("caret-color", "CSS UI", "auto | <color>", "auto", True)
add("color-scheme", "CSS Color Adjust", "normal | [ light | dark | <custom-ident> ]+ && only?", "normal", True, "all elements", False, False, False, "discrete")
add("forced-color-adjust", "CSS Color Adjust", "auto | none", "auto", True, "all elements", False, False, False, "discrete")
add("print-color-adjust", "CSS Color Adjust", "economy | exact", "economy", True, "all elements", False, False, False, "discrete")

# === FONTS ===
add("font", "CSS Fonts", "[ <font-style> || <font-variant-css2> || <font-weight> || <font-stretch-css3> ]? <font-size> [ / <line-height> ]? <font-family>", "see individual properties", True, "all elements", False, False, False, "see individual properties")
add("font-family", "CSS Fonts", "[ <family-name> | <generic-family> ]#", "depends on UA", True, "all elements", False, False, False, "discrete")
add("font-size", "CSS Fonts", "<absolute-size> | <relative-size> | <length-percentage>", "medium", True)
add("font-style", "CSS Fonts", "normal | italic | oblique <angle>?", "normal", True)
add("font-variant", "CSS Fonts", "normal | none | [ <common-lig-values> || <discretionary-lig-values> || ... ]", "normal", True, "all elements", False, False, False, "discrete")
add("font-weight", "CSS Fonts", "<font-weight-absolute> | bolder | lighter", "normal", True)
add("font-stretch", "CSS Fonts", "<percentage> | <font-stretch-absolute>", "normal", True)
add("font-feature-settings", "CSS Fonts", "normal | <feature-tag-value>#", "normal", True, "all elements", False, False, False, "discrete")
add("font-kerning", "CSS Fonts", "auto | normal | none", "auto", True, "all elements", False, False, False, "discrete")
add("font-language-override", "CSS Fonts", "normal | <string>", "normal", True, "all elements", False, False, False, "discrete")
add("font-optical-sizing", "CSS Fonts", "auto | none", "auto", True, "all elements", False, False, False, "discrete")
add("font-palette", "CSS Fonts", "normal | light | dark | <palette-identifier>", "normal", True, "all elements", False, False, False, "discrete")
add("font-size-adjust", "CSS Fonts", "none | <number>", "none", True)
add("font-synthesis", "CSS Fonts", "none | [ weight || style || small-caps ]", "weight style small-caps", True, "all elements", False, False, False, "discrete")
add("font-variant-alternates", "CSS Fonts", "normal | [ stylistic(<feature-value-name>) || ... ]", "normal", True, "all elements", False, False, False, "discrete")
add("font-variant-caps", "CSS Fonts", "normal | small-caps | all-small-caps | petite-caps | all-petite-caps | unicase | titling-caps", "normal", True, "all elements", False, False, False, "discrete")
add("font-variant-east-asian", "CSS Fonts", "normal | [ <east-asian-variant-values> || <east-asian-width-values> || ruby ]", "normal", True, "all elements", False, False, False, "discrete")
add("font-variant-ligatures", "CSS Fonts", "normal | none | [ <common-lig-values> || <discretionary-lig-values> || <historical-lig-values> || <contextual-alt-values> ]", "normal", True, "all elements", False, False, False, "discrete")
add("font-variant-numeric", "CSS Fonts", "normal | [ <numeric-figure-values> || <numeric-spacing-values> || <numeric-fraction-values> || ordinal || slashed-zero ]", "normal", True, "all elements", False, False, False, "discrete")
add("font-variant-position", "CSS Fonts", "normal | sub | super", "normal", True, "all elements", False, False, False, "discrete")
add("font-variation-settings", "CSS Fonts", "normal | [ <string> <number> ]#", "normal", True)
add("line-height", "CSS Inline", "normal | <number> | <length> | <percentage>", "normal", True)

# === TEXT ===
add("text-align", "CSS Text", "start | end | left | right | center | justify | match-parent | justify-all", "start", True, "block containers", False, False, False, "discrete")
add("text-align-last", "CSS Text", "auto | start | end | left | right | center | justify", "auto", True, "block containers", False, False, False, "discrete")
add("text-decoration", "CSS Text Decoration", "<text-decoration-line> || <text-decoration-style> || <text-decoration-color> || <text-decoration-thickness>", "see individual properties", False, "all elements", False, False, False, "see individual properties")
add("text-decoration-line", "CSS Text Decoration", "none | [ underline || overline || line-through || blink ]", "none", False, "all elements", False, False, False, "discrete")
add("text-decoration-style", "CSS Text Decoration", "solid | double | dotted | dashed | wavy", "solid", False, "all elements", False, False, False, "discrete")
add("text-decoration-color", "CSS Text Decoration", "<color>", "currentcolor", False)
add("text-decoration-thickness", "CSS Text Decoration", "auto | from-font | <length> | <percentage>", "auto", False)
add("text-emphasis", "CSS Text Decoration", "<text-emphasis-style> || <text-emphasis-color>", "see individual properties", False, "all elements", False, False, False, "see individual properties")
add("text-emphasis-style", "CSS Text Decoration", "none | [ [ filled | open ] || [ dot | circle | double-circle | triangle | sesame ] ] | <string>", "none", False, "all elements", False, False, False, "discrete")
add("text-emphasis-color", "CSS Text Decoration", "<color>", "currentcolor", False)
add("text-emphasis-position", "CSS Text Decoration", "[ over | under ] && [ right | left ]?", "over right", True, "all elements", False, False, False, "discrete")
add("text-indent", "CSS Text", "<length-percentage> && hanging? && each-line?", "0", True, "block containers")
add("text-justify", "CSS Text", "auto | inter-character | inter-word | none", "auto", True, "block containers and inline boxes", False, False, False, "discrete")
add("text-shadow", "CSS Text Decoration", "none | <shadow>#", "none", True)
add("text-transform", "CSS Text", "none | [ capitalize | uppercase | lowercase ] || full-width || full-size-kana", "none", True, "all elements", False, False, False, "discrete")
add("text-underline-offset", "CSS Text Decoration", "auto | <length> | <percentage>", "auto", True)
add("text-underline-position", "CSS Text Decoration", "auto | [ under || [ left | right ] ]", "auto", True, "all elements", False, False, False, "discrete")
add("letter-spacing", "CSS Text", "normal | <length>", "normal", True)
add("word-spacing", "CSS Text", "normal | <length>", "normal", True)
add("word-break", "CSS Text", "normal | break-all | keep-all | break-word", "normal", True, "all elements", False, False, False, "discrete")
add("word-wrap", "CSS Text", "normal | break-word | anywhere", "normal", True, "all elements", False, False, False, "discrete")
add("white-space", "CSS Text", "normal | pre | nowrap | pre-wrap | pre-line | break-spaces", "normal", True, "all elements", False, False, False, "discrete")
add("white-space-collapse", "CSS Text", "collapse | discard | preserve | preserve-breaks | preserve-spaces | break-spaces", "collapse", True, "all elements", False, False, False, "discrete")
add("text-wrap", "CSS Text", "wrap | nowrap | balance | pretty | stable", "wrap", True, "all elements", False, False, False, "discrete")
add("tab-size", "CSS Text", "<number> | <length>", "8", True, "block containers")
add("hyphens", "CSS Text", "none | manual | auto", "manual", True, "all elements", False, False, False, "discrete")
add("hanging-punctuation", "CSS Text", "none | [ first || [ force-end | allow-end ] || last ]", "none", True, "inline elements", False, False, False, "discrete")
add("line-break", "CSS Text", "auto | loose | normal | strict | anywhere", "auto", True, "all elements", False, False, False, "discrete")

# === FLEXBOX ===
add("flex", "CSS Flexbox", "none | [ <flex-grow> <flex-shrink>? || <flex-basis> ]", "0 1 auto", False, "flex items", False, False, False, "see individual properties")
add("flex-grow", "CSS Flexbox", "<number>", "0", False, "flex items")
add("flex-shrink", "CSS Flexbox", "<number>", "1", False, "flex items")
add("flex-basis", "CSS Flexbox", "content | <width>", "auto", False, "flex items")
add("flex-direction", "CSS Flexbox", "row | row-reverse | column | column-reverse", "row", False, "flex containers", False, False, False, "discrete")
add("flex-flow", "CSS Flexbox", "<flex-direction> || <flex-wrap>", "row nowrap", False, "flex containers", False, False, False, "discrete")
add("flex-wrap", "CSS Flexbox", "nowrap | wrap | wrap-reverse", "nowrap", False, "flex containers", False, False, False, "discrete")
add("justify-content", "CSS Box Alignment", "normal | <content-distribution> | <overflow-position>? [ <content-position> | left | right ]", "normal", False, "flex containers, grid containers, multicol containers", False, False, False, "discrete")
add("align-content", "CSS Box Alignment", "normal | <baseline-position> | <content-distribution> | <overflow-position>? <content-position>", "normal", False, "flex containers, grid containers, multicol containers, block containers", False, False, False, "discrete")
add("align-items", "CSS Box Alignment", "normal | stretch | <baseline-position> | [ <overflow-position>? <self-position> ]", "normal", False, "flex containers, grid containers", False, False, False, "discrete")
add("align-self", "CSS Box Alignment", "auto | normal | stretch | <baseline-position> | [ <overflow-position>? <self-position> ]", "auto", False, "flex items, grid items, absolutely-positioned boxes", False, False, False, "discrete")
add("order", "CSS Flexbox", "<integer>", "0", False, "flex items, grid items")
add("gap", "CSS Box Alignment", "<row-gap> <column-gap>?", "normal", False, "flex containers, grid containers, multicol containers")
add("row-gap", "CSS Box Alignment", "normal | <length-percentage>", "normal", False, "flex containers, grid containers, multicol containers")
add("column-gap", "CSS Box Alignment", "normal | <length-percentage>", "normal", False, "flex containers, grid containers, multicol containers")

# === GRID ===
add("grid", "CSS Grid", "<grid-template> | <grid-template-rows> / [ auto-flow && dense? ] <grid-auto-columns>?", "see individual properties", False, "grid containers", False, False, False, "see individual properties")
add("grid-template-rows", "CSS Grid", "none | <track-list> | <auto-track-list> | subgrid <line-name-list>?", "none", False, "grid containers", False, False, False, "discrete")
add("grid-template-columns", "CSS Grid", "none | <track-list> | <auto-track-list> | subgrid <line-name-list>?", "none", False, "grid containers", False, False, False, "discrete")
add("grid-template-areas", "CSS Grid", "none | <string>+", "none", False, "grid containers", False, False, False, "discrete")
add("grid-template", "CSS Grid", "none | [ <grid-template-rows> / <grid-template-columns> ]", "see individual properties", False, "grid containers", False, False, False, "see individual properties")
add("grid-auto-rows", "CSS Grid", "<track-size>+", "auto", False, "grid containers", False, False, False, "discrete")
add("grid-auto-columns", "CSS Grid", "<track-size>+", "auto", False, "grid containers", False, False, False, "discrete")
add("grid-auto-flow", "CSS Grid", "[ row | column ] || dense", "row", False, "grid containers", False, False, False, "discrete")
add("grid-area", "CSS Grid", "<grid-line> [ / <grid-line> ]{0,3}", "auto", False, "grid items", False, False, False, "discrete")
add("grid-row", "CSS Grid", "<grid-line> [ / <grid-line> ]?", "auto", False, "grid items", False, False, False, "discrete")
add("grid-row-start", "CSS Grid", "<grid-line>", "auto", False, "grid items", False, False, False, "discrete")
add("grid-row-end", "CSS Grid", "<grid-line>", "auto", False, "grid items", False, False, False, "discrete")
add("grid-column", "CSS Grid", "<grid-line> [ / <grid-line> ]?", "auto", False, "grid items", False, False, False, "discrete")
add("grid-column-start", "CSS Grid", "<grid-line>", "auto", False, "grid items", False, False, False, "discrete")
add("grid-column-end", "CSS Grid", "<grid-line>", "auto", False, "grid items", False, False, False, "discrete")
add("justify-items", "CSS Box Alignment", "normal | stretch | <baseline-position> | [ <overflow-position>? <self-position> ] | legacy", "legacy", False, "grid containers, flex containers", False, False, False, "discrete")
add("justify-self", "CSS Box Alignment", "auto | normal | stretch | <baseline-position> | [ <overflow-position>? <self-position> ]", "auto", False, "grid items, block-level boxes, absolutely-positioned boxes", False, False, False, "discrete")
add("place-content", "CSS Box Alignment", "<align-content> <justify-content>?", "normal", False, "flex containers, grid containers, multicol containers", False, False, False, "discrete")
add("place-items", "CSS Box Alignment", "<align-items> <justify-items>?", "normal legacy", False, "flex containers, grid containers", False, False, False, "discrete")
add("place-self", "CSS Box Alignment", "<align-self> <justify-self>?", "auto", False, "flex items, grid items", False, False, False, "discrete")

# === TABLE ===
add("table-layout", "CSS Table", "auto | fixed", "auto", False, "table elements", False, False, False, "discrete")
add("border-collapse", "CSS Table", "separate | collapse", "separate", True, "table elements", False, False, False, "discrete")
add("border-spacing", "CSS Table", "<length> <length>?", "0px 0px", True, "table elements")
add("caption-side", "CSS Table", "top | bottom", "top", True, "table-caption elements", False, False, False, "discrete")
add("empty-cells", "CSS Table", "show | hide", "show", True, "table-cell elements", False, False, False, "discrete")
add("vertical-align", "CSS Inline", "baseline | sub | super | text-top | text-bottom | middle | top | bottom | <percentage> | <length>", "baseline", False, "inline-level and table-cell elements")

# === LISTS ===
add("list-style", "CSS Lists", "<list-style-position> || <list-style-image> || <list-style-type>", "see individual properties", True, "list items", False, False, False, "see individual properties")
add("list-style-type", "CSS Lists", "<counter-style> | <string> | none", "disc", True, "list items", False, False, False, "discrete")
add("list-style-position", "CSS Lists", "inside | outside", "outside", True, "list items", False, False, False, "discrete")
add("list-style-image", "CSS Lists", "<image> | none", "none", True, "list items", False, False, False, "discrete")

# === MULTI-COLUMN ===
add("columns", "CSS Multi-column", "<column-width> || <column-count>", "see individual properties", False, "block containers (except table wrappers)", True, False, False, "see individual properties")
add("column-count", "CSS Multi-column", "auto | <integer>", "auto", False, "block containers (except table wrappers)", True)
add("column-fill", "CSS Multi-column", "auto | balance | balance-all", "balance", False, "multicol containers", False, False, False, "discrete")
add("column-rule", "CSS Multi-column", "<column-rule-width> || <column-rule-style> || <column-rule-color>", "see individual properties", False, "multicol containers", False, False, False, "see individual properties")
add("column-rule-color", "CSS Multi-column", "<color>", "currentcolor", False, "multicol containers")
add("column-rule-style", "CSS Multi-column", "<line-style>", "none", False, "multicol containers", False, False, False, "discrete")
add("column-rule-width", "CSS Multi-column", "<line-width>", "medium", False, "multicol containers")
add("column-span", "CSS Multi-column", "none | all", "none", False, "in-flow block-level elements in a multicol container", False, False, False, "discrete")
add("column-width", "CSS Multi-column", "auto | <length>", "auto", False, "block containers (except table wrappers)", True)

# === TRANSFORMS ===
add("transform", "CSS Transforms", "none | <transform-list>", "none", False, "transformable elements", False, True, True)
add("transform-origin", "CSS Transforms", "[ <length-percentage> | left | center | right ] [ <length-percentage> | top | center | bottom ] <length>?", "50% 50% 0", False, "transformable elements")
add("transform-style", "CSS Transforms", "flat | preserve-3d", "flat", False, "transformable elements", False, True, False, "discrete")
add("perspective", "CSS Transforms", "none | <length>", "none", False, "transformable elements", False, True, True)
add("perspective-origin", "CSS Transforms", "<position>", "50% 50%", False, "transformable elements")
add("backface-visibility", "CSS Transforms", "visible | hidden", "visible", False, "transformable elements", False, False, False, "discrete")
add("rotate", "CSS Transforms", "none | <angle> | [ x | y | z | <number>{3} ] && <angle>", "none", False, "transformable elements", False, True, True)
add("scale", "CSS Transforms", "none | [ <number> | <percentage> ]{1,3}", "none", False, "transformable elements", False, True, True)
add("translate", "CSS Transforms", "none | <length-percentage> [ <length-percentage> <length>? ]?", "none", False, "transformable elements", False, True, True)

# === TRANSITIONS ===
add("transition", "CSS Transitions", "<single-transition>#", "see individual properties", False, "all elements, ::before, ::after", False, False, False, "discrete")
add("transition-property", "CSS Transitions", "none | <single-transition-property>#", "all", False, "all elements", False, False, False, "discrete")
add("transition-duration", "CSS Transitions", "<time>#", "0s", False, "all elements", False, False, False, "discrete")
add("transition-timing-function", "CSS Transitions", "<easing-function>#", "ease", False, "all elements", False, False, False, "discrete")
add("transition-delay", "CSS Transitions", "<time>#", "0s", False, "all elements", False, False, False, "discrete")

# === ANIMATIONS ===
add("animation", "CSS Animations", "<single-animation>#", "see individual properties", False, "all elements", False, False, False, "discrete")
add("animation-name", "CSS Animations", "[ none | <keyframes-name> ]#", "none", False, "all elements", False, False, False, "discrete")
add("animation-duration", "CSS Animations", "<time>#", "0s", False, "all elements", False, False, False, "discrete")
add("animation-timing-function", "CSS Animations", "<easing-function>#", "ease", False, "all elements", False, False, False, "discrete")
add("animation-delay", "CSS Animations", "<time>#", "0s", False, "all elements", False, False, False, "discrete")
add("animation-iteration-count", "CSS Animations", "[ infinite | <number> ]#", "1", False, "all elements", False, False, False, "discrete")
add("animation-direction", "CSS Animations", "[ normal | reverse | alternate | alternate-reverse ]#", "normal", False, "all elements", False, False, False, "discrete")
add("animation-fill-mode", "CSS Animations", "[ none | forwards | backwards | both ]#", "none", False, "all elements", False, False, False, "discrete")
add("animation-play-state", "CSS Animations", "[ running | paused ]#", "running", False, "all elements", False, False, False, "discrete")
add("animation-timeline", "CSS Animations", "[ auto | none | <timeline-name> | scroll() | view() ]#", "auto", False, "all elements", False, False, False, "discrete")

# Motion path
add("offset", "CSS Motion Path", "[ <offset-position>? [ <offset-path> [ <offset-distance> || <offset-rotate> ]? ]? ]! [ / <offset-anchor> ]?", "see individual properties", False, "transformable elements", False, True, True, "see individual properties")
add("offset-path", "CSS Motion Path", "none | <offset-path> || <coord-box>", "none", False, "transformable elements", False, True, True)
add("offset-distance", "CSS Motion Path", "<length-percentage>", "0", False, "transformable elements")
add("offset-rotate", "CSS Motion Path", "[ auto | reverse ] || <angle>", "auto", False, "transformable elements")
add("offset-anchor", "CSS Motion Path", "auto | <position>", "auto", False, "transformable elements")
add("offset-position", "CSS Motion Path", "auto | <position>", "auto", False, "transformable elements")

# === FILTERS & COMPOSITING ===
add("filter", "CSS Filter Effects", "none | <filter-function-list>", "none", False, "all elements (SVG: container and graphics)", False, True, True)
add("backdrop-filter", "CSS Filter Effects", "none | <filter-function-list>", "none", False, "all elements", False, True, False)
add("mix-blend-mode", "CSS Compositing", "<blend-mode>", "normal", False, "all elements", False, True, False, "discrete")
add("background-blend-mode", "CSS Compositing", "<blend-mode>#", "normal", False, "all elements", False, False, False, "discrete")
add("isolation", "CSS Compositing", "auto | isolate", "auto", False, "all elements", False, True, False, "discrete")

# === MASKING & CLIPPING ===
add("mask", "CSS Masking", "<mask-layer>#", "see individual properties", False, "all elements (SVG: container and graphics)", False, False, False, "see individual properties")
add("mask-image", "CSS Masking", "<mask-reference>#", "none", False, "all elements", False, False, False, "discrete")
add("mask-mode", "CSS Masking", "<masking-mode>#", "match-source", False, "all elements", False, False, False, "discrete")
add("mask-repeat", "CSS Masking", "<repeat-style>#", "repeat", False, "all elements", False, False, False, "discrete")
add("mask-position", "CSS Masking", "<position>#", "0% 0%", False, "all elements")
add("mask-clip", "CSS Masking", "[ <geometry-box> | no-clip ]#", "border-box", False, "all elements", False, False, False, "discrete")
add("mask-origin", "CSS Masking", "<geometry-box>#", "border-box", False, "all elements", False, False, False, "discrete")
add("mask-size", "CSS Masking", "<bg-size>#", "auto", False, "all elements")
add("mask-composite", "CSS Masking", "<compositing-operator>#", "add", False, "all elements", False, False, False, "discrete")
add("mask-type", "CSS Masking", "luminance | alpha", "luminance", False, "SVG mask elements", False, False, False, "discrete")
add("clip-rule", "CSS Masking", "nonzero | evenodd", "nonzero", True, "SVG graphics elements", False, False, False, "discrete")

# === SHAPES ===
add("shape-outside", "CSS Shapes", "none | [ <basic-shape> || <shape-box> ] | <image>", "none", False, "floats")
add("shape-margin", "CSS Shapes", "<length-percentage>", "0", False, "floats")
add("shape-image-threshold", "CSS Shapes", "<number>", "0", False, "floats")

# === WRITING MODES ===
add("writing-mode", "CSS Writing Modes", "horizontal-tb | vertical-rl | vertical-lr | sideways-rl | sideways-lr", "horizontal-tb", True, "all elements", False, False, False, "discrete")
add("direction", "CSS Writing Modes", "ltr | rtl", "ltr", True, "all elements", False, False, False, "discrete")
add("unicode-bidi", "CSS Writing Modes", "normal | embed | isolate | bidi-override | isolate-override | plaintext", "normal", False, "all elements", False, False, False, "discrete")
add("text-orientation", "CSS Writing Modes", "mixed | upright | sideways", "mixed", True, "all elements (except table row groups, rows, column groups, columns)", False, False, False, "discrete")
add("text-combine-upright", "CSS Writing Modes", "none | all | [ digits <integer>? ]", "none", True, "inline elements", False, False, False, "discrete")

# === SCROLL & OVERSCROLL ===
add("scroll-behavior", "CSS Overflow", "auto | smooth", "auto", False, "scroll containers", False, False, False, "discrete")
for axis in ["", "-block", "-block-start", "-block-end", "-inline", "-inline-start", "-inline-end", "-top", "-right", "-bottom", "-left"]:
    add(f"scroll-margin{axis}", "CSS Scroll Snap", "<length>", "0", False, "all elements")
for axis in ["", "-block", "-block-start", "-block-end", "-inline", "-inline-start", "-inline-end", "-top", "-right", "-bottom", "-left"]:
    add(f"scroll-padding{axis}", "CSS Scroll Snap", "auto | <length-percentage>", "auto", False, "scroll containers")
add("scroll-snap-type", "CSS Scroll Snap", "none | [ x | y | block | inline | both ] [ mandatory | proximity ]?", "none", False, "scroll containers", False, False, False, "discrete")
add("scroll-snap-align", "CSS Scroll Snap", "[ none | start | end | center ]{1,2}", "none", False, "all elements", False, False, False, "discrete")
add("scroll-snap-stop", "CSS Scroll Snap", "normal | always", "normal", False, "all elements", False, False, False, "discrete")
add("overscroll-behavior", "CSS Overscroll", "[ contain | none | auto ]{1,2}", "auto", False, "scroll container elements", False, False, False, "discrete")
add("overscroll-behavior-x", "CSS Overscroll", "contain | none | auto", "auto", False, "scroll container elements", False, False, False, "discrete")
add("overscroll-behavior-y", "CSS Overscroll", "contain | none | auto", "auto", False, "scroll container elements", False, False, False, "discrete")
add("overscroll-behavior-block", "CSS Overscroll", "contain | none | auto", "auto", False, "scroll container elements", False, False, False, "discrete")
add("overscroll-behavior-inline", "CSS Overscroll", "contain | none | auto", "auto", False, "scroll container elements", False, False, False, "discrete")

# === CONTAINMENT ===
add("contain", "CSS Containment", "none | strict | content | [ size || inline-size || layout || style || paint ]", "none", False, "all elements", True, True, True, "discrete")
add("contain-intrinsic-size", "CSS Containment", "[ auto? [ none | <length> ] ]{1,2}", "none", False, "elements with size containment")
add("contain-intrinsic-width", "CSS Containment", "auto? [ none | <length> ]", "none", False, "elements with size containment")
add("contain-intrinsic-height", "CSS Containment", "auto? [ none | <length> ]", "none", False, "elements with size containment")
add("contain-intrinsic-block-size", "CSS Containment", "auto? [ none | <length> ]", "none", False, "elements with size containment")
add("contain-intrinsic-inline-size", "CSS Containment", "auto? [ none | <length> ]", "none", False, "elements with size containment")
add("container-type", "CSS Containment", "normal | size | inline-size", "normal", False, "all elements", True, False, False, "discrete")
add("container-name", "CSS Containment", "none | <custom-ident>+", "none", False, "all elements", False, False, False, "discrete")
add("container", "CSS Containment", "<container-name> [ / <container-type> ]?", "see individual properties", False, "all elements", True, False, False, "discrete")
add("content-visibility", "CSS Containment", "visible | auto | hidden", "visible", False, "all elements", True, True, True, "discrete")

# === SIZING ===
add("aspect-ratio", "CSS Box Sizing", "auto | <ratio>", "auto", False, "all elements except inline boxes and internal ruby/table boxes")
add("object-fit", "CSS Images", "fill | contain | cover | none | scale-down", "fill", False, "replaced elements", False, False, False, "discrete")
add("object-position", "CSS Images", "<position>", "50% 50%", False, "replaced elements")
add("image-rendering", "CSS Images", "auto | smooth | high-quality | pixelated | crisp-edges", "auto", True, "all elements", False, False, False, "discrete")
add("image-orientation", "CSS Images", "from-image | none | <angle>", "from-image", True, "all elements", False, False, False, "discrete")

# === INTERACTION ===
add("pointer-events", "CSS UI / SVG", "auto | none | visiblePainted | visibleFill | visibleStroke | visible | painted | fill | stroke | all", "auto", True, "all elements", False, False, False, "discrete")
add("touch-action", "CSS Pointer Events", "auto | none | [ [ pan-x | pan-left | pan-right ] || [ pan-y | pan-up | pan-down ] || pinch-zoom ] | manipulation", "auto", False, "all elements except non-replaced inline", False, False, False, "discrete")
add("user-select", "CSS UI", "auto | text | none | contain | all", "auto", False, "all elements, ::before, ::after", False, False, False, "discrete")
add("cursor", "CSS UI", "[ [ <url> [ <x> <y> ]? , ]* [ auto | default | none | context-menu | help | pointer | ... ] ]", "auto", True, "all elements", False, False, False, "discrete")
add("resize", "CSS UI", "none | both | horizontal | vertical | block | inline", "none", False, "elements with overflow != visible", False, False, False, "discrete")
add("scrollbar-color", "CSS Scrollbars", "auto | <color>{2}", "auto", False, "scroll containers")
add("scrollbar-gutter", "CSS Overflow", "auto | stable && both-edges?", "auto", False, "scroll containers", False, False, False, "discrete")
add("scrollbar-width", "CSS Scrollbars", "auto | thin | none", "auto", False, "scroll containers", False, False, False, "discrete")

# === FRAGMENTATION ===
add("break-before", "CSS Fragmentation", "auto | avoid | always | all | avoid-page | page | left | right | recto | verso | avoid-column | column | avoid-region | region", "auto", False, "block-level elements, table row groups, table rows", False, False, False, "discrete")
add("break-after", "CSS Fragmentation", "auto | avoid | always | all | avoid-page | page | left | right | recto | verso | avoid-column | column | avoid-region | region", "auto", False, "block-level elements, table row groups, table rows", False, False, False, "discrete")
add("break-inside", "CSS Fragmentation", "auto | avoid | avoid-page | avoid-column | avoid-region", "auto", False, "block-level elements, table row groups, table rows", False, False, False, "discrete")
add("orphans", "CSS Fragmentation", "<integer>", "2", True, "block containers")
add("widows", "CSS Fragmentation", "<integer>", "2", True, "block containers")
add("page-break-before", "CSS Fragmentation", "auto | always | avoid | left | right", "auto", False, "block-level elements", False, False, False, "discrete")
add("page-break-after", "CSS Fragmentation", "auto | always | avoid | left | right", "auto", False, "block-level elements", False, False, False, "discrete")
add("page-break-inside", "CSS Fragmentation", "auto | avoid", "auto", False, "block-level elements", False, False, False, "discrete")

# === WILL-CHANGE, APPEARANCE, ALL ===
add("will-change", "CSS Will Change", "auto | <animateable-feature>#", "auto", False, "all elements", False, True, True, "discrete")
add("appearance", "CSS UI", "none | auto", "none", False, "all elements", False, False, False, "discrete")
add("all", "CSS Cascade", "initial | inherit | unset | revert | revert-layer", "see individual properties", False, "all elements", False, False, False, "see individual properties")

# === ANCHOR POSITIONING ===
add("anchor-name", "CSS Anchor Positioning", "none | <dashed-ident>#", "none", False, "all elements", False, False, False, "discrete")
add("anchor-scope", "CSS Anchor Positioning", "none | all | <dashed-ident>#", "none", False, "all elements", False, False, False, "discrete")
add("position-anchor", "CSS Anchor Positioning", "auto | <anchor-element>", "auto", False, "absolutely positioned elements", False, False, False, "discrete")
add("position-area", "CSS Anchor Positioning", "none | <position-area>", "none", False, "absolutely positioned elements", False, False, False, "discrete")
add("position-try", "CSS Anchor Positioning", "[ <dashed-ident> | <try-tactic> ]#", "none", False, "absolutely positioned elements", False, False, False, "discrete")
add("position-try-fallbacks", "CSS Anchor Positioning", "none | [ <dashed-ident> | <try-tactic> ]#", "none", False, "absolutely positioned elements", False, False, False, "discrete")
add("position-try-order", "CSS Anchor Positioning", "normal | <try-size>", "normal", False, "absolutely positioned elements", False, False, False, "discrete")
add("position-visibility", "CSS Anchor Positioning", "always | anchors-visible | no-overflow", "always", False, "absolutely positioned elements", False, False, False, "discrete")

# === VIEW TRANSITIONS ===
add("view-transition-name", "CSS View Transitions", "none | <custom-ident>", "none", False, "all elements", False, False, False, "discrete")
add("view-transition-class", "CSS View Transitions", "none | <custom-ident>+", "none", False, "all elements", False, False, False, "discrete")

# === CSS RUBY ===
add("ruby-align", "CSS Ruby", "space-around | center | start | space-between", "space-around", True, "ruby annotation containers", False, False, False, "discrete")
add("ruby-position", "CSS Ruby", "alternate | over | under | inter-character", "alternate", True, "ruby annotation containers", False, False, False, "discrete")

# === CSS MATH (MathML) ===
add("math-style", "CSS MathML", "normal | compact", "normal", True, "all elements", False, False, False, "discrete")
add("math-depth", "CSS MathML", "auto-add | add(<integer>) | <integer>", "0", True, "all elements", False, False, False, "by computed value type")
add("math-shift", "CSS MathML", "normal | compact", "normal", True, "all elements", False, False, False, "discrete")

# === CSS TEXT LEVEL 4 (additions) ===
add("text-autospace", "CSS Text", "normal | no-autospace | [ ideograph-alpha || ideograph-numeric || punctuation ] || [ insert | replace ]", "normal", True, "block containers", False, False, False, "discrete")
add("text-spacing-trim", "CSS Text", "normal | space-all | space-first | trim-start | trim-both | trim-all", "normal", True, "block containers", False, False, False, "discrete")
add("initial-letter", "CSS Inline", "normal | <number> <integer>?", "normal", False, "::first-letter pseudo-elements and inline-level first children of block containers", False, False, False, "discrete")
add("initial-letter-align", "CSS Inline", "auto | alphabetic | hanging | ideographic", "auto", False, "::first-letter pseudo-elements and inline-level first children of block containers", False, False, False, "discrete")

# === SVG CSS PROPERTIES ===
add("fill", "CSS SVG", "<paint>", "black", True, "SVG graphics elements and text content elements", False, False, False, "by computed value type")
add("fill-opacity", "CSS SVG", "<number [0,1]>", "1", True, "SVG graphics elements", False, False, False, "by computed value type")
add("fill-rule", "CSS SVG", "nonzero | evenodd", "nonzero", True, "SVG graphics elements", False, False, False, "discrete")
add("stroke", "CSS SVG", "<paint>", "none", True, "SVG graphics elements and text content elements", False, False, False, "by computed value type")
add("stroke-dasharray", "CSS SVG", "none | <dasharray>", "none", True, "SVG graphics elements", False, False, False, "by computed value type")
add("stroke-dashoffset", "CSS SVG", "<length-percentage>", "0", True, "SVG graphics elements", False, False, False, "by computed value type")
add("stroke-linecap", "CSS SVG", "butt | round | square", "butt", True, "SVG graphics elements", False, False, False, "discrete")
add("stroke-linejoin", "CSS SVG", "miter | round | bevel", "miter", True, "SVG graphics elements", False, False, False, "discrete")
add("stroke-miterlimit", "CSS SVG", "<number>", "4", True, "SVG graphics elements", False, False, False, "by computed value type")
add("stroke-opacity", "CSS SVG", "<number [0,1]>", "1", True, "SVG graphics elements", False, False, False, "by computed value type")
add("stroke-width", "CSS SVG", "<length-percentage>", "1", True, "SVG graphics elements", False, False, False, "by computed value type")
add("paint-order", "CSS SVG", "normal | [ fill || stroke || markers ]", "normal", True, "SVG graphics elements and text content elements", False, False, False, "discrete")
add("marker-start", "CSS SVG", "none | <url>", "none", True, "SVG shape elements", False, False, False, "discrete")
add("marker-mid", "CSS SVG", "none | <url>", "none", True, "SVG shape elements", False, False, False, "discrete")
add("marker-end", "CSS SVG", "none | <url>", "none", True, "SVG shape elements", False, False, False, "discrete")
add("color-interpolation", "CSS SVG", "auto | sRGB | linearRGB", "sRGB", True, "SVG container elements, graphics elements, gradient elements", False, False, False, "discrete")
add("color-interpolation-filters", "CSS SVG", "auto | sRGB | linearRGB", "linearRGB", True, "SVG filter primitive elements", False, False, False, "discrete")
add("dominant-baseline", "CSS SVG", "auto | text-bottom | alphabetic | ideographic | middle | central | mathematical | hanging | text-top", "auto", False, "SVG text content elements, inline-level elements", False, False, False, "discrete")
add("text-anchor", "CSS SVG", "start | middle | end", "start", True, "SVG text content elements", False, False, False, "discrete")
add("stop-color", "CSS SVG", "<color>", "black", False, "SVG stop elements", False, False, False, "by computed value type")
add("stop-opacity", "CSS SVG", "<number [0,1]>", "1", False, "SVG stop elements", False, False, False, "by computed value type")
add("vector-effect", "CSS SVG", "none | non-scaling-stroke | non-scaling-size | non-rotation | fixed-position", "none", False, "SVG graphics elements", False, False, False, "discrete")

# === OUTPUT ===
with open("/home/user/Browser/catalog/css_properties.json", "w") as f:
    json.dump(properties, f, indent=2)
print(f"Wrote {len(properties)} CSS properties")

# Stats
modules = {}
for p in properties:
    m = p["module"]
    modules[m] = modules.get(m, 0) + 1
for m, c in sorted(modules.items(), key=lambda x: -x[1]):
    print(f"  {m}: {c}")
fmt = sum(1 for p in properties if p["creates_formatting_context"])
stk = sum(1 for p in properties if p["creates_stacking_context"])
cb = sum(1 for p in properties if p["creates_containing_block"])
print(f"\nContext creators: formatting={fmt}, stacking={stk}, containing_block={cb}")
inh = sum(1 for p in properties if p["inherited"])
print(f"Inherited: {inh}, Non-inherited: {len(properties)-inh}")

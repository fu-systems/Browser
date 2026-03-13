/*
 * properties.c -- CSS property metadata and lookup
 *
 * Auto-generated from css_properties.json and css_pair_chain_handling.md
 * 412 CSS properties, 74 shorthand mappings
 */

#include "properties.h"
#include <string.h>

/* ------------------------------------------------------------ */
/*  Static property table (indexed by CssPropId, 1-based)       */
/* ------------------------------------------------------------ */

static const CssPropInfo prop_table[CSS_PROP__COUNT] = {
    [CSS_PROP_NONE] = { "(none)", 0, CSS_PROP_NONE },
    [CSS_PROP_ACCENT_COLOR] = { "accent-color", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_ALIGN_CONTENT] = { "align-content", 0, CSS_PROP_NONE },
    [CSS_PROP_ALIGN_ITEMS] = { "align-items", 0, CSS_PROP_NONE },
    [CSS_PROP_ALIGN_SELF] = { "align-self", 0, CSS_PROP_NONE },
    [CSS_PROP_ALL] = { "all", 0, CSS_PROP_NONE },
    [CSS_PROP_ANCHOR_NAME] = { "anchor-name", 0, CSS_PROP_NONE },
    [CSS_PROP_ANCHOR_SCOPE] = { "anchor-scope", 0, CSS_PROP_NONE },
    [CSS_PROP_ANIMATION] = { "animation", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_ANIMATION_DELAY] = { "animation-delay", 0, CSS_PROP_NONE },
    [CSS_PROP_ANIMATION_DIRECTION] = { "animation-direction", 0, CSS_PROP_NONE },
    [CSS_PROP_ANIMATION_DURATION] = { "animation-duration", 0, CSS_PROP_NONE },
    [CSS_PROP_ANIMATION_FILL_MODE] = { "animation-fill-mode", 0, CSS_PROP_NONE },
    [CSS_PROP_ANIMATION_ITERATION_COUNT] = { "animation-iteration-count", 0, CSS_PROP_NONE },
    [CSS_PROP_ANIMATION_NAME] = { "animation-name", 0, CSS_PROP_NONE },
    [CSS_PROP_ANIMATION_PLAY_STATE] = { "animation-play-state", 0, CSS_PROP_NONE },
    [CSS_PROP_ANIMATION_TIMELINE] = { "animation-timeline", 0, CSS_PROP_NONE },
    [CSS_PROP_ANIMATION_TIMING_FUNCTION] = { "animation-timing-function", 0, CSS_PROP_NONE },
    [CSS_PROP_APPEARANCE] = { "appearance", 0, CSS_PROP_NONE },
    [CSS_PROP_ASPECT_RATIO] = { "aspect-ratio", 0, CSS_PROP_NONE },
    [CSS_PROP_BACKDROP_FILTER] = { "backdrop-filter", 0, CSS_PROP_NONE },
    [CSS_PROP_BACKFACE_VISIBILITY] = { "backface-visibility", 0, CSS_PROP_NONE },
    [CSS_PROP_BACKGROUND] = { "background", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_BACKGROUND_ATTACHMENT] = { "background-attachment", 0, CSS_PROP_NONE },
    [CSS_PROP_BACKGROUND_BLEND_MODE] = { "background-blend-mode", 0, CSS_PROP_NONE },
    [CSS_PROP_BACKGROUND_CLIP] = { "background-clip", 0, CSS_PROP_NONE },
    [CSS_PROP_BACKGROUND_COLOR] = { "background-color", 0, CSS_PROP_NONE },
    [CSS_PROP_BACKGROUND_IMAGE] = { "background-image", 0, CSS_PROP_NONE },
    [CSS_PROP_BACKGROUND_ORIGIN] = { "background-origin", 0, CSS_PROP_NONE },
    [CSS_PROP_BACKGROUND_POSITION] = { "background-position", 0, CSS_PROP_NONE },
    [CSS_PROP_BACKGROUND_REPEAT] = { "background-repeat", 0, CSS_PROP_NONE },
    [CSS_PROP_BACKGROUND_SIZE] = { "background-size", 0, CSS_PROP_NONE },
    [CSS_PROP_BLOCK_SIZE] = { "block-size", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER] = { "border", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_BORDER_BLOCK] = { "border-block", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_BORDER_BLOCK_COLOR] = { "border-block-color", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_BORDER_BLOCK_END] = { "border-block-end", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_BORDER_BLOCK_END_COLOR] = { "border-block-end-color", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_BLOCK_END_STYLE] = { "border-block-end-style", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_BLOCK_END_WIDTH] = { "border-block-end-width", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_BLOCK_START] = { "border-block-start", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_BORDER_BLOCK_START_COLOR] = { "border-block-start-color", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_BLOCK_START_STYLE] = { "border-block-start-style", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_BLOCK_START_WIDTH] = { "border-block-start-width", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_BLOCK_STYLE] = { "border-block-style", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_BORDER_BLOCK_WIDTH] = { "border-block-width", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_BORDER_BOTTOM] = { "border-bottom", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_BORDER_BOTTOM_COLOR] = { "border-bottom-color", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_BOTTOM_LEFT_RADIUS] = { "border-bottom-left-radius", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_BOTTOM_RIGHT_RADIUS] = { "border-bottom-right-radius", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_BOTTOM_STYLE] = { "border-bottom-style", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_BOTTOM_WIDTH] = { "border-bottom-width", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_COLLAPSE] = { "border-collapse", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_BORDER_COLOR] = { "border-color", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_BORDER_END_END_RADIUS] = { "border-end-end-radius", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_END_START_RADIUS] = { "border-end-start-radius", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_IMAGE] = { "border-image", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_BORDER_IMAGE_OUTSET] = { "border-image-outset", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_IMAGE_REPEAT] = { "border-image-repeat", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_IMAGE_SLICE] = { "border-image-slice", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_IMAGE_SOURCE] = { "border-image-source", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_IMAGE_WIDTH] = { "border-image-width", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_INLINE] = { "border-inline", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_BORDER_INLINE_COLOR] = { "border-inline-color", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_BORDER_INLINE_END] = { "border-inline-end", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_BORDER_INLINE_END_COLOR] = { "border-inline-end-color", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_INLINE_END_STYLE] = { "border-inline-end-style", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_INLINE_END_WIDTH] = { "border-inline-end-width", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_INLINE_START] = { "border-inline-start", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_BORDER_INLINE_START_COLOR] = { "border-inline-start-color", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_INLINE_START_STYLE] = { "border-inline-start-style", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_INLINE_START_WIDTH] = { "border-inline-start-width", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_INLINE_STYLE] = { "border-inline-style", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_BORDER_INLINE_WIDTH] = { "border-inline-width", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_BORDER_LEFT] = { "border-left", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_BORDER_LEFT_COLOR] = { "border-left-color", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_LEFT_STYLE] = { "border-left-style", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_LEFT_WIDTH] = { "border-left-width", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_RADIUS] = { "border-radius", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_BORDER_RIGHT] = { "border-right", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_BORDER_RIGHT_COLOR] = { "border-right-color", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_RIGHT_STYLE] = { "border-right-style", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_RIGHT_WIDTH] = { "border-right-width", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_SPACING] = { "border-spacing", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_BORDER_START_END_RADIUS] = { "border-start-end-radius", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_START_START_RADIUS] = { "border-start-start-radius", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_STYLE] = { "border-style", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_BORDER_TOP] = { "border-top", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_BORDER_TOP_COLOR] = { "border-top-color", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_TOP_LEFT_RADIUS] = { "border-top-left-radius", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_TOP_RIGHT_RADIUS] = { "border-top-right-radius", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_TOP_STYLE] = { "border-top-style", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_TOP_WIDTH] = { "border-top-width", 0, CSS_PROP_NONE },
    [CSS_PROP_BORDER_WIDTH] = { "border-width", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_BOTTOM] = { "bottom", 0, CSS_PROP_NONE },
    [CSS_PROP_BOX_DECORATION_BREAK] = { "box-decoration-break", 0, CSS_PROP_NONE },
    [CSS_PROP_BOX_SHADOW] = { "box-shadow", 0, CSS_PROP_NONE },
    [CSS_PROP_BOX_SIZING] = { "box-sizing", 0, CSS_PROP_NONE },
    [CSS_PROP_BREAK_AFTER] = { "break-after", 0, CSS_PROP_NONE },
    [CSS_PROP_BREAK_BEFORE] = { "break-before", 0, CSS_PROP_NONE },
    [CSS_PROP_BREAK_INSIDE] = { "break-inside", 0, CSS_PROP_NONE },
    [CSS_PROP_CAPTION_SIDE] = { "caption-side", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_CARET_COLOR] = { "caret-color", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_CLEAR] = { "clear", 0, CSS_PROP_NONE },
    [CSS_PROP_CLIP] = { "clip", 0, CSS_PROP_NONE },
    [CSS_PROP_CLIP_PATH] = { "clip-path", 0, CSS_PROP_NONE },
    [CSS_PROP_CLIP_RULE] = { "clip-rule", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_COLOR] = { "color", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_COLOR_INTERPOLATION] = { "color-interpolation", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_COLOR_INTERPOLATION_FILTERS] = { "color-interpolation-filters", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_COLOR_SCHEME] = { "color-scheme", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_COLUMN_COUNT] = { "column-count", 0, CSS_PROP_NONE },
    [CSS_PROP_COLUMN_FILL] = { "column-fill", 0, CSS_PROP_NONE },
    [CSS_PROP_COLUMN_GAP] = { "column-gap", 0, CSS_PROP_NONE },
    [CSS_PROP_COLUMN_RULE] = { "column-rule", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_COLUMN_RULE_COLOR] = { "column-rule-color", 0, CSS_PROP_NONE },
    [CSS_PROP_COLUMN_RULE_STYLE] = { "column-rule-style", 0, CSS_PROP_NONE },
    [CSS_PROP_COLUMN_RULE_WIDTH] = { "column-rule-width", 0, CSS_PROP_NONE },
    [CSS_PROP_COLUMN_SPAN] = { "column-span", 0, CSS_PROP_NONE },
    [CSS_PROP_COLUMN_WIDTH] = { "column-width", 0, CSS_PROP_NONE },
    [CSS_PROP_COLUMNS] = { "columns", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_CONTAIN] = { "contain", 0, CSS_PROP_NONE },
    [CSS_PROP_CONTAIN_INTRINSIC_BLOCK_SIZE] = { "contain-intrinsic-block-size", 0, CSS_PROP_NONE },
    [CSS_PROP_CONTAIN_INTRINSIC_HEIGHT] = { "contain-intrinsic-height", 0, CSS_PROP_NONE },
    [CSS_PROP_CONTAIN_INTRINSIC_INLINE_SIZE] = { "contain-intrinsic-inline-size", 0, CSS_PROP_NONE },
    [CSS_PROP_CONTAIN_INTRINSIC_SIZE] = { "contain-intrinsic-size", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_CONTAIN_INTRINSIC_WIDTH] = { "contain-intrinsic-width", 0, CSS_PROP_NONE },
    [CSS_PROP_CONTAINER] = { "container", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_CONTAINER_NAME] = { "container-name", 0, CSS_PROP_NONE },
    [CSS_PROP_CONTAINER_TYPE] = { "container-type", 0, CSS_PROP_NONE },
    [CSS_PROP_CONTENT] = { "content", 0, CSS_PROP_NONE },
    [CSS_PROP_CONTENT_VISIBILITY] = { "content-visibility", 0, CSS_PROP_NONE },
    [CSS_PROP_COUNTER_INCREMENT] = { "counter-increment", 0, CSS_PROP_NONE },
    [CSS_PROP_COUNTER_RESET] = { "counter-reset", 0, CSS_PROP_NONE },
    [CSS_PROP_COUNTER_SET] = { "counter-set", 0, CSS_PROP_NONE },
    [CSS_PROP_CURSOR] = { "cursor", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_DIRECTION] = { "direction", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_DISPLAY] = { "display", 0, CSS_PROP_NONE },
    [CSS_PROP_DOMINANT_BASELINE] = { "dominant-baseline", 0, CSS_PROP_NONE },
    [CSS_PROP_EMPTY_CELLS] = { "empty-cells", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_FILL] = { "fill", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_FILL_OPACITY] = { "fill-opacity", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_FILL_RULE] = { "fill-rule", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_FILTER] = { "filter", 0, CSS_PROP_NONE },
    [CSS_PROP_FLEX] = { "flex", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_FLEX_BASIS] = { "flex-basis", 0, CSS_PROP_NONE },
    [CSS_PROP_FLEX_DIRECTION] = { "flex-direction", 0, CSS_PROP_NONE },
    [CSS_PROP_FLEX_FLOW] = { "flex-flow", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_FLEX_GROW] = { "flex-grow", 0, CSS_PROP_NONE },
    [CSS_PROP_FLEX_SHRINK] = { "flex-shrink", 0, CSS_PROP_NONE },
    [CSS_PROP_FLEX_WRAP] = { "flex-wrap", 0, CSS_PROP_NONE },
    [CSS_PROP_FLOAT] = { "float", 0, CSS_PROP_NONE },
    [CSS_PROP_FONT] = { "font", PROP_INHERITED | PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_FONT_FAMILY] = { "font-family", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_FONT_FEATURE_SETTINGS] = { "font-feature-settings", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_FONT_KERNING] = { "font-kerning", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_FONT_LANGUAGE_OVERRIDE] = { "font-language-override", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_FONT_OPTICAL_SIZING] = { "font-optical-sizing", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_FONT_PALETTE] = { "font-palette", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_FONT_SIZE] = { "font-size", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_FONT_SIZE_ADJUST] = { "font-size-adjust", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_FONT_STRETCH] = { "font-stretch", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_FONT_STYLE] = { "font-style", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_FONT_SYNTHESIS] = { "font-synthesis", PROP_INHERITED | PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_FONT_VARIANT] = { "font-variant", PROP_INHERITED | PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_FONT_VARIANT_ALTERNATES] = { "font-variant-alternates", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_FONT_VARIANT_CAPS] = { "font-variant-caps", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_FONT_VARIANT_EAST_ASIAN] = { "font-variant-east-asian", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_FONT_VARIANT_LIGATURES] = { "font-variant-ligatures", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_FONT_VARIANT_NUMERIC] = { "font-variant-numeric", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_FONT_VARIANT_POSITION] = { "font-variant-position", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_FONT_VARIATION_SETTINGS] = { "font-variation-settings", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_FONT_WEIGHT] = { "font-weight", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_FORCED_COLOR_ADJUST] = { "forced-color-adjust", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_GAP] = { "gap", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_GRID] = { "grid", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_GRID_AREA] = { "grid-area", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_GRID_AUTO_COLUMNS] = { "grid-auto-columns", 0, CSS_PROP_NONE },
    [CSS_PROP_GRID_AUTO_FLOW] = { "grid-auto-flow", 0, CSS_PROP_NONE },
    [CSS_PROP_GRID_AUTO_ROWS] = { "grid-auto-rows", 0, CSS_PROP_NONE },
    [CSS_PROP_GRID_COLUMN] = { "grid-column", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_GRID_COLUMN_END] = { "grid-column-end", 0, CSS_PROP_NONE },
    [CSS_PROP_GRID_COLUMN_START] = { "grid-column-start", 0, CSS_PROP_NONE },
    [CSS_PROP_GRID_ROW] = { "grid-row", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_GRID_ROW_END] = { "grid-row-end", 0, CSS_PROP_NONE },
    [CSS_PROP_GRID_ROW_START] = { "grid-row-start", 0, CSS_PROP_NONE },
    [CSS_PROP_GRID_TEMPLATE] = { "grid-template", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_GRID_TEMPLATE_AREAS] = { "grid-template-areas", 0, CSS_PROP_NONE },
    [CSS_PROP_GRID_TEMPLATE_COLUMNS] = { "grid-template-columns", 0, CSS_PROP_NONE },
    [CSS_PROP_GRID_TEMPLATE_ROWS] = { "grid-template-rows", 0, CSS_PROP_NONE },
    [CSS_PROP_HANGING_PUNCTUATION] = { "hanging-punctuation", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_HEIGHT] = { "height", 0, CSS_PROP_NONE },
    [CSS_PROP_HYPHENS] = { "hyphens", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_IMAGE_ORIENTATION] = { "image-orientation", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_IMAGE_RENDERING] = { "image-rendering", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_INITIAL_LETTER] = { "initial-letter", 0, CSS_PROP_NONE },
    [CSS_PROP_INITIAL_LETTER_ALIGN] = { "initial-letter-align", 0, CSS_PROP_NONE },
    [CSS_PROP_INLINE_SIZE] = { "inline-size", 0, CSS_PROP_NONE },
    [CSS_PROP_INSET] = { "inset", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_INSET_BLOCK] = { "inset-block", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_INSET_BLOCK_END] = { "inset-block-end", 0, CSS_PROP_NONE },
    [CSS_PROP_INSET_BLOCK_START] = { "inset-block-start", 0, CSS_PROP_NONE },
    [CSS_PROP_INSET_INLINE] = { "inset-inline", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_INSET_INLINE_END] = { "inset-inline-end", 0, CSS_PROP_NONE },
    [CSS_PROP_INSET_INLINE_START] = { "inset-inline-start", 0, CSS_PROP_NONE },
    [CSS_PROP_ISOLATION] = { "isolation", 0, CSS_PROP_NONE },
    [CSS_PROP_JUSTIFY_CONTENT] = { "justify-content", 0, CSS_PROP_NONE },
    [CSS_PROP_JUSTIFY_ITEMS] = { "justify-items", 0, CSS_PROP_NONE },
    [CSS_PROP_JUSTIFY_SELF] = { "justify-self", 0, CSS_PROP_NONE },
    [CSS_PROP_LEFT] = { "left", 0, CSS_PROP_NONE },
    [CSS_PROP_LETTER_SPACING] = { "letter-spacing", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_LINE_BREAK] = { "line-break", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_LINE_HEIGHT] = { "line-height", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_LIST_STYLE] = { "list-style", PROP_INHERITED | PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_LIST_STYLE_IMAGE] = { "list-style-image", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_LIST_STYLE_POSITION] = { "list-style-position", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_LIST_STYLE_TYPE] = { "list-style-type", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_MARGIN] = { "margin", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_MARGIN_BLOCK] = { "margin-block", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_MARGIN_BLOCK_END] = { "margin-block-end", 0, CSS_PROP_NONE },
    [CSS_PROP_MARGIN_BLOCK_START] = { "margin-block-start", 0, CSS_PROP_NONE },
    [CSS_PROP_MARGIN_BOTTOM] = { "margin-bottom", 0, CSS_PROP_NONE },
    [CSS_PROP_MARGIN_INLINE] = { "margin-inline", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_MARGIN_INLINE_END] = { "margin-inline-end", 0, CSS_PROP_NONE },
    [CSS_PROP_MARGIN_INLINE_START] = { "margin-inline-start", 0, CSS_PROP_NONE },
    [CSS_PROP_MARGIN_LEFT] = { "margin-left", 0, CSS_PROP_NONE },
    [CSS_PROP_MARGIN_RIGHT] = { "margin-right", 0, CSS_PROP_NONE },
    [CSS_PROP_MARGIN_TOP] = { "margin-top", 0, CSS_PROP_NONE },
    [CSS_PROP_MARKER_END] = { "marker-end", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_MARKER_MID] = { "marker-mid", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_MARKER_START] = { "marker-start", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_MASK] = { "mask", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_MASK_CLIP] = { "mask-clip", 0, CSS_PROP_NONE },
    [CSS_PROP_MASK_COMPOSITE] = { "mask-composite", 0, CSS_PROP_NONE },
    [CSS_PROP_MASK_IMAGE] = { "mask-image", 0, CSS_PROP_NONE },
    [CSS_PROP_MASK_MODE] = { "mask-mode", 0, CSS_PROP_NONE },
    [CSS_PROP_MASK_ORIGIN] = { "mask-origin", 0, CSS_PROP_NONE },
    [CSS_PROP_MASK_POSITION] = { "mask-position", 0, CSS_PROP_NONE },
    [CSS_PROP_MASK_REPEAT] = { "mask-repeat", 0, CSS_PROP_NONE },
    [CSS_PROP_MASK_SIZE] = { "mask-size", 0, CSS_PROP_NONE },
    [CSS_PROP_MASK_TYPE] = { "mask-type", 0, CSS_PROP_NONE },
    [CSS_PROP_MATH_DEPTH] = { "math-depth", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_MATH_SHIFT] = { "math-shift", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_MATH_STYLE] = { "math-style", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_MAX_BLOCK_SIZE] = { "max-block-size", 0, CSS_PROP_NONE },
    [CSS_PROP_MAX_HEIGHT] = { "max-height", 0, CSS_PROP_NONE },
    [CSS_PROP_MAX_INLINE_SIZE] = { "max-inline-size", 0, CSS_PROP_NONE },
    [CSS_PROP_MAX_WIDTH] = { "max-width", 0, CSS_PROP_NONE },
    [CSS_PROP_MIN_BLOCK_SIZE] = { "min-block-size", 0, CSS_PROP_NONE },
    [CSS_PROP_MIN_HEIGHT] = { "min-height", 0, CSS_PROP_NONE },
    [CSS_PROP_MIN_INLINE_SIZE] = { "min-inline-size", 0, CSS_PROP_NONE },
    [CSS_PROP_MIN_WIDTH] = { "min-width", 0, CSS_PROP_NONE },
    [CSS_PROP_MIX_BLEND_MODE] = { "mix-blend-mode", 0, CSS_PROP_NONE },
    [CSS_PROP_OBJECT_FIT] = { "object-fit", 0, CSS_PROP_NONE },
    [CSS_PROP_OBJECT_POSITION] = { "object-position", 0, CSS_PROP_NONE },
    [CSS_PROP_OFFSET] = { "offset", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_OFFSET_ANCHOR] = { "offset-anchor", 0, CSS_PROP_NONE },
    [CSS_PROP_OFFSET_DISTANCE] = { "offset-distance", 0, CSS_PROP_NONE },
    [CSS_PROP_OFFSET_PATH] = { "offset-path", 0, CSS_PROP_NONE },
    [CSS_PROP_OFFSET_POSITION] = { "offset-position", 0, CSS_PROP_NONE },
    [CSS_PROP_OFFSET_ROTATE] = { "offset-rotate", 0, CSS_PROP_NONE },
    [CSS_PROP_OPACITY] = { "opacity", 0, CSS_PROP_NONE },
    [CSS_PROP_ORDER] = { "order", 0, CSS_PROP_NONE },
    [CSS_PROP_ORPHANS] = { "orphans", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_OUTLINE] = { "outline", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_OUTLINE_COLOR] = { "outline-color", 0, CSS_PROP_NONE },
    [CSS_PROP_OUTLINE_OFFSET] = { "outline-offset", 0, CSS_PROP_NONE },
    [CSS_PROP_OUTLINE_STYLE] = { "outline-style", 0, CSS_PROP_NONE },
    [CSS_PROP_OUTLINE_WIDTH] = { "outline-width", 0, CSS_PROP_NONE },
    [CSS_PROP_OVERFLOW] = { "overflow", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_OVERFLOW_BLOCK] = { "overflow-block", 0, CSS_PROP_NONE },
    [CSS_PROP_OVERFLOW_INLINE] = { "overflow-inline", 0, CSS_PROP_NONE },
    [CSS_PROP_OVERFLOW_WRAP] = { "overflow-wrap", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_OVERFLOW_X] = { "overflow-x", 0, CSS_PROP_NONE },
    [CSS_PROP_OVERFLOW_Y] = { "overflow-y", 0, CSS_PROP_NONE },
    [CSS_PROP_OVERSCROLL_BEHAVIOR] = { "overscroll-behavior", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_OVERSCROLL_BEHAVIOR_BLOCK] = { "overscroll-behavior-block", 0, CSS_PROP_NONE },
    [CSS_PROP_OVERSCROLL_BEHAVIOR_INLINE] = { "overscroll-behavior-inline", 0, CSS_PROP_NONE },
    [CSS_PROP_OVERSCROLL_BEHAVIOR_X] = { "overscroll-behavior-x", 0, CSS_PROP_NONE },
    [CSS_PROP_OVERSCROLL_BEHAVIOR_Y] = { "overscroll-behavior-y", 0, CSS_PROP_NONE },
    [CSS_PROP_PADDING] = { "padding", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_PADDING_BLOCK] = { "padding-block", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_PADDING_BLOCK_END] = { "padding-block-end", 0, CSS_PROP_NONE },
    [CSS_PROP_PADDING_BLOCK_START] = { "padding-block-start", 0, CSS_PROP_NONE },
    [CSS_PROP_PADDING_BOTTOM] = { "padding-bottom", 0, CSS_PROP_NONE },
    [CSS_PROP_PADDING_INLINE] = { "padding-inline", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_PADDING_INLINE_END] = { "padding-inline-end", 0, CSS_PROP_NONE },
    [CSS_PROP_PADDING_INLINE_START] = { "padding-inline-start", 0, CSS_PROP_NONE },
    [CSS_PROP_PADDING_LEFT] = { "padding-left", 0, CSS_PROP_NONE },
    [CSS_PROP_PADDING_RIGHT] = { "padding-right", 0, CSS_PROP_NONE },
    [CSS_PROP_PADDING_TOP] = { "padding-top", 0, CSS_PROP_NONE },
    [CSS_PROP_PAGE_BREAK_AFTER] = { "page-break-after", 0, CSS_PROP_NONE },
    [CSS_PROP_PAGE_BREAK_BEFORE] = { "page-break-before", 0, CSS_PROP_NONE },
    [CSS_PROP_PAGE_BREAK_INSIDE] = { "page-break-inside", 0, CSS_PROP_NONE },
    [CSS_PROP_PAINT_ORDER] = { "paint-order", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_PERSPECTIVE] = { "perspective", 0, CSS_PROP_NONE },
    [CSS_PROP_PERSPECTIVE_ORIGIN] = { "perspective-origin", 0, CSS_PROP_NONE },
    [CSS_PROP_PLACE_CONTENT] = { "place-content", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_PLACE_ITEMS] = { "place-items", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_PLACE_SELF] = { "place-self", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_POINTER_EVENTS] = { "pointer-events", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_POSITION] = { "position", 0, CSS_PROP_NONE },
    [CSS_PROP_POSITION_ANCHOR] = { "position-anchor", 0, CSS_PROP_NONE },
    [CSS_PROP_POSITION_AREA] = { "position-area", 0, CSS_PROP_NONE },
    [CSS_PROP_POSITION_TRY] = { "position-try", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_POSITION_TRY_FALLBACKS] = { "position-try-fallbacks", 0, CSS_PROP_NONE },
    [CSS_PROP_POSITION_TRY_ORDER] = { "position-try-order", 0, CSS_PROP_NONE },
    [CSS_PROP_POSITION_VISIBILITY] = { "position-visibility", 0, CSS_PROP_NONE },
    [CSS_PROP_PRINT_COLOR_ADJUST] = { "print-color-adjust", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_QUOTES] = { "quotes", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_RESIZE] = { "resize", 0, CSS_PROP_NONE },
    [CSS_PROP_RIGHT] = { "right", 0, CSS_PROP_NONE },
    [CSS_PROP_ROTATE] = { "rotate", 0, CSS_PROP_NONE },
    [CSS_PROP_ROW_GAP] = { "row-gap", 0, CSS_PROP_NONE },
    [CSS_PROP_RUBY_ALIGN] = { "ruby-align", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_RUBY_POSITION] = { "ruby-position", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_SCALE] = { "scale", 0, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_BEHAVIOR] = { "scroll-behavior", 0, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_MARGIN] = { "scroll-margin", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_MARGIN_BLOCK] = { "scroll-margin-block", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_MARGIN_BLOCK_END] = { "scroll-margin-block-end", 0, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_MARGIN_BLOCK_START] = { "scroll-margin-block-start", 0, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_MARGIN_BOTTOM] = { "scroll-margin-bottom", 0, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_MARGIN_INLINE] = { "scroll-margin-inline", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_MARGIN_INLINE_END] = { "scroll-margin-inline-end", 0, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_MARGIN_INLINE_START] = { "scroll-margin-inline-start", 0, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_MARGIN_LEFT] = { "scroll-margin-left", 0, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_MARGIN_RIGHT] = { "scroll-margin-right", 0, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_MARGIN_TOP] = { "scroll-margin-top", 0, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_PADDING] = { "scroll-padding", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_PADDING_BLOCK] = { "scroll-padding-block", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_PADDING_BLOCK_END] = { "scroll-padding-block-end", 0, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_PADDING_BLOCK_START] = { "scroll-padding-block-start", 0, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_PADDING_BOTTOM] = { "scroll-padding-bottom", 0, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_PADDING_INLINE] = { "scroll-padding-inline", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_PADDING_INLINE_END] = { "scroll-padding-inline-end", 0, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_PADDING_INLINE_START] = { "scroll-padding-inline-start", 0, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_PADDING_LEFT] = { "scroll-padding-left", 0, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_PADDING_RIGHT] = { "scroll-padding-right", 0, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_PADDING_TOP] = { "scroll-padding-top", 0, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_SNAP_ALIGN] = { "scroll-snap-align", 0, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_SNAP_STOP] = { "scroll-snap-stop", 0, CSS_PROP_NONE },
    [CSS_PROP_SCROLL_SNAP_TYPE] = { "scroll-snap-type", 0, CSS_PROP_NONE },
    [CSS_PROP_SCROLLBAR_COLOR] = { "scrollbar-color", 0, CSS_PROP_NONE },
    [CSS_PROP_SCROLLBAR_GUTTER] = { "scrollbar-gutter", 0, CSS_PROP_NONE },
    [CSS_PROP_SCROLLBAR_WIDTH] = { "scrollbar-width", 0, CSS_PROP_NONE },
    [CSS_PROP_SHAPE_IMAGE_THRESHOLD] = { "shape-image-threshold", 0, CSS_PROP_NONE },
    [CSS_PROP_SHAPE_MARGIN] = { "shape-margin", 0, CSS_PROP_NONE },
    [CSS_PROP_SHAPE_OUTSIDE] = { "shape-outside", 0, CSS_PROP_NONE },
    [CSS_PROP_STOP_COLOR] = { "stop-color", 0, CSS_PROP_NONE },
    [CSS_PROP_STOP_OPACITY] = { "stop-opacity", 0, CSS_PROP_NONE },
    [CSS_PROP_STROKE] = { "stroke", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_STROKE_DASHARRAY] = { "stroke-dasharray", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_STROKE_DASHOFFSET] = { "stroke-dashoffset", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_STROKE_LINECAP] = { "stroke-linecap", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_STROKE_LINEJOIN] = { "stroke-linejoin", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_STROKE_MITERLIMIT] = { "stroke-miterlimit", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_STROKE_OPACITY] = { "stroke-opacity", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_STROKE_WIDTH] = { "stroke-width", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_TAB_SIZE] = { "tab-size", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_TABLE_LAYOUT] = { "table-layout", 0, CSS_PROP_NONE },
    [CSS_PROP_TEXT_ALIGN] = { "text-align", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_TEXT_ALIGN_LAST] = { "text-align-last", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_TEXT_ANCHOR] = { "text-anchor", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_TEXT_AUTOSPACE] = { "text-autospace", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_TEXT_COMBINE_UPRIGHT] = { "text-combine-upright", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_TEXT_DECORATION] = { "text-decoration", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_TEXT_DECORATION_COLOR] = { "text-decoration-color", 0, CSS_PROP_NONE },
    [CSS_PROP_TEXT_DECORATION_LINE] = { "text-decoration-line", 0, CSS_PROP_NONE },
    [CSS_PROP_TEXT_DECORATION_STYLE] = { "text-decoration-style", 0, CSS_PROP_NONE },
    [CSS_PROP_TEXT_DECORATION_THICKNESS] = { "text-decoration-thickness", 0, CSS_PROP_NONE },
    [CSS_PROP_TEXT_EMPHASIS] = { "text-emphasis", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_TEXT_EMPHASIS_COLOR] = { "text-emphasis-color", 0, CSS_PROP_NONE },
    [CSS_PROP_TEXT_EMPHASIS_POSITION] = { "text-emphasis-position", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_TEXT_EMPHASIS_STYLE] = { "text-emphasis-style", 0, CSS_PROP_NONE },
    [CSS_PROP_TEXT_INDENT] = { "text-indent", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_TEXT_JUSTIFY] = { "text-justify", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_TEXT_ORIENTATION] = { "text-orientation", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_TEXT_OVERFLOW] = { "text-overflow", 0, CSS_PROP_NONE },
    [CSS_PROP_TEXT_SHADOW] = { "text-shadow", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_TEXT_SPACING_TRIM] = { "text-spacing-trim", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_TEXT_TRANSFORM] = { "text-transform", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_TEXT_UNDERLINE_OFFSET] = { "text-underline-offset", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_TEXT_UNDERLINE_POSITION] = { "text-underline-position", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_TEXT_WRAP] = { "text-wrap", PROP_INHERITED | PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_TOP] = { "top", 0, CSS_PROP_NONE },
    [CSS_PROP_TOUCH_ACTION] = { "touch-action", 0, CSS_PROP_NONE },
    [CSS_PROP_TRANSFORM] = { "transform", 0, CSS_PROP_NONE },
    [CSS_PROP_TRANSFORM_ORIGIN] = { "transform-origin", 0, CSS_PROP_NONE },
    [CSS_PROP_TRANSFORM_STYLE] = { "transform-style", 0, CSS_PROP_NONE },
    [CSS_PROP_TRANSITION] = { "transition", PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_TRANSITION_DELAY] = { "transition-delay", 0, CSS_PROP_NONE },
    [CSS_PROP_TRANSITION_DURATION] = { "transition-duration", 0, CSS_PROP_NONE },
    [CSS_PROP_TRANSITION_PROPERTY] = { "transition-property", 0, CSS_PROP_NONE },
    [CSS_PROP_TRANSITION_TIMING_FUNCTION] = { "transition-timing-function", 0, CSS_PROP_NONE },
    [CSS_PROP_TRANSLATE] = { "translate", 0, CSS_PROP_NONE },
    [CSS_PROP_UNICODE_BIDI] = { "unicode-bidi", 0, CSS_PROP_NONE },
    [CSS_PROP_USER_SELECT] = { "user-select", 0, CSS_PROP_NONE },
    [CSS_PROP_VECTOR_EFFECT] = { "vector-effect", 0, CSS_PROP_NONE },
    [CSS_PROP_VERTICAL_ALIGN] = { "vertical-align", 0, CSS_PROP_NONE },
    [CSS_PROP_VIEW_TRANSITION_CLASS] = { "view-transition-class", 0, CSS_PROP_NONE },
    [CSS_PROP_VIEW_TRANSITION_NAME] = { "view-transition-name", 0, CSS_PROP_NONE },
    [CSS_PROP_VISIBILITY] = { "visibility", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_WHITE_SPACE] = { "white-space", PROP_INHERITED | PROP_SHORTHAND, CSS_PROP_NONE },
    [CSS_PROP_WHITE_SPACE_COLLAPSE] = { "white-space-collapse", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_WIDOWS] = { "widows", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_WIDTH] = { "width", 0, CSS_PROP_NONE },
    [CSS_PROP_WILL_CHANGE] = { "will-change", 0, CSS_PROP_NONE },
    [CSS_PROP_WORD_BREAK] = { "word-break", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_WORD_SPACING] = { "word-spacing", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_WORD_WRAP] = { "word-wrap", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_WRITING_MODE] = { "writing-mode", PROP_INHERITED, CSS_PROP_NONE },
    [CSS_PROP_Z_INDEX] = { "z-index", 0, CSS_PROP_NONE },
};

/* ------------------------------------------------------------ */
/*  Lookup a property by name (linear scan)                     */
/* ------------------------------------------------------------ */

CssPropId css_prop_from_name(const char *name, size_t len)
{
    if (!name)
        return CSS_PROP_NONE;

    for (int i = 1; i < CSS_PROP__COUNT; i++) {
        const char *pname = prop_table[i].name;
        if (strlen(pname) == len && memcmp(pname, name, len) == 0)
            return (CssPropId)i;
    }
    return CSS_PROP_NONE;
}

/* ------------------------------------------------------------ */
/*  Property info accessors                                     */
/* ------------------------------------------------------------ */

const CssPropInfo *css_prop_info(CssPropId id)
{
    if (id < 0 || id >= CSS_PROP__COUNT)
        return &prop_table[CSS_PROP_NONE];
    return &prop_table[id];
}

const char *css_prop_name(CssPropId id)
{
    if (id < 0 || id >= CSS_PROP__COUNT)
        return prop_table[CSS_PROP_NONE].name;
    return prop_table[id].name;
}

bool css_prop_inherited(CssPropId id)
{
    if (id >= CSS_PROP__COUNT)
        return false;
    return (prop_table[id].flags & PROP_INHERITED) != 0;
}

bool css_prop_is_shorthand(CssPropId id)
{
    if (id >= CSS_PROP__COUNT)
        return false;
    return (prop_table[id].flags & PROP_SHORTHAND) != 0;
}

/* ------------------------------------------------------------ */
/*  Shorthand expansion (74 shorthands from Reference Table 1)  */
/* ------------------------------------------------------------ */

int css_prop_expand_shorthand(CssPropId shorthand, CssPropId *out, int max_out)
{
    int n = 0;

#define EMIT(prop) do { if (n < max_out) out[n] = (prop); n++; } while (0)

    switch (shorthand) {
    case CSS_PROP_ANIMATION:
        EMIT(CSS_PROP_ANIMATION_DELAY);
        EMIT(CSS_PROP_ANIMATION_DIRECTION);
        EMIT(CSS_PROP_ANIMATION_DURATION);
        EMIT(CSS_PROP_ANIMATION_FILL_MODE);
        EMIT(CSS_PROP_ANIMATION_ITERATION_COUNT);
        EMIT(CSS_PROP_ANIMATION_NAME);
        EMIT(CSS_PROP_ANIMATION_PLAY_STATE);
        EMIT(CSS_PROP_ANIMATION_TIMING_FUNCTION);
        break;
    case CSS_PROP_BACKGROUND:
        EMIT(CSS_PROP_BACKGROUND_ATTACHMENT);
        EMIT(CSS_PROP_BACKGROUND_CLIP);
        EMIT(CSS_PROP_BACKGROUND_COLOR);
        EMIT(CSS_PROP_BACKGROUND_IMAGE);
        EMIT(CSS_PROP_BACKGROUND_ORIGIN);
        EMIT(CSS_PROP_BACKGROUND_POSITION);
        EMIT(CSS_PROP_BACKGROUND_REPEAT);
        EMIT(CSS_PROP_BACKGROUND_SIZE);
        break;
    case CSS_PROP_BORDER:
        EMIT(CSS_PROP_BORDER_BOTTOM_COLOR);
        EMIT(CSS_PROP_BORDER_BOTTOM_STYLE);
        EMIT(CSS_PROP_BORDER_BOTTOM_WIDTH);
        EMIT(CSS_PROP_BORDER_LEFT_COLOR);
        EMIT(CSS_PROP_BORDER_LEFT_STYLE);
        EMIT(CSS_PROP_BORDER_LEFT_WIDTH);
        EMIT(CSS_PROP_BORDER_RIGHT_COLOR);
        EMIT(CSS_PROP_BORDER_RIGHT_STYLE);
        EMIT(CSS_PROP_BORDER_RIGHT_WIDTH);
        EMIT(CSS_PROP_BORDER_TOP_COLOR);
        EMIT(CSS_PROP_BORDER_TOP_STYLE);
        EMIT(CSS_PROP_BORDER_TOP_WIDTH);
        break;
    case CSS_PROP_BORDER_BLOCK:
        EMIT(CSS_PROP_BORDER_BLOCK_END_COLOR);
        EMIT(CSS_PROP_BORDER_BLOCK_END_STYLE);
        EMIT(CSS_PROP_BORDER_BLOCK_END_WIDTH);
        EMIT(CSS_PROP_BORDER_BLOCK_START_COLOR);
        EMIT(CSS_PROP_BORDER_BLOCK_START_STYLE);
        EMIT(CSS_PROP_BORDER_BLOCK_START_WIDTH);
        break;
    case CSS_PROP_BORDER_BLOCK_COLOR:
        EMIT(CSS_PROP_BORDER_BLOCK_END_COLOR);
        EMIT(CSS_PROP_BORDER_BLOCK_START_COLOR);
        break;
    case CSS_PROP_BORDER_BLOCK_END:
        EMIT(CSS_PROP_BORDER_BLOCK_END_COLOR);
        EMIT(CSS_PROP_BORDER_BLOCK_END_STYLE);
        EMIT(CSS_PROP_BORDER_BLOCK_END_WIDTH);
        break;
    case CSS_PROP_BORDER_BLOCK_START:
        EMIT(CSS_PROP_BORDER_BLOCK_START_COLOR);
        EMIT(CSS_PROP_BORDER_BLOCK_START_STYLE);
        EMIT(CSS_PROP_BORDER_BLOCK_START_WIDTH);
        break;
    case CSS_PROP_BORDER_BLOCK_STYLE:
        EMIT(CSS_PROP_BORDER_BLOCK_END_STYLE);
        EMIT(CSS_PROP_BORDER_BLOCK_START_STYLE);
        break;
    case CSS_PROP_BORDER_BLOCK_WIDTH:
        EMIT(CSS_PROP_BORDER_BLOCK_END_WIDTH);
        EMIT(CSS_PROP_BORDER_BLOCK_START_WIDTH);
        break;
    case CSS_PROP_BORDER_BOTTOM:
        EMIT(CSS_PROP_BORDER_BOTTOM_COLOR);
        EMIT(CSS_PROP_BORDER_BOTTOM_STYLE);
        EMIT(CSS_PROP_BORDER_BOTTOM_WIDTH);
        break;
    case CSS_PROP_BORDER_COLOR:
        EMIT(CSS_PROP_BORDER_BOTTOM_COLOR);
        EMIT(CSS_PROP_BORDER_LEFT_COLOR);
        EMIT(CSS_PROP_BORDER_RIGHT_COLOR);
        EMIT(CSS_PROP_BORDER_TOP_COLOR);
        break;
    case CSS_PROP_BORDER_IMAGE:
        EMIT(CSS_PROP_BORDER_IMAGE_OUTSET);
        EMIT(CSS_PROP_BORDER_IMAGE_REPEAT);
        EMIT(CSS_PROP_BORDER_IMAGE_SLICE);
        EMIT(CSS_PROP_BORDER_IMAGE_SOURCE);
        EMIT(CSS_PROP_BORDER_IMAGE_WIDTH);
        break;
    case CSS_PROP_BORDER_INLINE:
        EMIT(CSS_PROP_BORDER_INLINE_END_COLOR);
        EMIT(CSS_PROP_BORDER_INLINE_END_STYLE);
        EMIT(CSS_PROP_BORDER_INLINE_END_WIDTH);
        EMIT(CSS_PROP_BORDER_INLINE_START_COLOR);
        EMIT(CSS_PROP_BORDER_INLINE_START_STYLE);
        EMIT(CSS_PROP_BORDER_INLINE_START_WIDTH);
        break;
    case CSS_PROP_BORDER_INLINE_COLOR:
        EMIT(CSS_PROP_BORDER_INLINE_END_COLOR);
        EMIT(CSS_PROP_BORDER_INLINE_START_COLOR);
        break;
    case CSS_PROP_BORDER_INLINE_END:
        EMIT(CSS_PROP_BORDER_INLINE_END_COLOR);
        EMIT(CSS_PROP_BORDER_INLINE_END_STYLE);
        EMIT(CSS_PROP_BORDER_INLINE_END_WIDTH);
        break;
    case CSS_PROP_BORDER_INLINE_START:
        EMIT(CSS_PROP_BORDER_INLINE_START_COLOR);
        EMIT(CSS_PROP_BORDER_INLINE_START_STYLE);
        EMIT(CSS_PROP_BORDER_INLINE_START_WIDTH);
        break;
    case CSS_PROP_BORDER_INLINE_STYLE:
        EMIT(CSS_PROP_BORDER_INLINE_END_STYLE);
        EMIT(CSS_PROP_BORDER_INLINE_START_STYLE);
        break;
    case CSS_PROP_BORDER_INLINE_WIDTH:
        EMIT(CSS_PROP_BORDER_INLINE_END_WIDTH);
        EMIT(CSS_PROP_BORDER_INLINE_START_WIDTH);
        break;
    case CSS_PROP_BORDER_LEFT:
        EMIT(CSS_PROP_BORDER_LEFT_COLOR);
        EMIT(CSS_PROP_BORDER_LEFT_STYLE);
        EMIT(CSS_PROP_BORDER_LEFT_WIDTH);
        break;
    case CSS_PROP_BORDER_RADIUS:
        EMIT(CSS_PROP_BORDER_BOTTOM_LEFT_RADIUS);
        EMIT(CSS_PROP_BORDER_BOTTOM_RIGHT_RADIUS);
        EMIT(CSS_PROP_BORDER_TOP_LEFT_RADIUS);
        EMIT(CSS_PROP_BORDER_TOP_RIGHT_RADIUS);
        break;
    case CSS_PROP_BORDER_RIGHT:
        EMIT(CSS_PROP_BORDER_RIGHT_COLOR);
        EMIT(CSS_PROP_BORDER_RIGHT_STYLE);
        EMIT(CSS_PROP_BORDER_RIGHT_WIDTH);
        break;
    case CSS_PROP_BORDER_STYLE:
        EMIT(CSS_PROP_BORDER_BOTTOM_STYLE);
        EMIT(CSS_PROP_BORDER_LEFT_STYLE);
        EMIT(CSS_PROP_BORDER_RIGHT_STYLE);
        EMIT(CSS_PROP_BORDER_TOP_STYLE);
        break;
    case CSS_PROP_BORDER_TOP:
        EMIT(CSS_PROP_BORDER_TOP_COLOR);
        EMIT(CSS_PROP_BORDER_TOP_STYLE);
        EMIT(CSS_PROP_BORDER_TOP_WIDTH);
        break;
    case CSS_PROP_BORDER_WIDTH:
        EMIT(CSS_PROP_BORDER_BOTTOM_WIDTH);
        EMIT(CSS_PROP_BORDER_LEFT_WIDTH);
        EMIT(CSS_PROP_BORDER_RIGHT_WIDTH);
        EMIT(CSS_PROP_BORDER_TOP_WIDTH);
        break;
    case CSS_PROP_COLUMN_RULE:
        EMIT(CSS_PROP_COLUMN_RULE_COLOR);
        EMIT(CSS_PROP_COLUMN_RULE_STYLE);
        EMIT(CSS_PROP_COLUMN_RULE_WIDTH);
        break;
    case CSS_PROP_COLUMNS:
        EMIT(CSS_PROP_COLUMN_COUNT);
        EMIT(CSS_PROP_COLUMN_WIDTH);
        break;
    case CSS_PROP_CONTAIN_INTRINSIC_SIZE:
        EMIT(CSS_PROP_CONTAIN_INTRINSIC_HEIGHT);
        EMIT(CSS_PROP_CONTAIN_INTRINSIC_WIDTH);
        break;
    case CSS_PROP_CONTAINER:
        EMIT(CSS_PROP_CONTAINER_NAME);
        EMIT(CSS_PROP_CONTAINER_TYPE);
        break;
    case CSS_PROP_FLEX:
        EMIT(CSS_PROP_FLEX_BASIS);
        EMIT(CSS_PROP_FLEX_GROW);
        EMIT(CSS_PROP_FLEX_SHRINK);
        break;
    case CSS_PROP_FLEX_FLOW:
        EMIT(CSS_PROP_FLEX_DIRECTION);
        EMIT(CSS_PROP_FLEX_WRAP);
        break;
    case CSS_PROP_FONT:
        EMIT(CSS_PROP_FONT_FAMILY);
        EMIT(CSS_PROP_FONT_SIZE);
        EMIT(CSS_PROP_FONT_STRETCH);
        EMIT(CSS_PROP_FONT_STYLE);
        EMIT(CSS_PROP_FONT_VARIANT);
        EMIT(CSS_PROP_FONT_WEIGHT);
        EMIT(CSS_PROP_LINE_HEIGHT);
        break;
    case CSS_PROP_FONT_SYNTHESIS:
        break;
    case CSS_PROP_FONT_VARIANT:
        EMIT(CSS_PROP_FONT_VARIANT_ALTERNATES);
        EMIT(CSS_PROP_FONT_VARIANT_CAPS);
        EMIT(CSS_PROP_FONT_VARIANT_EAST_ASIAN);
        EMIT(CSS_PROP_FONT_VARIANT_LIGATURES);
        EMIT(CSS_PROP_FONT_VARIANT_NUMERIC);
        EMIT(CSS_PROP_FONT_VARIANT_POSITION);
        break;
    case CSS_PROP_GAP:
        EMIT(CSS_PROP_COLUMN_GAP);
        EMIT(CSS_PROP_ROW_GAP);
        break;
    case CSS_PROP_GRID:
        EMIT(CSS_PROP_GRID_AUTO_COLUMNS);
        EMIT(CSS_PROP_GRID_AUTO_FLOW);
        EMIT(CSS_PROP_GRID_AUTO_ROWS);
        EMIT(CSS_PROP_GRID_TEMPLATE_AREAS);
        EMIT(CSS_PROP_GRID_TEMPLATE_COLUMNS);
        EMIT(CSS_PROP_GRID_TEMPLATE_ROWS);
        break;
    case CSS_PROP_GRID_AREA:
        EMIT(CSS_PROP_GRID_COLUMN_END);
        EMIT(CSS_PROP_GRID_COLUMN_START);
        EMIT(CSS_PROP_GRID_ROW_END);
        EMIT(CSS_PROP_GRID_ROW_START);
        break;
    case CSS_PROP_GRID_COLUMN:
        EMIT(CSS_PROP_GRID_COLUMN_END);
        EMIT(CSS_PROP_GRID_COLUMN_START);
        break;
    case CSS_PROP_GRID_ROW:
        EMIT(CSS_PROP_GRID_ROW_END);
        EMIT(CSS_PROP_GRID_ROW_START);
        break;
    case CSS_PROP_GRID_TEMPLATE:
        EMIT(CSS_PROP_GRID_TEMPLATE_AREAS);
        EMIT(CSS_PROP_GRID_TEMPLATE_COLUMNS);
        EMIT(CSS_PROP_GRID_TEMPLATE_ROWS);
        break;
    case CSS_PROP_INSET:
        EMIT(CSS_PROP_BOTTOM);
        EMIT(CSS_PROP_LEFT);
        EMIT(CSS_PROP_RIGHT);
        EMIT(CSS_PROP_TOP);
        break;
    case CSS_PROP_INSET_BLOCK:
        EMIT(CSS_PROP_INSET_BLOCK_END);
        EMIT(CSS_PROP_INSET_BLOCK_START);
        break;
    case CSS_PROP_INSET_INLINE:
        EMIT(CSS_PROP_INSET_INLINE_END);
        EMIT(CSS_PROP_INSET_INLINE_START);
        break;
    case CSS_PROP_LIST_STYLE:
        EMIT(CSS_PROP_LIST_STYLE_IMAGE);
        EMIT(CSS_PROP_LIST_STYLE_POSITION);
        EMIT(CSS_PROP_LIST_STYLE_TYPE);
        break;
    case CSS_PROP_MARGIN:
        EMIT(CSS_PROP_MARGIN_BOTTOM);
        EMIT(CSS_PROP_MARGIN_LEFT);
        EMIT(CSS_PROP_MARGIN_RIGHT);
        EMIT(CSS_PROP_MARGIN_TOP);
        break;
    case CSS_PROP_MARGIN_BLOCK:
        EMIT(CSS_PROP_MARGIN_BLOCK_END);
        EMIT(CSS_PROP_MARGIN_BLOCK_START);
        break;
    case CSS_PROP_MARGIN_INLINE:
        EMIT(CSS_PROP_MARGIN_INLINE_END);
        EMIT(CSS_PROP_MARGIN_INLINE_START);
        break;
    case CSS_PROP_MASK:
        EMIT(CSS_PROP_MASK_CLIP);
        EMIT(CSS_PROP_MASK_COMPOSITE);
        EMIT(CSS_PROP_MASK_IMAGE);
        EMIT(CSS_PROP_MASK_MODE);
        EMIT(CSS_PROP_MASK_ORIGIN);
        EMIT(CSS_PROP_MASK_POSITION);
        EMIT(CSS_PROP_MASK_REPEAT);
        EMIT(CSS_PROP_MASK_SIZE);
        break;
    case CSS_PROP_OFFSET:
        EMIT(CSS_PROP_OFFSET_ANCHOR);
        EMIT(CSS_PROP_OFFSET_DISTANCE);
        EMIT(CSS_PROP_OFFSET_PATH);
        EMIT(CSS_PROP_OFFSET_POSITION);
        EMIT(CSS_PROP_OFFSET_ROTATE);
        break;
    case CSS_PROP_OUTLINE:
        EMIT(CSS_PROP_OUTLINE_COLOR);
        EMIT(CSS_PROP_OUTLINE_STYLE);
        EMIT(CSS_PROP_OUTLINE_WIDTH);
        break;
    case CSS_PROP_OVERFLOW:
        EMIT(CSS_PROP_OVERFLOW_X);
        EMIT(CSS_PROP_OVERFLOW_Y);
        break;
    case CSS_PROP_OVERSCROLL_BEHAVIOR:
        EMIT(CSS_PROP_OVERSCROLL_BEHAVIOR_X);
        EMIT(CSS_PROP_OVERSCROLL_BEHAVIOR_Y);
        break;
    case CSS_PROP_PADDING:
        EMIT(CSS_PROP_PADDING_BOTTOM);
        EMIT(CSS_PROP_PADDING_LEFT);
        EMIT(CSS_PROP_PADDING_RIGHT);
        EMIT(CSS_PROP_PADDING_TOP);
        break;
    case CSS_PROP_PADDING_BLOCK:
        EMIT(CSS_PROP_PADDING_BLOCK_END);
        EMIT(CSS_PROP_PADDING_BLOCK_START);
        break;
    case CSS_PROP_PADDING_INLINE:
        EMIT(CSS_PROP_PADDING_INLINE_END);
        EMIT(CSS_PROP_PADDING_INLINE_START);
        break;
    case CSS_PROP_PLACE_CONTENT:
        EMIT(CSS_PROP_ALIGN_CONTENT);
        EMIT(CSS_PROP_JUSTIFY_CONTENT);
        break;
    case CSS_PROP_PLACE_ITEMS:
        EMIT(CSS_PROP_ALIGN_ITEMS);
        EMIT(CSS_PROP_JUSTIFY_ITEMS);
        break;
    case CSS_PROP_PLACE_SELF:
        EMIT(CSS_PROP_ALIGN_SELF);
        EMIT(CSS_PROP_JUSTIFY_SELF);
        break;
    case CSS_PROP_POSITION_TRY:
        EMIT(CSS_PROP_POSITION_TRY_FALLBACKS);
        EMIT(CSS_PROP_POSITION_TRY_ORDER);
        break;
    case CSS_PROP_SCROLL_MARGIN:
        EMIT(CSS_PROP_SCROLL_MARGIN_BOTTOM);
        EMIT(CSS_PROP_SCROLL_MARGIN_LEFT);
        EMIT(CSS_PROP_SCROLL_MARGIN_RIGHT);
        EMIT(CSS_PROP_SCROLL_MARGIN_TOP);
        break;
    case CSS_PROP_SCROLL_MARGIN_BLOCK:
        EMIT(CSS_PROP_SCROLL_MARGIN_BLOCK_END);
        EMIT(CSS_PROP_SCROLL_MARGIN_BLOCK_START);
        break;
    case CSS_PROP_SCROLL_MARGIN_INLINE:
        EMIT(CSS_PROP_SCROLL_MARGIN_INLINE_END);
        EMIT(CSS_PROP_SCROLL_MARGIN_INLINE_START);
        break;
    case CSS_PROP_SCROLL_PADDING:
        EMIT(CSS_PROP_SCROLL_PADDING_BOTTOM);
        EMIT(CSS_PROP_SCROLL_PADDING_LEFT);
        EMIT(CSS_PROP_SCROLL_PADDING_RIGHT);
        EMIT(CSS_PROP_SCROLL_PADDING_TOP);
        break;
    case CSS_PROP_SCROLL_PADDING_BLOCK:
        EMIT(CSS_PROP_SCROLL_PADDING_BLOCK_END);
        EMIT(CSS_PROP_SCROLL_PADDING_BLOCK_START);
        break;
    case CSS_PROP_SCROLL_PADDING_INLINE:
        EMIT(CSS_PROP_SCROLL_PADDING_INLINE_END);
        EMIT(CSS_PROP_SCROLL_PADDING_INLINE_START);
        break;
    case CSS_PROP_TEXT_DECORATION:
        EMIT(CSS_PROP_TEXT_DECORATION_COLOR);
        EMIT(CSS_PROP_TEXT_DECORATION_LINE);
        EMIT(CSS_PROP_TEXT_DECORATION_STYLE);
        EMIT(CSS_PROP_TEXT_DECORATION_THICKNESS);
        break;
    case CSS_PROP_TEXT_EMPHASIS:
        EMIT(CSS_PROP_TEXT_EMPHASIS_COLOR);
        EMIT(CSS_PROP_TEXT_EMPHASIS_STYLE);
        break;
    case CSS_PROP_TEXT_WRAP:
        break;
    case CSS_PROP_TRANSITION:
        EMIT(CSS_PROP_TRANSITION_DELAY);
        EMIT(CSS_PROP_TRANSITION_DURATION);
        EMIT(CSS_PROP_TRANSITION_PROPERTY);
        EMIT(CSS_PROP_TRANSITION_TIMING_FUNCTION);
        break;
    case CSS_PROP_WHITE_SPACE:
        EMIT(CSS_PROP_WHITE_SPACE_COLLAPSE);
        break;
    default:
        break;
    }

#undef EMIT

    return n;
}

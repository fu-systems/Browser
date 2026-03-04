cmake_minimum_required(VERSION 3.20)
project(pane VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Source files ─────────────────────────────────────────────────────

set(PANE_SOURCES
    # Utilities
    src/util/arena.c
    src/util/str.c
    src/util/map.c
    # DOM
    src/dom/dom.c
    # HTML parser
    src/html/tokenizer.c
    src/html/tree_builder.c
    # CSS
    src/css/values.c
    src/css/css_tokenizer.c
    src/css/selectors.c
    src/css/properties.c
    src/css/css_parser.c
    src/css/cascade.c
    # Style
    src/style/computed.c
    src/style/context.c
    # Layout
    src/layout/box.c
    src/layout/block.c
    src/layout/inline.c
    src/layout/layout.c
    # Paint
    src/paint/paint.c
)

# ── Engine library ───────────────────────────────────────────────────

add_library(pane_engine ${PANE_SOURCES})

target_include_directories(pane_engine PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(pane_engine PRIVATE m)

# ── Main executable ──────────────────────────────────────────────────

add_executable(pane src/main.c)

target_link_libraries(pane PRIVATE pane_engine m)

target_include_directories(pane PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

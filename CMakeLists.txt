cmake_minimum_required(VERSION 3.20)
project(pane VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Find dependencies ───────────────────────────────────────────────

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(CAIRO REQUIRED cairo)
pkg_check_modules(FREETYPE REQUIRED freetype2)
pkg_check_modules(FONTCONFIG REQUIRED fontconfig)

# ── Core engine sources ─────────────────────────────────────────────

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
    # Public API
    src/pane.c
)

# ── Engine library ───────────────────────────────────────────────────

add_library(pane_engine ${PANE_SOURCES})

target_include_directories(pane_engine PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(pane_engine PRIVATE m)

# ── CLI demo executable ─────────────────────────────────────────────

add_executable(pane_cli src/main.c)

target_link_libraries(pane_cli PRIVATE pane_engine m)

target_include_directories(pane_cli PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# ── Font rendering library ──────────────────────────────────────────

add_library(pane_font
    src/font/font.c
)

target_include_directories(pane_font PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${FREETYPE_INCLUDE_DIRS}
    ${FONTCONFIG_INCLUDE_DIRS}
)

target_link_libraries(pane_font PUBLIC
    ${FREETYPE_LIBRARIES}
    ${FONTCONFIG_LIBRARIES}
)

# ── Cairo paint backend ─────────────────────────────────────────────

add_library(pane_cairo
    src/paint/cairo_backend.c
)

target_include_directories(pane_cairo PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CAIRO_INCLUDE_DIRS}
)

target_link_libraries(pane_cairo PUBLIC
    pane_engine
    pane_font
    ${CAIRO_LIBRARIES}
)

# ── Browser GUI executable ──────────────────────────────────────────

add_executable(pane
    src/ui/browser.c
    src/ui/main_gui.c
)

target_include_directories(pane PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${GTK3_INCLUDE_DIRS}
)

target_link_libraries(pane PRIVATE
    pane_engine
    pane_cairo
    pane_font
    ${GTK3_LIBRARIES}
    m
)

target_compile_options(pane PRIVATE ${GTK3_CFLAGS_OTHER})

# ── Offscreen render test ────────────────────────────────────────────

add_executable(pane_render_test src/ui/render_test.c)

target_include_directories(pane_render_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CAIRO_INCLUDE_DIRS}
)

target_link_libraries(pane_render_test PRIVATE
    pane_engine
    pane_cairo
    pane_font
    ${CAIRO_LIBRARIES}
    m
)

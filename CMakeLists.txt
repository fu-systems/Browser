cmake_minimum_required(VERSION 3.20)
project(pane VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Platform detection ───────────────────────────────────────────────

if(WIN32)
    set(PANE_PLATFORM "win32")
else()
    set(PANE_PLATFORM "linux")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    pkg_check_modules(CAIRO REQUIRED cairo)
    pkg_check_modules(FREETYPE REQUIRED freetype2)
    pkg_check_modules(FONTCONFIG REQUIRED fontconfig)
endif()

message(STATUS "Pane Browser — platform: ${PANE_PLATFORM}")

# ── Core engine sources (platform-independent) ──────────────────────

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
    src/layout/flex.c
    src/layout/grid.c
    src/layout/table.c
    src/layout/layout.c
    # Paint
    src/paint/paint.c
    # Plugin system
    src/plugin/plugin.c
    src/plugin/plugin_registry.c
    src/plugin/plugin_pipeline.c
    src/plugin/builtin/privacy_shield.c
    src/plugin/builtin/dark_mode.c
    src/plugin/builtin/cookie_manager.c
    # Public API
    src/pane.c
)

# ── Engine library ───────────────────────────────────────────────────

add_library(pane_engine ${PANE_SOURCES})

target_include_directories(pane_engine PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

if(WIN32)
    target_link_libraries(pane_engine PRIVATE)
else()
    target_link_libraries(pane_engine PRIVATE m)
endif()

# ── CLI demo executable ─────────────────────────────────────────────

add_executable(pane_cli src/main.c)

target_link_libraries(pane_cli PRIVATE pane_engine)
if(NOT WIN32)
    target_link_libraries(pane_cli PRIVATE m)
endif()

target_include_directories(pane_cli PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# ── Platform-specific builds ─────────────────────────────────────────

if(WIN32)
    # ── Windows: Win32 GDI browser ───────────────────────────────

    # Find FreeType (via vcpkg, manual install, or system)
    find_package(Freetype QUIET)
    if(Freetype_FOUND)
        message(STATUS "FreeType found: ${FREETYPE_LIBRARIES}")
    else()
        message(STATUS "FreeType not found — building without font rendering")
    endif()

    # HTTP client library (WinHTTP)
    add_library(pane_net src/net/http_win32.c)
    target_include_directories(pane_net PUBLIC ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(pane_net PUBLIC winhttp)

    # Font library (optional — uses FreeType if available)
    if(Freetype_FOUND)
        add_library(pane_font src/font/font.c)
        target_include_directories(pane_font PUBLIC
            ${CMAKE_SOURCE_DIR}/src
            ${FREETYPE_INCLUDE_DIRS}
        )
        target_link_libraries(pane_font PUBLIC Freetype::Freetype)
    endif()

    # Win32 browser
    add_executable(pane WIN32
        src/ui/win32_browser.c
        src/ui/main_win32.c
    )
    target_include_directories(pane PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(pane PRIVATE
        pane_engine
        pane_net
        gdi32 user32 comctl32
    )
    if(Freetype_FOUND)
        target_link_libraries(pane PRIVATE pane_font)
    endif()

else()
    # ── Linux: GTK3 + Cairo + FreeType browser ───────────────────

    # OpenSSL for HTTPS
    find_package(OpenSSL REQUIRED)

    # HTTP client library
    add_library(pane_net src/net/http.c)
    target_include_directories(pane_net PUBLIC ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(pane_net PUBLIC OpenSSL::SSL OpenSSL::Crypto)

    # Font library
    add_library(pane_font src/font/font.c)
    target_include_directories(pane_font PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${FREETYPE_INCLUDE_DIRS}
        ${FONTCONFIG_INCLUDE_DIRS}
    )
    target_link_libraries(pane_font PUBLIC
        ${FREETYPE_LIBRARIES}
        ${FONTCONFIG_LIBRARIES}
    )

    # Cairo paint backend
    add_library(pane_cairo src/paint/cairo_backend.c)
    target_include_directories(pane_cairo PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${CAIRO_INCLUDE_DIRS}
    )
    target_link_libraries(pane_cairo PUBLIC
        pane_engine
        pane_font
        ${CAIRO_LIBRARIES}
    )

    # GTK3 browser
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
        pane_net
        ${GTK3_LIBRARIES}
        m
    )
    target_compile_options(pane PRIVATE ${GTK3_CFLAGS_OTHER})

    # Offscreen render test
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
endif()

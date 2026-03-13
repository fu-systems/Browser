# Pane Browser — Windows Build Configuration
#
# This file is included by the main CMakeLists.txt when building for Windows.
# It builds the Win32 native browser (no GTK dependency).
#
# Usage (MSVC):
#   mkdir build-win && cd build-win
#   cmake .. -DPANE_WIN32=ON
#   cmake --build . --config Release
#
# Usage (MinGW cross-compile):
#   mkdir build-win && cd build-win
#   cmake .. -DCMAKE_TOOLCHAIN_FILE=cmake/win32-toolchain.cmake -DPANE_WIN32=ON
#   make -j

# Engine is always built the same way (pure C, no platform deps).
# Font module needs FreeType (get via vcpkg or download).
# UI uses Win32 GDI instead of GTK3.

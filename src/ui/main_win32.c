/*
 * Pane Browser — Windows Entry Point
 *
 * Uses native Win32 GDI for the UI (no GTK dependency).
 */

#ifdef _WIN32

#include "win32_browser.h"

#ifdef _MSC_VER
#include <windows.h>
#include <stdlib.h>
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmdLine, int show)
{
    (void)hInst; (void)hPrev; (void)cmdLine; (void)show;
    return win32_browser_run(__argc, __argv);
}
#else
int main(int argc, char **argv)
{
    return win32_browser_run(argc, argv);
}
#endif

#else
#include <stdio.h>
int main(void)
{
    fprintf(stderr, "This binary is for Windows only.\n");
    return 1;
}
#endif

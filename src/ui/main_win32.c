/*
 * Pane Browser — Windows Entry Point
 *
 * Uses native Win32 GDI for the UI (no GTK dependency).
 */

#ifdef _WIN32

#include "win32_browser.h"

int main(int argc, char **argv)
{
    return win32_browser_run(argc, argv);
}

#else
#include <stdio.h>
int main(void)
{
    fprintf(stderr, "This binary is for Windows only.\n");
    return 1;
}
#endif

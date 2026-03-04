/*
 * Pane — Win32 Browser UI
 *
 * Native Windows GUI using Win32 API + GDI for rendering.
 * No GTK dependency — uses only the Windows SDK.
 */

#ifndef PANE_WIN32_BROWSER_H
#define PANE_WIN32_BROWSER_H

#ifdef _WIN32

#include "../pane.h"

/* Launch the Win32 browser window.
 * This function enters the Win32 message loop and doesn't return
 * until the window is closed. */
int win32_browser_run(int argc, char **argv);

#endif /* _WIN32 */
#endif /* PANE_WIN32_BROWSER_H */

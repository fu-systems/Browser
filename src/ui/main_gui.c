/*
 * Pane Browser — GUI Entry Point
 *
 * Launches the GTK3 browser window with the Pane rendering engine.
 */

#include "browser.h"
#include <gtk/gtk.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);

    printf("Pane Browser v0.1.0\n");
    printf("GTK3 + Cairo + FreeType rendering engine\n");

    BrowserWindow *bw = browser_window_new();

    /* If a file argument is provided, navigate to it. */
    if (argc > 1) {
        browser_navigate(bw, argv[1]);
    }

    browser_run();

    browser_window_free(bw);

    printf("Pane Browser exited.\n");
    return 0;
}

/*
 * alock - bg_shade.c
 * Copyright (c) 2005 - 2007 Mathias Gumz <akira at fluxbox dot org>
 *               2014 Arkadiusz Bokowy
 *
 * This file is a part of an alock.
 *
 * This projected is licensed under the terms of the MIT license.
 *
 * This background module provides:
 *  -bg shade:color=<color>,shade=<int>
 *
 */

#include <stdlib.h>
#include <string.h>
#include <X11/extensions/Xrender.h>

#include "alock.h"


static Window *window = NULL;


static int alock_bg_shade_init(const char *args, struct aXInfo *xinfo) {

    if (!xinfo)
        return 0;

    XColor color;
    char *color_name = NULL;
    unsigned int shade = 80;
    int status = 1;

    Pixmap src_pm = None;
    Pixmap dst_pm = None;

    int width = 0;
    int height = 0;

    if (args && strstr(args, "shade:") == args) {
        char *arguments = strdup(&args[6]);
        char *arg;
        char *tmp;
        for (tmp = arguments; tmp; ) {
            arg = strsep(&tmp, ",");
            if (strstr(arg, "color=") == arg) {
                free(color_name);
                color_name = strdup(&arg[6]);
            }
            else if (strstr(arg, "shade=") == arg) {
                shade = strtol(&arg[6], NULL, 0);
                if (shade > 99) {
                    fprintf(stderr, "[shade]: shade not in range [0, 99]\n");
                    free(arguments);
                    goto return_error;
                }
            }
        }
        free(arguments);
    }

    if (!alock_check_xrender(xinfo))
        goto return_error;

    window = (Window*)malloc(sizeof(Window) * xinfo->screens);

    {
        int scr;
        for (scr = 0; scr < xinfo->screens; scr++) {

            /* get a color from color_name */
            alock_alloc_color(xinfo, scr, color_name, "black", &color);

            width = xinfo->root_width[scr];
            height = xinfo->root_height[scr];

            { /* xrender stuff */
                Display *dpy = xinfo->display;
                Window root = xinfo->root[scr];
                int depth = DefaultDepth(dpy, scr);
                GC gc = DefaultGC(dpy, scr);

                { /* grab whats on the screen */
                    XImage *image = XGetImage(dpy, root, 0, 0, width, height, AllPlanes, ZPixmap);
                    src_pm = XCreatePixmap(dpy, root, width, height, depth);
                    XPutImage(dpy, src_pm, gc, image, 0, 0, 0, 0, width, height);
                    XDestroyImage(image);
                }

                dst_pm = XCreatePixmap(dpy, root, width, height, depth);

                { /* tint the dst */
                    GC tintgc;
                    XGCValues tintval;

                    tintval.foreground = color.pixel;
                    tintgc = XCreateGC(dpy, dst_pm, GCForeground, &tintval);
                    XFillRectangle(dpy, dst_pm, tintgc, 0, 0, width, height);
                    XFreeGC(dpy, tintgc);
                }

                alock_shade_pixmap(xinfo, scr, src_pm, dst_pm, shade, 0, 0, 0, 0, width, height);
            }

            { /* create final window */
                XSetWindowAttributes xswa;

                xswa.override_redirect = True;
                xswa.colormap = xinfo->colormap[scr];
                xswa.background_pixmap = dst_pm;

                window[scr] = XCreateWindow(xinfo->display, xinfo->root[scr],
                        0, 0, width, height, 0,
                        CopyFromParent, InputOutput, CopyFromParent,
                        CWOverrideRedirect | CWColormap | CWBackPixmap,
                        &xswa);
                XFreePixmap(xinfo->display, src_pm);
                XFreePixmap(xinfo->display, dst_pm);
            }

            if (window[scr])
                xinfo->window[scr] = window[scr];
        }
    }

    goto return_success;

return_error:
    status = 0;
    free(window);
    window = NULL;

return_success:
    free(color_name);
    return status;
}

static int alock_bg_shade_deinit(struct aXInfo *xinfo) {

    if (!xinfo || !window)
        return 0;

    int scr;
    for (scr = 0; scr < xinfo->screens; scr++)
        XDestroyWindow(xinfo->display, window[scr]);
    free(window);

    return 1;
}


struct aBackground alock_bg_shade = {
    "shade",
    alock_bg_shade_init,
    alock_bg_shade_deinit,
};

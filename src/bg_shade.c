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
 * Used resources:
 *  ALock.Background.Shade.Color
 *  ALock.Background.Shade.Shade
 *
 */

#include <stdlib.h>
#include <string.h>
#include <X11/extensions/Xrender.h>

#include "alock.h"


static struct moduleData {
    struct aDisplayInfo *dinfo;
    Window *windows;
    char *colorname;
    unsigned int shade;
} data = { NULL, NULL, NULL, 80 };


static void module_loadargs(const char *args) {

    if (!args || strstr(args, "shade:") != args)
        return;

    char *arguments = strdup(&args[6]);
    char *arg;
    char *tmp;

    for (tmp = arguments; tmp; ) {
        arg = strsep(&tmp, ",");
        if (strstr(arg, "color=") == arg) {
            free(data.colorname);
            data.colorname = strdup(&arg[6]);
        }
        else if (strstr(arg, "shade=") == arg) {
            data.shade = strtol(&arg[6], NULL, 0);
            if (data.shade > 99)
                fprintf(stderr, "[shade]: shade not in range [0, 99]\n");
        }
    }

    free(arguments);
}

static void module_loadxrdb(XrmDatabase xrdb) {

    XrmValue value;
    char *type;

    if (XrmGetResource(xrdb, "alock.background.shade.color",
                "ALock.Background.Shade.Color", &type, &value))
        data.colorname = strdup(value.addr);

    if (XrmGetResource(xrdb, "alock.background.shade.shade",
                "ALock.Background.Shade.Shade", &type, &value))
        data.shade = strtol(value.addr, NULL, 0);

}

static int module_init(struct aDisplayInfo *dinfo) {

    if (!dinfo)
        return -1;

    Display *dpy = dinfo->display;

    if (!alock_check_xrender(dpy))
        return -1;

    data.dinfo = dinfo;
    data.windows = (Window *)malloc(sizeof(Window) * dinfo->screen_nb);

    {
        Pixmap src_pm = None;
        Pixmap dst_pm = None;
        XColor color;
        int scr;

        for (scr = 0; scr < dinfo->screen_nb; scr++) {

            Window root = dinfo->screens[scr].root;
            Colormap colormap = dinfo->screens[scr].colormap;
            int width = dinfo->screens[scr].width;
            int height = dinfo->screens[scr].height;

            alock_alloc_color(dpy, colormap, data.colorname, "black", &color);

            { /* xrender stuff */
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

                Visual *vis = DefaultVisual(dpy, scr);
                alock_shade_pixmap(dpy, vis, src_pm, dst_pm, data.shade, 0, 0, 0, 0, width, height);
            }

            { /* create final window */
                XSetWindowAttributes xswa;

                xswa.override_redirect = True;
                xswa.colormap = colormap;
                xswa.background_pixmap = dst_pm;

                data.windows[scr] = XCreateWindow(dpy, root,
                        0, 0, width, height, 0,
                        CopyFromParent, InputOutput, CopyFromParent,
                        CWOverrideRedirect | CWColormap | CWBackPixmap,
                        &xswa);
                XFreePixmap(dpy, src_pm);
                XFreePixmap(dpy, dst_pm);
            }

        }
    }

    return 0;
}

static void module_free() {

    if (data.windows) {
        int scr;
        for (scr = 0; scr < data.dinfo->screen_nb; scr++)
            XDestroyWindow(data.dinfo->display, data.windows[scr]);
        free(data.windows);
        data.windows = NULL;
    }

    free(data.colorname);
    data.colorname = NULL;

}

static Window module_getwindow(int screen) {
    if (!data.windows)
        return None;
    return data.windows[screen];
}


struct aModuleBackground alock_bg_shade = {
    { "shade",
        module_loadargs,
        module_loadxrdb,
        module_init,
        module_free,
    },
    module_getwindow,
};

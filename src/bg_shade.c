/*
 * alock - bg_shade.c
 * Copyright (c) 2005 - 2007 Mathias Gumz <akira at fluxbox dot org>
 *               2014 - 2016 Arkadiusz Bokowy
 *
 * This file is a part of an alock.
 *
 * This project is licensed under the terms of the MIT license.
 *
 * This background module provides:
 *  -bg shade:color=<color>,shade=<int>,blur=<int>,mono
 *
 * Used resources:
 *  ALock.Background.Shade.Color
 *  ALock.Background.Shade.Shade
 *  ALock.Background.Shade.Blur
 *  ALock.Background.Shade.Mono
 *
 */

#include "alock.h"

#include <stdlib.h>
#include <string.h>
#include <X11/extensions/Xrender.h>


static struct moduleData {
    Display *display;
    Window *windows;
    char *colorname;
    unsigned int shade;
    unsigned int blur;
    char monochrome;
} data = { NULL, NULL, NULL, 80, 0, 0 };


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
        }
        else if (strstr(arg, "blur=") == arg) {
            data.blur = strtol(&arg[5], NULL, 0);
        }
        else if (strcmp(arg, "mono") == 0) {
            data.monochrome = 1;
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

    if (XrmGetResource(xrdb, "alock.background.shade.blur",
                "ALock.Background.Shade.Blur", &type, &value))
        data.blur = strtol(value.addr, NULL, 0);

    if (XrmGetResource(xrdb, "alock.background.shade.mono",
                "ALock.Background.Shade.Mono", &type, &value))
        data.monochrome = strcmp(value.addr, "true") == 0;

}

static int module_init(Display *dpy) {

    if (!alock_check_xrender(dpy))
        return -1;

    /* show warning message when value is out of reasonable range */
    if (data.shade > 100)
        fprintf(stderr, "[shade]: shade not in range [0, 100]\n");
    if (data.blur > 100)
        fprintf(stderr, "[shade]: blur not in range [0, 100]\n");

    data.display = dpy;
    data.windows = (Window *)malloc(sizeof(Window) * ScreenCount(dpy));

    {
        Pixmap src_pm = None;
        Pixmap dst_pm = None;
        XColor color;
        int i;

        for (i = 0; i < ScreenCount(dpy); i++) {

            Screen *screen = ScreenOfDisplay(dpy, i);
            Window root = RootWindowOfScreen(screen);
            Colormap colormap = DefaultColormapOfScreen(screen);
            int width = WidthOfScreen(screen);
            int height = HeightOfScreen(screen);

            alock_alloc_color(dpy, colormap, data.colorname, "black", &color);

            { /* xrender stuff */

                int depth = DefaultDepthOfScreen(screen);
                GC gc = DefaultGCOfScreen(screen);

                { /* grab whats on the screen */
                    XImage *image = XGetImage(dpy, root, 0, 0, width, height, AllPlanes, ZPixmap);
                    if (data.monochrome) /* optional monochrome conversion */
                        alock_grayscale_image(image, 0, 0, width, height);
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

                Visual *vis = DefaultVisualOfScreen(screen);
                alock_shade_pixmap(dpy, vis, src_pm, dst_pm, data.shade, 0, 0, 0, 0, width, height);
                XCopyArea(dpy, dst_pm, src_pm, gc, 0, 0, width, height, 0, 0);
                alock_blur_pixmap(dpy, vis, src_pm, dst_pm, data.blur, 0, 0, 0, 0, width, height);
            }

            { /* create final window */
                XSetWindowAttributes xswa;

                xswa.override_redirect = True;
                xswa.colormap = colormap;
                xswa.background_pixmap = dst_pm;

                data.windows[i] = XCreateWindow(dpy, root,
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
        int i;
        for (i = 0; i < ScreenCount(data.display); i++)
            XDestroyWindow(data.display, data.windows[i]);
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

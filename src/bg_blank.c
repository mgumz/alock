/*
 * alock - bg_blank.c
 * Copyright (c) 2005 - 2007 Mathias Gumz <akira at fluxbox dot org>
 *               2014 Arkadiusz Bokowy
 *
 * This file is a part of an alock.
 *
 * This projected is licensed under the terms of the MIT license.
 *
 * This background module provides:
 *  -bg blank:color=<color>
 *
 * Used resources:
 *  ALock.Background.Blank.Color
 *
 */

#include "alock.h"

#include <stdlib.h>
#include <string.h>


static struct moduleData {
    struct aDisplayInfo *dinfo;
    Window *windows;
    char *colorname;
} data = { 0 };


static void module_loadargs(const char *args) {

    if (!args || strstr(args, "blank:") != args)
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
    }

    free(arguments);
}

static void module_loadxrdb(XrmDatabase xrdb) {

    XrmValue value;
    char *type;

    if (XrmGetResource(xrdb, "alock.background.blank.color",
                "ALock.Background.Blank.Color", &type, &value))
        data.colorname = strdup(value.addr);

}

static int module_init(struct aDisplayInfo *dinfo) {

    if (!dinfo)
        return -1;

    Display *dpy = dinfo->display;
    int scr;

    data.dinfo = dinfo;
    data.windows = (Window *)malloc(sizeof(Window) * dinfo->screen_nb);

    for (scr = 0; scr < dinfo->screen_nb; scr++) {

        Colormap colormap = dinfo->screens[scr].colormap;
        XSetWindowAttributes xswa;
        XColor color;

        alock_alloc_color(dpy, colormap, data.colorname, "black", &color);

        xswa.override_redirect = True;
        xswa.colormap = colormap;
        xswa.background_pixel = color.pixel;

        data.windows[scr] = XCreateWindow(dpy, dinfo->screens[scr].root,
                0, 0, dinfo->screens[scr].width, dinfo->screens[scr].height, 0,
                CopyFromParent, InputOutput, CopyFromParent,
                CWOverrideRedirect | CWColormap | CWBackPixel,
                &xswa);

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


struct aModuleBackground alock_bg_blank = {
    { "blank",
        module_loadargs,
        module_loadxrdb,
        module_init,
        module_free,
    },
    module_getwindow,
};

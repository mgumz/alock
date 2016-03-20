/*
 * alock - bg_none.c
 * Copyright (c) 2005 - 2007 Mathias Gumz <akira at fluxbox dot org>
 *               2014 - 2016 Arkadiusz Bokowy
 *
 * This file is a part of an alock.
 *
 * This projected is licensed under the terms of the MIT license.
 *
 * This background module provides:
 *  -bg none
 *
 */

#include "alock.h"

#include <stdlib.h>


static struct moduleData {
    Display *display;
    Window *windows;
} data = { 0 };


static int module_init(Display *dpy) {

    XSetWindowAttributes xswa = { .override_redirect = True };
    int i;

    data.display = dpy;
    data.windows = (Window *)malloc(sizeof(Window) * ScreenCount(dpy));

    for (i = 0; i < ScreenCount(dpy); i++)
        data.windows[i] = XCreateWindow(dpy, RootWindow(dpy, i),
                0, 0, 1, 1, 0, CopyFromParent, InputOutput, CopyFromParent,
                CWOverrideRedirect, &xswa);

    return 0;
}

static void module_free() {

    if (!data.windows)
        return;

    int i;

    for (i = 0; i < ScreenCount(data.display); i++)
        XDestroyWindow(data.display, data.windows[i]);

    free(data.windows);
    data.windows = NULL;
}

static Window module_getwindow(int screen) {
    if (!data.windows)
        return None;
    return data.windows[screen];
}


struct aModuleBackground alock_bg_none = {
    { "none",
        module_dummy_loadargs,
        module_dummy_loadxrdb,
        module_init,
        module_free,
    },
    module_getwindow,
};

/*
 * alock - bg_none.c
 * Copyright (c) 2005 - 2007 Mathias Gumz <akira at fluxbox dot org>
 *               2014 Arkadiusz Bokowy
 *
 * This file is a part of an alock.
 *
 * This projected is licensed under the terms of the MIT license.
 *
 * This background module provides:
 *  -bg none
 *
 */

#include <stdlib.h>

#include "alock.h"


static struct moduleData {
    struct aDisplayInfo *dinfo;
    Window *windows;
} data = { 0 };


static int module_init(struct aDisplayInfo *dinfo) {

    if (!dinfo)
        return -1;

    data.dinfo = dinfo;
    data.windows = (Window *)malloc(sizeof(Window) * dinfo->screen_nb);

    XSetWindowAttributes xswa;
    int scr;

    xswa.override_redirect = True;
    for (scr = 0; scr < dinfo->screen_nb; scr++)
        data.windows[scr] = XCreateWindow(dinfo->display, dinfo->screens[scr].root,
                0, 0, 1, 1, 0,
                CopyFromParent, InputOutput, CopyFromParent,
                CWOverrideRedirect, &xswa);

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

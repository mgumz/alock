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


static Window *window = NULL;


static int alock_bg_none_init(const char *args, struct aXInfo *xinfo) {

    if (!xinfo)
        return 0;

    XSetWindowAttributes xswa;
    int scr;

    window = (Window*)malloc(sizeof(Window) * xinfo->screens);

    xswa.override_redirect = True;

    for (scr = 0; scr < xinfo->screens; scr++) {
        window[scr] = XCreateWindow(xinfo->display, xinfo->root[scr],
                0, 0, 1, 1, 0,
                CopyFromParent, InputOutput, CopyFromParent,
                CWOverrideRedirect, &xswa);

        if (window[scr])
            xinfo->window[scr] = window[scr];
    }

    return 1;
}

static int alock_bg_none_deinit(struct aXInfo *xinfo) {

    if (!xinfo || !window)
        return 0;

    int scr;
    for (scr = 0; scr < xinfo->screens; scr++) {
        if (window[scr])
            XDestroyWindow(xinfo->display, window[scr]);
    }
    free(window);

    return 1;
}


struct aBackground alock_bg_none = {
    "none",
    alock_bg_none_init,
    alock_bg_none_deinit,
};

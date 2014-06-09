/*
 * alock - cursor_xcursor.c
 * Copyright (c) 2005 - 2007 Mathias Gumz <akira at fluxbox dot org>
 *               2014 Arkadiusz Bokowy
 *
 * This file is a part of an alock.
 *
 * This projected is licensed under the terms of the MIT license.
 *
 * This cursor module provides:
 *  -cursor xcursor:file=<file>
 *
 */

#include <stdlib.h>
#include <string.h>
#include <X11/Xcursor/Xcursor.h>

#include "alock.h"


static Cursor cursor = 0;


static int alock_cursor_xcursor_init(const char *args, struct aXInfo *xinfo) {

    if (!xinfo)
        return 0;

    char *file_name = NULL;
    int scr;

    if (args && strstr(args, "xcursor:") == args) {
        char *arguments = strdup(&args[8]);
        char *arg;
        char *tmp;
        for (tmp = arguments; tmp; ) {
            arg = strsep(&tmp, ",");
            if (strstr(arg, "file=") == arg) {
                free(file_name);
                file_name = strdup(&arg[5]);
            }
        }
        free(arguments);
    }

    if (file_name)
        cursor = XcursorFilenameLoadCursor(xinfo->display, file_name);

    if (cursor == 0) {
        fprintf(stderr, "[xcursor]: unable to load cursor file\n");
        return 0;
    }

    for (scr = 0; scr < xinfo->nr_screens; scr++)
        xinfo->cursor[scr] = cursor;

    free(file_name);
    return 1;
}

static int alock_cursor_xcursor_deinit(struct aXInfo *xinfo) {

    if (!xinfo || !cursor)
        return 0;

    XFreeCursor(xinfo->display, cursor);

    return 1;
}

struct aCursor alock_cursor_xcursor = {
    "xcursor",
    alock_cursor_xcursor_init,
    alock_cursor_xcursor_deinit,
};

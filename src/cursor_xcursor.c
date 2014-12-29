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


static struct moduleData {
    struct aDisplayInfo *dinfo;
    char *filename;
    Cursor cursor;
} data = { 0 };


static void module_loadargs(const char *args) {

    if (!args || strstr(args, "xcursor:") != args)
        return;

    char *arguments = strdup(&args[8]);
    char *arg;
    char *tmp;

    for (tmp = arguments; tmp; ) {
        arg = strsep(&tmp, ",");
        if (strstr(arg, "file=") == arg) {
            free(data.filename);
            data.filename = strdup(&arg[5]);
        }
    }

    free(arguments);
}


static int module_init(struct aDisplayInfo *dinfo) {

    if (!dinfo)
        return -1;

    data.dinfo = dinfo;

    if (data.filename)
        data.cursor = XcursorFilenameLoadCursor(dinfo->display, data.filename);

    if (data.cursor == 0) {
        fprintf(stderr, "[xcursor]: unable to load cursor file\n");
        return -1;
    }

    return 0;
}

static void module_free() {

    if (data.cursor)
        XFreeCursor(data.dinfo->display, data.cursor);

    free(data.filename);
    data.filename = NULL;
}

static Cursor module_getcursor(void) {
    return data.cursor;
}


struct aModuleCursor alock_cursor_xcursor = {
    { "xcursor",
        module_loadargs,
        module_dummy_loadxrdb,
        module_init,
        module_free,
    },
    module_getcursor,
};

/*
 * alock - cursor_blank.c
 * Copyright (c) 2016 Arkadiusz Bokowy
 *
 * This file is a part of an alock.
 *
 * This projected is licensed under the terms of the MIT license.
 *
 * This cursor module provides:
 *  -cursor blank
 *
 */

#include "alock.h"


static struct moduleData {
    struct aDisplayInfo *dinfo;
    Cursor cursor;
} data = { 0 };

static int module_init(struct aDisplayInfo *dinfo) {

    if (!dinfo)
        return -1;

    Display *dpy = dinfo->display;
    char no_data[8] = { 0 };
    XColor black;
    Pixmap blank;

    data.dinfo = dinfo;
    alock_alloc_color(dpy, dinfo->screens[0].colormap, NULL, "black", &black);

    blank = XCreateBitmapFromData(dpy, dinfo->screens[0].root, no_data, 8, 8);
    data.cursor = XCreatePixmapCursor(dpy, blank, blank, &black, &black, 0, 0);

    return 0;
}

static void module_free() {

    if (data.cursor)
        XFreeCursor(data.dinfo->display, data.cursor);

    /* Cursor was not visible, so it can be anywhere by now. Help to locate
     * it, by placing it in a center of the screen. */
    XWarpPointer(data.dinfo->display, None, data.dinfo->screens[0].root, 0, 0, 0, 0,
            data.dinfo->screens[0].width / 2, data.dinfo->screens[0].height / 2);

}

static Cursor module_getcursor(void) {
    return data.cursor;
}


struct aModuleCursor alock_cursor_blank = {
    { "blank",
        module_dummy_loadargs,
        module_dummy_loadxrdb,
        module_init,
        module_free,
    },
    module_getcursor,
};

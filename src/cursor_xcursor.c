/* ---------------------------------------------------------------- *\

  file    : cursor_xcursor.c
  author  : m. gumz <akira at fluxbox dot org>
  copyr   : copyright (c) 2005 - 2007 by m. gumz

  license : see LICENSE

  start   : Di 17 Mai 2005 12:14:36 CEST

\* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- *\

  about :

    provide -cursor xcursor:file

\* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- *\
  includes
\* ---------------------------------------------------------------- */

#include "alock.h"

#include <X11/Xcursor/Xcursor.h>
#include <string.h>

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */

static Cursor cursor = 0;

static int alock_cursor_xcursor_init(const char* args, struct aXInfo* xinfo) {

    if (!xinfo)
        return 0;

    if (!args || strlen(args) < 13) {
        printf("alock: error, missing arguments for [xcursor].\n");
        return 0;
    }

    if (strstr(args, "xcursor:") != args || strstr(&args[8], "file=") != &args[8]) {
        printf("alock: error, wrong arguments for [xcursor].\n");
        return 0;
    }

    if (!(cursor = XcursorFilenameLoadCursor(xinfo->display, &args[13]))) {
        printf("alock: error, couldnt load [%s]\n", &args[13]);
        return 0;
    }

    {
        int scr;
        for (scr = 0; scr < xinfo->nr_screens; scr++) {
            xinfo->cursor[scr] = cursor;
        }
    }

    return 1;
}

static int alock_cursor_xcursor_deinit(struct aXInfo* xinfo) {
    if (!xinfo || !cursor)
        return 0;

    XFreeCursor(xinfo->display, cursor);
    return 1;
}

struct aCursor alock_cursor_xcursor = {
    "xcursor",
    alock_cursor_xcursor_init,
    alock_cursor_xcursor_deinit
};


/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */


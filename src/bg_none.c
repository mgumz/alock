/* ---------------------------------------------------------------- *\

  file    : bg_none.c
  author  : m. gumz <akira at fluxbox dot org>
  copyr   : copyright (c) 2005 - 2007 by m. gumz

  license : see LICENSE

  start   : Sa 14 Mai 2005 14:27:48 CEST

\* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- *\

  about :

\* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- *\
  includes
\* ---------------------------------------------------------------- */

#include "alock.h"

#include <stdlib.h>
/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */

static Window* window = NULL;

static int alock_bg_none_init(const char* args, struct aXInfo* xinfo) {

    XSetWindowAttributes xswa;
    long xsmask = 0;

    if (!xinfo)
        return 0;

    window = (Window*)calloc(xinfo->nr_screens, sizeof(Window));

    xswa.override_redirect = True;
    xsmask |= CWOverrideRedirect;
    {
        int scr;
        for (scr = 0; scr < xinfo->nr_screens; scr++) {
            window[scr] = XCreateWindow(xinfo->display, xinfo->root[scr],
                                  0, 0, 1, 1,
                                  0, /* borderwidth */
                                  CopyFromParent, /* depth */
                                  InputOnly, /* class */
                                  CopyFromParent, /* visual */
                                  xsmask, &xswa);

            if (window[scr])
                xinfo->window[scr] = window[scr];
        }
    }

    return 1;
}

static int alock_bg_none_deinit(struct aXInfo* xinfo) {
    if (!xinfo || !window)
        return 0;
    {
        int scr;
        for (scr = 0; scr < xinfo->nr_screens; scr++) {
            XDestroyWindow(xinfo->display, window[scr]);
        }
    }
    return 1;
}

struct aBackground alock_bg_none = {
    "none",
    alock_bg_none_init,
    alock_bg_none_deinit
};

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */


/* ---------------------------------------------------------------- *\

  file    : bg_none.c
  author  : m. gumz <akira at fluxbox dot org>
  copyr   : copyright (c) 2005 by m. gumz

  license : see LICENSE
  
  start   : Sa 14 Mai 2005 14:27:48 CEST

  $Id$

\* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- *\

  about :

\* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- *\
  includes
\* ---------------------------------------------------------------- */
#include <X11/Xlib.h>
#include "alock.h"
/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */

static Window window = 0;

static int alock_bg_none_init(const char* args, struct aXInfo* xinfo) {

    XSetWindowAttributes xswa;
    long xsmask = 0;
    
    if (!xinfo)
        return 0;

    xswa.override_redirect = True;
    xsmask |= CWOverrideRedirect;

    window = XCreateWindow(xinfo->display, xinfo->root,
                          0, 0, 1, 1,
                          0, /* borderwidth */
                          CopyFromParent, /* depth */
                          InputOnly, /* class */
                          CopyFromParent, /* visual */
                          xsmask, &xswa);

    if (window)
        xinfo->window = window;
    
    return window;
}

static int alock_bg_none_deinit(struct aXInfo* xinfo) {
    if (!xinfo || !window)
        return 0;
    XDestroyWindow(xinfo->display, window);
    return 1;
}

struct aBackground alock_bg_none = {
    "none",
    alock_bg_none_init,
    alock_bg_none_deinit
};

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */


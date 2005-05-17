/* ---------------------------------------------------------------- *\

  file    : bg_blank.c
  author  : m. gumz <akira at fluxbox dot org>
  copyr   : copyright (c) 2005 by m. gumz

  license : see LICENSE
  
  start   : Di 17 Mai 2005 10:44:20 CEST

\* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- *\

  about :

    provides -bg blank:color

\* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- *\
  includes
\* ---------------------------------------------------------------- */
#include <X11/Xlib.h>
#include "alock.h"

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */

static Window window = 0;

static int alock_bg_blank_init(const char* args, struct aXInfo* xinfo) {

    XWindowAttributes xgwa;
    XSetWindowAttributes xswa;
    long xsmask = 0;
    
    if (!xinfo)
        return 0;

    /* TODO: parse args for color */
    
    XGetWindowAttributes(xinfo->display, xinfo->root, &xgwa);

    xswa.override_redirect = True;
    xswa.colormap = xinfo->colormap;
    xswa.background_pixel = BlackPixel(xinfo->display, 
                                       DefaultScreen(xinfo->display));
    
    xsmask |= CWOverrideRedirect;
    xsmask |= CWBackPixel;
    xsmask |= CWColormap;

    window = XCreateWindow(xinfo->display, xinfo->root,
                          0, 0, xgwa.width, xgwa.height,
                          0, /* borderwidth */
                          CopyFromParent, /* depth */
                          InputOutput, /* class */
                          CopyFromParent, /* visual */
                          xsmask, &xswa);

    if (window)
        xinfo->window = window;
    
    return window;
}

static int alock_bg_blank_deinit(struct aXInfo* xinfo) {
    if (!xinfo || !window)
        return 0;
    XDestroyWindow(xinfo->display, window);
    return 1;
}

struct aBackground alock_bg_blank = {
    "blank",
    alock_bg_blank_init,
    alock_bg_blank_deinit
};

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */


/* ---------------------------------------------------------------- *\

  file    : bg_blank.c
  author  : m. gumz <akira at fluxbox dot org>
  copyr   : copyright (c) 2005 by m. gumz

  license : see LICENSE
  
  start   : Di 17 Mai 2005 10:44:20 CEST

  $Id$

\* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- *\

  about :

    provides -bg blank:color

\* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- *\
  includes
\* ---------------------------------------------------------------- */
#include <X11/Xlib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "alock.h"

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */

static Window window = 0;
static XColor color;

static int alock_bg_blank_init(const char* args, struct aXInfo* xinfo) {

    XWindowAttributes xgwa;
    XSetWindowAttributes xswa;
    long xsmask = 0;
    XColor tmp_color;
    char* color_name = strdup("black");
    
    if (!xinfo || !args)
        return 0;

    if (strstr(args, "blank:") == args && strlen(&args[6]) > 0) {
        char* arguments = strdup(&args[6]);
        char* tmp;
        char* arg = NULL;
        for (tmp = arguments; tmp; ) {
            arg = strsep(&tmp, ",");
            if (arg) {
                if (strstr(arg, "color=") == arg && strlen(arg) > 6 && strlen(&arg[6])) {
                    free(color_name);
                    color_name = strdup(&arg[6]);
                }
            }
        }
        free(arguments);
    }
    
    alock_alloc_color(xinfo, color_name, "black", &color);
    free(color_name);
    
    XGetWindowAttributes(xinfo->display, xinfo->root, &xgwa);

    xswa.override_redirect = True;
    xswa.colormap = xinfo->colormap;
    xswa.background_pixel = color.pixel;

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


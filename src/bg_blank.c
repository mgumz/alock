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

static Window* window = NULL;
static XColor* color = NULL;

static int alock_bg_blank_init(const char* args, struct aXInfo* xinfo) {

    XWindowAttributes xgwa;
    XSetWindowAttributes xswa;
    long xsmask = 0;
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

    {
        color = (XColor*)calloc(xinfo->nr_screens, sizeof(XColor));
        window = (Window*)calloc(xinfo->nr_screens, sizeof(Window));
    }

    {
        int scr;
        for (scr = 0; scr < xinfo->nr_screens; scr++) {

            alock_alloc_color(xinfo, scr, color_name, "black", &color[scr]);

            XGetWindowAttributes(xinfo->display, xinfo->root[scr], &xgwa);

            xswa.override_redirect = True;
            xswa.colormap = xinfo->colormap[scr];
            xswa.background_pixel = color[scr].pixel;

            xsmask |= CWOverrideRedirect;
            xsmask |= CWBackPixel;
            xsmask |= CWColormap;

            window[scr] = XCreateWindow(xinfo->display, xinfo->root[scr],
                                  0, 0, xgwa.width, xgwa.height,
                                  0, /* borderwidth */
                                  CopyFromParent, /* depth */
                                  InputOutput, /* class */
                                  CopyFromParent, /* visual */
                                  xsmask, &xswa);


            if (window[scr])
                xinfo->window[scr] = window[scr];

        }
    }

    free(color_name); 

    return 1;
}

static int alock_bg_blank_deinit(struct aXInfo* xinfo) {
    if (!xinfo)
        return 0;

    {
        int scr;
        for (scr = 0; scr < xinfo->nr_screens; scr++) {
            if (window[scr])
                XDestroyWindow(xinfo->display, window[scr]);
        }
        free(window);
        free(color);
    }
    return 1;
}

struct aBackground alock_bg_blank = {
    "blank",
    alock_bg_blank_init,
    alock_bg_blank_deinit
};

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */


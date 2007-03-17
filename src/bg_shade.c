/* ---------------------------------------------------------------- *\

  file    : bg_shade.c
  author  : m. gumz <akira at fluxbox dot org>
  copyr   : copyright (c) 2005 - 2007 by m. gumz

  license : see LICENSE

  start   : Di 18 Mai 2005 10:44:20 CEST

\* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- *\

  about :

    provides -bg shade:color=<color>,shade=<int>

\* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- *\
  includes
\* ---------------------------------------------------------------- */

#include "alock.h"

#include <X11/extensions/Xrender.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */

static Window* window = NULL;
static XColor* color = NULL;

static int alock_bg_shade_init(const char* args, struct aXInfo* xinfo) {

    char* color_name = strdup("black");
    unsigned int shade = 80;

    Pixmap src_pm = None;
    Pixmap dst_pm = None;

    int width = 0;
    int height = 0;

    if (!xinfo || !args)
        return 0;

    if (strstr(args, "shade:") == args && strlen(&args[6]) > 0) {
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
                else if (strstr(arg, "shade=") == arg && strlen(arg) > 6 && strlen(&arg[6])) {
                    long int tmp_shade;
                    char* tmp_char;
                    tmp_shade = strtol(&arg[6], &tmp_char, 0);
                    if ((!tmp_shade || tmp_char != &arg[6]) && tmp_shade > 0 && tmp_shade < 100)
                        shade = tmp_shade;
                    else {
                        printf("alock: error, given value invalid or out of range for [shade].\n");
                        free(arguments);
                        free(color_name);
                        return 0;
                    }
                }
            }
        }
        free(arguments);
    }

    if (!alock_check_xrender(xinfo)) {
        free(color_name);
        return 0;
    }

    {
        window = (Window*)calloc(xinfo->nr_screens, sizeof(Window));
        color = (XColor*)calloc(xinfo->nr_screens, sizeof(XColor));
    }

    {
        int scr;
        for (scr = 0; scr < xinfo->nr_screens; scr++) {

            /* get a color from color_name */
            alock_alloc_color(xinfo, scr, color_name, "black", &color[scr]);

            { /* get dimension of the screen */
                XWindowAttributes xgwa;
                XGetWindowAttributes(xinfo->display, xinfo->root[scr], &xgwa);
                width = xgwa.width;
                height = xgwa.height;
            }
            { /* xrender stuff */
                Display* dpy = xinfo->display;
                Window root = xinfo->root[scr];
                int depth = DefaultDepth(dpy, scr);
                GC gc = DefaultGC(dpy, scr);

                { /* grab whats on the screen */
                    XImage* image = XGetImage(dpy, root, 0, 0, width, height, AllPlanes, ZPixmap);
                    src_pm = XCreatePixmap(dpy, root, width, height, depth);
                    XPutImage(dpy, src_pm, gc, image, 0, 0, 0, 0, width, height);
                    XDestroyImage(image);
                }

                dst_pm = XCreatePixmap(dpy, root, width, height, depth);

                { /* tint the dst*/
                    GC tintgc;
                    XGCValues tintval;

                    tintval.foreground = color[scr].pixel;
                    tintgc = XCreateGC(dpy, dst_pm, GCForeground, &tintval);
                    XFillRectangle(dpy, dst_pm, tintgc, 0, 0, width, height);
                    XFreeGC(dpy, tintgc);
                }

                alock_shade_pixmap(xinfo, scr, src_pm, dst_pm, shade, 0, 0, 0, 0, width, height);
            }

            { /* create final window */
                XSetWindowAttributes xswa;
                long xsmask = 0;

                xswa.override_redirect = True;
                xswa.colormap = xinfo->colormap[scr];
                xswa.background_pixmap = dst_pm;

                xsmask |= CWOverrideRedirect;
                xsmask |= CWBackPixmap;
                xsmask |= CWColormap;

                window[scr] = XCreateWindow(xinfo->display, xinfo->root[scr],
                                  0, 0, width, height,
                                  0, /* borderwidth */
                                  CopyFromParent, /* depth */
                                  InputOutput, /* class */
                                  CopyFromParent, /* visual */
                                  xsmask, &xswa);
                XFreePixmap(xinfo->display, src_pm);
                XFreePixmap(xinfo->display, dst_pm);
            }

            if (window[scr])
                xinfo->window[scr] = window[scr];
        }
        free(color_name);
    }

    return 1;
}

static int alock_bg_shade_deinit(struct aXInfo* xinfo) {
    if (!xinfo || !window)
        return 0;
    {
        int scr;
        for (scr = 0; scr < xinfo->nr_screens; scr++) {
            XDestroyWindow(xinfo->display, window[scr]);
        }
        free(window);
        free(color);
    }
    return 1;
}

struct aBackground alock_bg_shade = {
    "shade",
    alock_bg_shade_init,
    alock_bg_shade_deinit
};

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */


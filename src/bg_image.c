/* ---------------------------------------------------------------- *\

  file    : bg_image.c
  author  : m. gumz <akira at fluxbox dot org>
  copyr   : copyright (c) 2005 by m. gumz

  license : see LICENSE

  start   : Mi 18 Mai 2005 00:51:10 CEST

  $Id$

\* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- *\

  about :

    provide -bg image:filename via imlib2

\* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- *\
  includes
\* ---------------------------------------------------------------- */
#include <X11/Xlib.h>
#include <Imlib2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "alock.h"


/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */

enum {
    ALOCK_SCALE = 1,
    ALOCK_CENTER = 2,
    ALOCK_TILED = 4
};

static Window* window = NULL;
static Pixmap* pixmap = NULL;
static XColor* color = NULL;

static int alock_bg_image_init(const char* args, struct aXInfo* xinfo) {

    XWindowAttributes xgwa;
    XSetWindowAttributes xswa;
    long xsmask = 0;
    long options = ALOCK_SCALE;
    char* filename = NULL;
    char* color_name = strdup("black");
    unsigned int shade = 0;

    if (!xinfo || !args)
        return 0;

    if (strstr(args, "image:") == args && strlen(&args[6]) > 0) {
        char* arguments = strdup(&args[6]);
        char* tmp;
        char* arg = NULL;
        for (tmp = arguments; tmp; ) {
            arg = strsep(&tmp, ",");
            if (arg) {
                if (strstr(arg, "scale") == arg) {
                    options = ALOCK_SCALE;
                } else if (strstr(arg, "center")) {
                    options = ALOCK_CENTER;
                } else if (strstr(arg, "tile")) {
                    options = ALOCK_TILED;
                } else if (strstr(arg, "color=") == arg && strlen(arg) > 6 && strlen(&arg[6])) {
                    free(color_name);
                    color_name = strdup(&arg[6]);
                } else if (strstr(arg, "shade=") == arg && strlen(arg) > 6 && strlen(&arg[6])) {
                    unsigned int tmp_shade = atoi(&arg[6]);
                    if (tmp_shade > 0 && tmp_shade < 100) {
                        shade = tmp_shade;
                    } else {
                        printf("alock: error, shade not in range [1, 99] for [image].\n");
                        free(color_name);
                        if (filename)
                            free(filename);
                        free(arguments);
                        return 0;
                    }

                } else if (strstr(arg, "file=") == arg && strlen(arg) > 6) {
                    if (!filename)
                        filename = strdup(&arg[5]);
                }
            }
        }
        free(arguments);
    }

    if (!filename) {
        printf("alock: error, no filename specified for [image]\n");
        return 1;
    }

    if (!alock_check_xrender(xinfo)) {
        shade = 0;
    }

    {
        pixmap = (Pixmap*)calloc(xinfo->nr_screens, sizeof(Pixmap));
        window = (Window*)calloc(xinfo->nr_screens, sizeof(Window));
        color = (XColor*)calloc(xinfo->nr_screens, sizeof(XColor));
    }

    {
        int scr;
        for (scr = 0; xinfo->nr_screens; scr++) {

            alock_alloc_color(xinfo, scr, color_name, "black", &color[scr]);

            XGetWindowAttributes(xinfo->display, xinfo->root[scr], &xgwa);

            { /* get image and set it as the background pixmap for the window */
                Imlib_Context context = NULL;
                Imlib_Image image = NULL;

                context = imlib_context_new();
                imlib_context_push(context);
                imlib_context_set_display(xinfo->display);
                imlib_context_set_visual(DefaultVisual(xinfo->display,
                                         DefaultScreen(xinfo->display)));
                imlib_context_set_colormap(xinfo->colormap[scr]);

                image = imlib_load_image_without_cache(filename);
                if (image) {

                    char do_shade = shade > 0 && shade < 100;
                    int w;
                    int h;

                    pixmap[scr] = XCreatePixmap(xinfo->display, xinfo->root[scr],
                                       xgwa.width, xgwa.height,
                                       DefaultDepth(xinfo->display, scr));

                    imlib_context_set_drawable(pixmap[scr]);
                    imlib_context_set_image(image);

                    w = imlib_image_get_width();
                    h = imlib_image_get_height();

                    if (do_shade || options & ALOCK_CENTER) {

                        GC gc;
                        XGCValues gcval;

                        gcval.foreground = color[scr].pixel;
                        gc = XCreateGC(xinfo->display, xinfo->root[scr], GCForeground, &gcval);
                        XFillRectangle(xinfo->display, pixmap[scr], gc, 0, 0, xgwa.width, xgwa.height);
                        XFreeGC(xinfo->display, gc);
                    }

                    if (do_shade) {
                        GC gc;
                        XGCValues gcval;

                        Pixmap tmp_pixmap = XCreatePixmap(xinfo->display, xinfo->root[scr], w, h,
                                                          DefaultDepth(xinfo->display, scr));
                        Pixmap shaded_pixmap = XCreatePixmap(xinfo->display, xinfo->root[scr], w, h,
                                                          DefaultDepth(xinfo->display, scr));
                        gcval.foreground = color[scr].pixel;
                        gc = XCreateGC(xinfo->display, xinfo->root[scr], GCForeground, &gcval);
                        XFillRectangle(xinfo->display, shaded_pixmap, gc, 0, 0, w, h);

                        imlib_context_set_drawable(tmp_pixmap);
                        imlib_render_image_on_drawable(0, 0);

                        alock_shade_pixmap(xinfo, tmp_pixmap, shaded_pixmap, shade, 0, 0, 0, 0, w, h);

                        imlib_free_image_and_decache();
                        imlib_context_set_drawable(shaded_pixmap);

                        image = imlib_create_image_from_drawable(None, 0, 0, w, h, 0);

                        XFreePixmap(xinfo->display, shaded_pixmap);
                        XFreePixmap(xinfo->display, tmp_pixmap);

                        imlib_context_set_drawable(pixmap[scr]);
                        imlib_context_set_image(image);
                    }

                    if (options & ALOCK_CENTER) {
                        imlib_render_image_on_drawable((xgwa.width - w)/2, (xgwa.height - h)/2);
                    } else if (options & ALOCK_TILED) {
                        Pixmap tile;
                        GC gc;
                        XGCValues gcval;

                        tile = XCreatePixmap(xinfo->display, xinfo->root[scr],
                                             w, h, DefaultDepth(xinfo->display, scr));

                        imlib_render_image_on_drawable(0, 0);

                        gcval.fill_style = FillTiled;
                        gcval.tile = tile;
                        gc = XCreateGC(xinfo->display, tile, GCFillStyle|GCTile, &gcval);
                        XFillRectangle(xinfo->display, pixmap[scr], gc, 0, 0, xgwa.width, xgwa.height);

                        XFreeGC(xinfo->display, gc);
                        XFreePixmap(xinfo->display, tile);
                    } else {/* fallback is ALOCK_SCALE */
                        imlib_render_image_on_drawable_at_size(0, 0, xgwa.width, xgwa.height);
                    }
                    imlib_free_image_and_decache();

                } else {
                    printf("alock: error, couldnt load [%s].\n", filename);
                    if (filename)
                        free(filename);
                    XDestroyWindow(xinfo->display, window[scr]);
                    return 0;
                }

                imlib_context_pop();
                imlib_context_free(context);
            }

            xswa.override_redirect = True;
            xswa.colormap = xinfo->colormap[scr];
            xswa.background_pixmap = pixmap[scr];

            xsmask |= CWOverrideRedirect;
            xsmask |= CWColormap;
            xsmask |= CWBackPixmap;

            window[scr] = XCreateWindow(xinfo->display, xinfo->root[scr],
                                   0, 0, xgwa.width, xgwa.height,
                                   0, /* borderwidth */
                                   CopyFromParent, /* depth */
                                   InputOutput, /* class */
                                   CopyFromParent, /* visual */
                                   xsmask, &xswa);

            XMapWindow(xinfo->display, window[scr]);

            if (window[scr])
                xinfo->window[scr] = window[scr];

        }

        free(color_name);
    }

    if (filename)
        free(filename);

    return window;
}


static int alock_bg_image_deinit(struct aXInfo* xinfo) {
    if (!xinfo || !window)
        return 0;
    {
        int scr;
        for (scr = 0; scr < xinfo->nr_screens; scr++) {
            XDestroyWindow(xinfo->display, window[scr]);
            XFreePixmap(xinfo->display, pixmap[scr]);
        }
        free(color);
        free(pixmap);
        free(window);
    }
    return 1;
}

struct aBackground alock_bg_image = {
    "image",
    alock_bg_image_init,
    alock_bg_image_deinit
};

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */



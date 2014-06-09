/*
 * alock - bg_image.c
 * Copyright (c) 2005 - 2007 Mathias Gumz <akira at fluxbox dot org>
 *               2014 Arkadiusz Bokowy
 *
 * This file is a part of an alock.
 *
 * This projected is licensed under the terms of the MIT license.
 *
 * This background module provides:
 *  -bg image:file=<file>,shade=<int>,scale,center,tiled
 *
 */

#include <stdlib.h>
#include <string.h>
#include <Imlib2.h>

#include "alock.h"


enum {
    ALOCK_SCALE = 1,
    ALOCK_CENTER = 2,
    ALOCK_TILED = 4
};

static Window *window = NULL;
static Pixmap *pixmap = NULL;


static int alock_bg_image_init(const char *args, struct aXInfo *xinfo) {

    if (!xinfo)
        return 0;

    XSetWindowAttributes xswa;
    XColor color;
    long options = ALOCK_SCALE;
    char *file_name = NULL;
    char *color_name = NULL;
    unsigned int shade = 0;
    int status = 1;

    if (args && strstr(args, "image:") == args) {
        char *arguments = strdup(&args[6]);
        char *arg;
        char *tmp;
        for (tmp = arguments; tmp; ) {
            arg = strsep(&tmp, ",");
            if (strstr(arg, "file=") == arg) {
                free(file_name);
                file_name = strdup(&arg[5]);
            }
            else if (strcmp(arg, "scale") == 0) {
                options = ALOCK_SCALE;
            }
            else if (strcmp(arg, "center") == 0) {
                options = ALOCK_CENTER;
            }
            else if (strcmp(arg, "tiled") == 0) {
                options = ALOCK_TILED;
            }
            else if (strstr(arg, "color=") == arg) {
                free(color_name);
                color_name = strdup(&arg[6]);
            }
            else if (strstr(arg, "shade=") == arg) {
                shade = strtol(&arg[6], NULL, 0);
                if (shade > 99) {
                    fprintf(stderr, "[image]: shade not in range [0, 99]\n");
                    free(arguments);
                    goto return_error;
                }
            }
        }
        free(arguments);
    }

    if (!file_name) {
        fprintf(stderr, "[image]: file name not specified\n");
        goto return_error;
    }

    if (!alock_check_xrender(xinfo))
        shade = 0;

    window = (Window*)malloc(sizeof(Window) * xinfo->nr_screens);
    pixmap = (Pixmap*)malloc(sizeof(Pixmap) * xinfo->nr_screens);

    {
        int scr;
        for (scr = 0; scr < xinfo->nr_screens; scr++) {

            const int rwidth = xinfo->width_of_root[scr];
            const int rheight = xinfo->height_of_root[scr];
            alock_alloc_color(xinfo, scr, color_name, "black", &color);

            { /* get image and set it as the background pixmap for the window */
                Imlib_Context context = NULL;
                Imlib_Image image = NULL;

                context = imlib_context_new();
                imlib_context_push(context);
                imlib_context_set_display(xinfo->display);
                imlib_context_set_visual(DefaultVisual(xinfo->display,
                                         DefaultScreen(xinfo->display)));
                imlib_context_set_colormap(xinfo->colormap[scr]);

                image = imlib_load_image_without_cache(file_name);
                if (image) {

                    int w;
                    int h;

                    pixmap[scr] = XCreatePixmap(xinfo->display, xinfo->root[scr],
                                       rwidth, rheight,
                                       DefaultDepth(xinfo->display, scr));

                    imlib_context_set_drawable(pixmap[scr]);
                    imlib_context_set_image(image);

                    w = imlib_image_get_width();
                    h = imlib_image_get_height();

                    if (shade || options & ALOCK_CENTER) {

                        GC gc;
                        XGCValues gcval;

                        gcval.foreground = color.pixel;
                        gc = XCreateGC(xinfo->display, xinfo->root[scr], GCForeground, &gcval);
                        XFillRectangle(xinfo->display, pixmap[scr], gc, 0, 0, rwidth, rheight);
                        XFreeGC(xinfo->display, gc);
                    }

                    if (shade) {
                        GC gc;
                        XGCValues gcval;

                        Pixmap tmp_pixmap = XCreatePixmap(xinfo->display, xinfo->root[scr], w, h,
                                                          DefaultDepth(xinfo->display, scr));
                        Pixmap shaded_pixmap = XCreatePixmap(xinfo->display, xinfo->root[scr], w, h,
                                                          DefaultDepth(xinfo->display, scr));
                        gcval.foreground = color.pixel;
                        gc = XCreateGC(xinfo->display, xinfo->root[scr], GCForeground, &gcval);
                        XFillRectangle(xinfo->display, shaded_pixmap, gc, 0, 0, w, h);

                        imlib_context_set_drawable(tmp_pixmap);
                        imlib_render_image_on_drawable(0, 0);

                        alock_shade_pixmap(xinfo, scr, tmp_pixmap, shaded_pixmap, shade, 0, 0, 0, 0, w, h);

                        imlib_free_image_and_decache();
                        imlib_context_set_drawable(shaded_pixmap);

                        image = imlib_create_image_from_drawable(None, 0, 0, w, h, 0);

                        XFreePixmap(xinfo->display, shaded_pixmap);
                        XFreePixmap(xinfo->display, tmp_pixmap);

                        imlib_context_set_drawable(pixmap[scr]);
                        imlib_context_set_image(image);
                    }

                    if (options & ALOCK_CENTER) {
                        imlib_render_image_on_drawable((rwidth - w)/2, (rheight - h)/2);
                    }
                    else if (options & ALOCK_TILED) {
                        Pixmap tile;
                        GC gc;
                        XGCValues gcval;

                        tile = XCreatePixmap(xinfo->display, xinfo->root[scr],
                                             w, h, DefaultDepth(xinfo->display, scr));

                        imlib_render_image_on_drawable(0, 0);

                        gcval.fill_style = FillTiled;
                        gcval.tile = tile;
                        gc = XCreateGC(xinfo->display, tile, GCFillStyle|GCTile, &gcval);
                        XFillRectangle(xinfo->display, pixmap[scr], gc, 0, 0, rwidth, rheight);

                        XFreeGC(xinfo->display, gc);
                        XFreePixmap(xinfo->display, tile);
                    } else {/* fallback is ALOCK_SCALE */
                        imlib_render_image_on_drawable_at_size(0, 0, rwidth, rheight);
                    }
                    imlib_free_image_and_decache();

                }
                else {
                    fprintf(stderr, "[image]: unable to load image from file\n");
                    goto return_error;
                }

                imlib_context_pop();
                imlib_context_free(context);
            }

            xswa.override_redirect = True;
            xswa.colormap = xinfo->colormap[scr];
            xswa.background_pixmap = pixmap[scr];

            window[scr] = XCreateWindow(xinfo->display, xinfo->root[scr],
                    0, 0, rwidth, rheight, 0,
                    CopyFromParent, InputOutput, CopyFromParent,
                    CWOverrideRedirect | CWColormap | CWBackPixmap,
                    &xswa);

            XMapWindow(xinfo->display, window[scr]);

            if (window[scr])
                xinfo->window[scr] = window[scr];

        }
    }

    goto return_success;

return_error:
    status = 0;
    free(window);
    free(pixmap);
    window = NULL;
    pixmap = NULL;

return_success:
    free(file_name);
    free(color_name);
    return status;
}

static int alock_bg_image_deinit(struct aXInfo *xinfo) {

    if (!xinfo || !window)
        return 0;

    int scr;
    for (scr = 0; scr < xinfo->nr_screens; scr++) {
        XDestroyWindow(xinfo->display, window[scr]);
        XFreePixmap(xinfo->display, pixmap[scr]);
    }
    free(pixmap);
    free(window);

    return 1;
}


struct aBackground alock_bg_image = {
    "image",
    alock_bg_image_init,
    alock_bg_image_deinit,
};

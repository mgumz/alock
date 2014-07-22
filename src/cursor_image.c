/*
 * alock - cursor_image.c
 * Copyright (c) 2005 - 2007 Mathias Gumz <akira at fluxbox dot org>
 *               2014 Arkadiusz Bokowy
 *
 * This file is a part of an alock.
 *
 * This projected is licensed under the terms of the MIT license.
 *
 * This cursor module provides:
 *  -cursor image:file=<file>
 *
 * TODO: png-files can have transparency-colors, we want to ignore
 *       them.
 *
 */

#if HAVE_CONFIG_H
#include "../config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <X11/extensions/Xrender.h>
#if ENABLE_IMLIB2
#include <Imlib2.h>
#elif ENABLE_XPM
#include <X11/xpm.h>
#endif

#include "alock.h"


static Cursor cursor = 0;


static int alock_cursor_image_init(const char *args, struct aXInfo *xinfo) {

    if (!xinfo)
        return 0;

    char *file_name = NULL;
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
        }
        free(arguments);
    }

    if (!file_name) {
        fprintf(stderr, "[image]: file name not specified\n");
        goto return_error;
    }

    if (!alock_check_xrender(xinfo)) {
        fprintf(stderr, "[image]: no running xrender extension found\n");
        goto return_error;
    }

    {
        unsigned int w = 0;
        unsigned int h = 0;
        Pixmap cursor_pm = None;

#if ENABLE_IMLIB2
        {
            Imlib_Image img;
            Imlib_Context ctx = imlib_context_new();

            imlib_context_push(ctx);
            imlib_context_set_display(xinfo->display);
            imlib_context_set_visual(DefaultVisual(xinfo->display, DefaultScreen(xinfo->display)));
            imlib_context_set_colormap(xinfo->colormap[0]);

            img = imlib_load_image_without_cache(file_name);
            if (img) {
                imlib_context_set_image(img);
                w = imlib_image_get_width();
                h = imlib_image_get_height();

                { /* taken from cursor.c of libXcursor */
                    GC gc = None;
                    XImage ximage;
                    ximage.width = w;
                    ximage.height = h;
                    ximage.xoffset = 0;
                    ximage.format = ZPixmap;
                    ximage.data = (char *) imlib_image_get_data_for_reading_only();
                    ximage.byte_order = alock_native_byte_order();
                    ximage.bitmap_unit = 32;
                    ximage.bitmap_bit_order = ximage.byte_order;
                    ximage.bitmap_pad = 32;
                    ximage.depth = 32;
                    ximage.bits_per_pixel = 32;
                    ximage.bytes_per_line = w * 4;
                    ximage.red_mask = 0xff0000;
                    ximage.green_mask = 0x00ff00;
                    ximage.blue_mask = 0x0000ff;
                    ximage.obdata = 0;

                    XInitImage(&ximage);

                    cursor_pm = XCreatePixmap(xinfo->display, xinfo->root[0], w, h, 32);
                    gc = XCreateGC(xinfo->display, cursor_pm, 0, 0);
                    XPutImage(xinfo->display, cursor_pm, gc, &ximage, 0, 0, 0, 0, w, h);
                    XFreeGC(xinfo->display, gc);
                }
                imlib_free_image_and_decache();
            }
            imlib_context_pop();
            imlib_context_free(ctx);
        }
#elif ENABLE_XPM
        {
            XImage *img = NULL;
            XpmReadFileToImage(xinfo->display, file_name, &img, NULL, NULL);
            if (img) {
                GC gc = None;
                w = img->width;
                h = img->height;

                cursor_pm = XCreatePixmap(xinfo->display,
                                          xinfo->root[0],
                                          w, h,
                                          img->depth);
                gc = XCreateGC(xinfo->display, cursor_pm, 0, NULL);
                XPutImage(xinfo->display, cursor_pm, gc, img, 0, 0, 0, 0, w, h);
                XFreeGC(xinfo->display, gc);
                XDestroyImage(img);
            }
        }
#else
#warning compiling this file without having either imlib2 or xpm is pretty useless since no image can be loaded.
#endif

        if (!cursor_pm) {
            fprintf(stderr, "[image]: unable to load cursor from file\n");
            goto return_error;
        }

        {
            /* TODO: maybe replace this by an older xrenderapi-call, since
             * XRenderFindStandardFormat was introduced in xyz, dunno atm */
            XRenderPictFormat *format = XRenderFindStandardFormat(xinfo->display, PictStandardARGB32);
            if (format) {
                Picture cursor_pic = XRenderCreatePicture(xinfo->display, cursor_pm, format, 0, 0);
                cursor = XRenderCreateCursor(xinfo->display, cursor_pic, w / 2, h / 2);
                XRenderFreePicture(xinfo->display, cursor_pic);
            }
            else {
                fprintf(stderr, "[image]: error while finding a valid XRenderPictFormat\n");
                goto return_error;
            }

            XFreePixmap(xinfo->display, cursor_pm);
        }
    }

    {
        int scr;
        for (scr = 0; scr < xinfo->screens; scr++) {
            xinfo->cursor[scr] = cursor;
        }
    }

    goto return_success;

return_error:
    status = 0;
    if (cursor)
        XFreeCursor(xinfo->display, cursor);
    cursor = 0;

return_success:
    free(file_name);
    return status;
}

static int alock_cursor_image_deinit(struct aXInfo *xinfo) {

    if (!xinfo || !cursor)
        return 0;

    XFreeCursor(xinfo->display, cursor);

    return 1;
}


struct aCursor alock_cursor_image = {
    "image",
    alock_cursor_image_init,
    alock_cursor_image_deinit,
};

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

#include "alock.h"

#include <stdlib.h>
#include <string.h>
#include <X11/extensions/Xrender.h>
#if ENABLE_IMLIB2
#include <Imlib2.h>
#elif ENABLE_XPM
#include <X11/xpm.h>
#endif


static struct moduleData {
    struct aDisplayInfo *dinfo;
    char *filename;
    Cursor cursor;
} data = { 0 };


static void module_loadargs(const char *args) {

    if (!args || strstr(args, "image:") != args)
        return;

    char *arguments = strdup(&args[6]);
    char *arg;
    char *tmp;

    for (tmp = arguments; tmp; ) {
        arg = strsep(&tmp, ",");
        if (strstr(arg, "file=") == arg) {
            free(data.filename);
            data.filename = strdup(&arg[5]);
        }
    }

    free(arguments);
}


static int module_init(struct aDisplayInfo *dinfo) {

    if (!dinfo)
        return -1;

    if (!data.filename) {
        fprintf(stderr, "[image]: file name not specified\n");
        return -1;
    }

    Display *dpy = dinfo->display;

    if (!alock_check_xrender(dpy)) {
        fprintf(stderr, "[image]: no running xrender extension found\n");
        return -1;
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
            imlib_context_set_display(dpy);
            imlib_context_set_visual(DefaultVisual(dpy, DefaultScreen(dpy)));
            imlib_context_set_colormap(dinfo->screens[0].colormap);

            img = imlib_load_image_without_cache(data.filename);
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

                    cursor_pm = XCreatePixmap(dpy, dinfo->screens[0].root, w, h, 32);
                    gc = XCreateGC(dpy, cursor_pm, 0, 0);
                    XPutImage(dpy, cursor_pm, gc, &ximage, 0, 0, 0, 0, w, h);
                    XFreeGC(dpy, gc);
                }
                imlib_free_image_and_decache();
            }
            imlib_context_pop();
            imlib_context_free(ctx);
        }
#elif ENABLE_XPM
        {
            XImage *img = NULL;
            XpmReadFileToImage(dpy, data.filename, &img, NULL, NULL);
            if (img) {
                GC gc = None;
                w = img->width;
                h = img->height;

                cursor_pm = XCreatePixmap(dpy, dinfo->screens[0].root, w, h, img->depth);
                gc = XCreateGC(dpy, cursor_pm, 0, NULL);
                XPutImage(dpy, cursor_pm, gc, img, 0, 0, 0, 0, w, h);
                XFreeGC(dpy, gc);
                XDestroyImage(img);
            }
        }
#else
#warning compiling this file without having either imlib2 or xpm is pretty useless since no image can be loaded.
#endif

        if (!cursor_pm) {
            fprintf(stderr, "[image]: unable to load cursor from file\n");
            return -1;
        }

        {
            /* TODO: maybe replace this by an older xrenderapi-call, since
             * XRenderFindStandardFormat was introduced in xyz, dunno atm */
            XRenderPictFormat *format = XRenderFindStandardFormat(dpy, PictStandardARGB32);
            if (!format) {
                fprintf(stderr, "[image]: error while finding a valid XRenderPictFormat\n");
                XFreePixmap(dpy, cursor_pm);
                return -1;
            }

            Picture cursor_pic = XRenderCreatePicture(dpy, cursor_pm, format, 0, 0);
            data.cursor = XRenderCreateCursor(dpy, cursor_pic, w / 2, h / 2);
            XRenderFreePicture(dpy, cursor_pic);
        }

        XFreePixmap(dpy, cursor_pm);
    }

    return 0;
}

static void module_free() {

    if (data.cursor)
        XFreeCursor(data.dinfo->display, data.cursor);

    free(data.filename);
    data.filename = NULL;
}

static Cursor module_getcursor(void) {
    return data.cursor;
}


struct aModuleCursor alock_cursor_image = {
    { "image",
        module_loadargs,
        module_dummy_loadxrdb,
        module_init,
        module_free,
    },
    module_getcursor,
};

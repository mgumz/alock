/* ---------------------------------------------------------------- *\

  file    : cursor_image.c
  author  : m. gumz <akira at fluxbox dot org>
  copyr   : copyright (c) 2005 by m. gumz

  license : see LICENSE

  start   : Mi 01 June 2005 10:48:21 CEST

  $Id$

\* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- *\

  about :

    provide -cursor image:file=<file>

    TODO: png-files can have transparency-colors, we want to ignore
          them.

\* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- *\
  includes
\* ---------------------------------------------------------------- */
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#ifdef HAVE_IMLIB2
#    include <Imlib2.h>
#elif HAVE_XPM
#    include <X11/xpm.h>
#endif /* HAVE_IMLIB2 | HAVE_XPM */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "alock.h"

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */

static Cursor cursor = 0;

static int alock_cursor_image_init(const char* args, struct aXInfo* xinfo) {

    char* filename = NULL;

    if (!xinfo || !args)
        return 0;

    if (strstr(args, "image:") == args && strlen(&args[6]) > 0) {
        char* arguments = strdup(&args[6]);
        char* tmp;
        char* arg = NULL;
        for (tmp = arguments; tmp; ) {
            arg = strsep(&tmp, ",");
            if (arg) {
                if (strstr(arg, "file=") == arg && strlen(arg) > 6) {
                    if (!filename)
                        filename = strdup(&arg[5]);
                }
            }
        }
        free(arguments);
    }


    if (!filename) {
        printf("alock: error, missing argument for [image].\n");
        return 0;
    }

    if (!alock_check_xrender(xinfo)) {
        printf("alock: error, no running xrender extension found [image].\n");
        free(filename);
        return 0;
    }

    {
        unsigned int w = 0;
        unsigned int h = 0;
        Pixmap cursor_pm = None;

#ifdef HAVE_IMLIB2
        {
            Imlib_Image img;
            Imlib_Context ctx = imlib_context_new();

            imlib_context_push(ctx);
            imlib_context_set_display(xinfo->display);
            imlib_context_set_visual(DefaultVisual(xinfo->display, DefaultScreen(xinfo->display)));
            imlib_context_set_colormap(xinfo->colormap[0]);

            img = imlib_load_image_without_cache(filename);
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
#elif HAVE_XPM
        if (!cursor_pm) {
            XImage* img = NULL;
            XpmReadFileToImage(xinfo->display, filename, &img, NULL, NULL);
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
#endif /* HAVE_XPM|HAVE_IMLIB2 */

        if (!cursor_pm) {
            printf("alock: error while loading [%s] in [image].\n", filename);
            free(filename);
            return 0;
        }

        {
            /* TODO: maybe replace this by an older xrenderapi-call, since
             * XRenderFindStandardFormat was introduced in xyz, dunno atm */
            XRenderPictFormat* format = XRenderFindStandardFormat (xinfo->display, PictStandardARGB32);
            if (format) {
                Picture cursor_pic = XRenderCreatePicture(xinfo->display, cursor_pm, format, 0, 0);
                cursor = XRenderCreateCursor(xinfo->display, cursor_pic, w / 2, h / 2);
                XRenderFreePicture(xinfo->display, cursor_pic);
            } else {
                printf("alock: error while finding a valid XRenderPictFormat in [image].\n");
                free(filename);
                return 0;
            }

            XFreePixmap(xinfo->display, cursor_pm);
        }
    }

    free(filename);

    {
        int scr;
        for (scr = 0; scr < xinfo->nr_screens; scr++) {
            xinfo->cursor[scr] = cursor;
        }
    }

    return cursor;
}

static int alock_cursor_image_deinit(struct aXInfo* xinfo) {
    if (!xinfo || !cursor)
        return 0;

    XFreeCursor(xinfo->display, cursor);
    return 1;
}

struct aCursor alock_cursor_image = {
    "image",
    alock_cursor_image_init,
    alock_cursor_image_deinit
};


/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */


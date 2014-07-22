/*
 * alock - utils.c
 * Copyright (c) 2005 - 2007 Mathias Gumz <akira at fluxbox dot org>
 *               2014 Arkadiusz Bokowy
 *
 * This file is a part of an alock.
 *
 * This projected is licensed under the terms of the MIT license.
 *
 */

#if HAVE_CONFIG_H
#include "../config.h"
#endif

#include <string.h>
#include <ctype.h>
#include <time.h>
#if ENABLE_XRENDER
#include <X11/extensions/Xrender.h>
#endif

#include "alock.h"


/* Get system time-stamp in milliseconds without discontinuities. */
unsigned long alock_mtime() {
    struct timespec t;
    clock_gettime(CLOCK_BOOTTIME, &t);
    return t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

/* Allocate colormap entry by the given color name. When the color_name
 * parameter is NULL, then fallback value is used right away. */
int alock_alloc_color(const struct aXInfo *xinfo,
        int scr,
        const char *color_name,
        const char *fallback_name,
        XColor *result) {

    static XColor tmp;

    if (!xinfo ||
        !xinfo->colormap || xinfo->screens < scr || scr < 0 ||
        !fallback_name || !result)
        return 0;

    if (!color_name || XAllocNamedColor(xinfo->display, xinfo->colormap[scr], color_name, &tmp, result) == 0)
        if (XAllocNamedColor(xinfo->display, xinfo->colormap[scr], fallback_name, &tmp, result) == 0)
            return 0;
    return 1;
}

int alock_native_byte_order() {
    int x = 1;
    return (*((char *) &x) == 1) ? LSBFirst : MSBFirst;
}

/*------------------------------------------------------------------*\
 * taken from cursor.c of libXcursor
\*------------------------------------------------------------------*/
int alock_check_xrender(const struct aXInfo* xinfo) {
#if ENABLE_XRENDER
    static int have_xrender = 0;
    static int checked_already = 0;

    if (!checked_already) {
        int major_opcode, first_event, first_error;
        if (XQueryExtension(xinfo->display, "RENDER",
                            &major_opcode,
                            &first_event, &first_error) == False) {
            fprintf(stderr, "alock: no xrender-support found\n");
            have_xrender = 0;
        }
        else
            have_xrender = 1;

        checked_already = 1;
    }
    return have_xrender;
#else
    fprintf(stderr, "alock: i wasnt compiled to support xrender\n");
    return 0;
#endif /* ENABLE_XRENDER */
}

int alock_shade_pixmap(const struct aXInfo* xinfo,
        int scr,
        const Pixmap src_pm,
        Pixmap dst_pm,
        unsigned char shade,
        int src_x, int src_y,
        int dst_x, int dst_y,
        unsigned int width,
        unsigned int height) {
#if ENABLE_XRENDER
    Display* dpy = xinfo->display;
    Visual* vis = DefaultVisual(dpy, scr);

    Picture alpha_pic = None;
    XRenderPictFormat* format = None;

    {
        XRenderPictFormat alpha_format;
        unsigned long mask = PictFormatType|PictFormatDepth|PictFormatAlpha|PictFormatAlphaMask;
        alpha_format.type = PictTypeDirect;
        alpha_format.depth = 8;
        alpha_format.direct.alpha = 0;
        alpha_format.direct.alphaMask = 0xff;

        format = XRenderFindFormat(dpy, mask, &alpha_format, 0);
    }

    if (!format) {
        fprintf(stderr, "alock: couldnt find valid format for alpha\n");
        XFreePixmap(dpy, dst_pm);
        XFreePixmap(dpy, src_pm);
        return 0;
    }

    { /* fill the alpha-picture */
        Pixmap alpha_pm = None;

        XRenderColor alpha_color;
        XRenderPictureAttributes alpha_attr;

        alpha_color.alpha = 0xffff * (shade)/100;

        alpha_attr.repeat = True;

        alpha_pm = XCreatePixmap(dpy, src_pm, 1, 1, 8);
        alpha_pic = XRenderCreatePicture(dpy, alpha_pm, format, CPRepeat, &alpha_attr);
        XRenderFillRectangle(dpy, PictOpSrc, alpha_pic, &alpha_color, 0, 0, 1, 1);
        XFreePixmap(dpy, alpha_pm);
    }

    { /* blend all together */
        Picture src_pic;
        Picture dst_pic;

        format = XRenderFindVisualFormat(dpy, vis);

        src_pic = XRenderCreatePicture(dpy, src_pm, format, 0, 0);
        dst_pic = XRenderCreatePicture(dpy, dst_pm, format, 0, 0);

        XRenderComposite(dpy, PictOpOver,
                         src_pic, alpha_pic, dst_pic,
                         src_x, src_y, 0, 0, dst_x, dst_y, width, height);
        XRenderFreePicture(dpy, src_pic);
        XRenderFreePicture(dpy, dst_pic);
    }
    return 1;
#else
    return 0;
#endif /* ENABLE_XRENDER */
}

/* ---------------------------------------------------------------- *\

  file    : alock_utils.c
  author  : m. gumz <akira at fluxbox dot org>
  copyr   : copyright (c) 2005 by m. gumz

  license : see LICENSE
  
  start   : Mo 23 Mai 2005 13:55:24 CEST

\* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- *\

  about : collection of shared code among the modules

\* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- *\
  includes
\* ---------------------------------------------------------------- */
#include <X11/Xlib.h>
#ifdef HAVE_XRENDER
#    include <X11/extensions/Xrender.h>
#endif /* HAVE_XRENDER */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "alock.h"

/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/
void alock_string2lower(char* string) {
    static unsigned int i;
    for(i = strlen(string) - 1; i; --i)
        tolower(string[i]);
}

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */
int alock_alloc_color(const struct aXInfo* xinfo, const char* color_name,
        const char* fallback_name, XColor* result) {

    static XColor tmp;

    if (!xinfo || !color_name || !fallback_name || !result)
        return 0;

    if((XAllocNamedColor(xinfo->display, xinfo->colormap, color_name, &tmp, result)) == 0)
        if ((XAllocNamedColor(xinfo->display, xinfo->colormap, fallback_name, &tmp, result)) == 0)
            return 0;
    return 1;
}

/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/
int alock_check_xrender(const struct aXInfo* xinfo) {
#ifdef HAVE_XRENDER
    int major_opcode, first_event, first_error;
    if (XQueryExtension(xinfo->display, "RENDER",
                        &major_opcode,
                        &first_event, &first_error) == False) {
        printf("alock: error, no xrender-support found\n");
        return 0;
    }
    return 1;
#else
    printf("alock: error, i wasnt compiled to support xrender.\n");
    return 0;
#endif /* HAVE_XRENDER */
}

int alock_shade_pixmap(const struct aXInfo* xinfo,
        const Pixmap src_pm,
        Pixmap dst_pm, 
        unsigned char shade, 
        int src_x, int src_y,
        int dst_x, int dst_y,
        unsigned int width,
        unsigned int height) {
#ifdef HAVE_XRENDER
    Display* dpy = xinfo->display;
    int scrnr = DefaultScreen(dpy);
    Visual* vis = DefaultVisual(dpy, scrnr);

    Picture alpha_pic = None;
    XRenderPictFormat* format = None;
    XRenderPictFormat alpha_format;

    alpha_format.type = PictTypeDirect;
    alpha_format.depth = 8;
    alpha_format.direct.alpha = 0;
    alpha_format.direct.alphaMask = 0xff;

    format = XRenderFindStandardFormat(dpy, PictStandardA8);
    if (!format) {
        printf("error, couldnt find valid format for alpha.\n");
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
#endif /* HAVE_XRENDER */
}




/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */



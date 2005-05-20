/* ---------------------------------------------------------------- *\

  file    : bg_shade.c
  author  : m. gumz <akira at fluxbox dot org>
  copyr   : copyright (c) 2005 by m. gumz

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
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "alock.h"

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */

static Window window = 0;
static XColor color;

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
    
    { /* check for RENDER-extension */
        int major_opcode, first_event, first_error;
        if (XQueryExtension(xinfo->display, "RENDER",
                            &major_opcode,
                            &first_event, &first_error) == False) {
            printf("alock: error, no xrender-support found\n");
            free(color_name);
            return 0;
        }
    }

    { /* get a color from color_name */
        XColor tmp_color;
        if((XAllocNamedColor(xinfo->display, xinfo->colormap, color_name, &tmp_color, &color)) == 0)
            XAllocNamedColor(xinfo->display, xinfo->colormap, "black", &tmp_color, &color);
 
        free(color_name);
    }
    
    { /* get dimension of the screen */
        XWindowAttributes xgwa;
        XGetWindowAttributes(xinfo->display, xinfo->root, &xgwa);
        width = xgwa.width;
        height = xgwa.height;
    }

    { /* xrender stuff */
        Display* dpy = xinfo->display;
        Window root = xinfo->root;
        int scrnr = DefaultScreen(dpy);
        int depth = DefaultDepth(dpy, scrnr);
        Visual* vis = DefaultVisual(dpy, scrnr);
        GC gc = DefaultGC(dpy, scrnr);

        { /* grab whats on the screen */
            XImage* image = XGetImage(dpy, root, 0, 0, width, height, AllPlanes, ZPixmap);
            src_pm = XCreatePixmap(dpy, root, width, height, depth);
            XPutImage(dpy, src_pm, gc, image, 0, 0, 0, 0, width, height);
            XDestroyImage(image);
        }

        dst_pm = XCreatePixmap(dpy, root, width, height, depth);

        { /* tint the src */
            GC tintgc;
            XGCValues tintval;

            tintval.foreground = color.pixel;
            tintgc = XCreateGC(dpy, dst_pm, GCForeground, &tintval);
            XFillRectangle(dpy, dst_pm, tintgc, 0, 0, width, height);
            XFreeGC(dpy, tintgc);
        }

        { /* now do the "hot" stuff */
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
                //alpha_attr.component_alpha = True;

                alpha_pm = XCreatePixmap(dpy, src_pm, 1, 1, 8);
                alpha_pic = XRenderCreatePicture(dpy, alpha_pm, format, CPRepeat/*|CPComponentAlpha*/, &alpha_attr);
                XRenderFillRectangle(dpy, PictOpSrc, alpha_pic, &alpha_color, 0, 0, 1, 1);
                XFreePixmap(dpy, alpha_pm);
            }
            
            { /* blend all together */
                Picture src_pic;
                Picture dst_pic;
                
                format = XRenderFindVisualFormat(dpy, vis);

                src_pic = XRenderCreatePicture(dpy, src_pm, format, 0, 0);
                dst_pic = XRenderCreatePicture(dpy, dst_pm, format, 0, 0);

                XRenderComposite(dpy, PictOpOver, src_pic, alpha_pic, dst_pic, 0, 0, 0, 0, 0, 0, width, height);
                XRenderFreePicture(dpy, src_pic);
                XRenderFreePicture(dpy, dst_pic);
            }
        }

    }

    { /* create final window */
        XSetWindowAttributes xswa;
        long xsmask = 0;

        xswa.override_redirect = True;
        xswa.colormap = xinfo->colormap;
        xswa.background_pixmap = dst_pm;

        xsmask |= CWOverrideRedirect;
        xsmask |= CWBackPixmap;
        xsmask |= CWColormap;
    
        window = XCreateWindow(xinfo->display, xinfo->root,
                          0, 0, width, height,
                          0, /* borderwidth */
                          CopyFromParent, /* depth */
                          InputOutput, /* class */
                          CopyFromParent, /* visual */
                          xsmask, &xswa);
        XFreePixmap(xinfo->display, src_pm);
        XFreePixmap(xinfo->display, dst_pm);
    }
    
    if (window)
        xinfo->window = window;
    
    return window;
}

static int alock_bg_shade_deinit(struct aXInfo* xinfo) {
    if (!xinfo || !window)
        return 0;
    XDestroyWindow(xinfo->display, window);
    return 1;
}

struct aBackground alock_bg_shade = {
    "shade",
    alock_bg_shade_init,
    alock_bg_shade_deinit
};

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */


/* ---------------------------------------------------------------- *\

  file    : bg_imlib2.c
  author  : m. gumz <akira at fluxbox dot org>
  copyr   : copyright (c) 2005 by m. gumz

  license : see LICENSE
  
  start   : Mi 18 Mai 2005 00:51:10 CEST

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

static Window window = 0;
static Pixmap pixmap = 0;
static XColor color;

static int alock_bg_imlib2_init(const char* args, struct aXInfo* xinfo) {
        
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
                        free(arguments);
                        return 0;
                    }
                    
                } else {
                    if (!filename)
                        filename = strdup(arg);
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
    
    alock_alloc_color(xinfo, color_name, "black", &color);
    free(color_name);

    XGetWindowAttributes(xinfo->display, xinfo->root, &xgwa);
    { /* get image and set it as the background pixmap for the window */
        Imlib_Context context = NULL;
        Imlib_Image image = NULL;

        context = imlib_context_new();
        imlib_context_push(context);
        imlib_context_set_display(xinfo->display);
        imlib_context_set_visual(DefaultVisual(xinfo->display, 
                                 DefaultScreen(xinfo->display)));
        imlib_context_set_colormap(xinfo->colormap);

        image = imlib_load_image_without_cache(filename);
        if (image) {

            char do_shade = shade > 0 && shade < 100;
            int w;
            int h;

            pixmap = XCreatePixmap(xinfo->display, xinfo->root,
                               xgwa.width, xgwa.height, 
                               DefaultDepth(xinfo->display, DefaultScreen(xinfo->display)));

            imlib_context_set_drawable(pixmap);
            imlib_context_set_image(image);
             
            w = imlib_image_get_width();
            h = imlib_image_get_height();

            if (do_shade || options & ALOCK_CENTER) {
                
                GC gc;
                XGCValues gcval;

                gcval.foreground = color.pixel;
                gc = XCreateGC(xinfo->display, xinfo->root, GCForeground, &gcval);
                XFillRectangle(xinfo->display, pixmap, gc, 0, 0, xgwa.width, xgwa.height);
                XFreeGC(xinfo->display, gc);
            }

            if (do_shade) {
                GC gc;
                XGCValues gcval;

                Pixmap tmp_pixmap = XCreatePixmap(xinfo->display, xinfo->root, w, h, 
                                                  DefaultDepth(xinfo->display, DefaultScreen(xinfo->display)));
                Pixmap shaded_pixmap = XCreatePixmap(xinfo->display, xinfo->root, w, h, 
                                                  DefaultDepth(xinfo->display, DefaultScreen(xinfo->display)));
                gcval.foreground = color.pixel;
                gc = XCreateGC(xinfo->display, xinfo->root, GCForeground, &gcval);
                XFillRectangle(xinfo->display, shaded_pixmap, gc, 0, 0, w, h);

                imlib_context_set_drawable(tmp_pixmap);
                imlib_render_image_on_drawable(0, 0);
                
                alock_shade_pixmap(xinfo, tmp_pixmap, shaded_pixmap, shade, 0, 0, 0, 0, w, h);
                
                imlib_free_image_and_decache();
                imlib_context_set_drawable(shaded_pixmap);
                
                image = imlib_create_image_from_drawable(None, 0, 0, w, h, 0);
                
                XFreePixmap(xinfo->display, shaded_pixmap);
                XFreePixmap(xinfo->display, tmp_pixmap);

                imlib_context_set_drawable(pixmap);
                imlib_context_set_image(image);
            }

            if (options & ALOCK_CENTER) {
                imlib_render_image_on_drawable((xgwa.width - w)/2, (xgwa.height - h)/2);
            } else if (options & ALOCK_TILED) {
                Pixmap tile;
                GC gc;
                XGCValues gcval;
        
                tile = XCreatePixmap(xinfo->display, xinfo->root,
                                     w, h, DefaultDepth(xinfo->display, DefaultScreen(xinfo->display)));
                
                imlib_render_image_on_drawable(0, 0);
                
                gcval.fill_style = FillTiled;
                gcval.tile = tile;
                gc = XCreateGC(xinfo->display, tile, GCFillStyle|GCTile, &gcval);
                XFillRectangle(xinfo->display, pixmap, gc, 0, 0, xgwa.width, xgwa.height);
                
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
            XDestroyWindow(xinfo->display, window);
            return 0;
        }
            
        imlib_context_pop();
        imlib_context_free(context);
    }

    xswa.override_redirect = True;
    xswa.colormap = xinfo->colormap;
    xswa.background_pixmap = pixmap;

    xsmask |= CWOverrideRedirect;
    xsmask |= CWColormap;
    xsmask |= CWBackPixmap;

    window = XCreateWindow(xinfo->display, xinfo->root,
                           0, 0, xgwa.width, xgwa.height,
                           0, /* borderwidth */
                           CopyFromParent, /* depth */
                           InputOutput, /* class */
                           CopyFromParent, /* visual */
                           xsmask, &xswa);
    
    XMapWindow(xinfo->display, window);

    if (window)
        xinfo->window = window;

    if (filename)
        free(filename);

    return window;
}


static int alock_bg_imlib2_deinit(struct aXInfo* xinfo) {
    if (!xinfo || !window)
        return 0;
    XDestroyWindow(xinfo->display, window);
    XFreePixmap(xinfo->display, pixmap);
    return 1;
}

struct aBackground alock_bg_imlib2 = {
    "image",
    alock_bg_imlib2_init,
    alock_bg_imlib2_deinit
};

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */



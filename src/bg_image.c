/*
 * alock - bg_image.c
 * Copyright (c) 2005 - 2007 Mathias Gumz <akira at fluxbox dot org>
 *               2014 - 2016 Arkadiusz Bokowy
 *
 * This file is a part of an alock.
 *
 * This project is licensed under the terms of the MIT license.
 *
 * This background module provides:
 *  -bg image:file=<file>,color=<color>,shade=<int>,scale,center,tiled
 *
 * Used resources:
 *  ALock.Background.Image.Color
 *  ALock.Background.Image.Shade
 *  ALock.Background.Image.Option
 *
 */

#include "alock.h"

#include <stdlib.h>
#include <string.h>
#include <Imlib2.h>


enum aImageOption {
    AIMAGE_OPTION_NONE = 0,
    AIMAGE_OPTION_SCALE,
    AIMAGE_OPTION_CENTER,
    AIMAGE_OPTION_TILED,
};

static struct moduleData {
    Display *display;
    Pixmap *pixmaps;
    Window *windows;
    char *colorname;
    char *filename;
    unsigned int shade;
    enum aImageOption option;
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
        else if (strcmp(arg, "scale") == 0) {
            data.option = AIMAGE_OPTION_SCALE;
        }
        else if (strcmp(arg, "center") == 0) {
            data.option = AIMAGE_OPTION_CENTER;
        }
        else if (strcmp(arg, "tiled") == 0) {
            data.option = AIMAGE_OPTION_TILED;
        }
        else if (strstr(arg, "color=") == arg) {
            free(data.colorname);
            data.colorname = strdup(&arg[6]);
        }
        else if (strstr(arg, "shade=") == arg) {
            data.shade = strtol(&arg[6], NULL, 0);
            if (data.shade > 99)
                fprintf(stderr, "[shade]: shade not in range [0, 99]\n");
        }
    }

    free(arguments);
}

static void module_loadxrdb(XrmDatabase xrdb) {

    XrmValue value;
    char *type;

    if (XrmGetResource(xrdb, "alock.background.image.option",
                "ALock.Background.Image.Option", &type, &value)) {
        if (strcmp(value.addr, "scale") == 0)
            data.option = AIMAGE_OPTION_SCALE;
        else if (strcmp(value.addr, "center") == 0)
            data.option = AIMAGE_OPTION_CENTER;
        else if (strcmp(value.addr, "tiled") == 0)
            data.option = AIMAGE_OPTION_TILED;
    }

    if (XrmGetResource(xrdb, "alock.background.image.color",
                "ALock.Background.Image.Color", &type, &value))
        data.colorname = strdup(value.addr);

    if (XrmGetResource(xrdb, "alock.background.image.shade",
                "ALock.Background.Image.Shade", &type, &value))
        data.shade = strtol(value.addr, NULL, 0);

}

static int module_init(Display *dpy) {

    if (!data.filename) {
        fprintf(stderr, "[image]: file name not specified\n");
        return -1;
    }

    if (!alock_check_xrender(dpy))
        data.shade = 0;

    data.windows = (Window *)malloc(sizeof(Window) * ScreenCount(dpy));
    data.pixmaps = (Pixmap *)malloc(sizeof(Pixmap) * ScreenCount(dpy));

    {
        XSetWindowAttributes xswa;
        XColor color;
        int i;

        for (i = 0; i < ScreenCount(dpy); i++) {

            Screen *screen = ScreenOfDisplay(dpy, i);
            Colormap colormap = DefaultColormapOfScreen(screen);
            Window root = RootWindowOfScreen(screen);
            const int depth = DefaultDepthOfScreen(screen);
            const int rwidth = WidthOfScreen(screen);
            const int rheight = HeightOfScreen(screen);
            alock_alloc_color(dpy, colormap, data.colorname, "black", &color);

            { /* get image and set it as the background pixmap for the window */
                Imlib_Context context = NULL;
                Imlib_Image image = NULL;

                context = imlib_context_new();
                imlib_context_push(context);
                imlib_context_set_display(dpy);
                imlib_context_set_visual(DefaultVisualOfScreen(screen));
                imlib_context_set_colormap(colormap);

                image = imlib_load_image_without_cache(data.filename);
                if (image) {

                    int w;
                    int h;

                    data.pixmaps[i] = XCreatePixmap(dpy, root, rwidth, rheight, depth);

                    imlib_context_set_drawable(data.pixmaps[i]);
                    imlib_context_set_image(image);

                    w = imlib_image_get_width();
                    h = imlib_image_get_height();

                    if (data.shade || data.option == AIMAGE_OPTION_CENTER) {
                        GC gc;
                        XGCValues gcval;

                        gcval.foreground = color.pixel;
                        gc = XCreateGC(dpy, root, GCForeground, &gcval);
                        XFillRectangle(dpy, data.pixmaps[i], gc, 0, 0, rwidth, rheight);
                        XFreeGC(dpy, gc);
                    }

                    if (data.shade) {
                        GC gc;
                        XGCValues gcval;

                        Pixmap tmp_pixmap = XCreatePixmap(dpy, root, w, h, depth);
                        Pixmap shaded_pixmap = XCreatePixmap(dpy, root, w, h, depth);
                        gcval.foreground = color.pixel;
                        gc = XCreateGC(dpy, root, GCForeground, &gcval);
                        XFillRectangle(dpy, shaded_pixmap, gc, 0, 0, w, h);

                        imlib_context_set_drawable(tmp_pixmap);
                        imlib_render_image_on_drawable(0, 0);

                        Visual *vis = DefaultVisualOfScreen(screen);
                        alock_shade_pixmap(dpy, vis, tmp_pixmap, shaded_pixmap, data.shade, 0, 0, 0, 0, w, h);

                        imlib_free_image_and_decache();
                        imlib_context_set_drawable(shaded_pixmap);

                        image = imlib_create_image_from_drawable(None, 0, 0, w, h, 0);

                        XFreePixmap(dpy, shaded_pixmap);
                        XFreePixmap(dpy, tmp_pixmap);

                        imlib_context_set_drawable(data.pixmaps[i]);
                        imlib_context_set_image(image);
                    }

                    if (data.option == AIMAGE_OPTION_CENTER) {
                        imlib_render_image_on_drawable((rwidth - w)/2, (rheight - h)/2);
                    }
                    else if (data.option == AIMAGE_OPTION_TILED) {
                        Pixmap tile;
                        GC gc;
                        XGCValues gcval;

                        tile = XCreatePixmap(dpy, root, w, h, depth);

                        imlib_render_image_on_drawable(0, 0);

                        gcval.fill_style = FillTiled;
                        gcval.tile = tile;
                        gc = XCreateGC(dpy, tile, GCFillStyle|GCTile, &gcval);
                        XFillRectangle(dpy, data.pixmaps[i], gc, 0, 0, rwidth, rheight);

                        XFreeGC(dpy, gc);
                        XFreePixmap(dpy, tile);
                    } else { /* fallback is AIMAGE_OPTION_SCALE */
                        imlib_render_image_on_drawable_at_size(0, 0, rwidth, rheight);
                    }
                    imlib_free_image_and_decache();

                }
                else {
                    fprintf(stderr, "[image]: unable to load image from file\n");
                    free(data.windows);
                    free(data.pixmaps);
                    data.windows = NULL;
                    data.pixmaps = NULL;
                    return -1;
                }

                imlib_context_pop();
                imlib_context_free(context);
            }

            xswa.override_redirect = True;
            xswa.colormap = colormap;
            xswa.background_pixmap = data.pixmaps[i];

            data.windows[i] = XCreateWindow(dpy, root,
                    0, 0, rwidth, rheight, 0,
                    CopyFromParent, InputOutput, CopyFromParent,
                    CWOverrideRedirect | CWColormap | CWBackPixmap,
                    &xswa);

            XMapWindow(dpy, data.windows[i]);

        }
    }

    return 0;
}

static void module_free() {

    if (data.windows) {
        int i;
        for (i = 0; i < ScreenCount(data.display); i++) {
            XDestroyWindow(data.display, data.windows[i]);
            XFreePixmap(data.display, data.pixmaps[i]);
        }
        free(data.windows);
        free(data.pixmaps);
        data.windows = NULL;
        data.pixmaps = NULL;
    }

    free(data.colorname);
    data.colorname = NULL;
    free(data.filename);
    data.filename = NULL;

}

static Window module_getwindow(int screen) {
    if (!data.windows)
        return None;
    return data.windows[screen];
}


struct aModuleBackground alock_bg_image = {
    { "image",
        module_loadargs,
        module_loadxrdb,
        module_init,
        module_free,
    },
    module_getwindow,
};

/*
 * alock - input_frame.c
 * Copyright (c) 2014 Arkadiusz Bokowy
 *
 * This file is a part of an alock.
 *
 * This projected is licensed under the terms of the MIT license.
 *
 * This input module provides:
 *  -input frame:color=<input_color>|<check_color>|<error_color>,width=<int>
 *
 */

#if HAVE_CONFIG_H
#include "../config.h"
#endif

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#if HAVE_X11_EXTENSIONS_SHAPE_H
#include <X11/extensions/shape.h>
#endif

#include "alock.h"


struct aFrame {
    struct aXInfo *xi;
    Window window;
    GC gc;

    XColor color_input;
    XColor color_check;
    XColor color_error;
    int width;
};

static struct aFrame *frame = NULL;


static int alock_input_frame_init(const char *args, struct aXInfo *xinfo) {
    debug("init: %s", args);

    if (!xinfo)
        return 0;

    Display *dpy = xinfo->display;
    XSetWindowAttributes xswa;
    char *color_name[3] = { NULL };
    int status = 1;

    frame = (struct aFrame*)malloc(sizeof(struct aFrame));
    frame->xi = xinfo;
    frame->width = 10;

    if (args && strstr(args, "frame:") == args) {
        char *arguments = strdup(&args[6]);
        char *arg;
        char *tmp;
        for (tmp = arguments; tmp; ) {
            arg = strsep(&tmp, ",");
            if (strstr(arg, "color=") == arg) {
                char *tmp2;
                int i;
                for (tmp2 = &arg[6], i = 0; tmp2 && i < 3; i++) {
                    free(color_name[i]);
                    color_name[i] = strdup(strsep(&tmp2, "|"));
                }
            }
            else if (strstr(arg, "width=") == arg) {
                frame->width = strtol(&arg[6], NULL, 0);
                if (frame->width < 1) {
                    fprintf(stderr, "[frame]: invalid width value\n");
                    free(arguments);
                    goto return_error;
                }
            }
        }
        free(arguments);
    }

    xswa.override_redirect = True;
    xswa.colormap = xinfo->colormap[0];
    frame->window = XCreateWindow(dpy, xinfo->root[0],
            0, 0, xinfo->root_width[0], xinfo->root_height[0], 0,
            CopyFromParent, InputOutput, CopyFromParent, CWOverrideRedirect | CWColormap, &xswa);
    frame->gc = XCreateGC(dpy, frame->window, 0, 0);

    debug("selected colors: `%s`, `%s`, `%s`", color_name[0], color_name[1], color_name[2]);
    alock_alloc_color(xinfo, 0, color_name[0], "green", &frame->color_input);
    alock_alloc_color(xinfo, 0, color_name[1], "yellow", &frame->color_check);
    alock_alloc_color(xinfo, 0, color_name[2], "red", &frame->color_error);

#if HAVE_X11_EXTENSIONS_SHAPE_H
    XRectangle rect = {
        0, 0,
        xinfo->root_width[0] - 2 * frame->width,
        xinfo->root_height[0] - 2 * frame->width,
    };
    XShapeCombineRectangles(dpy, frame->window, ShapeBounding,
            frame->width, frame->width, &rect, 1, ShapeSubtract, 0);
#endif

    goto return_success;

return_error:
    free(frame);
    status = 0;
    frame = NULL;

return_success:
    free(color_name[0]);
    free(color_name[1]);
    free(color_name[2]);
    return status;
}

static int alock_input_frame_deinit(struct aXInfo *xinfo) {
    debug("deinit");

    if (!xinfo || !frame)
        return 0;

    Display *dpy = xinfo->display;

    XFreeGC(dpy, frame->gc);
    XDestroyWindow(dpy, frame->window);
    free(frame);

    return 1;
}

static void alock_input_frame_setstate(enum aInputState state) {
    debug("setstate: %d", state);

    if (!frame)
        return;

    Display *dpy = frame->xi->display;
    Window window = frame->window;
    XGCValues gcvals;
    int height, width;

    if (state == AINPUT_STATE_NONE) {
        /* hide input frame indicator */
        XUnmapWindow(dpy, window);
        return;
    }
    if (state == AINPUT_STATE_INIT) {
        /* show input frame indicator */
        XMapWindow(dpy, window);
        XRaiseWindow(dpy, window);
    }

    switch (state) {
    case AINPUT_STATE_CHECK:
        gcvals.foreground = frame->color_check.pixel;
        break;
    case AINPUT_STATE_ERROR:
        gcvals.foreground = frame->color_error.pixel;
        break;
    default:
        gcvals.foreground = frame->color_input.pixel;
    }

    height = frame->xi->root_height[0];
    width = frame->xi->root_width[0];

    XChangeGC(dpy, frame->gc, GCForeground, &gcvals);
    XFillRectangle(dpy, window, frame->gc, 0, 0, width, frame->width);
    XFillRectangle(dpy, window, frame->gc, 0, 0, frame->width, height);
    XFillRectangle(dpy, window, frame->gc, 0, height - frame->width, width, frame->width);
    XFillRectangle(dpy, window, frame->gc, width - frame->width, 0, frame->width, height);
    XFlush(dpy);

    /* internal input penalty for error */
    if (state == AINPUT_STATE_ERROR) {
        sleep(1);
        /* discard all pending events in the queue */
        XSync(dpy, True);
    }
}

static KeySym alock_input_frame_keypress(KeySym ks) {

    /* suppress navigation of input current position */
    if (ks == XK_Begin || ks == XK_Home || ks == XK_End ||
            ks == XK_Left || ks == XK_Right)
        return NoSymbol;

    /* treat delete key as backspace */
    if (ks == XK_Delete)
        return XK_BackSpace;

    return ks;
}


struct aInput alock_input_frame = {
    "frame",
    alock_input_frame_init,
    alock_input_frame_deinit,
    alock_input_frame_setstate,
    alock_input_frame_keypress,
};

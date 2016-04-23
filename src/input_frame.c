/*
 * alock - input_frame.c
 * Copyright (c) 2014 - 2016 Arkadiusz Bokowy
 *
 * This file is a part of an alock.
 *
 * This project is licensed under the terms of the MIT license.
 *
 * This input module provides:
 *  -input frame:input=<color>,check=<color>,error=<color>,width=<int>
 *
 * Used resources:
 *  ALock.Input.Frame.Color.Input
 *  ALock.Input.Frame.Color.Check
 *  ALock.Input.Frame.Color.Error
 *  ALock.Input.Frame.Width
 *
 */

#include "alock.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#if HAVE_X11_EXTENSIONS_SHAPE_H
#include <X11/extensions/shape.h>
#endif


struct colorPixel {
    char *name;
    unsigned long pixel;
};

static struct moduleData {
    Display *display;
    Window window;
    struct colorPixel color_input;
    struct colorPixel color_check;
    struct colorPixel color_error;
    int width;
} data = { NULL, None, { 0 }, { 0 }, { 0 }, 10 };


static void module_loadargs(const char *args) {

    if (!args || strstr(args, "frame:") != args)
        return;

    char *arguments = strdup(&args[6]);
    char *arg;
    char *tmp;

    for (tmp = arguments; tmp; ) {
        arg = strsep(&tmp, ",");
        if (strstr(arg, "width=") == arg) {
            data.width = strtol(&arg[6], NULL, 0);
        }
        else if (strstr(arg, "input=") == arg) {
            free(data.color_input.name);
            data.color_input.name = strdup(&arg[6]);
        }
        else if (strstr(arg, "check=") == arg) {
            free(data.color_check.name);
            data.color_check.name = strdup(&arg[6]);
        }
        else if (strstr(arg, "error=") == arg) {
            free(data.color_error.name);
            data.color_error.name = strdup(&arg[6]);
        }
    }

    free(arguments);
}

static void module_loadxrdb(XrmDatabase xrdb) {

    XrmValue value;
    char *type;

    if (XrmGetResource(xrdb, "alock.input.frame.width",
                "ALock.Input.Frame.Width", &type, &value))
        data.width = strtol(value.addr, NULL, 0);

    if (XrmGetResource(xrdb, "alock.input.frame.color.input",
                "ALock.Input.Frame.Color.Input", &type, &value))
        data.color_input.name = strdup(value.addr);
    if (XrmGetResource(xrdb, "alock.input.frame.color.check",
                "ALock.Input.Frame.Color.Check", &type, &value))
        data.color_check.name = strdup(value.addr);
    if (XrmGetResource(xrdb, "alock.input.frame.color.error",
                "ALock.Input.Frame.Color.Error", &type, &value))
        data.color_error.name = strdup(value.addr);

}

static int module_init(Display *dpy) {

    Screen *screen = DefaultScreenOfDisplay(dpy);
    Colormap colormap = DefaultColormapOfScreen(screen);
    XSetWindowAttributes xswa;
    XColor color;

    data.display = dpy;

    xswa.override_redirect = True;
    xswa.colormap = colormap;
    data.window = XCreateWindow(dpy, RootWindowOfScreen(screen),
            0, 0, WidthOfScreen(screen), HeightOfScreen(screen), 0,
            CopyFromParent, InputOutput, CopyFromParent, CWOverrideRedirect | CWColormap, &xswa);

    debug("selected colors: `%s`, `%s`, `%s`", data.color_input.name,
            data.color_check.name, data.color_error.name);
    alock_alloc_color(dpy, colormap, data.color_input.name, "green", &color);
    data.color_input.pixel = color.pixel;
    alock_alloc_color(dpy, colormap, data.color_check.name, "yellow", &color);
    data.color_check.pixel = color.pixel;
    alock_alloc_color(dpy, colormap, data.color_error.name, "red", &color);
    data.color_error.pixel = color.pixel;

#if HAVE_X11_EXTENSIONS_SHAPE_H
    XRectangle rect = {
        0, 0,
        WidthOfScreen(screen) - 2 * data.width,
        HeightOfScreen(screen) - 2 * data.width,
    };
    XShapeCombineRectangles(dpy, data.window, ShapeBounding,
            data.width, data.width, &rect, 1, ShapeSubtract, 0);
#endif

    return 0;
}

static void module_free(void) {

    XDestroyWindow(data.display, data.window);

    free(data.color_input.name);
    data.color_input.name = NULL;
    free(data.color_check.name);
    data.color_check.name = NULL;
    free(data.color_error.name);
    data.color_error.name = NULL;

}

static Window module_getwindow(int screen) {
    (void)screen;
    return data.window;
}

static KeySym module_keypress(KeySym key) {

    /* suppress navigation of input current position */
    if (key == XK_Begin || key == XK_Home || key == XK_End ||
            key == XK_Left || key == XK_Right)
        return NoSymbol;

    /* treat delete key as backspace */
    if (key == XK_Delete)
        return XK_BackSpace;

    return key;
}

static void module_setstate(enum aInputState state) {
    debug("setstate: %d", state);

    Display *dpy = data.display;
    XGCValues gcvals;
    int height, width;

    if (state == AINPUT_STATE_NONE) {
        /* hide input frame indicator */
        XUnmapWindow(dpy, data.window);
        return;
    }
    if (state == AINPUT_STATE_INIT) {
        /* show input frame indicator */
        XMapWindow(dpy, data.window);
        XRaiseWindow(dpy, data.window);
    }

    switch (state) {
    case AINPUT_STATE_CHECK:
        gcvals.foreground = data.color_check.pixel;
        break;
    case AINPUT_STATE_ERROR:
        gcvals.foreground = data.color_error.pixel;
        break;
    default:
        gcvals.foreground = data.color_input.pixel;
    }

    Screen *screen = DefaultScreenOfDisplay(dpy);
    width = WidthOfScreen(screen);
    height = HeightOfScreen(screen);

    GC gc = XCreateGC(dpy, data.window, 0, 0);
    XChangeGC(dpy, gc, GCForeground, &gcvals);
    XFillRectangle(dpy, data.window, gc, 0, 0, width, data.width);
    XFillRectangle(dpy, data.window, gc, 0, 0, data.width, height);
    XFillRectangle(dpy, data.window, gc, 0, height - data.width, width, data.width);
    XFillRectangle(dpy, data.window, gc, width - data.width, 0, data.width, height);
    XFreeGC(dpy, gc);
    XFlush(dpy);

    /* internal input penalty for error */
    if (state == AINPUT_STATE_ERROR) {
        sleep(1);
        /* discard all pending events in the queue */
        XSync(dpy, True);
    }
}


struct aModuleInput alock_input_frame = {
    {  "frame",
        module_loadargs,
        module_loadxrdb,
        module_init,
        module_free,
    },
    module_getwindow,
    module_keypress,
    module_setstate,
};

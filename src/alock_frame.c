/* ---------------------------------------------------------------- *\

  file    : alock_frame.c
  author  : m. gumz <akira at fluxbox dot org>
  copyr   : copyright (c) 2008 by m. gumz

  license : see LICENSE

  start   : Sa 04 Okt 2008 12:47:35 CEST

\* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- *\

  about :

\* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- *\
  includes
\* ---------------------------------------------------------------- */
#include "alock_frame.h"
#include "alock.h"

#include <X11/Xlib.h>
#include <stdlib.h>

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */

struct aSide {
    Window win;
    int width;
    int height;
    GC gc;
};

struct aFrame {
    struct aSide top;
    struct aSide left;
    struct aSide right;
    struct aSide bottom;
    XColor color;
    struct aXInfo* xi;
};

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */

struct aFrame* alock_create_frame(struct aXInfo* xi, int x, int y, int width, int height, int line_width) {

    Display* dpy = xi->display;
    struct aFrame* frame;
    struct aSide* side;
    int i;
    XSetWindowAttributes xswa;
    xswa.override_redirect = True;
    xswa.colormap = xi->colormap[0];

    frame = (struct aFrame*)calloc(1, sizeof(struct aFrame));

    if (frame == 0)
        return 0;

    frame->xi = xi;
    /*

       ascii - kungfoo

       p1 ------------------------------------------------- p2
       |                        top                          |
       p3 --------------------------------------------------p4
       p4 --- p5                                     p6 --- p7
       |   l   |                                     |   r   |
       |   e   |                                     |   i   |
       |   f   |                                     |   g   |
       |   t   |                                     |   h   |
       |       |                                     |   t   |
       p8 --- p9                                     pa --- pb
       pc ------------------------------------------------- pd
       |                      bottom                         |
       pe ------------------------------------------------- pf

    */

    frame->top.width = width;
    frame->top.height = line_width;
    frame->top.win = XCreateWindow(dpy, xi->root[0],
                x, y, frame->top.width, frame->top.height,
                0, CopyFromParent, InputOutput, CopyFromParent, CWOverrideRedirect|CWColormap, &xswa);
    frame->bottom.width = width;
    frame->bottom.height = line_width;
    frame->bottom.win = XCreateWindow(dpy, xi->root[0],
                x, y + height - line_width, frame->bottom.width, frame->bottom.height,
                0, CopyFromParent, InputOutput, CopyFromParent, CWOverrideRedirect|CWColormap, &xswa);

    frame->left.width = line_width;
    frame->left.height = height - line_width - 1;
    frame->left.win = XCreateWindow(dpy, xi->root[0],
                x, y + line_width, frame->left.width, frame->left.height,
                0, CopyFromParent, InputOutput, CopyFromParent, CWOverrideRedirect|CWColormap, &xswa);
    frame->right.width = line_width;
    frame->right.height = height - line_width - 1;
    frame->right.win = XCreateWindow(dpy, xi->root[0],
                x + width - line_width, y + line_width, frame->right.width, frame->right.height,
                0, CopyFromParent, InputOutput, CopyFromParent, CWOverrideRedirect|CWColormap, &xswa);


    side = (struct aSide*)&frame->top;
    for (i = 0; i < 4; i++) {
        XMapWindow(dpy, side[i].win);
        XRaiseWindow(dpy, side[i].win);
        side[i].gc = XCreateGC(dpy, side[i].win, 0, 0);
    }

    return frame;
}

void alock_free_frame(struct aFrame* frame) {
    struct aSide* side = (struct aSide*)&frame->top;
    struct aXInfo* xi = frame->xi;
    int i;

    for (i = 0; i < 4; i++) {
        XFreeGC(xi->display, side[i].gc);
        XDestroyWindow(xi->display, side[i].win);
    }

    free(frame);
}

void alock_draw_frame(struct aFrame* frame, const char* color_name) {

    struct aSide* side = (struct aSide*)&frame->top;
    struct aXInfo* xi = frame->xi;
    Display* dpy = xi->display;
    XGCValues gcvals;
    XColor tmp;
    int i;

    XAllocNamedColor(dpy, xi->colormap[0], color_name, &frame->color, &tmp);
    gcvals.foreground = frame->color.pixel;

    for (i = 0; i < 4; i++) {
        XChangeGC(dpy, side[i].gc, GCForeground, &gcvals);
        XFillRectangle(dpy, side[i].win, side[i].gc, 0, 0, side[i].width, side[i].height);
    }
}

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */



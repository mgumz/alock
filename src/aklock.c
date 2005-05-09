/*------------------------------------------------------------------*\

    file: aklock.c

        X Transparent Lock

    copyright:

        Copyright (C)1993,1994 Ian Jackson   (xtrlock)
        Copyright (C)2005 Mathias Gumz (forked aklock)

    license:

        This is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 2, or (at your option)
        any later version.

        This is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

\*------------------------------------------------------------------*/

/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xos.h>

#ifdef HAVE_XPM
#   include <X11/xpm.h>
#endif /* HAVE_XPM */
#ifdef HAVE_XCURSOR
#   include <X11/Xcursor/Xcursor.h>
#endif /* HAVE_XCURSOR */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*----------------------------------------------*\
\*----------------------------------------------*/

#include "aklock.h"

struct akXInfo {

    Display* display;
    Window   root;
    Window   window;
    Colormap colormap;

    Cursor   cursor;

    int width;
    int height;
};
/*------------------------------------------------------------------*\
    globals
\*------------------------------------------------------------------*/

#define TIMEOUTPERATTEMPT 30000
#define MAXGOODWILL  (TIMEOUTPERATTEMPT*5)
#define INITIALGOODWILL MAXGOODWILL
#define GOODWILLPORTION 0.3

#ifdef DEBUG
#    define DBGMSG fprintf(stderr, "%s : %d\n", __FUNCTION__, __LINE__); fflush(stderr)
#else
#    define DBGMSG
#endif // DEBUG

static struct akAuth* ak_authmodules[] = {
    &aklock_auth_none,
#ifdef HASH_PWD
    &aklock_auth_md5,
    &aklock_auth_sha1,
#endif /* HASH_PWD */
#ifdef PASSWD_PWD
    &aklock_auth_passwd,
#endif /* PASSWD_PWD */
#ifdef PAM_PWD
    &aklock_auth_pam,
#endif
    NULL
};

/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/

void displayUsage() {
    printf("\n");
    printf("aklock [-h] [-v] [-blank] [-cursor <theme|xcursor:file>]\n");
    printf("       [-auth list|<none");
#ifdef PASSWD_PWD
    printf("|passwd");
#endif /* PASSWD_PWD */
#ifdef PAM_PWD
    printf("|pam");
#endif /* PAM_PWD */
#ifdef HASH_PWD
    printf("|md5:pwdhash|sha1:pwdhash");
#endif /* HASH_PWD */
    printf(">]\n");
}

/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/

void initOpts(struct akOpts* opts) {

    opts->auth = ak_authmodules[0];
    opts->use_blank = 0;

    opts->cursor_name = "mini";

    opts->color_bg = "steelblue3";
    opts->color_fg = "grey25";

}

void initXInfo(struct akXInfo* xinfo, struct akOpts* opts) {


    Display* dpy = XOpenDisplay(NULL);
    XColor color_fg;
    XColor color_bg;
    XColor tmp_color;
    Pixmap pixmap_cursor;
    Pixmap pixmap_cursor_mask;
    XWindowAttributes xgwa;
    struct akCursor* cursor = NULL;

    if (!dpy) {
        perror("aklock: error, can't open connection to X");
        exit(1);
    }

    xinfo->display = dpy;
    xinfo->window = 0;

    xinfo->root = DefaultRootWindow(dpy);
    xinfo->colormap = DefaultColormap(dpy, DefaultScreen(dpy));

    XGetWindowAttributes(dpy, xinfo->root, &xgwa);

    if (opts->use_blank) {
        xinfo->width = xgwa.width;
        xinfo->height = xgwa.height;
    } else {
        xinfo->width = 1;
        xinfo->height = 1;
    }

    if((XAllocNamedColor(dpy, xinfo->colormap, opts->color_bg, &tmp_color, &color_bg)) == 0)
        XAllocNamedColor(dpy, xinfo->colormap, "black", &tmp_color, &color_bg);
    if((XAllocNamedColor(dpy, xinfo->colormap, opts->color_fg, &tmp_color, &color_fg)) == 0)
        XAllocNamedColor(dpy, xinfo->colormap, "white", &tmp_color, &color_fg);

    /* load cursors */
#ifdef HAVE_XCURSOR
    if (opts->cursor_name && (strstr(opts->cursor_name, "xcursor:"))) {
        Cursor xcursor;
        if ((xcursor = XcursorFilenameLoadCursor(dpy, &opts->cursor_name[8]))) {
            xinfo->cursor = xcursor;
            return;
        } else {
            printf("aklock: error, couldnt load [%s]\n", &opts->cursor_name[8]);
        }
    }
#endif /* HAVE_XCURSOR */

    /* TODO: doesnt work yet. */
/*
 * if (opts->cursor_name && (strstr(opts->cursor_name, "xbm:"))) {
        unsigned int w, h, xhot, yhot;
        if (XReadBitmapFile(dpy, xinfo->root, &opts->cursor_name[4],
                            &w, &h, &pixmap_cursor, &xhot, &yhot)) {
            xinfo->cursor = XCreatePixmapCursor(dpy,
                                                pixmap_cursor, NULL,
                                                &color_fg, &color_bg,
                                                xhot, yhot);
            return;
        } else {
            printf("aklock: error, couldnt load [%s]\n", &opts->cursor_name[4]);
        }
    }
   */
    /* look internal cursors */
    for(cursor = ak_cursors; cursor->name != NULL; cursor++) {
        if (!strcmp(cursor->name, opts->cursor_name))
                break;
    }

    if (!cursor->name)
        cursor = ak_cursors;

    pixmap_cursor = XCreateBitmapFromData(dpy, xinfo->root, cursor->bits, cursor->width, cursor->height);
    pixmap_cursor_mask = XCreateBitmapFromData(dpy, xinfo->root, cursor->mask, cursor->width, cursor->height);

    xinfo->cursor = XCreatePixmapCursor(dpy,
                                        pixmap_cursor, pixmap_cursor_mask,
                                        &color_fg, &color_bg,
                                        cursor->x_hot, cursor->y_hot);


}

int main(int argc, char **argv) {

    XEvent ev;
    KeySym ks;
    char cbuf[10], rbuf[50];
    int clen, rlen = 0;
    long goodwill = INITIALGOODWILL, timeout = 0;

    XSetWindowAttributes xswa;
    long xsmask = 0;

    struct akXInfo xinfo;
    struct akOpts opts;

    int arg = 0;

    initOpts(&opts);

    /*  parse options */
    if (argc != 1) {
        for(arg = 1; arg <= argc; arg++) {
            if (!strcmp(argv[arg - 1], "-blank")) {
                opts.use_blank = 1;
            } else if (!strcmp(argv[arg - 1], "-auth")) {
                if (arg < argc) {

                    char* char_tmp;
                    struct akAuth* auth_tmp = NULL;
                    struct akAuth** i;
                    if (!strcmp(argv[arg], "list")) {
                        for(i = ak_authmodules; *i; ++i) {
                            printf("%s\n", (*i)->name);
                        }
                        exit(0);
                    }

                    for(i = ak_authmodules; *i; ++i) {
                        char_tmp = strstr(argv[arg], (*i)->name);
                        if(char_tmp && char_tmp == argv[arg]) {
                            auth_tmp = (*i);
                            if (!auth_tmp->init(argv[arg])) {
                                fprintf(stderr, "aklock: error, failed init of [%s].\n", auth_tmp->name);
                                exit(1);
                            }
                            opts.auth = auth_tmp;
                            ++arg;
                            break;
                        }
                    }

                    if (!auth_tmp) {
                        fprintf(stderr, "aklock: error, couldnt find the auth module you specified.\n");
                        exit(1);
                    }

                } else {
                    fprintf(stderr, "aklock, error, missing argument\n");
                    displayUsage();
                    exit(1);
                }
            } else if (!strcmp(argv[arg - 1], "-cursor")) {
                if (arg < argc)
                    opts.cursor_name = argv[arg];
                else {
                    printf("aklock: error, missing argument\n");
                    displayUsage();
                    exit(1);
                }
            } else if (!strcmp(argv[arg - 1], "-h")) {
                displayUsage();
                exit(0);
            } else if (!strcmp(argv[arg - 1], "-v")) {
                printf("aklock-%s by m.gumz 2005\n", VERSION);
                exit(0);
            }
        }
    }

    initXInfo(&xinfo, &opts);

    /* create the windows */
    xswa.override_redirect = True;
    xsmask |= CWOverrideRedirect;

    if (opts.use_blank) {

        xswa.background_pixel = BlackPixel(xinfo.display, DefaultScreen(xinfo.display));
        xswa.colormap = xinfo.colormap;
        xsmask |= CWBackPixel;
        xsmask |= CWColormap;
    }

    xinfo.window = XCreateWindow(xinfo.display, xinfo.root,
                          0, 0, xinfo.width, xinfo.height,
                          0, /* borderwidth */
                          CopyFromParent, /* depth */
                          InputOutput, /* class */
                          CopyFromParent, /* visual */
                          xsmask, &xswa);



    XSelectInput(xinfo.display, xinfo.window, KeyPressMask|KeyReleaseMask);
    XMapWindow(xinfo.display, xinfo.window);
    XRaiseWindow(xinfo.display, xinfo.window);
    
    /* TODO: -bg <blank|transparent|shaded> */
 /*   if (opts.use_blank) {
        XImage* ximage;
                
        ximage = XGetImage (xinfo.display, xinfo.root, 0, 0,
                    xinfo.width, xinfo.height, AllPlanes, ZPixmap);

        if (ximage) {
            GC gc;
            XGCValues xgcv;
            xgcv.background = BlackPixel(xinfo.display, DefaultScreen(xinfo.display));
            xgcv.foreground = 0;
            
            gc = XCreateGC(xinfo.display, xinfo.window, GCForeground|GCBackground, &xgcv);

            XPutImage(xinfo.display, xinfo.window, 
                      gc, 
                      ximage, 
                      0, 0, 
                      0, 0, xinfo.width, xinfo.height);
            XDestroyImage(ximage);
            XFreeGC(xinfo.display, gc);
        }
    } */
    /* try to grab 2 times, another process (windowmanager) may have grabbed
     * the keyboard already */
    if ((XGrabKeyboard(xinfo.display, xinfo.window, True, GrabModeAsync, GrabModeAsync,
                      CurrentTime)) != GrabSuccess) {
        sleep(1);
        if ((XGrabKeyboard(xinfo.display, xinfo.window, True, GrabModeAsync, GrabModeAsync,
                        CurrentTime)) != GrabSuccess) {
            perror("aklock: couldnt grab the keyboard.\n");
            exit(1);
        }
    }

    if (XGrabPointer(xinfo.display, xinfo.window, False, (KeyPressMask|KeyReleaseMask) & 0,
                     GrabModeAsync, GrabModeAsync, None, xinfo.cursor, CurrentTime) != GrabSuccess) {
        XUngrabKeyboard(xinfo.display, CurrentTime);
        perror("aklock: couldnt grab the pointer.\n");
        exit(1);
    }

    /* eventhandling */
    for(;;) {
        XNextEvent(xinfo.display, &ev);
        switch (ev.type) {
        case KeyPress:
            
            if (ev.xkey.time < timeout) {
                XBell(xinfo.display, 0);
                break;
            }

            clen = XLookupString(&ev.xkey, cbuf, 9, &ks, 0);
            switch (ks) {
            case XK_Escape:
            case XK_Clear:
                rlen = 0;
                break;
            case XK_Delete:
            case XK_BackSpace:
                if (rlen > 0)
                    rlen--;
                break;
            case XK_Linefeed:
            case XK_Return:
                if (rlen == 0)
                    break;
                rbuf[rlen] = 0;

                if (opts.auth->auth(rbuf))
                    goto exit;
                
                XSync(xinfo.display, True); /* discard pending events to start really fresh */
                XBell(xinfo.display, 0);
                rlen = 0;
                if (timeout) {
                    goodwill += ev.xkey.time - timeout;
                    if (goodwill > MAXGOODWILL) {
                        goodwill = MAXGOODWILL;
                    }
                }
                timeout = -goodwill * GOODWILLPORTION;
                goodwill += timeout;
                timeout += ev.xkey.time + TIMEOUTPERATTEMPT;
                break;
            default:
                if (clen != 1)
                    break;
                if (rlen < sizeof(rbuf))
                    rbuf[rlen] = cbuf[0];
                rlen++;
                break;
            }
            break;
        default:
                break;
        }
    }

exit:

    opts.auth->deinit();

    XDestroyWindow(xinfo.display, xinfo.window);
    XFreeCursor(xinfo.display, xinfo.cursor);
    XCloseDisplay(xinfo.display);

    return 0;
}


/*------------------------------------------------------------------*\

    file: alock.c

        X Transparent Lock

    copyright:

        Copyright (C)2005 Mathias Gumz (forked alock)
        Copyright (C)1993,1994 Ian Jackson   (xtrlock)

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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*----------------------------------------------*\
\*----------------------------------------------*/

#include "alock.h"

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

static struct aAuth* alock_authmodules[] = {
#ifdef PAM_PWD
    &alock_auth_pam,
#endif
#ifdef PASSWD_PWD
    &alock_auth_passwd,
#endif /* PASSWD_PWD */
#ifdef HASH_PWD
    &alock_auth_md5,
    &alock_auth_sha1,
#endif /* HASH_PWD */
    &alock_auth_none,
    NULL
};

static struct aBackground* alock_backgrounds[] = {
    &alock_bg_none,
    &alock_bg_blank,
    NULL
};

static struct aCursor* alock_cursors[] = {
    &alock_cursor_theme,
    &alock_cursor_none,
    &alock_cursor_font,
#ifdef HAVE_XCURSOR
    &alock_cursor_xcursor,
#endif /* HAVE_XCURSOR */
    NULL
};

/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/

void displayUsage() {
    printf("alock [-hv] [-bg type:options] [-cursor type:options] ");
    printf("[-auth type:options]\n");
}

/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/

void initXInfo(struct aXInfo* xinfo, struct aOpts* opts) {

    Display* dpy = XOpenDisplay(NULL);

    if (!dpy) {
        perror("alock: error, can't open connection to X");
        exit(1);
    }

    xinfo->display = dpy;
    xinfo->window = 0;

    xinfo->root = DefaultRootWindow(dpy);
    xinfo->colormap = DefaultColormap(dpy, DefaultScreen(dpy));
    
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
            printf("alock: error, couldnt load [%s]\n", &opts->cursor_name[4]);
        }
    }
   */
}

int main(int argc, char **argv) {

    XEvent ev;
    KeySym ks;
    char cbuf[10], rbuf[50];
    int clen, rlen = 0;
    long goodwill = INITIALGOODWILL, timeout = 0;

    struct aXInfo xinfo;
    struct aOpts opts;

    int arg = 0;
    const char* cursor_args = NULL;
    const char* background_args = NULL;

    opts.auth = alock_authmodules[0];
    opts.cursor = alock_cursors[0];
    opts.background = alock_backgrounds[0];

    /*  parse options */
    if (argc != 1) {
        for(arg = 1; arg <= argc; arg++) {
            if (!strcmp(argv[arg - 1], "-bg")) {
                if (arg < argc) {

                    char* char_tmp;
                    struct aBackground* bg_tmp = NULL;
                    struct aBackground** i;
                    if (!strcmp(argv[arg], "list")) {
                        for(i = alock_backgrounds; *i; ++i) {
                            printf("%s\n", (*i)->name);
                        }
                        exit(0);
                    }

                    for(i = alock_backgrounds; *i; ++i) {
                        char_tmp = strstr(argv[arg], (*i)->name);
                        if(char_tmp && char_tmp == argv[arg]) {
                            background_args = char_tmp;
                            bg_tmp = *i;
                            opts.background = bg_tmp;
                            ++arg;
                            break;
                        }
                    }

                    if (!bg_tmp) {
                        fprintf(stderr, "alock: error, couldnt find the bg-module you specified.\n");
                        exit(1);
                    }

                } else {
                    fprintf(stderr, "alock, error, missing argument\n");
                    displayUsage();
                    exit(1);
                }
            } else if (!strcmp(argv[arg - 1], "-auth")) {
                if (arg < argc) {

                    char* char_tmp;
                    struct aAuth* auth_tmp = NULL;
                    struct aAuth** i;
                    if (!strcmp(argv[arg], "list")) {
                        for(i = alock_authmodules; *i; ++i) {
                            printf("%s\n", (*i)->name);
                        }
                        exit(0);
                    }

                    for(i = alock_authmodules; *i; ++i) {
                        char_tmp = strstr(argv[arg], (*i)->name);
                        if(char_tmp && char_tmp == argv[arg]) {
                            auth_tmp = (*i);
                            if (!auth_tmp->init(argv[arg])) {
                                fprintf(stderr, "alock: error, failed init of [%s].\n", auth_tmp->name);
                                exit(1);
                            }
                            opts.auth = auth_tmp;
                            ++arg;
                            break;
                        }
                    }

                    if (!auth_tmp) {
                        fprintf(stderr, "alock: error, couldnt find the auth-module you specified.\n");
                        exit(1);
                    }

                } else {
                    fprintf(stderr, "alock, error, missing argument\n");
                    displayUsage();
                    exit(1);
                }
            } else if (!strcmp(argv[arg - 1], "-cursor")) {
                if (arg < argc) {

                    char* char_tmp;
                    struct aCursor* cursor_tmp = NULL;
                    struct aCursor** i;
                    if (!strcmp(argv[arg], "list")) {
                        for(i = alock_cursors; *i; ++i) {
                            printf("%s\n", (*i)->name);
                        }
                        exit(0);
                    }

                    for(i = alock_cursors; *i; ++i) {
                        char_tmp = strstr(argv[arg], (*i)->name);
                        if(char_tmp && char_tmp == argv[arg]) {
                            cursor_args = char_tmp;
                            cursor_tmp = *i;
                            opts.cursor= cursor_tmp;
                            ++arg;
                            break;
                        }
                    }

                    if (!cursor_tmp) {
                        fprintf(stderr, "alock: error, couldnt find the cursor-module you specified.\n");
                        exit(1);
                    }

                } else {
                    fprintf(stderr, "alock, error, missing argument\n");
                    displayUsage();
                    exit(1);
                }
            } else if (!strcmp(argv[arg - 1], "-h")) {
                displayUsage();
                exit(0);
            } else if (!strcmp(argv[arg - 1], "-v")) {
                printf("alock-%s by m.gumz 2005\n", VERSION);
                exit(0);
            }
        }
    }

    initXInfo(&xinfo, &opts);

    if (!opts.background->init(background_args, &xinfo)) {
        printf("alock: error, couldnt init [%s] with [%s].\n", 
               opts.background->name,
               background_args);
        exit(1);
    }

    if (!opts.cursor->init(cursor_args, &xinfo)) {
        printf("alock: error, couldnt init [%s] with [%s].\n", 
               opts.cursor->name,
               cursor_args);
        exit(1);
    }

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
            perror("alock: couldnt grab the keyboard.\n");
            exit(1);
        }
    }

    if (XGrabPointer(xinfo.display, xinfo.window, False, (KeyPressMask|KeyReleaseMask) & 0,
                     GrabModeAsync, GrabModeAsync, None, xinfo.cursor, CurrentTime) != GrabSuccess) {
        XUngrabKeyboard(xinfo.display, CurrentTime);
        perror("alock: couldnt grab the pointer.\n");
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
    opts.cursor->deinit(&xinfo);
    opts.background->deinit(&xinfo);
    XCloseDisplay(xinfo.display);
    
    return 0;
}


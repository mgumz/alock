/* ---------------------------------------------------------------- *\

  file    : alock.c
  author  : m. gumz <akira at fluxbox dot org>
  copyr   : copyright (c) 2005 - 2007 by m. gumz

  license : see LICENSE

  start   : Sa 30 April 2005 14:19:44 CEST

\* ---------------------------------------------------------------- */

#include "alock.h"

#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xos.h>
#include <stdlib.h>
#include <string.h>

/*----------------------------------------------*\
\*----------------------------------------------*/


/*------------------------------------------------------------------*\
    globals
\*------------------------------------------------------------------*/
extern struct aAuth alock_auth_none;
#ifdef HAVE_HASH
extern struct aAuth alock_auth_md5;
extern struct aAuth alock_auth_sha1;
extern struct aAuth alock_auth_sha256;
extern struct aAuth alock_auth_sha384;
extern struct aAuth alock_auth_sha512;
extern struct aAuth alock_auth_wpool;
#endif /* HAVE_HASH */
#ifdef HAVE_PASSWD
extern struct aAuth alock_auth_passwd;
#endif /* HAVE_PASSWD */
#ifdef HAVE_PAM
extern struct aAuth alock_auth_pam;
#endif /* HAVE_PAM */

static struct aAuth* alock_authmodules[] = {
    &alock_auth_none,
#ifdef HAVE_PAM
    &alock_auth_pam,
#endif /* HAVE_PAM */
#ifdef HAVE_PASSWD
    &alock_auth_passwd,
#endif /* HAVE_PASSWD */
#ifdef HAVE_HASH
    &alock_auth_md5,
    &alock_auth_sha1,
    &alock_auth_sha256,
    &alock_auth_sha384,
    &alock_auth_sha512,
    &alock_auth_wpool,
#endif /* HAVE_HASH */
    NULL
};
/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/
extern struct aBackground alock_bg_none;
extern struct aBackground alock_bg_blank;
#ifdef HAVE_IMLIB2
extern struct aBackground alock_bg_image;
#endif /* HAVE_IMLIB2 */
#ifdef HAVE_XRENDER
extern struct aBackground alock_bg_shade;
#endif /* HAVE_XRENDER */

static struct aBackground* alock_backgrounds[] = {
    &alock_bg_none,
    &alock_bg_blank,
#ifdef HAVE_IMLIB2
    &alock_bg_image,
#endif /* HAVE_IMLIB2 */
#ifdef HAVE_XRENDER
    &alock_bg_shade,
#endif /* HAVE_XRENDER */
    NULL
};
/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */
extern struct aCursor alock_cursor_none;
extern struct aCursor alock_cursor_glyph;
extern struct aCursor alock_cursor_theme;
#ifdef HAVE_XCURSOR
extern struct aCursor alock_cursor_xcursor;
#if (defined(HAVE_XRENDER) && (defined(HAVE_XPM) || (defined(HAVE_IMLIB2))))
extern struct aCursor alock_cursor_image;
#endif /* HAVE_XRENDER && (HAVE_XPM || HAVE_IMLIB2) */
#endif /* HAVE_XCURSOR */

static struct aCursor* alock_cursors[] = {
    &alock_cursor_none,
    &alock_cursor_theme,
    &alock_cursor_glyph,
#ifdef HAVE_XCURSOR
    &alock_cursor_xcursor,
#if (defined(HAVE_XRENDER) && ((defined(HAVE_XPM) || defined(HAVE_IMLIB2))))
    &alock_cursor_image,
#endif /* HAVE_XRENDER && (HAVE_XPM || HAVE_IMLIB2) */
#endif /* HAVE_XCURSOR */
    NULL
};
/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/

static void displayUsage() {
    printf("alock [-hv] [-bg type:options] [-cursor type:options] ");
    printf("[-auth type:options]\n");
}

/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/

static void initXInfo(struct aXInfo* xinfo) {

    Display* dpy = XOpenDisplay(NULL);

    if (!dpy) {
        perror("alock: error, can't open connection to X");
        exit(EXIT_FAILURE);
    }

    {
        xinfo->display = dpy;
        xinfo->nr_screens = ScreenCount(dpy);
        xinfo->window = (Window*)calloc((size_t)xinfo->nr_screens, sizeof(Window));
        xinfo->root = (Window*)calloc((size_t)xinfo->nr_screens, sizeof(Window));
        xinfo->colormap = (Colormap*)calloc((size_t)xinfo->nr_screens, sizeof(Colormap));
        xinfo->cursor = (Cursor*)calloc((size_t)xinfo->nr_screens, sizeof(Cursor));
    }
    {
        int scr;
        for (scr = 0; scr < xinfo->nr_screens; scr++) {
            xinfo->window[scr] = None;
            xinfo->root[scr] = RootWindow(dpy, scr);
            xinfo->colormap[scr] = DefaultColormap(dpy, scr);
        }
    }
}

static int event_loop(struct aOpts* opts, struct aXInfo* xinfo) {

    XEvent ev;
    KeySym ks;
    char cbuf[10], rbuf[50];
    unsigned int clen, rlen = 0;

    const long max_goodwill = 5 * 30000; /* 150 seconds */
    long goodwill = max_goodwill;
    long timeout = 0;

    for(;;) {

        XNextEvent(xinfo->display, &ev);
        switch (ev.type) {
        case KeyPress:

            if (ev.xkey.time < timeout) {
                XBell(xinfo->display, 0);
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
                if (rlen < sizeof(rbuf))
                    rbuf[rlen] = 0;

                if (opts->auth->auth(rbuf))
                    return 1;

                XSync(xinfo->display, True); /* discard pending events to start really fresh */
                XBell(xinfo->display, 0);
                rlen = 0;

                if (timeout) {
                    goodwill += ev.xkey.time - timeout;
                    if (goodwill > max_goodwill) {
                        goodwill = max_goodwill;
                    }
                }

                {
                    long offset;

                    offset = goodwill * 0.3;
                    goodwill = goodwill - offset;
                    timeout = ev.xkey.time + 30000 - offset;
                }
                break;
            default:
                if (clen != 1)
                    break;
                if (rlen < (sizeof(rbuf) - 1)) {
                    rbuf[rlen] = cbuf[0];
                    rlen++;
                }
                break;
            }
            break;
        default:
            break;
        }
    }

    return 0;
}

int main(int argc, char **argv) {


    struct aXInfo xinfo;
    struct aOpts opts;

    int arg = 0;
    const char* cursor_args = NULL;
    const char* background_args = NULL;

    opts.auth = NULL;
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
                    if (strcmp(argv[arg], "list") == 0) {
                        for(i = alock_backgrounds; *i; ++i) {
                            printf("%s\n", (*i)->name);
                        }
                        exit(EXIT_SUCCESS);
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

                    if (bg_tmp == NULL) {
                        printf("alock: error, couldnt find the bg-module you specified.\n");
                        exit(EXIT_FAILURE);
                    }

                } else {
                    printf("alock, error, missing argument\n");
                    displayUsage();
                    exit(EXIT_FAILURE);
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
                        exit(EXIT_SUCCESS);
                    }

                    for(i = alock_authmodules; *i; ++i) {
                        char_tmp = strstr(argv[arg], (*i)->name);
                        if(char_tmp && char_tmp == argv[arg]) {
                            auth_tmp = (*i);
                            if (auth_tmp->init(argv[arg]) == 0) {
                                printf("alock: error, failed init of [%s].\n", auth_tmp->name);
                                exit(EXIT_FAILURE);
                            }
                            opts.auth = auth_tmp;
                            ++arg;
                            break;
                        }
                    }

                    if (auth_tmp == NULL) {
                        printf("alock: error, couldnt find the auth-module you specified.\n");
                        exit(EXIT_FAILURE);
                    }

                } else {
                    printf("alock, error, missing argument\n");
                    displayUsage();
                    exit(EXIT_FAILURE);
                }
            } else if (strcmp(argv[arg - 1], "-cursor") == 0) {
                if (arg < argc) {

                    char* char_tmp;
                    struct aCursor* cursor_tmp = NULL;
                    struct aCursor** i;
                    if (strcmp(argv[arg], "list") == 0) {
                        for(i = alock_cursors; *i; ++i) {
                            printf("%s\n", (*i)->name);
                        }
                        exit(EXIT_SUCCESS);
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
                        printf("alock: error, couldnt find the cursor-module you specified.\n");
                        exit(EXIT_FAILURE);
                    }

                } else {
                    printf("alock, error, missing argument\n");
                    displayUsage();
                    exit(EXIT_FAILURE);
                }
            } else if (strcmp(argv[arg - 1], "-h") == 0) {
                displayUsage();
                exit(EXIT_SUCCESS);
            } else if (strcmp(argv[arg - 1], "-v") == 0) {
                printf("alock-%s by m.gumz 2005 - 2006\n", VERSION);
                exit(EXIT_SUCCESS);
            }
        }
    }

    if (!opts.auth) {
        printf("alock: error, no auth-method specified.\n");
        displayUsage();
        exit(EXIT_FAILURE);
    }

    initXInfo(&xinfo);

    if (opts.background->init(background_args, &xinfo) == 0) {
        printf("alock: error, couldnt init [%s] with [%s].\n",
               opts.background->name,
               background_args);
        exit(EXIT_FAILURE);
    }

    if (opts.cursor->init(cursor_args, &xinfo) == 0) {
        printf("alock: error, couldnt init [%s] with [%s].\n",
               opts.cursor->name,
               cursor_args);
        exit(EXIT_FAILURE);
    }

    {
        int scr;
        for (scr = 0; scr < xinfo.nr_screens; scr++) {

            XSelectInput(xinfo.display, xinfo.window[scr], KeyPressMask|KeyReleaseMask);
            XMapWindow(xinfo.display, xinfo.window[scr]);
            XRaiseWindow(xinfo.display, xinfo.window[scr]);

        }
    }

    /* try to grab 2 times, another process (windowmanager) may have grabbed
     * the keyboard already */
    if ((XGrabKeyboard(xinfo.display, xinfo.window[0], True, GrabModeAsync, GrabModeAsync,
                          CurrentTime)) != GrabSuccess) {
        sleep(1);
        if ((XGrabKeyboard(xinfo.display, xinfo.window[0], True, GrabModeAsync, GrabModeAsync,
                        CurrentTime)) != GrabSuccess) {
            printf("alock: couldnt grab the keyboard.\n");
            exit(EXIT_FAILURE);
        }
    }


    /* TODO: think about it: do we really need NR_SCREEN cursors ? we grab the
     * pointer on :*.0 anyway ... */
    if (XGrabPointer(xinfo.display, xinfo.window[0], False, None,
                     GrabModeAsync, GrabModeAsync, None, xinfo.cursor[0], CurrentTime) != GrabSuccess) {
        XUngrabKeyboard(xinfo.display, CurrentTime);
        printf("alock: couldnt grab the pointer.\n");
        exit(EXIT_FAILURE);
    }

    event_loop(&opts, &xinfo);

    opts.auth->deinit();
    opts.cursor->deinit(&xinfo);
    opts.background->deinit(&xinfo);
    XCloseDisplay(xinfo.display);

    return EXIT_SUCCESS;
}


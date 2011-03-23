/* ---------------------------------------------------------------- *\

  file    : alock.c
  author  : m. gumz <akira at fluxbox dot org>
  copyr   : copyright (c) 2005 - 2007 by m. gumz

  license : see LICENSE

  start   : Sa 30 April 2005 14:19:44 CEST

\* ---------------------------------------------------------------- */

#include "alock.h"
#include "alock_frame.h"

#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/Xos.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <poll.h>

#ifdef HAVE_XF86MISC
#include <X11/extensions/xf86misc.h>
#endif

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
/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */
static struct timeval alock_start_time;

static void initStartTime() {
    X_GETTIMEOFDAY(&alock_start_time);
}

// taken from gdk.c
static long elapsedTime() {

    static struct timeval end;
    static struct timeval elapsed;
    long milliseconds;

    X_GETTIMEOFDAY(&end);

    if( alock_start_time.tv_usec > end.tv_usec ) {

        end.tv_usec += 1000000;
        end.tv_sec--;
    }

    elapsed.tv_sec = end.tv_sec - alock_start_time.tv_sec;
    elapsed.tv_usec = end.tv_usec - alock_start_time.tv_usec;

    milliseconds = (elapsed.tv_sec * 1000) + (elapsed.tv_usec / 1000);

    return milliseconds;
}

/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/

static void displayUsage() {
    printf("%s", "alock [-hv] [-bg type:options] [-cursor type:options] "
                 "[-auth type:options]\n");
}

/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/

static void initXInfo(struct aXInfo* xi) {

    Display* dpy = XOpenDisplay(NULL);

    if (!dpy) {
        perror("alock: error, can't open connection to X");
        exit(EXIT_FAILURE);
    }

    {
        xi->display = dpy;
        xi->pid_atom = XInternAtom(dpy, "_ALOCK_PID", False);
        xi->nr_screens = ScreenCount(dpy);
        xi->window = (Window*)calloc((size_t)xi->nr_screens, sizeof(Window));
        xi->root = (Window*)calloc((size_t)xi->nr_screens, sizeof(Window));
        xi->colormap = (Colormap*)calloc((size_t)xi->nr_screens, sizeof(Colormap));
        xi->cursor = (Cursor*)calloc((size_t)xi->nr_screens, sizeof(Cursor));
        xi->width_of_root = (int*)calloc(xi->nr_screens, sizeof(int));
        xi->height_of_root = (int*)calloc(xi->nr_screens, sizeof(int));
    }
    {
        XWindowAttributes xgwa;
        int scr;
        for (scr = 0; scr < xi->nr_screens; scr++) {
            xi->window[scr] = None;
            xi->root[scr] = RootWindow(dpy, scr);
            xi->colormap[scr] = DefaultColormap(dpy, scr);

            XGetWindowAttributes(dpy, xi->root[scr], &xgwa);
            xi->width_of_root[scr] = xgwa.width;
            xi->height_of_root[scr] = xgwa.height;
        }
    }
}

/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/

enum {
    INITIAL = 0,
    TYPING,
    WRONG
};

static void visualFeedback(struct aFrame* frame, int mode) {

    static int old_mode = INITIAL;
    int redraw = 0;

    if (old_mode != mode)
        redraw = 1;

    old_mode = mode;

    switch (mode) {
    case INITIAL:
        alock_hide_frame(frame);
        break;
    case TYPING:
        if (redraw) {
            alock_draw_frame(frame, "green");
        }
        break;
    case WRONG:
        if (redraw) {
            alock_draw_frame(frame, "red");
        }
        break;
    };
}

static int eventLoop(struct aOpts* opts, struct aXInfo* xi) {

    Display* dpy = xi->display;
    XEvent ev;
    KeySym ks;
    char cbuf[10];
    char rbuf[50];
    unsigned int clen, rlen = 0;
    long current_time = 0;
    long last_key_time = 0;
    const long penalty = 1000;
    long timeout = 0;
    int mode = INITIAL;

    struct aFrame* frame = alock_create_frame(xi, 0, 0, xi->width_of_root[0], xi->height_of_root[0], 10);

    for(;;) {

        current_time = elapsedTime();

        // check for any keypresses
        if (XCheckWindowEvent(dpy, xi->window[0], KeyPressMask|KeyReleaseMask, &ev) == True) {

            switch (ev.type) {
            case KeyPress:

                last_key_time = current_time;

                if (last_key_time < timeout) {
                    XBell(dpy, 0);
                    break;
                }

                // swallow up first keypress to indicate "enter mode"
                if (mode == INITIAL) {
                    mode = TYPING;
                    break;
                }

                mode = TYPING;

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

                    // some auth() methods have their own penalty system
                    // so we draw a 'yellow' frame to show 'checking' state.

                    alock_draw_frame(frame, "yellow");
                    XSync(dpy, True);

                    if (opts->auth->auth(rbuf)) {
                        alock_free_frame(frame);
                        return 1;
                    }

                    XBell(dpy, 0);
                    mode = WRONG;
                    timeout = elapsedTime() + penalty;
                    rlen = 0;
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
            case Expose: {
                    XExposeEvent* eev = (XExposeEvent*)&ev;
                    XClearWindow(xi->display, eev->window);
                }
                break;
            default:
                break;
            }

        } else { // wait a bit

            long delta = current_time - last_key_time;

            if (mode == TYPING && (delta > 10000)) { // user fell asleep while typing .)
                mode = INITIAL;
            } else if (mode == WRONG && (current_time > timeout)) { // end of timeout for wrong password
                mode = TYPING;
                last_key_time = timeout; // start 'idle' timer correctly by a fake keypress
            }

            visualFeedback(frame, mode);

            poll(NULL, 0, 25);
        }

    }

    // normally, we shouldnt arrive here at all
    alock_free_frame(frame);
    return 0;
}

/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/

static pid_t getPidAtom(struct aXInfo* xinfo) {

    Atom ret_type;
    int ret_fmt;
    unsigned long nr_read;
    unsigned long nr_bytes_left;
    pid_t* ret_data;

    if (XGetWindowProperty(xinfo->display, xinfo->root[0],
                xinfo->pid_atom, 0L, 1L, False, XA_CARDINAL,
                &ret_type, &ret_fmt, &nr_read, &nr_bytes_left,
                (unsigned char**)&ret_data) == Success && ret_type != None && ret_data) {
        pid_t pid = *ret_data;
        XFree(ret_data);
        return pid;
    }

    return 0;
}

static int detectOtherInstance(struct aXInfo* xinfo) {

    pid_t pid = getPidAtom(xinfo);
    int process_alive = kill(pid, 0);

    if (pid > 0 && process_alive == 0) {
        return 1;
    }

    if (process_alive) {
        perror("alock: info, found _ALOCK_PID");
    }

    return 0;
}

static int registerInstance(struct aXInfo* xinfo) {

    pid_t pid = getpid();
    XChangeProperty(xinfo->display, xinfo->root[0],
            xinfo->pid_atom, XA_CARDINAL,
            sizeof(pid_t) * 8, PropModeReplace,
            (unsigned char*)&pid, 1);
    return 1;
}

static int unregisterInstance(struct aXInfo* xinfo) {

    XDeleteProperty(xinfo->display, xinfo->root[0], xinfo->pid_atom);
    return 1;
}

/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/

int main(int argc, char **argv) {


    struct aXInfo xinfo;
    struct aOpts opts;

#if HAVE_XF86MISC
    int xf86misc_major = -1;
    int xf86misc_minor = -1;
#endif

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
                        printf("%s", "alock: error, couldnt find the bg-module you specified.\n");
                        exit(EXIT_FAILURE);
                    }

                } else {
                    printf("%s", "alock, error, missing argument\n");
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
                        printf("%s", "alock: error, couldnt find the auth-module you specified.\n");
                        exit(EXIT_FAILURE);
                    }

                } else {
                    printf("%s", "alock, error, missing argument\n");
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
                        printf("%s", "alock: error, couldnt find the cursor-module you specified.\n");
                        exit(EXIT_FAILURE);
                    }

                } else {
                    printf("%s", "alock, error, missing argument\n");
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


    initStartTime();
    initXInfo(&xinfo);
    if (detectOtherInstance(&xinfo)) {
        printf("%s", "alock: error, another instance seems to be running\n");
        exit(EXIT_FAILURE);
    }

    if (!opts.auth) {
        printf("%s", "alock: error, no auth-method specified.\n");
        displayUsage();
        exit(EXIT_FAILURE);
    }

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
            printf("%s", "alock: couldnt grab the keyboard.\n");
            exit(EXIT_FAILURE);
        }
    }

#if HAVE_XF86MISC
    {
        if (XF86MiscQueryVersion(xinfo.display, &xf86misc_major, &xf86misc_minor) == True) {

            if (xf86misc_major >= 0 &&
                xf86misc_minor >= 5 &&
                XF86MiscSetGrabKeysState(xinfo.display, False) == MiscExtGrabStateLocked) {

                printf("%s", "alock: cant disable xserver hotkeys to remove grabs.\n");
                exit(EXIT_FAILURE);
            }

            printf("%s", "disabled AllowDeactivateGrabs and AllowClosedownGrabs\n.");
        }
    }
#endif

    /* TODO: think about it: do we really need NR_SCREEN cursors ? we grab the
     * pointer on :*.0 anyway ... */
    if (XGrabPointer(xinfo.display, xinfo.window[0], False, None,
                     GrabModeAsync, GrabModeAsync, None, xinfo.cursor[0], CurrentTime) != GrabSuccess) {
        XUngrabKeyboard(xinfo.display, CurrentTime);
        printf("%s", "alock: couldnt grab the pointer.\n");
        exit(EXIT_FAILURE);
    }

    registerInstance(&xinfo);
    eventLoop(&opts, &xinfo);
    unregisterInstance(&xinfo);

    opts.auth->deinit();
    opts.cursor->deinit(&xinfo);
    opts.background->deinit(&xinfo);

#if HAVE_XF86MISC
    if (xf86misc_major >= 0 && xf86misc_minor >= 5) {
        XF86MiscSetGrabKeysState(xinfo.display, True);
        XFlush(xinfo.display);
    }
#endif

    XCloseDisplay(xinfo.display);

    return EXIT_SUCCESS;
}


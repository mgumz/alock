/*
 * alock - alock.c
 * Copyright (c) 2005 - 2007 Mathias Gumz <akira at fluxbox dot org>
 *               2014 Arkadiusz Bokowy
 *
 * This file is a part of an alock.
 *
 * This projected is licensed under the terms of the MIT license.
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <locale.h>
#include <ctype.h>
#include <wchar.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/Xos.h>

#ifdef HAVE_XF86MISC
#include <X11/extensions/xf86misc.h>
#endif

#include "alock.h"


static struct aAuth *alock_authmodules[] = {
#ifdef HAVE_PAM
    &alock_auth_pam,
#endif /* HAVE_PAM */
#ifdef HAVE_PASSWD
    &alock_auth_passwd,
#endif /* HAVE_PASSWD */
#ifdef HAVE_HASH
    &alock_auth_hash,
#endif /* HAVE_HASH */
    &alock_auth_none,
    NULL
};

static struct aInput *alock_inputs[] = {
    &alock_input_frame,
    &alock_input_none,
    NULL
};

static struct aBackground *alock_backgrounds[] = {
    &alock_bg_blank,
#ifdef HAVE_IMLIB2
    &alock_bg_image,
#endif /* HAVE_IMLIB2 */
#ifdef HAVE_XRENDER
    &alock_bg_shade,
#endif /* HAVE_XRENDER */
    &alock_bg_none,
    NULL
};

static struct aCursor *alock_cursors[] = {
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


static void initXInfo(struct aXInfo *xi) {

    Display *dpy = XOpenDisplay(NULL);
    int screens;

    if (!dpy) {
        perror("alock: error, can't open connection to X");
        exit(EXIT_FAILURE);
    }

    screens = ScreenCount(dpy);
    xi->display = dpy;
    xi->screens = screens;
    xi->pid_atom = XInternAtom(dpy, "_ALOCK_PID", False);
    xi->root = (Window*)malloc(sizeof(Window) * screens);
    xi->colormap = (Colormap*)malloc(sizeof(Colormap) * screens);
    xi->window = (Window*)malloc(sizeof(Window) * screens);
    xi->cursor = (Cursor*)malloc( sizeof(Cursor) * screens);
    xi->root_width = (int*)malloc(sizeof(int) * screens);
    xi->root_height = (int*)malloc(sizeof(int) * screens);

    {
        XWindowAttributes xgwa;
        int scr;
        for (scr = 0; scr < screens; scr++) {
            xi->root[scr] = RootWindow(dpy, scr);
            xi->colormap[scr] = DefaultColormap(dpy, scr);
            xi->window[scr] = None;
            xi->cursor[scr] = None;

            XGetWindowAttributes(dpy, xi->root[scr], &xgwa);
            xi->root_width[scr] = xgwa.width;
            xi->root_height[scr] = xgwa.height;
        }
    }
}

static void eventLoop(struct aOpts *opts, struct aXInfo *xi) {

    Display *dpy = xi->display;
    XEvent ev;
    KeySym ks;
    char cbuf[10];
    wchar_t pass[128];
    unsigned int clen;
    unsigned int pass_pos = 0, pass_len = 0;
    unsigned long keypress_time = 0;
    char rbuf[sizeof(pass)];

    debug("entering event main loop");
    for (;;) {

        if (keypress_time) {
            /* check for any key press event */
            if (XCheckWindowEvent(dpy, xi->window[0], KeyPressMask, &ev) == False) {

                /* user fell asleep while typing (5 seconds inactivity) */
                if (alock_mtime() - keypress_time > 5000) {
                    opts->input->setstate(AINPUT_STATE_NONE);
                    keypress_time = 0;
                }

                /* wait a bit */
                usleep(25000);
                continue;
            }
        }
        else {
            /* block until any key press event arrives */
            XWindowEvent(dpy, xi->window[0], KeyPressMask, &ev);
        }

        switch (ev.type) {
        case KeyPress:

            /* swallow up first key press to indicate "enter mode" */
            if (keypress_time == 0) {
                opts->input->setstate(AINPUT_STATE_INIT);
                keypress_time = alock_mtime();
                pass_pos = pass_len = 0;
                pass[0] = '\0';
                break;
            }

            keypress_time = alock_mtime();
            clen = XLookupString(&ev.xkey, cbuf, sizeof(cbuf), &ks, NULL);
            debug("key input: %lx, %d, `%s`", ks, clen, cbuf);

            /* translate key press symbol */
            ks = opts->input->keypress(ks);

            switch (ks) {
            case NoSymbol:
                break;

            /* clear/initialize input buffer */
            case XK_Escape:
            case XK_Clear:
                pass_pos = pass_len = 0;
                pass[0] = '\0';
                break;

            /* input position navigation */
            case XK_Begin:
            case XK_Home:
                pass_pos = 0;
                break;
            case XK_End:
                pass_pos = pass_len;
                break;
            case XK_Left:
                if (pass_pos > 0)
                    pass_pos--;
                break;
            case XK_Right:
                if (pass_pos < pass_len)
                    pass_pos++;
                break;

            /* remove entered characters */
            case XK_Delete:
                if (pass_pos < pass_len) {
                    wmemmove(&pass[pass_pos], &pass[pass_pos + 1], pass_len - pass_pos);
                    pass_len--;
                }
                break;
            case XK_BackSpace:
                if (pass_pos > 0) {
                    wmemmove(&pass[pass_pos - 1], &pass[pass_pos], pass_len - pass_pos + 1);
                    pass_pos--;
                    pass_len--;
                }
                break;

            /* input confirmation and authentication test */
            case XK_Linefeed:
            case XK_Return:
                opts->input->setstate(AINPUT_STATE_CHECK);
                wcstombs(rbuf, pass, sizeof(rbuf));
                if (opts->auth->auth(rbuf)) {
                    opts->input->setstate(AINPUT_STATE_VALID);
                    return;
                }
                opts->input->setstate(AINPUT_STATE_ERROR);
                opts->input->setstate(AINPUT_STATE_INIT);
                keypress_time = alock_mtime();
                pass_pos = pass_len = 0;
                pass[0] = '\0';
                break;

            /* input new character at the current input position */
            default:
                if (clen > 0 && !iscntrl(cbuf[0]) && pass_len < (sizeof(pass) / sizeof(*pass) - 1)) {
                    wmemmove(&pass[pass_pos + 1], &pass[pass_pos], pass_len - pass_pos + 1);
                    mbtowc(&pass[pass_pos], cbuf, clen);
                    pass_pos++;
                    pass_len++;
                }
                break;
            }

            debug("entered phrase [%zu]: `%ls`", wcslen(pass), pass);
            break;

        case Expose:
            XClearWindow(xi->display, ((XExposeEvent*)&ev)->window);
            break;
        }
    }
}

static pid_t getPidAtom(struct aXInfo *xinfo) {

    Atom ret_type;
    int ret_fmt;
    unsigned long nr_read;
    unsigned long nr_bytes_left;
    pid_t *ret_data;

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

static int detectOtherInstance(struct aXInfo *xinfo) {

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

static int registerInstance(struct aXInfo *xinfo) {

    pid_t pid = getpid();
    XChangeProperty(xinfo->display, xinfo->root[0],
            xinfo->pid_atom, XA_CARDINAL,
            sizeof(pid_t) * 8, PropModeReplace,
            (unsigned char*)&pid, 1);
    return 1;
}

static int unregisterInstance(struct aXInfo *xinfo) {

    XDeleteProperty(xinfo->display, xinfo->root[0], xinfo->pid_atom);
    return 1;
}

int main(int argc, char **argv) {

    struct aXInfo xinfo;
    struct aOpts opts = {
        alock_authmodules[0],
        alock_inputs[0],
        alock_cursors[0],
        alock_backgrounds[0],
    };

#if HAVE_XF86MISC
    int xf86misc_major = -1;
    int xf86misc_minor = -1;
#endif

    int arg;
    const char *optarg;
    const char *auth_args = NULL;
    const char *input_args = NULL;
    const char *cursor_args = NULL;
    const char *background_args = NULL;

    /* parse options */
    if (argc > 1) {
        for (arg = 1; arg < argc; arg++) {
            if (!strcmp(argv[arg], "-bg")) {
                optarg = argv[++arg];
                if (optarg != NULL) {

                    struct aBackground **i;
                    if (strcmp(optarg, "list") == 0) {
                        printf("list of available background modules:\n");
                        for (i = alock_backgrounds; *i; ++i) {
                            printf("%s\n", (*i)->name);
                        }
                        exit(EXIT_SUCCESS);
                    }

                    for (i = alock_backgrounds; *i; ++i) {
                        if (strstr(optarg, (*i)->name) == optarg) {
                            background_args = optarg;
                            opts.background = *i;
                            break;
                        }
                    }

                    if (*i == NULL) {
                        fprintf(stderr, "alock: background module not found\n");
                        exit(EXIT_FAILURE);
                    }

                }
                else {
                    fprintf(stderr, "alock: option requires an argument -- '%s'\n", "bg");
                    exit(EXIT_FAILURE);
                }
            }
            else if (!strcmp(argv[arg], "-auth")) {
                optarg = argv[++arg];
                if (optarg != NULL) {

                    struct aAuth **i;
                    if (strcmp(optarg, "list") == 0) {
                        printf("list of available authentication modules:\n");
                        for (i = alock_authmodules; *i; ++i) {
                            printf("%s\n", (*i)->name);
                        }
                        exit(EXIT_SUCCESS);
                    }

                    for (i = alock_authmodules; *i; ++i) {
                        if (strstr(optarg, (*i)->name) == optarg) {
                            auth_args = optarg;
                            opts.auth = *i;
                            break;
                        }
                    }

                    if (*i == NULL) {
                        fprintf(stderr, "alock: authentication module not found\n");
                        exit(EXIT_FAILURE);
                    }

                }
                else {
                    fprintf(stderr, "alock: option requires an argument -- '%s'\n", "auth");
                    exit(EXIT_FAILURE);
                }
            }
            else if (!strcmp(argv[arg], "-cursor")) {
                optarg = argv[++arg];
                if (optarg != NULL) {

                    struct aCursor **i;
                    if (strcmp(argv[arg], "list") == 0) {
                        printf("list of available cursor modules:\n");
                        for (i = alock_cursors; *i; ++i) {
                            printf("%s\n", (*i)->name);
                        }
                        exit(EXIT_SUCCESS);
                    }

                    for (i = alock_cursors; *i; ++i) {
                        if (strstr(optarg, (*i)->name) == optarg) {
                            cursor_args = optarg;
                            opts.cursor = *i;
                            break;
                        }
                    }

                    if (*i == NULL) {
                        fprintf(stderr, "alock: cursor module not found\n");
                        exit(EXIT_FAILURE);
                    }

                }
                else {
                    fprintf(stderr, "alock: option requires an argument -- '%s'\n", "cursor");
                    exit(EXIT_FAILURE);
                }
            }
            else if (!strcmp(argv[arg], "-input")) {
                optarg = argv[++arg];
                if (optarg != NULL) {

                    struct aInput **i;
                    if (strcmp(argv[arg], "list") == 0) {
                        printf("list of available input modules:\n");
                        for (i = alock_inputs; *i; ++i) {
                            printf("%s\n", (*i)->name);
                        }
                        exit(EXIT_SUCCESS);
                    }

                    for (i = alock_inputs; *i; ++i) {
                        if (strstr(optarg, (*i)->name) == optarg) {
                            input_args = optarg;
                            opts.input = *i;
                            break;
                        }
                    }

                    if (*i == NULL) {
                        fprintf(stderr, "alock: input module not found\n");
                        exit(EXIT_FAILURE);
                    }

                }
                else {
                    fprintf(stderr, "alock: option requires an argument -- '%s'\n", "input");
                    exit(EXIT_FAILURE);
                }
            }
            else if (strcmp(argv[arg], "-h") == 0) {
                printf("alock [-h] [-bg type:options] [-cursor type:options] "
                       "[-auth type:options] [-input type:options]\n");
                exit(EXIT_SUCCESS);
            }
            else {
                fprintf(stderr, "alock: invalid option '%s'\n", argv[arg]);
                exit(EXIT_FAILURE);
            }
        }
    }

    /* required for correct input handling */
    setlocale(LC_ALL, "");

    initXInfo(&xinfo);
    if (detectOtherInstance(&xinfo)) {
        fprintf(stderr, "alock: another instance seems to be running\n");
        exit(EXIT_FAILURE);
    }

    if (opts.auth->init(auth_args) == 0) {
        fprintf(stderr, "alock: failed init of [%s] with [%s]\n",
                opts.auth->name, auth_args);
        exit(EXIT_FAILURE);
    }

    /* We can be installed setuid root to support shadow passwords,
       and we don't need root privileges any longer.  --marekm */
    setuid(getuid());

    if (opts.input->init(input_args, &xinfo) == 0) {
        fprintf(stderr, "alock: failed init of [%s] with [%s]\n",
                opts.input->name, input_args);
        exit(EXIT_FAILURE);
    }
    if (opts.background->init(background_args, &xinfo) == 0) {
        fprintf(stderr, "alock: failed init of [%s] with [%s]\n",
                opts.background->name, background_args);
        exit(EXIT_FAILURE);
    }
    if (opts.cursor->init(cursor_args, &xinfo) == 0) {
        fprintf(stderr, "alock: failed init of [%s] with [%s]\n",
                opts.cursor->name, cursor_args);
        exit(EXIT_FAILURE);
    }

    {
        int scr;
        for (scr = 0; scr < xinfo.screens; scr++) {

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
            printf("alock: couldnt grab the keyboard\n");
            exit(EXIT_FAILURE);
        }
    }

#if HAVE_XF86MISC
    {
        if (XF86MiscQueryVersion(xinfo.display, &xf86misc_major, &xf86misc_minor) == True) {

            if (xf86misc_major >= 0 &&
                xf86misc_minor >= 5 &&
                XF86MiscSetGrabKeysState(xinfo.display, False) == MiscExtGrabStateLocked) {

                fprintf(stderr, "alock: cant disable xserver hotkeys to remove grabs\n");
                exit(EXIT_FAILURE);
            }

            printf("disabled AllowDeactivateGrabs and AllowClosedownGrabs\n");
        }
    }
#endif

    /* TODO: think about it: do we really need NR_SCREEN cursors ? we grab the
     * pointer on :*.0 anyway ... */
    if (XGrabPointer(xinfo.display, xinfo.window[0], False, None,
                     GrabModeAsync, GrabModeAsync, None, xinfo.cursor[0], CurrentTime) != GrabSuccess) {
        XUngrabKeyboard(xinfo.display, CurrentTime);
        fprintf(stderr, "alock: couldnt grab the pointer\n");
        exit(EXIT_FAILURE);
    }

    registerInstance(&xinfo);
    eventLoop(&opts, &xinfo);
    unregisterInstance(&xinfo);

    opts.auth->deinit();
    opts.input->deinit(&xinfo);
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

/*
 * alock - alock.c
 * Copyright (c) 2005 - 2007 Mathias Gumz <akira at fluxbox dot org>
 *               2014 - 2015 Arkadiusz Bokowy
 *
 * This file is a part of an alock.
 *
 * This projected is licensed under the terms of the MIT license.
 *
 */

#if HAVE_CONFIG_H
#include "../config.h"
#endif

#include <ctype.h>
#include <getopt.h>
#include <locale.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <sys/mman.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#if HAVE_X11_EXTENSIONS_XF86MISC_H
#include <X11/extensions/xf86misc.h>
#endif

#include "alock.h"


static struct aModuleAuth *alock_modules_auth[] = {
#if ENABLE_PAM
    &alock_auth_pam,
#endif
#if ENABLE_PASSWD
    &alock_auth_passwd,
#endif
#if ENABLE_HASH
    &alock_auth_hash,
#endif
    &alock_auth_none,
    NULL
};

static struct aModuleBackground *alock_modules_background[] = {
    &alock_bg_blank,
#if ENABLE_IMLIB2
    &alock_bg_image,
#endif
#if ENABLE_XRENDER
    &alock_bg_shade,
#endif
    &alock_bg_none,
    NULL
};

static struct aModuleCursor *alock_modules_cursor[] = {
    &alock_cursor_none,
    &alock_cursor_glyph,
#if ENABLE_XCURSOR
    &alock_cursor_xcursor,
#if (ENABLE_XRENDER && (ENABLE_XPM || ENABLE_IMLIB2))
    &alock_cursor_image,
#endif
#endif
    NULL
};

static struct aModuleInput *alock_modules_input[] = {
    &alock_input_frame,
    &alock_input_none,
    NULL
};


/* Update/initialize display info structure. */
static int updateDisplayInfo(struct aDisplayInfo *dinfo) {

    Display *dpy = dinfo->display;
    XWindowAttributes xwa;
    Window root;
    int i;

    dinfo->screen_nb = XScreenCount(dpy);
    dinfo->screens = (struct aScreenInfo *)realloc(dinfo->screens,
            sizeof(*dinfo->screens) * dinfo->screen_nb);

    for (i = 0; i < dinfo->screen_nb; i++) {
        root = XRootWindow(dpy, i);
        XGetWindowAttributes(dpy, root, &xwa);
        dinfo->screens[i].colormap = XDefaultColormap(dpy, i);
        dinfo->screens[i].root = root;
        dinfo->screens[i].width = xwa.width;
        dinfo->screens[i].height = xwa.height;
    }

    return dinfo->screen_nb;
}

/* Free resources allocated by the display_info_update function. */
static void freeDisplayInfo(struct aDisplayInfo *dinfo) {
    free(dinfo->screens);
}

/* Register alock instance. This function returns 0 on success or -1 when
 * another instance is already registered. Note, that this function does
 * not guarantee 100% assurance, it is NOT multi-process safe! */
static int registerInstance(struct aDisplayInfo *dinfo) {

    Atom atom = XInternAtom(dinfo->display, "ALOCK_INSTANCE_PID", False);
    pid_t pid = 0;

    { /* detect previous instance */

        Atom ret_type;
        int ret_fmt;
        unsigned long ret_nb;
        unsigned long ret_bleft;
        pid_t *ret_data;
        int rv;

        rv = XGetWindowProperty(dinfo->display, dinfo->screens[0].root, atom,
                0L, 1L, False, XA_CARDINAL, &ret_type, &ret_fmt,
                &ret_nb, &ret_bleft, (unsigned char **)&ret_data);
        if (rv == Success && ret_type != None && ret_data) {
            pid = *ret_data;
            XFree(ret_data);
        }

        if (pid && kill(pid, 0) == 0)
            /* atom was registered and the found process is alive */
            return -1;

    }

    debug("registering instance");
    pid = getpid();
    XChangeProperty(dinfo->display, dinfo->screens[0].root, atom, XA_CARDINAL,
            sizeof(pid_t) * 8, PropModeReplace, (unsigned char *)&pid, 1);
    XFlush(dinfo->display);

    return 0;
}

/* Unregister alock instance. */
static void unregisterInstance(struct aDisplayInfo *dinfo) {
    Atom atom = XInternAtom(dinfo->display, "ALOCK_INSTANCE_PID", True);
    if (atom != None) {
        debug("unregistering instance");
        XDeleteProperty(dinfo->display, dinfo->screens[0].root, atom);
    }
}

/* Lock current display and grab pointer and keyboard. On successful
 * lock this function returns 0, otherwise -1. */
static int lockDisplay(struct aDisplayInfo *dinfo, struct aModules *modules) {

    Display *dpy = dinfo->display;
    Window window;
    Cursor cursor;
    int i;

    /* select input event and raise background window */
    for (i = 0; i < dinfo->screen_nb; i++) {
        window = modules->background->getwindow(i);
        XSelectInput(dpy, window, KeyPressMask);
        XMapWindow(dpy, window);
        XRaiseWindow(dpy, window);
        /* receive notification about root window geometry change */
        XSelectInput(dpy, dinfo->screens[i].root, StructureNotifyMask);
    }

    /* grab pointer and keyboard from the first (default) screen */
    window = modules->background->getwindow(0);
    cursor = modules->cursor->getcursor();

    if (XGrabPointer(dpy, window, False, None, GrabModeAsync, GrabModeAsync, None,
                cursor, CurrentTime) != GrabSuccess) {
        fprintf(stderr, "error: grab pointer failed\n");
        return -1;
    }

    /* try to grab 2 times, another process (windowmanager) may have grabbed
     * the keyboard already */
    if (XGrabKeyboard(dpy, window, True, GrabModeAsync, GrabModeAsync,
                CurrentTime) != GrabSuccess) {
        sleep(1);
        if (XGrabKeyboard(dpy, window, True, GrabModeAsync, GrabModeAsync,
                    CurrentTime) != GrabSuccess) {
            fprintf(stderr, "error: grab keyboard failed\n");
            return -1;
        }
    }

    return 0;
}

static void eventLoop(struct aDisplayInfo *dinfo, struct aModules *modules) {

    Display *dpy = dinfo->display;
    XEvent ev;
    KeySym ks;
    char cbuf[10];
    wchar_t pass[128];
    unsigned int clen;
    unsigned int pass_pos = 0, pass_len = 0;
    unsigned long keypress_time = 0;

    /* if possible do not page this address to the swap area */
    mlock(pass, sizeof(pass));

    debug("entering event main loop");
    for (;;) {

        if (keypress_time) {
            /* check for any key press event (or root window state change) */
            if (XCheckMaskEvent(dpy, KeyPressMask | StructureNotifyMask, &ev) == False) {

                /* user fell asleep while typing (5 seconds inactivity) */
                if (alock_mtime() - keypress_time > 5000) {
                    modules->input->setstate(AINPUT_STATE_NONE);
                    keypress_time = 0;
                }

                /* wait a bit */
                usleep(25000);
                continue;
            }
        }
        else {
            /* block until any key press event arrives */
            XMaskEvent(dpy, KeyPressMask | StructureNotifyMask, &ev);
        }

        switch (ev.type) {
        case KeyPress:

            /* swallow up first key press to indicate "enter mode" */
            if (keypress_time == 0) {
                modules->input->setstate(AINPUT_STATE_INIT);
                keypress_time = alock_mtime();
                pass_pos = pass_len = 0;
                pass[0] = '\0';
                break;
            }

            keypress_time = alock_mtime();
            clen = XLookupString(&ev.xkey, cbuf, sizeof(cbuf), &ks, NULL);
            debug("key input: %lx, %d, `%s`", ks, clen, cbuf);

            /* translate key press symbol */
            ks = modules->input->keypress(ks);

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
                {
                    char rbuf[sizeof(pass)];
                    int rv;

                    modules->input->setstate(AINPUT_STATE_CHECK);

                    wcstombs(rbuf, pass, sizeof(rbuf));
                    rv = modules->auth->authenticate(rbuf);

                    memset(rbuf, 0, sizeof(rbuf));
                    memset(pass, 0, sizeof(pass));
                    pass_pos = pass_len = 0;

                    if (rv == 0) { /* successful authentication */
                        modules->input->setstate(AINPUT_STATE_VALID);
                        return;
                    }

                    modules->input->setstate(AINPUT_STATE_ERROR);
                    modules->input->setstate(AINPUT_STATE_INIT);
                    keypress_time = alock_mtime();
                }
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

        case ConfigureNotify:
            /* NOTE: This event should be generated for the root window upon
             *       the display reconfiguration (e.g. resolution change). */

            debug("received configure notify event");
            break;

#if 0
        case Expose:
            XClearWindow(dpy, ((XExposeEvent*)&ev)->window);
            break;
#endif

        }
    }
}

int main(int argc, char **argv) {

    int opt;
    struct option longopts[] = {
        {"help", no_argument, NULL, 'h'},
        {"modules", no_argument, NULL, 'm'},
        {"auth", required_argument, NULL, 'a'},
        {"bg", required_argument, NULL, 'b'},
        {"cursor", required_argument, NULL, 'c'},
        {"input", required_argument, NULL, 'i'},
        {0, 0, 0, 0},
    };

    struct aDisplayInfo dinfo;
    struct aModules modules;
    int retval;

    const char *args_auth = NULL;
    const char *args_background = NULL;
    const char *args_cursor = NULL;
    const char *args_input = NULL;

    /* set-up default modules */
    modules.auth = alock_modules_auth[0];
    modules.background = alock_modules_background[0];
    modules.cursor = alock_modules_cursor[0];
    modules.input = alock_modules_input[0];

    /* parse options */
    while ((opt = getopt_long_only(argc, argv, "hma:b:c:i:", longopts, NULL)) != -1)
        switch (opt) {
        case 'h':
            printf("%s [-help] [-modules] [-auth type:options] [-bg type:options]"
                    " [-cursor type:options] [-input type:options]\n", argv[0]);
            return EXIT_SUCCESS;

        case 'm':
            { /* list available modules */

                struct aModuleAuth **ia;
                struct aModuleBackground **ib;
                struct aModuleCursor **ic;
                struct aModuleInput **ii;

                printf("authentication modules:\n");
                for (ia = alock_modules_auth; *ia; ++ia)
                    printf("  %s\n", (*ia)->m.name);

                printf("background modules:\n");
                for (ib = alock_modules_background; *ib; ++ib)
                    printf("  %s\n", (*ib)->m.name);

                printf("cursor modules:\n");
                for (ic = alock_modules_cursor; *ic; ++ic)
                    printf("  %s\n", (*ic)->m.name);

                printf("input modules:\n");
                for (ii = alock_modules_input; *ii; ++ii)
                    printf("  %s\n", (*ii)->m.name);

            }
            return EXIT_SUCCESS;

        case 'a':
            { /* authentication module */

                struct aModuleAuth **i;
                for (i = alock_modules_auth; *i; ++i)
                    if (strstr(optarg, (*i)->m.name) == optarg) {
                        args_auth = optarg;
                        modules.auth = *i;
                        break;
                    }

                if (*i == NULL) {
                    fprintf(stderr, "alock: authentication module `%s` not found\n", optarg);
                    return EXIT_FAILURE;
                }
            }
            break;

        case 'b':
            { /* background module */

                struct aModuleBackground **i;
                for (i = alock_modules_background; *i; ++i)
                    if (strstr(optarg, (*i)->m.name) == optarg) {
                        args_background = optarg;
                        modules.background = *i;
                        break;
                    }

                if (*i == NULL) {
                    fprintf(stderr, "alock: background module `%s` not found\n", optarg);
                    return EXIT_FAILURE;
                }
            }
            break;

        case 'c':
            { /* cursor module */

                struct aModuleCursor **i;
                for (i = alock_modules_cursor; *i; ++i)
                    if (strstr(optarg, (*i)->m.name) == optarg) {
                        args_cursor = optarg;
                        modules.cursor = *i;
                        break;
                    }

                if (*i == NULL) {
                    fprintf(stderr, "alock: cursor module `%s` not found\n", optarg);
                    return EXIT_FAILURE;
                }
            }
            break;

        case 'i':
            { /* input module */

                struct aModuleInput **i;
                for (i = alock_modules_input; *i; ++i)
                    if (strstr(optarg, (*i)->m.name) == optarg) {
                        args_input = optarg;
                        modules.input = *i;
                        break;
                    }

                if (*i == NULL) {
                    fprintf(stderr, "alock: input module `%s` not found\n", optarg);
                    return EXIT_FAILURE;
                }
            }
            break;

        default:
            fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
            return EXIT_FAILURE;
        }

    /* required for correct input handling */
    setlocale(LC_ALL, "");

    dinfo.display = XOpenDisplay(NULL);
    dinfo.screens = NULL;
    if (dinfo.display == NULL) {
        fprintf(stderr, "error: unable to connect to the X display\n");
        return EXIT_FAILURE;
    }

    if (!updateDisplayInfo(&dinfo)) {
        fprintf(stderr, "error: display initialization failed\n");
        XCloseDisplay(dinfo.display);
        return EXIT_FAILURE;
    }

    /* make sure, that only one instance of alock is running */
    if (registerInstance(&dinfo)) {
        fprintf(stderr, "error: another instance seems to be running\n");
        freeDisplayInfo(&dinfo);
        XCloseDisplay(dinfo.display);
        return EXIT_FAILURE;
    }

#if WITH_DUNST
    /* pause notification daemon */
    system("pkill -x -SIGUSR1 dunst");
#endif

    { /* try to initialize selected modules */

        int rv = 0;

        XrmInitialize();
        XrmDatabase xrdb = XrmGetStringDatabase(XResourceManagerString(dinfo.display));

        modules.auth->m.loadxrdb(xrdb);
        modules.background->m.loadxrdb(xrdb);
        modules.cursor->m.loadxrdb(xrdb);
        modules.input->m.loadxrdb(xrdb);

        XrmDestroyDatabase(xrdb);

        modules.auth->m.loadargs(args_auth);
        modules.background->m.loadargs(args_background);
        modules.cursor->m.loadargs(args_cursor);
        modules.input->m.loadargs(args_input);

        if (modules.auth->m.init(&dinfo)) {
            fprintf(stderr, "alock: failed init of [%s] with [%s]\n",
                    modules.auth->m.name, args_auth);
            rv |= 1;
        }

#if ENABLE_PASSWD
        /* We can be installed setuid root to support shadow passwords,
         * and we don't need root privileges any longer.  --marekm */
        if (setuid(getuid()) != 0)
            perror("alock: root privilege drop failed");
#endif

        if (modules.background->m.init(&dinfo)) {
            fprintf(stderr, "alock: failed init of [%s] with [%s]\n",
                    modules.background->m.name, args_background);
            rv |= 1;
        }
        if (modules.cursor->m.init(&dinfo)) {
            fprintf(stderr, "alock: failed init of [%s] with [%s]\n",
                    modules.cursor->m.name, args_cursor);
            rv |= 1;
        }
        if (modules.input->m.init(&dinfo)) {
            fprintf(stderr, "alock: failed init of [%s] with [%s]\n",
                    modules.input->m.name, args_input);
            rv |= 1;
        }

        if (rv) /* initialization failed */
            goto return_failure;

    }

#if HAVE_X11_EXTENSIONS_XF86MISC_H
    int xf86misc_major = -1;
    int xf86misc_minor = -1;

    if (XF86MiscQueryVersion(dinfo->display, &xf86misc_major, &xf86misc_minor) == True) {

        if (xf86misc_major >= 0 && xf86misc_minor >= 5 &&
                XF86MiscSetGrabKeysState(dinfo->display, False) == MiscExtGrabStateLocked) {

            fprintf(stderr, "error: unable to disable Xorg hotkeys to remove grabs\n");
            goto return_failure;
        }

        fprintf(stderr, "info: disabled AllowDeactivateGrabs and AllowClosedownGrabs\n");
    }
#endif

    /* raise our background window and grab input, if this action has failed,
     * we are not able to lock the screen, then we're fucked... */
    if (lockDisplay(&dinfo, &modules))
        goto return_failure;

    debug("entering main event loop");
    eventLoop(&dinfo, &modules);

    retval = EXIT_SUCCESS;
    goto return_success;

return_failure:
    retval = EXIT_FAILURE;

return_success:

#if HAVE_X11_EXTENSIONS_XF86MISC_H
    if (xf86misc_major >= 0 && xf86misc_minor >= 5) {
        XF86MiscSetGrabKeysState(dinfo->display, True);
        XFlush(dinfo->display);
    }
#endif

    modules.auth->m.free();
    modules.background->m.free();
    modules.cursor->m.free();
    modules.input->m.free();

    unregisterInstance(&dinfo);
    freeDisplayInfo(&dinfo);
    XCloseDisplay(dinfo.display);

#if WITH_DUNST
    /* resume notification daemon */
    system("pkill -x -SIGUSR2 dunst");
#endif

    return retval;
}

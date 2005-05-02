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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>

#ifdef SHADOW_PWD
#    include <shadow.h>
#endif /* SHADOW_PWD */

#ifdef PAM_PWD
#    include <security/pam_appl.h>
#    ifdef LINUX
#        include <security/pam_misc.h>
#    endif
#endif /* PAM_PWD */

/*----------------------------------------------*\
\*----------------------------------------------*/

#include "aklock.h"

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

struct passwd *pw;



/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/

void displayUsage() {

    printf("\naklock [-hv] [-blank]\n\n"
"    -h      shows this little help\n"
"    -v      shows the version number\n"
"    -blank  hides the content of the screen\n"
"    -cursor theme for the cursor\n"
"\n"
"when the screen is locked, just enter your password.\n");
}





/*------------------------------------------------------------------*\
    pam-related stuff
    
    taken from pure-ftpd's authstuff, but you can see similar stuff
    in xlockmore, openssh and basicly all pam-related apps :)
\*------------------------------------------------------------------*/
#ifdef PAM_PWD 
#    define PAM_YN { \
                if (PAM_error != 0 || pam_error != PAM_SUCCESS) { \
                    fprintf(stderr, "pam error:%s\n", pam_strerror(pam_handle, pam_error)); \
                    pam_end(pam_handle, pam_error); \
                    PAM_username = NULL; \
                    PAM_password = NULL; \
                return 0;\
            } \
     }

#   define GET_MEM \
       size += sizeof(struct pam_response); \
       if ((reply = realloc(reply, size)) == NULL) { \
           PAM_error = 1; \
           return PAM_CONV_ERR; \
       }

static const char* PAM_username = NULL;
static const char* PAM_password = NULL;
static int PAM_error = 0;
static int pam_error = PAM_SUCCESS;

static int PAM_conv(int num_msg, const struct pam_message **msgs,
	              struct pam_response **resp, void *appdata_ptr) {

    int count = 0;
    unsigned int replies = 0U;
    struct pam_response *reply = NULL;
    size_t size = (size_t) 0U;

    (void) appdata_ptr;
    *resp = NULL;
    for (count = 0; count < num_msg; count++) {
        switch (msgs[count]->msg_style) {
        case PAM_PROMPT_ECHO_ON:
            GET_MEM;
            memset(&reply[replies], 0, sizeof reply[replies]);
            if ((reply[replies].resp = strdup(PAM_username)) == NULL) {
#    ifdef PAM_BUF_ERR
                reply[replies].resp_retcode = PAM_BUF_ERR;
#    endif
                PAM_error = 1;
                return PAM_CONV_ERR;
            }
            reply[replies++].resp_retcode = PAM_SUCCESS;
            /* PAM frees resp */
            break;
        case PAM_PROMPT_ECHO_OFF:
            GET_MEM;
            memset(&reply[replies], 0, sizeof reply[replies]);
            if ((reply[replies].resp = strdup(PAM_password)) == NULL) {
#    ifdef PAM_BUF_ERR
                reply[replies].resp_retcode = PAM_BUF_ERR;
#    endif
                PAM_error = 1;
                return PAM_CONV_ERR;
            }
            reply[replies++].resp_retcode = PAM_SUCCESS;
            /* PAM frees resp */
            break;
        case PAM_TEXT_INFO:
            /* ignore it... */
            break;
        case PAM_ERROR_MSG:
        default:
            /* Must be an error of some sort... */
            free(reply);
            PAM_error = 1;
            return PAM_CONV_ERR;
        }
    }
    *resp = reply;
    return PAM_SUCCESS;
}

static struct pam_conv PAM_conversation = {
    &PAM_conv, NULL
};
#endif /* PAM_PWD */ 
/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/

int passwordOk(const char *s) {
#if 0
    char key[3];
    char *encr;

    key[0] = *(pw->pw_passwd);
    key[1] = (pw->pw_passwd)[1];
    key[2] = 0;
    encr = crypt(s, key);
    return !strcmp(encr, pw->pw_passwd);
#else
    /* simpler, and should work with crypt() algorithms using longer
       salt strings (like the md5-based one on freebsd).  --marekm */
#ifdef PAM_PWD
    pam_handle_t* pam_handle = NULL;
    PAM_username = pw->pw_name;
    PAM_password = s;
    pam_error = pam_start("login", PAM_username, &PAM_conversation, &pam_handle);
    PAM_YN;
    pam_error = pam_authenticate(pam_handle, 0);
    PAM_YN;
    pam_error = pam_end(pam_handle, pam_error);
    PAM_YN;
    return 1;
#else
    return !strcmp(crypt(s, pw->pw_passwd), pw->pw_passwd);
#endif /* PAM_PWD */
#endif /* 0 */
}

/*------------------------------------------------------------------*\
    check if the system would be able to authentificate the user 
\*------------------------------------------------------------------*/
void checkAuth() {

#ifdef SHADOW_PWD
    struct spwd *sp;
#endif

    errno = 0;
    pw = getpwuid(getuid());
    if (!pw) {
        perror("password entry for uid not found");
        exit(1);
    }

#ifdef SHADOW_PWD
    sp = getspnam(pw->pw_name);
    if (sp)
        pw->pw_passwd = sp->sp_pwdp;
    endspent();
#endif
    /* we can be installed setuid root to support shadow passwords,
       and we don't need root privileges any longer.  --marekm */
    setuid(getuid());
#ifndef PAM_PWD
    if (strlen(pw->pw_passwd) < 13) {
        perror("aklock: password entry has no pwd\n");
        exit(1);
    }
#endif /* PAM_PWD */

}



/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/


/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/

void initOpts(struct akOpts* opts) {
    
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
    Colormap color_map;
    Pixmap pixmap_cursor;
    Pixmap pixmap_cursor_mask;
    XWindowAttributes xgwa;
    struct akCursor* cursor;
    
    if (!dpy) {
        perror("aklock: error, can't open connection to X");
        exit(1);
    }

    xinfo->display = dpy;
    xinfo->window = 0;

    xinfo->root = DefaultRootWindow(dpy);
    color_map = DefaultColormap(dpy, DefaultScreen(dpy));
    
    XGetWindowAttributes(dpy, xinfo->root, &xgwa);

    xinfo->width = xgwa.width;
    xinfo->height = xgwa.height;
    
    for(cursor = ak_cursors; cursor->name != NULL; cursor++) {
        if (!strcmp(cursor->name, opts->cursor_name))
            break;
    }

    if (cursor == NULL)
        cursor = ak_cursors;
    
    pixmap_cursor = XCreateBitmapFromData(dpy, xinfo->root, cursor->bits, cursor->width, cursor->height);
    pixmap_cursor_mask = XCreateBitmapFromData(dpy, xinfo->root, cursor->mask, cursor->width, cursor->height);
    
    if((XAllocNamedColor(dpy, color_map, opts->color_bg, &tmp_color, &color_bg)) == 0)
        XAllocNamedColor(dpy, color_map, "black", &tmp_color, &color_bg);
    if((XAllocNamedColor(dpy, color_map, opts->color_fg, &tmp_color, &color_fg)) == 0)
        XAllocNamedColor(dpy, color_map, "white", &tmp_color, &color_fg);

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

    // parse options
    if (argc != 1) {
        for(arg = 1; arg <= argc; arg++) {
            if (!strcmp(argv[arg - 1], "-blank")) {
                opts.use_blank = 1;
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

    checkAuth();

    initXInfo(&xinfo, &opts);
    

    /* create the windows */
    xswa.override_redirect = True;
    xsmask |= CWOverrideRedirect;
    
    if (opts.use_blank) {

        xswa.background_pixel = BlackPixel(xinfo.display, DefaultScreen(xinfo.display));
        xsmask |= CWBackPixel;

        xinfo.window = XCreateWindow(xinfo.display, xinfo.root,
                              0, 0, xinfo.width, xinfo.height, 
                              0, /* borderwidth */
                              CopyFromParent, /* depth */
                              InputOutput, /* class */
                              CopyFromParent, /* visual */
                              xsmask, &xswa);
    } else {

        xinfo.window = XCreateWindow(xinfo.display, xinfo.root,
                              0, 0, 1, 1, 
                              0, /* borderwidth */
                              CopyFromParent, /* depth */
                              InputOnly, /* class */
                              CopyFromParent, /* visual */
                              xsmask, &xswa);
    }

    XSelectInput(xinfo.display, xinfo.window, KeyPressMask|KeyReleaseMask);
    XMapWindow(xinfo.display, xinfo.window);
    XRaiseWindow(xinfo.display, xinfo.window);
 
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
                if (passwordOk(rbuf))
                    goto exit;
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
    XDestroyWindow(xinfo.display, xinfo.window);
    XCloseDisplay(xinfo.display);

    return 0;
}


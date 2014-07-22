/*
 * alock - alock.h
 * Copyright (c) 2005 - 2007 Mathias Gumz <akira at fluxbox dot org>
 *               2014 Arkadiusz Bokowy
 *
 * This file is a part of an alock.
 *
 * This projected is licensed under the terms of the MIT license.
 *
 */

#ifndef __ALOCK_H
#define __ALOCK_H

#include <stdio.h>
#include <X11/Xlib.h>


#if DEBUG
#define debug(M, ...) fprintf(stderr, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define debug(M, ...)
#endif


struct aXInfo {
    Display *display;
    Atom pid_atom;

    int screens;
    Window *root;
    Colormap *colormap;
    Window *window;
    Cursor *cursor;
    int *root_width;
    int *root_height;
};

struct aAuth {
    const char *name;
    int (*init)(const char *args);
    int (*deinit)();
    int (*auth)(const char *pass);
};

enum aInputState {
    AINPUT_STATE_NONE,
    AINPUT_STATE_INIT,
    AINPUT_STATE_CHECK,
    AINPUT_STATE_VALID,
    AINPUT_STATE_ERROR,
};
struct aInput {
    const char *name;
    int (*init)(const char *args, struct aXInfo *xinfo);
    int (*deinit)(struct aXInfo *xinfo);
    void (*setstate)(enum aInputState state);
    KeySym (*keypress)(KeySym ks);
};

struct aCursor {
    const char *name;
    int (*init)(const char *args, struct aXInfo *xinfo);
    int (*deinit)(struct aXInfo *xinfo);
};

struct aBackground {
    const char *name;
    int (*init)(const char *args, struct aXInfo *xinfo);
    int (*deinit)(struct aXInfo *xinfo);
};

struct aOpts {
    struct aAuth *auth;
    struct aInput *input;
    struct aCursor *cursor;
    struct aBackground *background;
};


/* authentication modules */
extern struct aAuth alock_auth_none;
#if ENABLE_HASH
extern struct aAuth alock_auth_hash;
#endif
#if ENABLE_PASSWD
extern struct aAuth alock_auth_passwd;
#endif
#if ENABLE_PAM
extern struct aAuth alock_auth_pam;
#endif

/* input modules */
extern struct aInput alock_input_none;
extern struct aInput alock_input_frame;

/* background modules */
extern struct aBackground alock_bg_none;
extern struct aBackground alock_bg_blank;
#if ENABLE_IMLIB2
extern struct aBackground alock_bg_image;
#endif
#if ENABLE_XRENDER
extern struct aBackground alock_bg_shade;
#endif

/* cursor modules */
extern struct aCursor alock_cursor_none;
extern struct aCursor alock_cursor_glyph;
extern struct aCursor alock_cursor_theme;
#if ENABLE_XCURSOR
extern struct aCursor alock_cursor_xcursor;
#if (ENABLE_XRENDER && (ENABLE_XPM || ENABLE_IMLIB2))
extern struct aCursor alock_cursor_image;
#endif
#endif


unsigned long alock_mtime(void);
int alock_native_byte_order(void);
int alock_alloc_color(const struct aXInfo *xinfo,
        int scr,
        const char *color_name,
        const char *fallback_name,
        XColor *result);
int alock_check_xrender(const struct aXInfo *xinfo);
int alock_shade_pixmap(const struct aXInfo *xinfo,
        int scr,
        const Pixmap src_pm,
        Pixmap dst_pm,
        unsigned char shade,
        int src_x, int src_y,
        int dst_x, int dst_y,
        unsigned int width,
        unsigned int height);

#endif /* __ALOCK_H */

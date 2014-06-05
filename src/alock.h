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

#ifdef DEBUG
#define debug(M, ...) fprintf(stderr, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define debug(M, ...)
#endif /* DEBUG */


struct aXInfo {

    Display*  display;

    Atom      pid_atom;

    int       nr_screens;

    int*      width_of_root;
    int*      height_of_root;

    Window*   root;
    Colormap* colormap;

    Window*   window;
    Cursor*   cursor;
};

struct aAuth {
    const char* name;
    int (*init)(const char* args);
    int (*auth)(const char* pass);
    int (*deinit)();
};

enum aInputState {
    AINPUT_STATE_NONE,
    AINPUT_STATE_INIT,
    AINPUT_STATE_CHECK,
    AINPUT_STATE_VALID,
    AINPUT_STATE_ERROR,
};
struct aInput {
    const char* name;
    int (*init)(const char* args, struct aXInfo* xinfo);
    int (*deinit)(struct aXInfo* xinfo);
    void (*setstate)(enum aInputState state);
    KeySym (*keypress)(KeySym ks);
};

struct aCursor {
    const char* name;
    int (*init)(const char* args, struct aXInfo* xinfo);
    int (*deinit)(struct aXInfo* xinfo);
};

struct aBackground {
    const char* name;
    int (*init)(const char* args, struct aXInfo* xinfo);
    int (*deinit)(struct aXInfo* xinfo);
};

struct aOpts {
    struct aAuth* auth;
    struct aInput* input;
    struct aCursor* cursor;
    struct aBackground* background;
};


void alock_string2lower(char* string);
unsigned long alock_mtime(void);
int alock_native_byte_order(void);
int alock_alloc_color(const struct aXInfo* xinfo, int scr,
        const char* color_name,
        const char* fallback_name,
        XColor* result);
int alock_check_xrender(const struct aXInfo* xinfo);
int alock_shade_pixmap(const struct aXInfo* xinfo,
        int scr,
        const Pixmap src_pm,
        Pixmap dst_pm,
        unsigned char shade,
        int src_x, int src_y,
        int dst_x, int dst_y,
        unsigned int width,
        unsigned int height);

#endif /* __ALOCK_H */

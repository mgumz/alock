/*
 * alock - cursor_none.c
 * Copyright (c) 2005 - 2007 Mathias Gumz <akira at fluxbox dot org>
 *
 * This file is a part of an alock.
 *
 * This projected is licensed under the terms of the MIT license.
 *
 * This cursor module provides:
 *  -cursor none
 *
 */

#include "alock.h"


static int alock_cursor_none_init(const char *args, struct aXInfo *xinfo) {
    return 1;
}

static int alock_cursor_none_deinit(struct aXInfo *xinfo) {
    return 1;
}


struct aCursor alock_cursor_none = {
    "none",
    alock_cursor_none_init,
    alock_cursor_none_deinit,
};

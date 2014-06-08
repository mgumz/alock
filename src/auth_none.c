/*
 * alock - auth_none.c
 * Copyright (c) 2005 - 2007 Mathias Gumz <akira at fluxbox dot org>
 *
 * This file is a part of an alock.
 *
 * This projected is licensed under the terms of the MIT license.
 *
 */

#include "alock.h"


static int alock_auth_none_init(const char *args) {
    return 1;
}

static int alock_auth_none_deinit() {
    return 1;
}

static int alock_auth_none_auth(const char *pass) {
    return 1;
}

struct aAuth alock_auth_none = {
    "none",
    alock_auth_none_init,
    alock_auth_none_deinit,
    alock_auth_none_auth,
};

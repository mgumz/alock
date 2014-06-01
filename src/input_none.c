/*
 * alock - input_none.c
 * Copyright (c) 2014 Arkadiusz Bokowy
 *
 * This file is a part of an alock.
 *
 * This projected is licensed under the terms of the MIT license.
 *
 */

#include "alock.h"


static int alock_input_none_init(const char* args, struct aXInfo* xinfo) {
    debug("init: %s", args);
    return 1;
}

static int alock_input_none_deinit(struct aXInfo* xinfo) {
    debug("deinit");
    return 1;
}

static void alock_input_none_setstate(enum aInputState state) {
    debug("setstate: %d", state);
}

static void alock_input_none_keypress(char chr) {
    debug("keypress: %c", chr);
}


struct aInput alock_input_none = {
    "none",
    alock_input_none_init,
    alock_input_none_deinit,
    alock_input_none_setstate,
    alock_input_none_keypress
};

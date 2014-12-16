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


static void alock_input_none_setstate(enum aInputState state) {
    debug("setstate: %d", state);
}

static KeySym alock_input_none_keypress(KeySym ks) {
    debug("keypress: %lx", ks);
    return ks;
}


struct aInput alock_input_none = {
    "none",
    module_dummy_init,
    module_dummy_deinit,
    alock_input_none_setstate,
    alock_input_none_keypress,
};

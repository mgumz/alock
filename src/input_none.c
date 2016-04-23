/*
 * alock - input_none.c
 * Copyright (c) 2014 Arkadiusz Bokowy
 *
 * This file is a part of an alock.
 *
 * This project is licensed under the terms of the MIT license.
 *
 * This input module provides:
 *  -input none
 *
 */

#include "alock.h"


static Window module_getwindow(int screen) {
    (void)screen;
    return None;
}

static void module_setstate(enum aInputState state) {
    (void)state;
    debug("setstate: %d", state);
}

static KeySym module_keypress(KeySym key) {
    debug("keypress: %lx", key);
    return key;
}


struct aModuleInput alock_input_none = {
    {  "none",
        module_dummy_loadargs,
        module_dummy_loadxrdb,
        module_dummy_init,
        module_dummy_free,
    },
    module_getwindow,
    module_keypress,
    module_setstate,
};

/*
 * alock - bg_none.c
 * Copyright (c) 2005 - 2007 Mathias Gumz <akira at fluxbox dot org>
 *               2014 - 2016 Arkadiusz Bokowy
 *
 * This file is a part of an alock.
 *
 * This projected is licensed under the terms of the MIT license.
 *
 * This background module provides:
 *  -bg none
 *
 */

#include "alock.h"


static Window module_getwindow(int screen) {
    (void)screen;
    return None;
}


struct aModuleBackground alock_bg_none = {
    { "none",
        module_dummy_loadargs,
        module_dummy_loadxrdb,
        module_dummy_init,
        module_dummy_free,
    },
    module_getwindow,
};

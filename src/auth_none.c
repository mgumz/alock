/*
 * alock - auth_none.c
 * Copyright (c) 2005 - 2007 Mathias Gumz <akira at fluxbox dot org>
 *               2014 Arkadiusz Bokowy
 *
 * This file is a part of an alock.
 *
 * This projected is licensed under the terms of the MIT license.
 *
 */

#include "alock.h"


static int module_authenticate(const char *pass) {
    (void)pass;
    return 0;
}


struct aModuleAuth alock_auth_none = {
    { "none",
        module_dummy_loadargs,
        module_dummy_init,
        module_dummy_free,
    },
    module_authenticate,
};

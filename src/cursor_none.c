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


struct aCursor alock_cursor_none = {
    "none",
    module_dummy_init,
    module_dummy_deinit,
};

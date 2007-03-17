/* ---------------------------------------------------------------- *\

  file    : cursor_none.c
  author  : m. gumz <akira at fluxbox dot org>
  copyr   : copyright (c) 2005 - 2007 by m. gumz

  license : see LICENSE

  start   : Di 17 Mai 2005 12:10:35 CEST

\* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- *\

  about :

    provide -cursor none

\* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- *\
  includes
\* ---------------------------------------------------------------- */

#include "alock.h"

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */

static int alock_cursor_none_init(const char* args, struct aXInfo* xinfo) {

    int scr;
    for (scr = 0; scr < xinfo->nr_screens; scr++) {
        xinfo->cursor[scr] = None;
    }
    return 1;
}

static int alock_cursor_none_deinit(struct aXInfo* xinfo) {
    return 1;
}

struct aCursor alock_cursor_none = {
    "none",
    alock_cursor_none_init,
    alock_cursor_none_deinit
};



/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */



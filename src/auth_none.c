/* ---------------------------------------------------------------- *\

  file    : auth_none.c
  author  : m. gumz <akira at fluxbox dot org>
  copyr   : copyright (c) 2005 - 2007 by m. gumz

  license : see LICENSE

  start   : Sa 07 Mai 2005 16:41:28 CEST

\* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- *\

  about :

    provide -auth none, any "password" is accepted

\* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- *\
  includes
\* ---------------------------------------------------------------- */
#include "alock.h"

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */

static int alock_auth_none_init(const char* args) {
    return 1;
}

static int alock_auth_none_deinit() {
    return 1;
}

static int alock_auth_none_auth(const char* passwd) {
    return 1;
}

struct aAuth alock_auth_none = {
    "none",
    alock_auth_none_init,
    alock_auth_none_auth,
    alock_auth_none_deinit
};

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */


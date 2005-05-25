/* ---------------------------------------------------------------- *\

  file    : auth_passwd.c
  author  : m. gumz <akira at fluxbox dot org>
  copyr   : copyright (c) 2005 by m. gumz

  license : see LICENSE
  
  start   : Sa 07 Mai 2005 16:40:01 CEST

  $Id$

\* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- *\

  about :

    provide -auth passwd, normal unix-login, either with or without
    shadowsupport

\* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- *\
  includes
\* ---------------------------------------------------------------- */
#include <X11/Xlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <crypt.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#ifdef SHADOW_PWD
#    include <shadow.h>
#endif /* SHADOW_PWD */

#include "alock.h"

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */

static struct passwd* pwd_entry = NULL;

static int alock_auth_passwd_init(const char* args) {

#ifdef SHADOW_PWD
    struct spwd* sp = NULL;
#endif

    errno = 0;
    pwd_entry = getpwuid(getuid());
    if (!pwd_entry) {
        perror("password entry for uid not found");
        return 0;
    }

#ifdef SHADOW_PWD
    sp = getspnam(pwd_entry->pw_name);
    if (sp)
        pwd_entry->pw_passwd = sp->sp_pwdp;
    endspent();
#endif

    /* we can be installed setuid root to support shadow passwords,
       and we don't need root privileges any longer.  --marekm */
    setuid(getuid());

    if (strlen(pwd_entry->pw_passwd) < 13) {
        perror("password entry has no pwd\n");
        return 0;
    }
    
    return 1;
}

static int alock_auth_passwd_deinit() { 
    return 1;
}

static int alock_auth_passwd_auth(const char* pass) {
#if 0
    char key[3];
    char *encr;

    if (!pass || !pwd_entry)
        return 0;

    key[0] = *(pwd_entry->pw_passwd);
    key[1] = (pwd_entry->pw_passwd)[1];
    key[2] = 0;
    encr = crypt(pass, key);
    return !strcmp(encr, pw->pw_passwd);
#else
    if (!pass || !pwd_entry)
        return 0;

    /* simpler, and should work with crypt() algorithms using longer
       salt strings (like the md5-based one on freebsd).  --marekm */
    return !strcmp(crypt(pass, pwd_entry->pw_passwd), pwd_entry->pw_passwd);
#endif /* 0 */
}

struct aAuth alock_auth_passwd = {
    "passwd",
    alock_auth_passwd_init,
    alock_auth_passwd_auth,
    alock_auth_passwd_deinit
};

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */



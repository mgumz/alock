/*
 * alock - auth_passwd.c
 * Copyright (c) 2005 - 2007 Mathias Gumz <akira at fluxbox dot org>
 *
 * This file is a part of an alock.
 *
 * This projected is licensed under the terms of the MIT license.
 *
 * This authentication module provides:
 *  -auth passwd
 *
 */

#define _XOPEN_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#ifdef __linux
#ifdef HAVE_SHADOW
#include <shadow.h>
#endif /* HAVE_SHADOW */
#endif /* __linux */

#include "alock.h"


static struct passwd *pwd_entry = NULL;


static int alock_auth_passwd_init(const char *args) {

    errno = 0;
    pwd_entry = getpwuid(getuid());
    if (!pwd_entry) {
        perror("[passwd]: password entry for uid not found");
        return 0;
    }

#ifdef __linux
#ifdef HAVE_SHADOW
    {
        struct spwd *sp = NULL;

        sp = getspnam(pwd_entry->pw_name);
        if (sp)
            pwd_entry->pw_passwd = sp->sp_pwdp;

    }
#endif
#endif

    if (strlen(pwd_entry->pw_passwd) < 13) {
        perror("[passwd]: password entry has no pwd");
        pwd_entry = NULL;
        return 0;
    }

    return 1;
}

static int alock_auth_passwd_deinit() {
    return 1;
}

static int alock_auth_passwd_auth(const char *pass) {
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
    if (pass == NULL || pwd_entry == NULL)
        return 0;

    /* simpler, and should work with crypt() algorithms using longer
       salt strings (like the md5-based one on freebsd).  --marekm */
    return !strcmp(crypt(pass, pwd_entry->pw_passwd), pwd_entry->pw_passwd);
#endif /* 0 */
}

struct aAuth alock_auth_passwd = {
    "passwd",
    alock_auth_passwd_init,
    alock_auth_passwd_deinit,
    alock_auth_passwd_auth,
};

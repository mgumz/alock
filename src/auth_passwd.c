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

#if HAVE_CONFIG_H
#include "../config.h"
#endif

#define _XOPEN_SOURCE

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>

#if __linux && HAVE_SHADOW_H
#include <shadow.h>
#endif

#include "alock.h"


static struct passwd *pwd_entry = NULL;


static int module_init(struct aDisplayInfo *dinfo) {

    errno = 0;
    pwd_entry = getpwuid(getuid());
    if (!pwd_entry) {
        perror("[passwd]: password entry for uid not found");
        return -1;
    }

#if __linux && HAVE_SHADOW_H
    {
        struct spwd *sp = NULL;

        sp = getspnam(pwd_entry->pw_name);
        if (sp)
            pwd_entry->pw_passwd = sp->sp_pwdp;

    }
#endif

    if (strlen(pwd_entry->pw_passwd) < 13) {
        perror("[passwd]: password entry has no pwd");
        pwd_entry = NULL;
        return -1;
    }

    return 0;
}

static int module_authenticate(const char *pass) {

    if (pass == NULL || pwd_entry == NULL)
        return -1;

    /* simpler, and should work with crypt() algorithms using longer
       salt strings (like the md5-based one on freebsd).  --marekm */
    return strcmp(crypt(pass, pwd_entry->pw_passwd), pwd_entry->pw_passwd);
}


struct aModuleAuth alock_auth_passwd = {
    { "passwd",
        module_dummy_loadargs,
        module_init,
        module_dummy_free,
    },
    module_authenticate,
};

/*
 * alock - auth_pam.c
 * Copyright (c) 2005 - 2007 Mathias Gumz <akira at fluxbox dot org>
 *               2014 Arkadiusz Bokowy
 *
 * This file is a part of an alock.
 *
 * This projected is licensed under the terms of the MIT license.
 *
 * This authentication module provides:
 *  -auth pam
 *
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <security/pam_appl.h>

#include "alock.h"


static const char *username = NULL;
static const char *password = NULL;


static int alock_auth_pam_conv(int num_msg,
        const struct pam_message **msgs,
        struct pam_response **response,
        void *appdata_ptr) {

    if (num_msg <= 0)
        return PAM_CONV_ERR;

    *response = (struct pam_response *)calloc(num_msg, sizeof(struct pam_response));
    if (*response == NULL)
        return PAM_CONV_ERR;

    int i;
    for (i = 0; i < num_msg; i++) {
        switch (msgs[i]->msg_style) {
        case PAM_PROMPT_ECHO_OFF:
            (*response)[i].resp = strdup(password);
            (*response)[i].resp_retcode = 0;
            break;
        case PAM_ERROR_MSG:
        case PAM_TEXT_INFO:
            fprintf(stderr, "[pam]: %s\n", msgs[i]->msg);
            break;
        default:
            free(*response);
            return PAM_CONV_ERR;
        }
    }

    return PAM_SUCCESS;
}

static int alock_auth_pam_init(const char *args) {

    struct passwd *pwd;

    errno = 0;
    if (!(pwd = getpwuid(getuid()))) {
        perror("[pam]: password entry for uid not found");
        return 0;
    }

    username = pwd->pw_name;
    return 1;
}

static int alock_auth_pam_deinit() {

    password = NULL;
    return 0;
}

static int alock_auth_pam_auth(const char *pass) {

    if (!username)
        return 0;

    pam_handle_t *pam_handle = NULL;
    struct pam_conv conv = {
        &alock_auth_pam_conv, NULL
    };
    int retval;

    password = pass;
    retval = pam_start("login", username, &conv, &pam_handle);

    if (retval == PAM_SUCCESS)
        retval = pam_set_item(pam_handle, PAM_TTY, ttyname(0));
    if (retval == PAM_SUCCESS)
        retval = pam_authenticate(pam_handle, 0);

    pam_end(pam_handle, retval);
    return retval == PAM_SUCCESS;
}

struct aAuth alock_auth_pam = {
    "pam",
    alock_auth_pam_init,
    alock_auth_pam_deinit,
    alock_auth_pam_auth,
};

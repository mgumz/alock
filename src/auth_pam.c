/*
 * alock - auth_pam.c
 * Copyright (c) 2005 - 2007 Mathias Gumz <akira at fluxbox dot org>
 *
 * This file is a part of an alock.
 *
 * This projected is licensed under the terms of the MIT license.
 *
 * This authentication module provides:
 *  -auth pam
 *
 * Taken from pure-ftpd's authstuff, but you can see similar stuff
 * in xlockmore, openssh and basically all pam-related apps :)
 *
 */

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

#include <security/pam_appl.h>
#ifdef __linux
#include <security/pam_misc.h>
#endif /* __linux */

#include "alock.h"


#define PAM_YN { \
    if (PAM_error != 0 || pam_error != PAM_SUCCESS) { \
        fprintf(stderr, "[pam]: %s\n", pam_strerror(pam_handle, pam_error)); \
        pam_end(pam_handle, pam_error); \
        PAM_username = NULL; \
        PAM_password = NULL; \
        return 0;\
    } \
}

#define GET_MEM \
   size += sizeof(struct pam_response); \
   if ((reply = realloc(reply, size)) == NULL) { \
       PAM_error = 1; \
       return PAM_CONV_ERR; \
   }


static const char *PAM_username = NULL;
static const char *PAM_password = NULL;
static int PAM_error = 0;
static int pam_error = PAM_SUCCESS;


static int PAM_conv(int num_msg, const struct pam_message **msgs,
                    struct pam_response **resp, void *appdata_ptr) {

    int count = 0;
    unsigned int replies = 0U;
    struct pam_response *reply = NULL;
    size_t size = (size_t) 0U;

    (void) appdata_ptr;
    *resp = NULL;
    for (count = 0; count < num_msg; count++) {
        switch (msgs[count]->msg_style) {
        case PAM_PROMPT_ECHO_ON:
            GET_MEM;
            memset(&reply[replies], 0, sizeof reply[replies]);
            if ((reply[replies].resp = strdup(PAM_username)) == NULL) {
#ifdef PAM_BUF_ERR
                reply[replies].resp_retcode = PAM_BUF_ERR;
#endif
                PAM_error = 1;
                return PAM_CONV_ERR;
            }
            reply[replies++].resp_retcode = PAM_SUCCESS;
            /* PAM frees resp */
            break;
        case PAM_PROMPT_ECHO_OFF:
            GET_MEM;
            memset(&reply[replies], 0, sizeof reply[replies]);
            if ((reply[replies].resp = strdup(PAM_password)) == NULL) {
#ifdef PAM_BUF_ERR
                reply[replies].resp_retcode = PAM_BUF_ERR;
#endif
                PAM_error = 1;
                return PAM_CONV_ERR;
            }
            reply[replies++].resp_retcode = PAM_SUCCESS;
            /* PAM frees resp */
            break;
        case PAM_TEXT_INFO:
            /* ignore it... */
            break;
        case PAM_ERROR_MSG:
        default:
            /* Must be an error of some sort... */
            free(reply);
            PAM_error = 1;
            return PAM_CONV_ERR;
        }
    }
    *resp = reply;
    return PAM_SUCCESS;
}

static struct pam_conv PAM_conversation = {
    &PAM_conv, NULL
};


static struct passwd *pwd_entry = NULL;


static int alock_auth_pam_init(const char *args) {

    errno = 0;
    pwd_entry = getpwuid(getuid());
    if (!pwd_entry) {
        perror("[pam]: password entry for uid not found");
        return 0;
    }
    return 1;
}

static int alock_auth_pam_deinit() {

    pwd_entry = NULL;

    return 0;
}

static int alock_auth_pam_auth(const char *pass) {

    pam_handle_t *pam_handle = NULL;

    if (!pass || strlen(pass) < 1 || !pwd_entry)
        return 0;

    PAM_username = pwd_entry->pw_name;
    PAM_password = pass;
    pam_error = pam_start("login", PAM_username, &PAM_conversation, &pam_handle);
    PAM_YN;
    pam_error = pam_set_item(pam_handle, PAM_TTY, ttyname(0));
    PAM_YN;
    pam_error = pam_authenticate(pam_handle, 0);
    PAM_YN;
    pam_error = pam_end(pam_handle, pam_error);
    PAM_YN;

    return 1;
}

struct aAuth alock_auth_pam = {
    "pam",
    alock_auth_pam_init,
    alock_auth_pam_deinit,
    alock_auth_pam_auth,
};

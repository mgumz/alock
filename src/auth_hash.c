/*
 * alock - auth_hash.c
 * Copyright (c) 2014 Arkadiusz Bokowy
 *
 * This file is a part of an alock.
 *
 * This projected is licensed under the terms of the MIT license.
 *
 * This authentication module provides:
 *  -auth hash:type=<type>,hash=<hash>,file=<filename>
 *
 */

#if HAVE_CONFIG_H
#include "../config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <gcrypt.h>

#include "alock.h"


#define HASH_DIGEST_MAX_LEN 64
static int algorithms[] = {
    GCRY_MD_MD5,
    GCRY_MD_SHA1,
    GCRY_MD_SHA256,
    GCRY_MD_SHA384,
    GCRY_MD_SHA512,
    GCRY_MD_WHIRLPOOL,
};
static int selected_algorithm = 0;
static int selected_digest_len;
static unsigned char *user_digest = NULL;


static unsigned char *hex2mem(unsigned char *mem, const char *str, int len) {

    int i;

    /* one byte is represented as two characters */
    if (len % 2 != 0)
        return NULL;

    len /= 2;
    for (i = 0; i < len; i++) {
        if (isdigit(str[i * 2]))
            mem[i] = (str[i * 2] - '0') << 4;
        else
            mem[i] = (tolower(str[i * 2]) - 'a' + 10) << 4;

        if (isdigit(str[i * 2 + 1]))
            mem[i] += str[i * 2 + 1] - '0';
        else
            mem[i] += tolower(str[i * 2 + 1]) - 'a' + 10;
    }

    return mem;
}

#if DEBUG
static char *mem2hex(char *str, const unsigned char *mem, int len) {

    char hexchars[] = "0123456789abcdef", *ptr;
    int i;

    for (i = 0, ptr = str; i < len; i++) {
        *(ptr++) = hexchars[(mem[i] >> 4) & 0x0f];
        *(ptr++) = hexchars[mem[i] & 0x0f];
    }
    *ptr = 0;
    return str;
}
#endif

static void alock_auth_hash_list(void) {

    printf("list of available hashing algorithms:\n");

    unsigned int i;
    for (i = 0; i < sizeof(algorithms) / sizeof(int); i++)
        if (gcry_md_test_algo(algorithms[i]) == 0)
            printf("  %s\n", gcry_md_algo_name(algorithms[i]));

}

static int alock_auth_hash_init(const char *args) {

    char *user_hash = NULL;
    int status = 1;
    int len;

    if (args && strstr(args, "hash:") == args) {
        char *arguments = strdup(&args[5]);
        char *arg;
        char *tmp;
        for (tmp = arguments; tmp; ) {
            arg = strsep(&tmp, ",");
            if (strcmp(arg, "list") == 0) {
                alock_auth_hash_list();
                exit(EXIT_SUCCESS);
            }
            if (strstr(arg, "type=") == arg) {
                selected_algorithm = gcry_md_map_name(&arg[5]);
            }
            else if (strstr(arg, "hash=") == arg) {
                free(user_hash);
                user_hash = strdup(&arg[5]);
            }
            else if (strstr(arg, "file=") == arg) {
                FILE *f;
                int len;

                if ((f = fopen(&arg[5], "r")) == NULL) {
                    perror("[hash]: unable to read file");
                    free(arguments);
                    goto return_error;
                }

                /* allocate enough memory to contain all kind of hashes */
                free(user_hash);
                user_hash = (char*)malloc(HASH_DIGEST_MAX_LEN * 2 + 1);

                fgets(user_hash, HASH_DIGEST_MAX_LEN * 2 + 1, f);
                fclose(f);

                /* strip trailing new line character, if any */
                len = strlen(user_hash);
                if (len > 0 && user_hash[len - 1] == '\n')
                    user_hash[len - 1] = '\0';
            }
        }
        free(arguments);
    }

    if (selected_algorithm == 0) {
        fprintf(stderr, "[hash]: invalid or not specified type\n");
        goto return_error;
    }

    if (user_hash == NULL) {
        fprintf(stderr, "[hash]: not specified hash nor file\n");
        goto return_error;
    }

    /* initialize gcrypt subsystem */
    gcry_check_version(NULL);

    selected_digest_len = gcry_md_get_algo_dlen(selected_algorithm);
    len = strlen(user_hash);

    if (selected_digest_len * 2 > len) {
        fprintf(stderr, "[hash]: incorrect hash for given type\n");
        goto return_error;
    }

    user_digest = (unsigned char*)malloc(selected_digest_len);
    hex2mem(user_digest, user_hash, selected_digest_len * 2);

    goto return_success;

return_error:
    status = 0;
    selected_algorithm = 0;

return_success:
    free(user_hash);
    return status;
}

static int alock_auth_hash_deinit() {
    free(user_digest);
    return 1;
}

static int alock_auth_hash_auth(const char *pass) {

    unsigned char *digest;
    int status;

    if (selected_algorithm == 0)
        return 0;

    digest = (unsigned char*)malloc(selected_digest_len);
    gcry_md_hash_buffer(selected_algorithm, digest, pass, strlen(pass));

#if DEBUG
    char user_hash[HASH_DIGEST_MAX_LEN * 2 + 1];
    char test_hash[HASH_DIGEST_MAX_LEN * 2 + 1];
    mem2hex(user_hash, user_digest, selected_digest_len);
    mem2hex(test_hash, digest, selected_digest_len);
    debug("user hash: %s", user_hash);
    debug("test hash: %s", test_hash);
#endif

    status = !memcmp(user_digest, digest, selected_digest_len);
    free(digest);

    return status;
}


struct aAuth alock_auth_hash = {
    "hash",
    alock_auth_hash_init,
    alock_auth_hash_deinit,
    alock_auth_hash_auth,
};

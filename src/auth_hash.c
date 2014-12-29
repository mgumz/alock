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

#include <stdio.h>
#include <stdlib.h>
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

static struct moduleData {
    int selected_algorithm;
    int selected_digest_len;
    unsigned char *user_digest;
    char *user_hash;
} data = { 0 };


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

static void module_cmd_list(void) {

    printf("list of available hashing algorithms:\n");

    unsigned int i;
    for (i = 0; i < sizeof(algorithms) / sizeof(int); i++)
        if (gcry_md_test_algo(algorithms[i]) == 0)
            printf("  %s\n", gcry_md_algo_name(algorithms[i]));

}

static void module_loadargs(const char *args) {

    if (!args || strstr(args, "hash:") != args)
        return;

    char *arguments = strdup(&args[5]);
    char *arg;
    char *tmp;

    for (tmp = arguments; tmp; ) {
        arg = strsep(&tmp, ",");
        if (strcmp(arg, "list") == 0) {
            module_cmd_list();
            exit(EXIT_SUCCESS);
        }
        if (strstr(arg, "type=") == arg) {
            data.selected_algorithm = gcry_md_map_name(&arg[5]);
        }
        else if (strstr(arg, "hash=") == arg) {
            free(data.user_hash);
            data.user_hash = strdup(&arg[5]);
        }
        else if (strstr(arg, "file=") == arg) {
            FILE *f;
            int len;

            if ((f = fopen(&arg[5], "r")) == NULL) {
                perror("[hash]: unable to read file");
                goto return_error;
            }

            /* allocate enough memory to contain all kind of hashes */
            free(data.user_hash);
            data.user_hash = (char *)malloc(HASH_DIGEST_MAX_LEN * 2 + 1);

            fgets(data.user_hash, HASH_DIGEST_MAX_LEN * 2 + 1, f);
            fclose(f);

            /* strip trailing new line character, if any */
            len = strlen(data.user_hash);
            if (len > 0 && data.user_hash[len - 1] == '\n')
                data.user_hash[len - 1] = '\0';
        }
    }

return_error:
    free(arguments);
}

static int module_init(struct aDisplayInfo *dinfo) {

    int status = 1;
    int len;

    if (data.selected_algorithm == 0) {
        fprintf(stderr, "[hash]: invalid or not specified type\n");
        return -1;
    }

    if (data.user_hash == NULL) {
        fprintf(stderr, "[hash]: not specified hash nor file\n");
        return -1;
    }

    /* initialize gcrypt subsystem */
    gcry_check_version(NULL);

    data.selected_digest_len = gcry_md_get_algo_dlen(data.selected_algorithm);
    len = strlen(data.user_hash);

    if (data.selected_digest_len * 2 > len) {
        fprintf(stderr, "[hash]: incorrect hash for given type\n");
        return -1;
    }

    data.user_digest = (unsigned char *)malloc(data.selected_digest_len);
    hex2mem(data.user_digest, data.user_hash, data.selected_digest_len * 2);

    return 0;
}

static void module_free(void) {
    free(data.user_digest);
    data.user_digest = NULL;
    free(data.user_hash);
    data.user_hash = NULL;
}

static int module_authenticate(const char *pass) {

    if (data.selected_algorithm == 0)
        return -1;

    unsigned char *digest;

    digest = (unsigned char *)malloc(data.selected_digest_len);
    gcry_md_hash_buffer(data.selected_algorithm, digest, pass, strlen(pass));

#if DEBUG
    char user_hash[HASH_DIGEST_MAX_LEN * 2 + 1];
    char test_hash[HASH_DIGEST_MAX_LEN * 2 + 1];
    mem2hex(user_hash, data.user_digest, data.selected_digest_len);
    mem2hex(test_hash, digest, data.selected_digest_len);
    debug("user hash: %s", data.user_hash);
    debug("test hash: %s", test_hash);
#endif

    int status = memcmp(data.user_digest, digest, data.selected_digest_len);
    free(digest);

    return status;
}


struct aModuleAuth alock_auth_hash = {
    { "hash",
        module_dummy_loadargs,
        module_dummy_loadxrdb,
        module_init,
        module_dummy_free,
    },
    module_authenticate,
};

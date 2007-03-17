/* ---------------------------------------------------------------- *\

  file    : auth_sha1.c
  author  : m. gumz <akira at fluxbox dot org>
  copyr   : copyright (c) 2005 - 2007 by m. gumz

  license : based on: openbsd sha1.c/h

      SHA-1 in C
      By Steve Reid <steve@edmweb.com>
      100% Public Domain

  start   : So 08 Mai 2005 13:21:45 CEST

\* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- *\

  about :

    provide -auth sha1:hash=<hash>,file=<filename>

\* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- *\
  includes
\* ---------------------------------------------------------------- */

#ifndef STAND_ALONE
#    include "alock.h"
#endif /* STAND_ALONE */

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/param.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/

enum {
    SHA1_BLOCK_LENGTH = 64,
    SHA1_DIGEST_LENGTH = 20,
    SHA1_DIGEST_STRING_LENGTH = (SHA1_DIGEST_LENGTH * 2 + 1)
};

typedef struct {
    u_int32_t state[5];
    u_int64_t count;
    u_int8_t buffer[SHA1_BLOCK_LENGTH];
} sha1Context;


static void sha1_init(sha1Context *);
static void sha1_pad(sha1Context *);
static void sha1_transform(u_int32_t [5], const u_int8_t [SHA1_BLOCK_LENGTH]);
static void sha1_update(sha1Context *, const u_int8_t *, size_t);
static void sha1_final(u_int8_t [SHA1_DIGEST_LENGTH], sha1Context *);

#define HTONDIGEST(x) do {              \
        x[0] = htonl(x[0]);             \
        x[1] = htonl(x[1]);             \
        x[2] = htonl(x[2]);             \
        x[3] = htonl(x[3]);             \
        x[4] = htonl(x[4]); } while (0)

#define NTOHDIGEST(x) do {              \
        x[0] = ntohl(x[0]);             \
        x[1] = ntohl(x[1]);             \
        x[2] = ntohl(x[2]);             \
        x[3] = ntohl(x[3]);             \
        x[4] = ntohl(x[4]); } while (0)

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

/*------------------------------------------------------------------*\
   blk0() and blk() perform the initial expand.
   I got the idea of expanding during the round function from SSLeay
\*------------------------------------------------------------------*/
#if BYTE_ORDER == LITTLE_ENDIAN
#    define blk0(i) (block->l[i] = (rol(block->l[i],24)&0xFF00FF00) \
                    |(rol(block->l[i],8)&0x00FF00FF))
#else
#    define blk0(i) block->l[i]
#endif /* LITTLE_ENDIAN */
#define blk(i) (block->l[i&15] = rol(block->l[(i+13)&15]^block->l[(i+8)&15] \
    ^block->l[(i+2)&15]^block->l[i&15],1))



/*------------------------------------------------------------------*\
   (R0+R1), R2, R3, R4 are the different operations (rounds) used
   in SHA1
\*------------------------------------------------------------------*/
#define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk0(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R2(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0x6ED9EBA1+rol(v,5);w=rol(w,30);
#define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+blk(i)+0x8F1BBCDC+rol(v,5);w=rol(w,30);
#define R4(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0xCA62C1D6+rol(v,5);w=rol(w,30);

/*------------------------------------------------------------------*\
   Hash a single 512-bit block. This is the core of the algorithm.
\*------------------------------------------------------------------*/
static void sha1_transform(u_int32_t state[5], const u_int8_t buffer[SHA1_BLOCK_LENGTH]) {
    u_int32_t a, b, c, d, e;
    u_int8_t workspace[SHA1_BLOCK_LENGTH];
    typedef union {
        u_int8_t c[64];
        u_int32_t l[16];
    } CHAR64LONG16;
    CHAR64LONG16 *block = (CHAR64LONG16 *)workspace;

    (void)memcpy(block, buffer, SHA1_BLOCK_LENGTH);

    /* Copy context->state[] to working vars */
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];

    /* 4 rounds of 20 operations each. Loop unrolled. */
    R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
    R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
    R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
    R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
    R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
    R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
    R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
    R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
    R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
    R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
    R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
    R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
    R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
    R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
    R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
    R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
    R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
    R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
    R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
    R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);

    /* Add the working vars back into context.state[] */
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;

    /* Wipe variables */
    a = b = c = d = e = 0;
}

/*------------------------------------------------------------------*\
   Initialize new context
\*------------------------------------------------------------------*/
static void sha1_init(sha1Context *context) {

    /* SHA1 initialization constants */
    context->count = 0;
    context->state[0] = 0x67452301;
    context->state[1] = 0xEFCDAB89;
    context->state[2] = 0x98BADCFE;
    context->state[3] = 0x10325476;
    context->state[4] = 0xC3D2E1F0;
}

/*------------------------------------------------------------------*\
   Run your data through this.
\*------------------------------------------------------------------*/
static void sha1_update(sha1Context *context, const u_int8_t *data, size_t len) {

    size_t i, j;

    j = (size_t)((context->count >> 3) & 63);
    context->count += (len << 3);
    if ((j + len) > 63) {
        (void)memcpy(&context->buffer[j], data, (i = 64-j));
        sha1_transform(context->state, context->buffer);
        for ( ; i + 63 < len; i += 64)
            sha1_transform(context->state, (u_int8_t *)&data[i]);
        j = 0;
    } else {
        i = 0;
    }
    (void)memcpy(&context->buffer[j], &data[i], len - i);
}

/*------------------------------------------------------------------*\
   Add padding and return the message digest.
\*------------------------------------------------------------------*/
static void sha1_pad(sha1Context *context) {

    u_int8_t finalcount[8];
    u_int i;

    for (i = 0; i < 8; i++) {
        finalcount[i] = (u_int8_t)((context->count >>
            ((7 - (i & 7)) * 8)) & 255);    /* Endian independent */
    }
    sha1_update(context, (u_int8_t *)"\200", 1);
    while ((context->count & 504) != 448)
        sha1_update(context, (u_int8_t *)"\0", 1);
    sha1_update(context, finalcount, 8); /* rsShould cause a sha1_transform() */
}

static void sha1_final(u_int8_t digest[SHA1_DIGEST_LENGTH], sha1Context *context) {

    u_int i;

    sha1_pad(context);
    if (digest) {
        for (i = 0; i < SHA1_DIGEST_LENGTH; i++) {
            digest[i] = (u_int8_t)
               ((context->state[i>>2] >> ((3-(i & 3)) * 8) ) & 255);
        }
        memset(context, 0, sizeof(*context));
    }
}

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */
#ifndef STAND_ALONE

static char* userhash = NULL;

static int alock_auth_sha1_init(const char* args) {

    if (!args) {
        fprintf(stderr, "alock: error, missing arguments for [sha1].\n");
        return 0;
    }

    if (strstr(args, "sha1:") == args && strlen(&args[5]) > 0) {
        char* arguments = strdup(&args[5]);
        char* tmp;
        char* arg = NULL;
        for (tmp = arguments; tmp; ) {
            arg = strsep(&tmp, ",");
            if (arg && !userhash) {
                if (strstr(arg, "hash=") == arg && strlen(arg) > 5) {
                    if (strlen(&arg[5]) == SHA1_DIGEST_STRING_LENGTH - 1) {
                        if (!userhash)
                            userhash = strdup(&arg[5]);
                    } else {
                        fprintf(stderr, "alock: error, missing or incorrect hash for [sha1].\n");
                        free(arguments);
                        return 0;
                    }
                } else if (strstr(arg, "file=") == arg && strlen(arg) > 6) {
                    char* tmp_hash = NULL;
                    FILE* hashfile = fopen(&arg[5], "r");
                    if (hashfile) {
                        int c;
                        size_t i = 0;
                        tmp_hash = (char*)malloc(SHA1_DIGEST_STRING_LENGTH);
                        memset(tmp_hash, 0, SHA1_DIGEST_STRING_LENGTH);
                        for(i = 0, c = fgetc(hashfile);
                            i < SHA1_DIGEST_STRING_LENGTH - 1 && c != EOF; i++, c = fgetc(hashfile)) {
                            tmp_hash[i] = c;
                        }
                        fclose(hashfile);
                    } else {
                        fprintf(stderr, "alock: error, couldnt read [%s] for [sha1].\n",
                                &arg[5]);
                        free(arguments);
                        return 0;
                    }

                    if (!tmp_hash || strlen(tmp_hash) != SHA1_DIGEST_STRING_LENGTH - 1) {
                        fprintf(stderr, "alock: error, given file [%s] doesnt contain a valid hash for [sha1].\n",
                                &arg[5]);
                        if (tmp_hash)
                            free(tmp_hash);
                        free(arguments);
                        return 0;
                    }

                    userhash = tmp_hash;
                }
            }
        }
        free(arguments);
    } else {
        fprintf(stderr, "alock: error, missing arguments for [sha1].\n");
        return 0;
    }

    if (!userhash) {
        printf("alock: error, missing hash for [sha1].\n");
        return 0;
    }

    alock_string2lower(userhash);

    return 1;
}

static int alock_auth_sha1_deinit() {
    if (userhash)
        free(userhash);
    return 1;
}

static int alock_auth_sha1_auth(const char* pass) {

    unsigned char digest[SHA1_DIGEST_LENGTH];
    unsigned char stringdigest[SHA1_DIGEST_STRING_LENGTH];
    size_t i;
    sha1Context sha1;

    if (!pass || !userhash)
        return 0;

    sha1_init(&sha1);
    sha1_update(&sha1, (unsigned char*)pass, strlen(pass));
    sha1_final(digest, &sha1);

    memset(stringdigest, 0, SHA1_DIGEST_STRING_LENGTH);
    for (i = 0; i < SHA1_DIGEST_LENGTH; i++) {
        sprintf((char*)&stringdigest[i*2], "%02x", digest[i]);
    }

    return (strcmp((char*)stringdigest, userhash) == 0);
}

struct aAuth alock_auth_sha1 = {
    "sha1",
    alock_auth_sha1_init,
    alock_auth_sha1_auth,
    alock_auth_sha1_deinit
};

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */
#else

int main(int argc, char* argv[]) {

    unsigned char digest[SHA1_DIGEST_LENGTH];
    unsigned int i;
    unsigned char c;
    sha1Context sha1;

    if (argc > 1) {
        printf("asha1 - reads from stdin to calculate a sha1-hash.\n");
        exit(EXIT_SUCCESS);
    }

    sha1_init(&sha1);
    while((c = fgetc(stdin)) != (unsigned char)EOF) {
        sha1_update(&sha1, &c, 1);
    }
    sha1_final(digest, &sha1);

    for(i = 0; i < SHA1_DIGEST_LENGTH; ++i)
        printf("%02x", digest[i]);
    printf("\n");
    fflush(stdout);

    return EXIT_SUCCESS;
}

#endif /* STAND_ALONE */

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */


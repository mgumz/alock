/* ---------------------------------------------------------------- *\

  file    : auth_sha2.c
  author  : m. gumz <akira at fluxbox dot org>
  copyr   : copyright (c) 2005 by m. gumz

  license : based on: openbsd sha2.c/h

      SHA-2 in C
      Aaron D. Gifford <me@aarongifford.com>
      100% Public Domain

  start   : So 08 Mai 2005 13:21:45 CEST

  $Id: auth_sha1.c 31 2005-05-25 06:43:42Z mathias $

\* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- *\

  about :

    provide -auth sha1:hash=<hash>,file=<filename>

\* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- *\
  includes
\* ---------------------------------------------------------------- */
#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/param.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#ifndef STAND_ALONE
#    include <X11/Xlib.h>
#    include "alock.h"
#endif /* STAND_ALONE */
/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/

enum {
    SHA256_BLOCK_LENGTH = 64,
    SHA256_SHORT_BLOCK_LENGTH = (SHA256_BLOCK_LENGTH - 8),
    SHA256_DIGEST_LENGTH = 32,
    SHA256_DIGEST_STRING_LENGTH = (SHA256_DIGEST_LENGTH * 2 + 1),

    SHA384_BLOCK_LENGTH = 128,
    SHA384_SHORT_BLOCK_LENGTH = (SHA384_BLOCK_LENGTH - 16),
    SHA384_DIGEST_LENGTH = 48,
    SHA384_DIGEST_STRING_LENGTH = (SHA384_DIGEST_LENGTH * 2 + 1),

    SHA512_BLOCK_LENGTH = 128,
    SHA512_SHORT_BLOCK_LENGTH = (SHA512_BLOCK_LENGTH - 16),
    SHA512_DIGEST_LENGTH = 64,
    SHA512_DIGEST_STRING_LENGTH = (SHA512_DIGEST_LENGTH * 2 + 1)
};

typedef struct _sha256Context {
    u_int32_t    state[8];
    u_int64_t    bitcount;
    u_int8_t    buffer[SHA256_BLOCK_LENGTH];
} sha256Context;

typedef struct _sha512Context {
    u_int64_t    state[8];
    u_int64_t    bitcount[2];
    u_int8_t    buffer[SHA512_BLOCK_LENGTH];
} sha512Context;

typedef sha512Context sha384Context;

static void sha256_init(sha256Context *);
static void sha256_update(sha256Context *, const u_int8_t *, size_t);
static void sha256_final(u_int8_t[SHA256_DIGEST_LENGTH], sha256Context *);
static void sha256_transform(sha256Context *, const u_int8_t *);

static void sha384_init(sha384Context *);
static void sha384_update(sha384Context *, const u_int8_t *, size_t);
static void sha384_final(u_int8_t[SHA384_DIGEST_LENGTH], sha384Context *);

static void sha512_init(sha512Context *);
static void sha512_update(sha512Context *, const u_int8_t *, size_t);
static void sha512_final(u_int8_t[SHA512_DIGEST_LENGTH], sha512Context *);
static void sha512_last(sha512Context *);
static void sha512_transform(sha512Context *, const u_int8_t *);

/*** ENDIAN REVERSAL MACROS *******************************************/
#if BYTE_ORDER == LITTLE_ENDIAN
#define REVERSE32(w,x)    { \
    u_int32_t tmp = (w); \
    tmp = (tmp >> 16) | (tmp << 16); \
    (x) = ((tmp & 0xff00ff00UL) >> 8) | ((tmp & 0x00ff00ffUL) << 8); \
}
#define REVERSE64(w,x)    { \
    u_int64_t tmp = (w); \
    tmp = (tmp >> 32) | (tmp << 32); \
    tmp = ((tmp & 0xff00ff00ff00ff00ULL) >> 8) | \
          ((tmp & 0x00ff00ff00ff00ffULL) << 8); \
    (x) = ((tmp & 0xffff0000ffff0000ULL) >> 16) | \
          ((tmp & 0x0000ffff0000ffffULL) << 16); \
}
#endif /* BYTE_ORDER == LITTLE_ENDIAN */


/*------------------------------------------------------------------*\
  Macro for incrementally adding the unsigned 64-bit integer n to the
  unsigned 128-bit integer (represented using a two-element array of
  64-bit words):
\*------------------------------------------------------------------*/
#define ADDINC128(w,n)    { \
    (w)[0] += (u_int64_t)(n); \
    if ((w)[0] < (n)) { \
        (w)[1]++; \
    } \
}

/*------------------------------------------------------------------*\
    THE SIX LOGICAL FUNCTIONS

  Bit shifting and rotation (used by the six SHA-XYZ logical functions:

  NOTE:  The naming of R and S appears backwards here (R is a SHIFT and
  S is a ROTATION) because the SHA-256/384/512 description document
  (see http://csrc.nist.gov/cryptval/shs/sha256-384-512.pdf) uses this
  same "backwards" definition.
\*------------------------------------------------------------------*/

/* Shift-right (used in SHA-256, SHA-384, and SHA-512): */
#define R(b,x)         ((x) >> (b))
/* 32-bit Rotate-right (used in SHA-256): */
#define S32(b,x)    (((x) >> (b)) | ((x) << (32 - (b))))
/* 64-bit Rotate-right (used in SHA-384 and SHA-512): */
#define S64(b,x)    (((x) >> (b)) | ((x) << (64 - (b))))

/* Two of six logical functions used in SHA-256, SHA-384, and SHA-512: */
#define Ch(x,y,z)    (((x) & (y)) ^ ((~(x)) & (z)))
#define Maj(x,y,z)    (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

/* Four of six logical functions used in SHA-256: */
#define Sigma0_256(x)    (S32(2,  (x)) ^ S32(13, (x)) ^ S32(22, (x)))
#define Sigma1_256(x)    (S32(6,  (x)) ^ S32(11, (x)) ^ S32(25, (x)))
#define sigma0_256(x)    (S32(7,  (x)) ^ S32(18, (x)) ^ R(3 ,   (x)))
#define sigma1_256(x)    (S32(17, (x)) ^ S32(19, (x)) ^ R(10,   (x)))

/* Four of six logical functions used in SHA-384 and SHA-512: */
#define Sigma0_512(x)    (S64(28, (x)) ^ S64(34, (x)) ^ S64(39, (x)))
#define Sigma1_512(x)    (S64(14, (x)) ^ S64(18, (x)) ^ S64(41, (x)))
#define sigma0_512(x)    (S64( 1, (x)) ^ S64( 8, (x)) ^ R( 7,   (x)))
#define sigma1_512(x)    (S64(19, (x)) ^ S64(61, (x)) ^ R( 6,   (x)))


/*------------------------------------------------------------------*\
    SHA-XYZ INITIAL HASH VALUES AND CONSTANTS
\*------------------------------------------------------------------*/
const static u_int32_t K256[64] = {
    0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL,
    0x3956c25bUL, 0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL,
    0xd807aa98UL, 0x12835b01UL, 0x243185beUL, 0x550c7dc3UL,
    0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL, 0xc19bf174UL,
    0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL,
    0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL,
    0x983e5152UL, 0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL,
    0xc6e00bf3UL, 0xd5a79147UL, 0x06ca6351UL, 0x14292967UL,
    0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL, 0x53380d13UL,
    0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
    0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL,
    0xd192e819UL, 0xd6990624UL, 0xf40e3585UL, 0x106aa070UL,
    0x19a4c116UL, 0x1e376c08UL, 0x2748774cUL, 0x34b0bcb5UL,
    0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL, 0x682e6ff3UL,
    0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
    0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL
};

/* Initial hash value H for SHA-256: */
const static u_int32_t sha256_initial_hash_value[8] = {
    0x6a09e667UL,
    0xbb67ae85UL,
    0x3c6ef372UL,
    0xa54ff53aUL,
    0x510e527fUL,
    0x9b05688cUL,
    0x1f83d9abUL,
    0x5be0cd19UL
};

/* Hash constant words K for SHA-384 and SHA-512: */
const static u_int64_t K512[80] = {
    0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL,
    0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL,
    0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL, 0x12835b0145706fbeULL,
    0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL,
    0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL,
    0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL,
    0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL,
    0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL,
    0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL,
    0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
    0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL,
    0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL,
    0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL, 0xd69906245565a910ULL,
    0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL,
    0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL,
    0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL,
    0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL,
    0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
    0xca273eceea26619cULL, 0xd186b8c721c0c207ULL,
    0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
    0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL,
    0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
    0x28db77f523047d84ULL, 0x32caab7b40c72493ULL,
    0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
    0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL,
    0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
};

/* Initial hash value H for SHA-384 */
const static u_int64_t sha384_initial_hash_value[8] = {
    0xcbbb9d5dc1059ed8ULL,
    0x629a292a367cd507ULL,
    0x9159015a3070dd17ULL,
    0x152fecd8f70e5939ULL,
    0x67332667ffc00b31ULL,
    0x8eb44a8768581511ULL,
    0xdb0c2e0d64f98fa7ULL,
    0x47b5481dbefa4fa4ULL
};

/* Initial hash value H for SHA-512 */
const static u_int64_t sha512_initial_hash_value[8] = {
    0x6a09e667f3bcc908ULL,
    0xbb67ae8584caa73bULL,
    0x3c6ef372fe94f82bULL,
    0xa54ff53a5f1d36f1ULL,
    0x510e527fade682d1ULL,
    0x9b05688c2b3e6c1fULL,
    0x1f83d9abfb41bd6bULL,
    0x5be0cd19137e2179ULL
};


/*------------------------------------------------------------------*\
    SHA-256:
\*------------------------------------------------------------------*/
void sha256_init(sha256Context *context) {

    if (context == NULL)
        return;
    bcopy(sha256_initial_hash_value, context->state, SHA256_DIGEST_LENGTH);
    bzero(context->buffer, SHA256_BLOCK_LENGTH);
    context->bitcount = 0;
}

#ifdef SHA2_UNROLL_TRANSFORM

/* Unrolled SHA-256 round macros: */

#define ROUND256_0_TO_15(a,b,c,d,e,f,g,h) do {                    \
    W256[j] = (u_int32_t)data[3] | ((u_int32_t)data[2] << 8) |        \
        ((u_int32_t)data[1] << 16) | ((u_int32_t)data[0] << 24);        \
    data += 4;                                \
    T1 = (h) + Sigma1_256((e)) + Ch((e), (f), (g)) + K256[j] + W256[j]; \
    (d) += T1;                                \
    (h) = T1 + Sigma0_256((a)) + Maj((a), (b), (c));            \
    j++;                                    \
} while(0)

#define ROUND256(a,b,c,d,e,f,g,h) do {                        \
    s0 = W256[(j+1)&0x0f];                            \
    s0 = sigma0_256(s0);                            \
    s1 = W256[(j+14)&0x0f];                            \
    s1 = sigma1_256(s1);                            \
    T1 = (h) + Sigma1_256((e)) + Ch((e), (f), (g)) + K256[j] +        \
         (W256[j&0x0f] += s1 + W256[(j+9)&0x0f] + s0);            \
    (d) += T1;                                \
    (h) = T1 + Sigma0_256((a)) + Maj((a), (b), (c));            \
    j++;                                    \
} while(0)

void sha256_transform(sha256Context *context, const u_int8_t *data) {

    u_int32_t    a, b, c, d, e, f, g, h, s0, s1;
    u_int32_t    T1, *W256;
    int        j;

    W256 = (u_int32_t *)context->buffer;

    /* Initialize registers with the prev. intermediate value */
    a = context->state[0];
    b = context->state[1];
    c = context->state[2];
    d = context->state[3];
    e = context->state[4];
    f = context->state[5];
    g = context->state[6];
    h = context->state[7];

    j = 0;
    do {
        /* Rounds 0 to 15 (unrolled): */
        ROUND256_0_TO_15(a,b,c,d,e,f,g,h);
        ROUND256_0_TO_15(h,a,b,c,d,e,f,g);
        ROUND256_0_TO_15(g,h,a,b,c,d,e,f);
        ROUND256_0_TO_15(f,g,h,a,b,c,d,e);
        ROUND256_0_TO_15(e,f,g,h,a,b,c,d);
        ROUND256_0_TO_15(d,e,f,g,h,a,b,c);
        ROUND256_0_TO_15(c,d,e,f,g,h,a,b);
        ROUND256_0_TO_15(b,c,d,e,f,g,h,a);
    } while (j < 16);

    /* Now for the remaining rounds to 64: */
    do {
        ROUND256(a,b,c,d,e,f,g,h);
        ROUND256(h,a,b,c,d,e,f,g);
        ROUND256(g,h,a,b,c,d,e,f);
        ROUND256(f,g,h,a,b,c,d,e);
        ROUND256(e,f,g,h,a,b,c,d);
        ROUND256(d,e,f,g,h,a,b,c);
        ROUND256(c,d,e,f,g,h,a,b);
        ROUND256(b,c,d,e,f,g,h,a);
    } while (j < 64);

    /* Compute the current intermediate hash value */
    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;

    /* Clean up */
    a = b = c = d = e = f = g = h = T1 = 0;
}

#else /* SHA2_UNROLL_TRANSFORM */

void sha256_transform(sha256Context *context, const u_int8_t *data) {

    u_int32_t    a, b, c, d, e, f, g, h, s0, s1;
    u_int32_t    T1, T2, *W256;
    int        j;

    W256 = (u_int32_t *)context->buffer;

    /* Initialize registers with the prev. intermediate value */
    a = context->state[0];
    b = context->state[1];
    c = context->state[2];
    d = context->state[3];
    e = context->state[4];
    f = context->state[5];
    g = context->state[6];
    h = context->state[7];

    j = 0;
    do {
        W256[j] = (u_int32_t)data[3] | ((u_int32_t)data[2] << 8) |
            ((u_int32_t)data[1] << 16) | ((u_int32_t)data[0] << 24);
        data += 4;
        /* Apply the SHA-256 compression function to update a..h */
        T1 = h + Sigma1_256(e) + Ch(e, f, g) + K256[j] + W256[j];
        T2 = Sigma0_256(a) + Maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;

        j++;
    } while (j < 16);

    do {
        /* Part of the message block expansion: */
        s0 = W256[(j+1)&0x0f];
        s0 = sigma0_256(s0);
        s1 = W256[(j+14)&0x0f];    
        s1 = sigma1_256(s1);

        /* Apply the SHA-256 compression function to update a..h */
        T1 = h + Sigma1_256(e) + Ch(e, f, g) + K256[j] + 
             (W256[j&0x0f] += s1 + W256[(j+9)&0x0f] + s0);
        T2 = Sigma0_256(a) + Maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;

        j++;
    } while (j < 64);

    /* Compute the current intermediate hash value */
    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;

    /* Clean up */
    a = b = c = d = e = f = g = h = T1 = T2 = 0;
}

#endif /* SHA2_UNROLL_TRANSFORM */

void sha256_update(sha256Context *context, const u_int8_t *data, size_t len) {

    size_t    freespace, usedspace;

    /* Calling with no data is valid (we do nothing) */
    if (len == 0)
        return;

    usedspace = (context->bitcount >> 3) % SHA256_BLOCK_LENGTH;
    if (usedspace > 0) {
        /* Calculate how much free space is available in the buffer */
        freespace = SHA256_BLOCK_LENGTH - usedspace;

        if (len >= freespace) {
            /* Fill the buffer completely and process it */
            bcopy(data, &context->buffer[usedspace], freespace);
            context->bitcount += freespace << 3;
            len -= freespace;
            data += freespace;
            sha256_transform(context, context->buffer);
        } else {
            /* The buffer is not yet full */
            bcopy(data, &context->buffer[usedspace], len);
            context->bitcount += len << 3;
            /* Clean up: */
            usedspace = freespace = 0;
            return;
        }
    }
    while (len >= SHA256_BLOCK_LENGTH) {
        /* Process as many complete blocks as we can */
        sha256_transform(context, data);
        context->bitcount += SHA256_BLOCK_LENGTH << 3;
        len -= SHA256_BLOCK_LENGTH;
        data += SHA256_BLOCK_LENGTH;
    }
    if (len > 0) {
        /* There's left-overs, so save 'em */
        bcopy(data, context->buffer, len);
        context->bitcount += len << 3;
    }
    /* Clean up: */
    usedspace = freespace = 0;
}

void sha256_final(u_int8_t digest[], sha256Context *context) {

    u_int32_t    *d = (u_int32_t *)digest;
    unsigned int    usedspace;

    /* If no digest buffer is passed, we don't bother doing this: */
    if (digest != NULL) {
        usedspace = (context->bitcount >> 3) % SHA256_BLOCK_LENGTH;
#if BYTE_ORDER == LITTLE_ENDIAN
        /* Convert FROM host byte order */
        REVERSE64(context->bitcount,context->bitcount);
#endif
        if (usedspace > 0) {
            /* Begin padding with a 1 bit: */
            context->buffer[usedspace++] = 0x80;

            if (usedspace <= SHA256_SHORT_BLOCK_LENGTH) {
                /* Set-up for the last transform: */
                bzero(&context->buffer[usedspace], SHA256_SHORT_BLOCK_LENGTH - usedspace);
            } else {
                if (usedspace < SHA256_BLOCK_LENGTH) {
                    bzero(&context->buffer[usedspace], SHA256_BLOCK_LENGTH - usedspace);
                }
                /* Do second-to-last transform: */
                sha256_transform(context, context->buffer);

                /* And set-up for the last transform: */
                bzero(context->buffer, SHA256_SHORT_BLOCK_LENGTH);
            }
        } else {
            /* Set-up for the last transform: */
            bzero(context->buffer, SHA256_SHORT_BLOCK_LENGTH);

            /* Begin padding with a 1 bit: */
            *context->buffer = 0x80;
        }
        /* Set the bit count: */
        *(u_int64_t *)&context->buffer[SHA256_SHORT_BLOCK_LENGTH] = context->bitcount;

        /* Final transform: */
        sha256_transform(context, context->buffer);

#if BYTE_ORDER == LITTLE_ENDIAN
        {
            /* Convert TO host byte order */
            int    j;
            for (j = 0; j < 8; j++) {
                REVERSE32(context->state[j],context->state[j]);
                *d++ = context->state[j];
            }
        }
#else
        bcopy(context->state, d, SHA256_DIGEST_LENGTH);
#endif
    }

    /* Clean up state data: */
    bzero(context, sizeof(*context));
    usedspace = 0;
}

/*------------------------------------------------------------------*\
    SHA-512:
\*------------------------------------------------------------------*/
void sha512_init(sha512Context *context) {

    if (context == NULL)
        return;
    bcopy(sha512_initial_hash_value, context->state, SHA512_DIGEST_LENGTH);
    bzero(context->buffer, SHA512_BLOCK_LENGTH);
    context->bitcount[0] = context->bitcount[1] =  0;
}

#ifdef SHA2_UNROLL_TRANSFORM

/* Unrolled SHA-512 round macros: */

#define ROUND512_0_TO_15(a,b,c,d,e,f,g,h) do {                    \
    W512[j] = (u_int64_t)data[7] | ((u_int64_t)data[6] << 8) |        \
        ((u_int64_t)data[5] << 16) | ((u_int64_t)data[4] << 24) |        \
        ((u_int64_t)data[3] << 32) | ((u_int64_t)data[2] << 40) |        \
        ((u_int64_t)data[1] << 48) | ((u_int64_t)data[0] << 56);        \
    data += 8;                                \
    T1 = (h) + Sigma1_512((e)) + Ch((e), (f), (g)) + K512[j] + W512[j]; \
    (d) += T1;                                \
    (h) = T1 + Sigma0_512((a)) + Maj((a), (b), (c));            \
    j++;                                    \
} while(0)


#define ROUND512(a,b,c,d,e,f,g,h) do {                        \
    s0 = W512[(j+1)&0x0f];                            \
    s0 = sigma0_512(s0);                            \
    s1 = W512[(j+14)&0x0f];                            \
    s1 = sigma1_512(s1);                            \
    T1 = (h) + Sigma1_512((e)) + Ch((e), (f), (g)) + K512[j] +        \
             (W512[j&0x0f] += s1 + W512[(j+9)&0x0f] + s0);            \
    (d) += T1;                                \
    (h) = T1 + Sigma0_512((a)) + Maj((a), (b), (c));            \
    j++;                                    \
} while(0)

void sha512_transform(sha512Context *context, const u_int8_t *data) {

    u_int64_t    a, b, c, d, e, f, g, h, s0, s1;
    u_int64_t    T1, *W512 = (u_int64_t *)context->buffer;
    int        j;

    /* Initialize registers with the prev. intermediate value */
    a = context->state[0];
    b = context->state[1];
    c = context->state[2];
    d = context->state[3];
    e = context->state[4];
    f = context->state[5];
    g = context->state[6];
    h = context->state[7];

    j = 0;
    do {
        ROUND512_0_TO_15(a,b,c,d,e,f,g,h);
        ROUND512_0_TO_15(h,a,b,c,d,e,f,g);
        ROUND512_0_TO_15(g,h,a,b,c,d,e,f);
        ROUND512_0_TO_15(f,g,h,a,b,c,d,e);
        ROUND512_0_TO_15(e,f,g,h,a,b,c,d);
        ROUND512_0_TO_15(d,e,f,g,h,a,b,c);
        ROUND512_0_TO_15(c,d,e,f,g,h,a,b);
        ROUND512_0_TO_15(b,c,d,e,f,g,h,a);
    } while (j < 16);

    /* Now for the remaining rounds up to 79: */
    do {
        ROUND512(a,b,c,d,e,f,g,h);
        ROUND512(h,a,b,c,d,e,f,g);
        ROUND512(g,h,a,b,c,d,e,f);
        ROUND512(f,g,h,a,b,c,d,e);
        ROUND512(e,f,g,h,a,b,c,d);
        ROUND512(d,e,f,g,h,a,b,c);
        ROUND512(c,d,e,f,g,h,a,b);
        ROUND512(b,c,d,e,f,g,h,a);
    } while (j < 80);

    /* Compute the current intermediate hash value */
    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;

    /* Clean up */
    a = b = c = d = e = f = g = h = T1 = 0;
}

#else /* SHA2_UNROLL_TRANSFORM */

void sha512_transform(sha512Context *context, const u_int8_t *data) {

    u_int64_t    a, b, c, d, e, f, g, h, s0, s1;
    u_int64_t    T1, T2, *W512 = (u_int64_t *)context->buffer;
    int        j;

    /* Initialize registers with the prev. intermediate value */
    a = context->state[0];
    b = context->state[1];
    c = context->state[2];
    d = context->state[3];
    e = context->state[4];
    f = context->state[5];
    g = context->state[6];
    h = context->state[7];

    j = 0;
    do {
        W512[j] = (u_int64_t)data[7] | ((u_int64_t)data[6] << 8) |
            ((u_int64_t)data[5] << 16) | ((u_int64_t)data[4] << 24) |
            ((u_int64_t)data[3] << 32) | ((u_int64_t)data[2] << 40) |
            ((u_int64_t)data[1] << 48) | ((u_int64_t)data[0] << 56);
        data += 8;
        /* Apply the SHA-512 compression function to update a..h */
        T1 = h + Sigma1_512(e) + Ch(e, f, g) + K512[j] + W512[j];
        T2 = Sigma0_512(a) + Maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;

        j++;
    } while (j < 16);

    do {
        /* Part of the message block expansion: */
        s0 = W512[(j+1)&0x0f];
        s0 = sigma0_512(s0);
        s1 = W512[(j+14)&0x0f];
        s1 =  sigma1_512(s1);

        /* Apply the SHA-512 compression function to update a..h */
        T1 = h + Sigma1_512(e) + Ch(e, f, g) + K512[j] +
             (W512[j&0x0f] += s1 + W512[(j+9)&0x0f] + s0);
        T2 = Sigma0_512(a) + Maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;

        j++;
    } while (j < 80);

    /* Compute the current intermediate hash value */
    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;

    /* Clean up */
    a = b = c = d = e = f = g = h = T1 = T2 = 0;
}

#endif /* SHA2_UNROLL_TRANSFORM */

void sha512_update(sha512Context *context, const u_int8_t *data, size_t len) {

    size_t    freespace, usedspace;

    /* Calling with no data is valid (we do nothing) */
    if (len == 0)
        return;

    usedspace = (context->bitcount[0] >> 3) % SHA512_BLOCK_LENGTH;
    if (usedspace > 0) {
        /* Calculate how much free space is available in the buffer */
        freespace = SHA512_BLOCK_LENGTH - usedspace;

        if (len >= freespace) {
            /* Fill the buffer completely and process it */
            bcopy(data, &context->buffer[usedspace], freespace);
            ADDINC128(context->bitcount, freespace << 3);
            len -= freespace;
            data += freespace;
            sha512_transform(context, context->buffer);
        } else {
            /* The buffer is not yet full */
            bcopy(data, &context->buffer[usedspace], len);
            ADDINC128(context->bitcount, len << 3);
            /* Clean up: */
            usedspace = freespace = 0;
            return;
        }
    }
    while (len >= SHA512_BLOCK_LENGTH) {
        /* Process as many complete blocks as we can */
        sha512_transform(context, data);
        ADDINC128(context->bitcount, SHA512_BLOCK_LENGTH << 3);
        len -= SHA512_BLOCK_LENGTH;
        data += SHA512_BLOCK_LENGTH;
    }
    if (len > 0) {
        /* There's left-overs, so save 'em */
        bcopy(data, context->buffer, len);
        ADDINC128(context->bitcount, len << 3);
    }
    /* Clean up: */
    usedspace = freespace = 0;
}

void sha512_last(sha512Context *context) {

    unsigned int    usedspace;

    usedspace = (context->bitcount[0] >> 3) % SHA512_BLOCK_LENGTH;
#if BYTE_ORDER == LITTLE_ENDIAN
    /* Convert FROM host byte order */
    REVERSE64(context->bitcount[0],context->bitcount[0]);
    REVERSE64(context->bitcount[1],context->bitcount[1]);
#endif
    if (usedspace > 0) {
        /* Begin padding with a 1 bit: */
        context->buffer[usedspace++] = 0x80;

        if (usedspace <= SHA512_SHORT_BLOCK_LENGTH) {
            /* Set-up for the last transform: */
            bzero(&context->buffer[usedspace], SHA512_SHORT_BLOCK_LENGTH - usedspace);
        } else {
            if (usedspace < SHA512_BLOCK_LENGTH) {
                bzero(&context->buffer[usedspace], SHA512_BLOCK_LENGTH - usedspace);
            }
            /* Do second-to-last transform: */
            sha512_transform(context, context->buffer);

            /* And set-up for the last transform: */
            bzero(context->buffer, SHA512_BLOCK_LENGTH - 2);
        }
    } else {
        /* Prepare for final transform: */
        bzero(context->buffer, SHA512_SHORT_BLOCK_LENGTH);

        /* Begin padding with a 1 bit: */
        *context->buffer = 0x80;
    }
    /* Store the length of input data (in bits): */
    *(u_int64_t *)&context->buffer[SHA512_SHORT_BLOCK_LENGTH] = context->bitcount[1];
    *(u_int64_t *)&context->buffer[SHA512_SHORT_BLOCK_LENGTH+8] = context->bitcount[0];

    /* Final transform: */
    sha512_transform(context, context->buffer);
}

void sha512_final(u_int8_t digest[], sha512Context *context) {

    u_int64_t    *d = (u_int64_t *)digest;

    /* If no digest buffer is passed, we don't bother doing this: */
    if (digest != NULL) {
        sha512_last(context);

        /* Save the hash data for output: */
#if BYTE_ORDER == LITTLE_ENDIAN
        {
            /* Convert TO host byte order */
            int    j;
            for (j = 0; j < 8; j++) {
                REVERSE64(context->state[j],context->state[j]);
                *d++ = context->state[j];
            }
        }
#else
        bcopy(context->state, d, SHA512_DIGEST_LENGTH);
#endif
    }

    /* Zero out state data */
    bzero(context, sizeof(*context));
}

/*------------------------------------------------------------------*\
    SHA-384:
\*------------------------------------------------------------------*/
void sha384_init(sha384Context *context) {

    if (context == NULL)
        return;
    bcopy(sha384_initial_hash_value, context->state, SHA512_DIGEST_LENGTH);
    bzero(context->buffer, SHA384_BLOCK_LENGTH);
    context->bitcount[0] = context->bitcount[1] = 0;
}

void sha384_update(sha384Context *context, const u_int8_t *data, size_t len) {

    sha512_update((sha512Context *)context, data, len);
}

void sha384_final(u_int8_t digest[], sha384Context *context) {

    u_int64_t    *d = (u_int64_t *)digest;

    /* If no digest buffer is passed, we don't bother doing this: */
    if (digest != NULL) {
        sha512_last((sha512Context *)context);

        /* Save the hash data for output: */
#if BYTE_ORDER == LITTLE_ENDIAN
        {
            /* Convert TO host byte order */
            int j;
            for (j = 0; j < 6; j++) {
                REVERSE64(context->state[j],context->state[j]);
                *d++ = context->state[j];
            }
        }
#else
        bcopy(context->state, d, SHA384_DIGEST_LENGTH);
#endif
    }

    /* Zero out state data */
    bzero(context, sizeof(*context));
}

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */
#ifndef STAND_ALONE

static char* userhash = NULL;
static unsigned int method = 0;
static char* method_string = NULL;
static size_t method_digest_string_length = 0;

enum {
    NONE    = 0,
    SHA256  = 256,
    SHA384  = 384,
    SHA512  = 512
};

static int alock_auth_sha2_init(const char* args) {

    if (!args) {
        fprintf(stderr, "alock: error, missing arguments for [sha2].\n");
        return 0;
    }

    if (strstr(args, "sha256:") == args) {
        method = SHA256;
        method_string = strdup("sha256");
        method_digest_string_length = SHA256_DIGEST_STRING_LENGTH;
    } else if (strstr(args, "sha512:") == args) {
        method = SHA512;
        method_string = strdup("sha512");
        method_digest_string_length = SHA512_DIGEST_STRING_LENGTH;
    } else if (strstr(args, "sha384:") == args) {
        method = SHA384;
        method_string = strdup("sha384");
        method_digest_string_length = SHA384_DIGEST_STRING_LENGTH;
    } else {
        printf("alock: error, not supported hash in [sha2].\n");
        return 0;
    }

    if (strlen(&args[7]) > 0) {

        char* arguments = strdup(&args[7]);
        char* tmp;
        char* arg = NULL;

        for (tmp = arguments; tmp; ) {
            arg = strsep(&tmp, ",");
            if (arg && !userhash) {
                if (strstr(arg, "hash=") == arg && strlen(arg) > 5) {
                    if (strlen(&arg[5]) == method_digest_string_length - 1) {
                        if (!userhash)
                            userhash = strdup(&arg[5]);
                    } else {
                        printf("alock: error, missing or incorrect hash for [%s].\n", method_string);
                        free(arguments);
                        return 0;
                    }
                } else if (strstr(arg, "file=") == arg && strlen(arg) > 6) {
                    char* tmp_hash = NULL;
                    FILE* hashfile = fopen(&arg[5], "r");
                    if (hashfile) {
                        int c;
                        unsigned int i = 0;
                        tmp_hash = (char*)malloc(method_digest_string_length);
                        memset(tmp_hash, 0, method_digest_string_length);
                        for(i = 0, c = fgetc(hashfile);
                            i < method_digest_string_length - 1 && c != EOF; i++, c = fgetc(hashfile)) {
                            tmp_hash[i] = tolower(c);
                        }
                        fclose(hashfile);
                    } else {
                        printf("alock: error, couldnt read [%s] for [%s].\n",
                                &arg[5], method_string);
                        free(method_string);
                        free(arguments);
                        return 0;
                    }

                    if (!tmp_hash || strlen(tmp_hash) != method_digest_string_length - 1) {
                        printf("alock: error, given file [%s] doesnt contain a valid hash for [%s].\n",
                                &arg[5], method_string);
                        free(method_string);
                        free(arguments);
                        return 0;
                    }

                    userhash = tmp_hash;
                }
            }
        }
        free(arguments);
    } else {
        fprintf(stderr, "alock: error, missing arguments for [%s].\n", method_string);
        free(method_string);
        return 0;
    }

    if (!userhash) {
        printf("alock: error, missing hash for [%s].\n", method_string);
        free(method_string);
        return 0;
    }

    alock_string2lower(userhash);

    return 1;
}

static int alock_auth_sha2_deinit() {

    if (userhash)
        free(userhash);

    userhash = NULL;
    if (method_string)
        free(method_string);
    method_string = NULL;
    method = NONE;
    method_digest_string_length = 0;
    return 1;
}

static int alock_auth_sha2_auth(const char* pass) {

    if (!pass || !userhash || !method)
        return 0;

    switch (method) {
    case SHA256: {
            unsigned char digest[SHA256_DIGEST_LENGTH];
            unsigned char stringdigest[SHA256_DIGEST_STRING_LENGTH];
            unsigned int i;
            sha256Context sha256;

            sha256_init(&sha256);
            sha256_update(&sha256, (unsigned char*)pass, strlen(pass));
            sha256_final(digest, &sha256);

            memset(stringdigest, 0, SHA256_DIGEST_STRING_LENGTH);
            for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
                sprintf((char*)&stringdigest[i*2], "%02x", digest[i]);
            }
            return !strcmp((char*)stringdigest, userhash);
        }
        break;
    case SHA512: {
            unsigned char digest[SHA512_DIGEST_LENGTH];
            unsigned char stringdigest[SHA512_DIGEST_STRING_LENGTH];
            unsigned int i;
            sha512Context sha512;

            sha512_init(&sha512);
            sha512_update(&sha512, (unsigned char*)pass, strlen(pass));
            sha512_final(digest, &sha512);

            memset(stringdigest, 0, SHA512_DIGEST_STRING_LENGTH);
            for (i = 0; i < SHA512_DIGEST_LENGTH; i++) {
                sprintf((char*)&stringdigest[i*2], "%02x", digest[i]);
            }
            return !strcmp((char*)stringdigest, userhash);
        }
        break;
    case SHA384: {
            unsigned char digest[SHA384_DIGEST_LENGTH];
            unsigned char stringdigest[SHA384_DIGEST_STRING_LENGTH];
            unsigned int i;
            sha384Context sha384;

            sha384_init(&sha384);
            sha384_update(&sha384, (unsigned char*)pass, strlen(pass));
            sha384_final(digest, &sha384);

            memset(stringdigest, 0, SHA384_DIGEST_STRING_LENGTH);
            for (i = 0; i < SHA384_DIGEST_LENGTH; i++) {
                sprintf((char*)&stringdigest[i*2], "%02x", digest[i]);
            }
            return !strcmp((char*)stringdigest, userhash);
        }
        break;
    };

    return 0;
}

struct aAuth alock_auth_sha256 = {
    "sha256",
    alock_auth_sha2_init,
    alock_auth_sha2_auth,
    alock_auth_sha2_deinit
};

struct aAuth alock_auth_sha384 = {
    "sha384",
    alock_auth_sha2_init,
    alock_auth_sha2_auth,
    alock_auth_sha2_deinit
};

struct aAuth alock_auth_sha512 = {
    "sha512",
    alock_auth_sha2_init,
    alock_auth_sha2_auth,
    alock_auth_sha2_deinit
};



/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */
#else

void usage() {
    printf("asha2 - reads from stdin to calculate a sha2-hash.\n"
           "usage:\n"
           "  asha2 <256|384|512>\n");
}

int main(int argc, char* argv[]) {
 
    unsigned char digest[SHA512_DIGEST_LENGTH];
    unsigned int i;
    unsigned char c;
    unsigned int method = 0;
    size_t method_digest_length = 0;

    if (argc < 2) {
        usage();
        exit(EXIT_SUCCESS);
    }

    method = atoi(argv[1]);

    switch (method) {

        case SHA256: {
                sha256Context sha256;
                sha256_init(&sha256);
                while((c = fgetc(stdin)) != (unsigned char)EOF) {
                    sha256_update(&sha256, &c, 1);
                }
                sha256_final(digest, &sha256);
                method_digest_length = SHA256_DIGEST_LENGTH;
            }
            break;
        case SHA384: {
                sha384Context sha384;
                sha384_init(&sha384);
                while((c = fgetc(stdin)) != (unsigned char)EOF) {
                    sha384_update(&sha384, &c, 1);
                }
                sha384_final(digest, &sha384);
                method_digest_length = SHA384_DIGEST_LENGTH;
            }
            break;
        case SHA512: {
                sha512Context sha512;
                sha512_init(&sha512);
                while((c = fgetc(stdin)) != (unsigned char)EOF) {
                    sha512_update(&sha512, &c, 1);
                }
                sha512_final(digest, &sha512);
                method_digest_length = SHA512_DIGEST_LENGTH;
            }
            break;
        default:
            usage();
            exit(EXIT_FAILURE);
            break;
    };

    for(i = 0; i < method_digest_length; ++i)
        printf("%02x", digest[i]);
    printf("\n");
    fflush(stdout);

    return EXIT_SUCCESS;
}

#endif /* STAND_ALONE */

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */


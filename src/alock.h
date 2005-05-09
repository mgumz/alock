
#ifndef _ALOCK_H_
#define _ALOCK_H_

/* ---------------------------------------------------------------- *\

  file    : aklock.h
  author  : m. gumz <akira at fluxbox dot org>
  copyr   : copyright (c) 2005 by m. gumz

  license : see LICENSE

  start   : Sa 30 Apr 2005 11:51:52 CEST

\* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- *\

  about :

\* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */

struct aAuth {
    const char* name;
    int (*init)(const char* args);
    int (*auth)(const char* pass);
    int (*deinit)();
};

struct aCursor {
    const char* name;
    unsigned int width;
    unsigned int height;
    unsigned int x_hot;
    unsigned int y_hot;

    unsigned char* bits;
    unsigned char* mask;
};



struct aOpts {

    struct aAuth* auth;
    char use_blank;

    char* cursor_name;

    char* color_fg;
    char* color_bg;
};


/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/
extern struct aCursor alock_cursors[];


/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/
extern struct aAuth alock_auth_none;
#ifdef HASH_PWD
extern struct aAuth alock_auth_md5;
extern struct aAuth alock_auth_sha1;
#endif /* HASH_PWD */
#ifdef PASSWD_PWD
extern struct aAuth alock_auth_passwd;
#endif /* PASSWD_PWD */
#ifdef PAM_PWD
extern struct aAuth alock_auth_pam;
#endif /* PAM_PWD */
/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */
#endif // _ALOCK_H_


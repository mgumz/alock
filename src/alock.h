
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



struct aXInfo {

    Display* display;
    Window   root;
    Colormap colormap;
    
    Window   window;
    Cursor   cursor;
};

struct aAuth {
    const char* name;
    int (*init)(const char* args);
    int (*auth)(const char* pass);
    int (*deinit)();
};

struct aCursor {
    const char* name;
    int (*init)(const char* args, struct aXInfo* xinfo);
    int (*deinit)(struct aXInfo* xinfo);
};

struct aBackground {
    const char* name;
    int (*init)(const char* args, struct aXInfo* xinfo);
    int (*deinit)(struct aXInfo* xinfo);
};

struct aOpts {
    struct aAuth* auth;
    struct aCursor* cursor;
    struct aBackground* background;
};


/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/
extern struct aBackground alock_bg_none;
extern struct aBackground alock_bg_blank;
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
/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/
extern struct aCursor alock_cursor_none;
extern struct aCursor alock_cursor_theme;
extern struct aCursor alock_cursor_font;
#ifdef HAVE_XCURSOR
extern struct aCursor alock_cursor_xcursor;
#endif /* HAVE_XCURSOR */
/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */
#endif // _ALOCK_H_


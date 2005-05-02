
#ifndef _AKLOCK_H_
#define _AKLOCK_H_

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

struct akCursor {
    const char* name;
    unsigned int width;
    unsigned int height;
    unsigned int x_hot;
    unsigned int y_hot;

    unsigned char* bits;
    unsigned char* mask;
};

struct akXInfo {
    
    Display* display;
    Window   root;
    Window   window;

    Cursor   cursor;
    
    int width;
    int height;
};

struct akOpts {

    char use_blank;

    char* cursor_name;
    
    char* color_fg;
    char* color_bg;
};


/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/
extern struct akCursor ak_cursors[];

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */
#endif // _AKLOCK_H_


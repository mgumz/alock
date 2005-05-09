/* ---------------------------------------------------------------- *\

  file    : akcursors.c
  author  : m. gumz <akira at fluxbox dot org>
  copyr   : copyright (c) 2005 by m. gumz

  license : see LICENSE
  
  start   : Sa 30 Apr 2005 12:02:47 CEST

\* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- *\

  about :

\* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- *\
  includes
\* ---------------------------------------------------------------- */
#include <X11/Xlib.h>
#include "alock.h"

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */

#include <X11/bitmaps/xlogo16>

#include "../bitmaps/mini.bitmap"
#include "../bitmaps/mini_mask.bitmap"

#include "../bitmaps/xtr.bitmap"
#include "../bitmaps/xtr_mask.bitmap"

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */

struct aCursor alock_cursors[] = {

    { "mini", 
      mini_width, mini_height, mini_x_hot, mini_y_hot, 
      mini_bits, mini_mask_bits },

    { "xtr",
      xtr_width, xtr_height, xtr_x_hot, xtr_y_hot,
      xtr_bits, xtr_mask_bits },

    { "xlogo16",
      xlogo16_width, xlogo16_height, xlogo16_width / 2, xlogo16_height / 2,
      xlogo16_bits, xlogo16_bits },

    { NULL, 0, 0, 0, 0, NULL, NULL }
};

/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/


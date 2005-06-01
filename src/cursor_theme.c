/* ---------------------------------------------------------------- *\

  file    : cursor_theme.c
  author  : m. gumz <akira at fluxbox dot org>
  copyr   : copyright (c) 2005 by m. gumz

  license : see LICENSE
  
  start   : Sa 30 Apr 2005 12:02:47 CEST

  $Id$

  $Id$

\* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- *\

  about :
    
    provide -cursor theme:name,bg=color,fg=color

\* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- *\
  includes
\* ---------------------------------------------------------------- */
#include <X11/Xlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "alock.h"

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */

#include <X11/bitmaps/xlogo16>

#include "../bitmaps/mini.xbm"
#include "../bitmaps/mini_mask.xbm"

#include "../bitmaps/xtr.xbm"
#include "../bitmaps/xtr_mask.xbm"

#include "../bitmaps/alock.xbm"
#include "../bitmaps/alock_mask.xbm"

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */

struct ThemeCursor {
    const char* name;
    unsigned int width;
    unsigned int height;
    unsigned int x_hot;
    unsigned int y_hot;
    char* bits;
    char* mask;
};

static struct ThemeCursor cursors[] = {

    { "alock",
      alock_width, alock_height, alock_x_hot, alock_y_hot,
      alock_bits, alock_mask_bits },
    
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

static Cursor cursor = 0;
static XColor color_fg;
static XColor color_bg;
static int alock_cursor_theme_init(const char* args, struct aXInfo* xinfo) {

    char* color_bg_name = strdup("steelblue3");
    char* color_fg_name = strdup("grey25");
    Pixmap pixmap_cursor;
    Pixmap pixmap_cursor_mask;
    const struct ThemeCursor* theme = cursors;

    if (!xinfo || !args)
        return 0;
    
    if (strstr(args, "theme:") == args && strlen(&args[6]) > 0) {
        char* arguments = strdup(&args[6]);
        char* tmp;
        char* arg = NULL;
        for (tmp = arguments; tmp; ) {
            arg = strsep(&tmp, ",");
            if (arg) {
                const struct ThemeCursor* cursor_theme_name;
                    
                if (!strcmp(arg, "list")) {
                    for (cursor_theme_name = cursors; cursor_theme_name->name; ++cursor_theme_name) {
                        printf("%s\n", cursor_theme_name->name);
                    }
                    free(color_fg_name);
                    free(color_bg_name);
                    free(arguments);
                    exit(0);
                } else if (strstr(arg, "fg=") == arg && strlen(arg) > 4) {
                    free(color_fg_name);
                    color_fg_name = strdup(&arg[3]);
                } else if (strstr(arg, "bg=") == arg && strlen(arg) > 4) {
                    free(color_bg_name);
                    color_bg_name = strdup(&arg[3]);
                } else if (strstr(arg, "name=") == arg && strlen(arg) > 6) {
                    for (cursor_theme_name = cursors; cursor_theme_name->name; ++cursor_theme_name) {
                        if(!strcmp(cursor_theme_name->name, &arg[5])) {
                            theme = cursor_theme_name;
                            break;
                        }
                    }
                    if (!cursor_theme_name->name) {
                        printf("alock: error, couldnt find [%s]\n", arg);
                        free(color_bg_name);
                        free(color_fg_name);
                        free(arguments);
                        return 0;
                    }
                }
            }
        }
        free(arguments);
    }

    alock_alloc_color(xinfo, color_fg_name, "white", &color_fg);
    alock_alloc_color(xinfo, color_bg_name, "blank", &color_bg);
    
    free(color_fg_name);
    free(color_bg_name);
    
    pixmap_cursor = XCreateBitmapFromData(xinfo->display, xinfo->root, 
                                          theme->bits, theme->width, theme->height);
    pixmap_cursor_mask = XCreateBitmapFromData(xinfo->display, xinfo->root, 
                                               theme->mask, theme->width, theme->height);

    cursor = XCreatePixmapCursor(xinfo->display,
                                 pixmap_cursor, pixmap_cursor_mask,
                                 &color_fg, &color_bg,
                                 theme->x_hot, theme->y_hot);

    if (cursor) {
        xinfo->cursor = cursor;
        return 1;
    } else
        return 0;
}

static int alock_cursor_theme_deinit(struct aXInfo* xinfo) {
    if (!xinfo || !cursor)
        return 0;

    XFreeCursor(xinfo->display, cursor);
    return 1;
}

struct aCursor alock_cursor_theme = {
    "theme",
    alock_cursor_theme_init, 
    alock_cursor_theme_deinit
};

/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/


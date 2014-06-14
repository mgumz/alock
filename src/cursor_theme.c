/*
 * alock - cursor_theme.c
 * Copyright (c) 2005 - 2007 Mathias Gumz <akira at fluxbox dot org>
 *               2014 Arkadiusz Bokowy
 *
 * This file is a part of an alock.
 *
 * This projected is licensed under the terms of the MIT license.
 *
 * This cursor module provides:
 *  -cursor theme:name=<glyph>,fg=<color>,bg=<color>
 *
 */

#include <stdlib.h>
#include <string.h>

#ifdef HAVE_THEME
#include <X11/bitmaps/xlogo16>
#endif /* HAVE_THEME */

#include "alock.h"

#include "../bitmaps/mini.xbm"
#include "../bitmaps/mini_mask.xbm"
#include "../bitmaps/xtr.xbm"
#include "../bitmaps/xtr_mask.xbm"
#include "../bitmaps/alock.xbm"
#include "../bitmaps/alock_mask.xbm"


struct ThemeCursor {
    const char *name;
    unsigned int width;
    unsigned int height;
    unsigned int x_hot;
    unsigned int y_hot;
    char *bits;
    char *mask;
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

#ifdef HAVE_THEME
    { "xlogo16",
      xlogo16_width, xlogo16_height, xlogo16_width / 2, xlogo16_height / 2,
      xlogo16_bits, xlogo16_bits },
#endif /* HAVE_THEME */

    { NULL, 0, 0, 0, 0, NULL, NULL }
};


static Cursor *cursor = NULL;
static XColor *color_fg = NULL;
static XColor *color_bg = NULL;


static int alock_cursor_theme_init(const char *args, struct aXInfo *xinfo) {

    if (!xinfo)
        return 0;

    char *color_bg_name = NULL;
    char *color_fg_name = NULL;
    Pixmap pixmap_cursor;
    Pixmap pixmap_cursor_mask;
    const struct ThemeCursor *theme = cursors;

    if (args && strstr(args, "theme:") == args) {
        char *arguments = strdup(&args[6]);
        char *arg;
        char *tmp;
        for (tmp = arguments; tmp; ) {
            arg = strsep(&tmp, ",");
            if (strcmp(arg, "list") == 0) {
                const struct ThemeCursor *cursor;
                printf("list of available cursor themes:\n");
                for (cursor = cursors; cursor->name; cursor++)
                    printf("%s\n", cursor->name);
                exit(EXIT_SUCCESS);
            }
            if (strstr(arg, "name=") == arg) {
                const struct ThemeCursor *cursor;
                for (cursor = cursors; cursor->name; cursor++) {
                    if (!strcmp(cursor->name, &arg[5])) {
                        theme = cursor;
                        break;
                    }
                }
            }
            else if (strstr(arg, "fg=") == arg) {
                free(color_fg_name);
                color_fg_name = strdup(&arg[3]);
            }
            else if (strstr(arg, "bg=") == arg) {
                free(color_bg_name);
                color_bg_name = strdup(&arg[3]);
            }
        }
        free(arguments);
    }

    {
        int scr;

        cursor = (Cursor*)calloc(xinfo->screens, sizeof(Cursor));
        color_bg = (XColor*)calloc(xinfo->screens, sizeof(XColor));
        color_fg = (XColor*)calloc(xinfo->screens, sizeof(XColor));

        for (scr = 0; scr < xinfo->screens; scr++) {

            alock_alloc_color(xinfo, scr, color_fg_name, "white", &color_fg[scr]);
            alock_alloc_color(xinfo, scr, color_bg_name, "blank", &color_bg[scr]);

            pixmap_cursor = XCreateBitmapFromData(xinfo->display, xinfo->root[scr],
                                          theme->bits, theme->width, theme->height);
            pixmap_cursor_mask = XCreateBitmapFromData(xinfo->display, xinfo->root[scr],
                                           theme->mask, theme->width, theme->height);

            cursor[scr] = XCreatePixmapCursor(xinfo->display,
                                 pixmap_cursor, pixmap_cursor_mask,
                                 &color_fg[scr], &color_bg[scr],
                                 theme->x_hot, theme->y_hot);

        }

        if (cursor)
            xinfo->cursor = cursor;

    }

    free(color_fg_name);
    free(color_bg_name);
    return 1;
}

static int alock_cursor_theme_deinit(struct aXInfo *xinfo) {

    if (!xinfo || !cursor)
        return 0;

    int scr;
    for (scr = 0; scr < xinfo->screens; scr++)
        XFreeCursor(xinfo->display, cursor[scr]);
    free(cursor);
    free(color_bg);
    free(color_fg);

    return 1;
}


struct aCursor alock_cursor_theme = {
    "theme",
    alock_cursor_theme_init,
    alock_cursor_theme_deinit,
};

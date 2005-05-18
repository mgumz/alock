/* ---------------------------------------------------------------- *\

  file    : cursor_font.c
  author  : m. gumz <akira at fluxbox dot org>
  copyr   : copyright (c) 2005 by m. gumz

  license : see LICENSE
  
  start   : Di 17 Mai 2005 14:19:44 CEST

\* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- *\

  about :

    provide -cursor font:name

\* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- *\
  includes
\* ---------------------------------------------------------------- */
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "alock.h"

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */

struct CursorFontName {
    const char* name;
    unsigned int shape;
};

static const struct CursorFontName cursor_names[] = {
    { "num_glyphs", XC_num_glyphs },
    { "X_cursor", XC_X_cursor },
    { "arrow", XC_arrow },
    { "based_arrow_down", XC_based_arrow_down },
    { "based_arrow_up", XC_based_arrow_up },
    { "boat", XC_boat },
    { "bogosity", XC_bogosity },
    { "bottom_left_corner", XC_bottom_left_corner },
    { "bottom_right_corner", XC_bottom_right_corner },
    { "bottom_side", XC_bottom_side },
    { "bottom_tee", XC_bottom_tee },
    { "box_spiral", XC_box_spiral },
    { "center_ptr", XC_center_ptr },
    { "circle", XC_circle },
    { "clock", XC_clock },
    { "coffee_mug", XC_coffee_mug },
    { "cross", XC_cross },
    { "cross_reverse", XC_cross_reverse },
    { "crosshair", XC_crosshair },
    { "diamond_cross", XC_diamond_cross },
    { "dot", XC_dot },
    { "dotbox", XC_dotbox },
    { "double_arrow", XC_double_arrow },
    { "draft_large", XC_draft_large },
    { "draft_small", XC_draft_small },
    { "draped_box", XC_draped_box },
    { "exchange", XC_exchange },
    { "fleur", XC_fleur },
    { "gobbler", XC_gobbler },
    { "gumby", XC_gumby },
    { "hand1", XC_hand1 },
    { "hand2", XC_hand2 },
    { "heart", XC_heart },
    { "icon", XC_icon },
    { "iron_cross", XC_iron_cross },
    { "left_ptr", XC_left_ptr },
    { "left_side", XC_left_side },
    { "left_tee", XC_left_tee },
    { "leftbutton", XC_leftbutton },
    { "ll_angle", XC_ll_angle },
    { "lr_angle", XC_lr_angle },
    { "man", XC_man },
    { "middlebutton", XC_middlebutton },
    { "mouse", XC_mouse },
    { "pencil", XC_pencil },
    { "pirate", XC_pirate },
    { "plus", XC_plus },
    { "question_arrow", XC_question_arrow },
    { "right_ptr", XC_right_ptr },
    { "right_side", XC_right_side },
    { "right_tee", XC_right_tee },
    { "rightbutton", XC_rightbutton },
    { "rtl_logo", XC_rtl_logo },
    { "sailboat", XC_sailboat },
    { "sb_down_arrow", XC_sb_down_arrow },
    { "sb_h_double_arrow", XC_sb_h_double_arrow },
    { "sb_left_arrow", XC_sb_left_arrow },
    { "sb_right_arrow", XC_sb_right_arrow },
    { "sb_up_arrow", XC_sb_up_arrow },
    { "sb_v_double_arrow", XC_sb_v_double_arrow },
    { "shuttle", XC_shuttle },
    { "sizing", XC_sizing },
    { "spider", XC_spider },
    { "spraycan", XC_spraycan },
    { "star", XC_star },
    { "target", XC_target },
    { "tcross", XC_tcross },
    { "top_left_arrow", XC_top_left_arrow },
    { "top_left_corner", XC_top_left_corner },
    { "top_right_corner", XC_top_right_corner },
    { "top_side", XC_top_side },
    { "top_tee", XC_top_tee },
    { "trek", XC_trek },
    { "ul_angle", XC_ul_angle },
    { "umbrella", XC_umbrella },
    { "ur_angle", XC_ur_angle },
    { "watch", XC_watch },
    { "xterm", XC_xterm },
    { NULL, 0 }
};

/*------------------------------------------------------------------*\
   
\*------------------------------------------------------------------*/

static Cursor cursor = 0;
static XColor color_fg;
static XColor color_bg;

static int alock_cursor_font_init(const char* args, struct aXInfo* xinfo) {

    XColor tmp_color;
    char* color_bg_name = strdup("steelblue3");
    char* color_fg_name = strdup("grey25");
    unsigned int shape = 0; /* XC_X_cursor */

    if (!xinfo || !args)
        return 0;

    if (strstr(args, "font:") == args && strlen(&args[5]) > 0) {
        char* arguments = strdup(&args[5]);
        char* tmp;
        char* arg = NULL;
        for (tmp = arguments; tmp; ) {
            arg = strsep(&tmp, ",");
            if (arg) {
                const struct CursorFontName* cursor_font_name;
                    
                if (!strcmp(arg, "list")) {
                    for (cursor_font_name = cursor_names; cursor_font_name->name; ++cursor_font_name) {
                        printf("%s\n", cursor_font_name->name);
                    }
                    free(color_fg_name);
                    free(color_bg_name);
                    free(arguments);
                    exit(0);
                }
                if (strstr(arg, "fg=") == arg && strlen(arg) > 3 && strlen(&arg[3])) {
                    free(color_fg_name);
                    color_fg_name = strdup(&arg[3]);
                }
                else if (strstr(arg, "bg=") == arg && strlen(arg) > 3 && strlen(&arg[3])) {
                    free(color_bg_name);
                    color_bg_name = strdup(&arg[3]);
                }
                else {
                    /* TODO: accept  <int> as well -> strtol */
                    for (cursor_font_name = cursor_names; cursor_font_name->name; ++cursor_font_name) {
                        if(!strcmp(cursor_font_name->name, arg)) {
                            shape = cursor_font_name->shape;
                            break;
                        }
                    }
                    if (!cursor_font_name->name) {
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
    
    if((XAllocNamedColor(xinfo->display, xinfo->colormap, color_bg_name, &tmp_color, &color_bg)) == 0)
        XAllocNamedColor(xinfo->display, xinfo->colormap, "black", &tmp_color, &color_bg);
    if((XAllocNamedColor(xinfo->display, xinfo->colormap, color_fg_name, &tmp_color, &color_fg)) == 0)
        XAllocNamedColor(xinfo->display, xinfo->colormap, "white", &tmp_color, &color_fg);

    free(color_fg_name);
    free(color_bg_name);
    
    /* create cursor from X11/cursorfont.h */
    if ((cursor = XCreateFontCursor(xinfo->display, shape))) {
        XRecolorCursor(xinfo->display, cursor, &color_fg, &color_bg);
        xinfo->cursor = cursor;
    } else {
        printf("alock: error, couldnt create fontcursor [%d].\n", shape);
        return 0;
    }

    return 1;
}

static int alock_cursor_font_deinit(struct aXInfo* xinfo) {

    if (!xinfo || !cursor)
        return 0;

    XFreeCursor(xinfo->display, cursor);
    return 1;
}


struct aCursor alock_cursor_font = {
    "font",
    alock_cursor_font_init,
    alock_cursor_font_deinit 
};




/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */


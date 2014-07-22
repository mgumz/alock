# alock - Makefile.am
# Copyright (c) 2014 Arkadiusz Bokowy

AM_CFLAGS = -Wall -Wextra

bin_PROGRAMS = alock

alock_SOURCES = \
	main.c \
	utils.c \
	auth_none.c \
	input_none.c \
	input_frame.c \
	bg_none.c \
	bg_blank.c \
	cursor_none.c \
	cursor_glyph.c \
	cursor_theme.c

alock_CFLAGS = \
	@X11_CFLAGS@ \
	@XCURSOR_CFLAGS@ \
	@XPM_CFLAGS@ \
	@XRENDER_CFLAGS@ \
	@IMLIB2_CFLAGS@

alock_LDFLAGS = \
	@X11_LIBS@ \
	@XCURSOR_LIBS@ \
	@XPM_LIBS@ \
	@XRENDER_LIBS@ \
	@IMLIB2_LIBS@

if ENABLE_PAM
alock_SOURCES += auth_pam.c
endif
if ENABLE_PASSWD
alock_SOURCES += auth_passwd.c
endif
if ENABLE_HASH
alock_SOURCES += auth_hash.c
endif

if ENABLE_XRENDER
alock_SOURCES += bg_shade.c
endif
if ENABLE_IMLIB2
alock_SOURCES += bg_image.c
alock_SOURCES += cursor_image.c
else
if ENABLE_XPM
alock_SOURCES += cursor_image.c
endif
endif
if ENABLE_XCURSOR
alock_SOURCES += cursor_xcursor.c
endif

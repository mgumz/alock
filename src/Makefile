
###
#
#
$(info check for ../config.mk ...)
ifeq ($(wildcard ../config.mk),../config.mk)
$(info ok.)
include ../config.mk
else
$(error not ok. no '../config.mk' found. call 'configure'.)
endif


##########################
MAKEDEP ?= gcc -MM
CC ?= gcc
CFLAGS += -DVERSION=$(VERSION)
LDFLAGS ?=
LIBS ?= -lX11

##########################

SRC := alock.c alock_utils.c \
	  auth_none.c \
	  bg_none.c bg_blank.c \
	  cursor_none.c cursor_glyph.c cursor_theme.c
SRC_PAM := auth_pam.c
SRC_PASSWD := auth_passwd.c
SRC_HASH := auth_sha1.c \
		   auth_sha2.c \
		   auth_md5.c
SRC_XRENDER := bg_shade.c
SRC_IMLIB2 := cursor_image.c \
			 bg_image.c
ifdef WITH_XRENDER
SRC_XPM := cursor_image.c
SRC_XCURSOR := cursor_xcursor.c
endif

######################

ifdef WITH_PAM
SRC += $(SRC_PAM)
CFLAGS += $(CFLAGS_PAM)
LDFLAGS += $(LDFLAGS_PAM)
LIBS += $(LIBS_PAM)
endif

ifdef WITH_PASSWD
SRC += $(SRC_PASSWD)
CFLAGS += $(CFLAGS_PASSWD)
LDFLAGS += $(LDFLAGS_PASSWD)
LIBS += $(LIBS_PASSWD)
endif

ifdef WITH_HASH
SRC += $(SRC_HASH)
CFLAGS += $(CFLAGS_HASH)
LDFLAGS += $(LDFLAGS_HASH)
LIBS += $(LIBS_HASH)
endif

ifdef WITH_THEME
CFLAGS += $(CFLAGS_THEME) -DHAVE_THEME
endif

ifdef WITH_XRENDER
SRC += $(SRC_XRENDER)
CFLAGS += $(CFLAGS_XRENDER) -DHAVE_XRENDER
LDFLAGS += $(LDFLAGS_XRENDER)
LIBS += $(LIBS_XRENDER)
endif

ifdef WITH_XCURSOR
SRC += $(SRC_XCURSOR)
CFLAGS += $(CFLAGS_XCURSOR) -DHAVE_XCURSOR
LDFLAGS += $(LDFLAGS_XCURSOR)
LIBS += $(LIBS_XCURSOR)
endif


ifdef WITH_IMLIB2
SRC += $(SRC_IMLIB2)
CFLAGS += $(CFLAGS_IMLIB2) -DHAVE_IMLIB2
LDFLAGS += $(LDFLAGS_IMLIB2)
LIBS += $(LIBS_IMLIB2)
endif

ifdef WITH_XPM
SRC += $(SRC_XPM)
CFLAGS += $(CFLAGS_XPM) -DHAVE_XPM
LDFLAGS += $(LDFLAGS_XPM)
LIBS += $(LIBS_XPM)
endif

#######################

SOURCES = $(sort $(SRC))
OBJECTS = $(SOURCES:.c=.o)
DEPS := $(foreach deps, $(SOURCES), $(deps:.c=.d))
TARGET := alock
#######################


#######################
first: ../config.mk $(TARGET)

clean:
	@rm -fr $(OBJECTS)

distclean:
	@rm -fr $(OBJECTS) $(DEPS)

deps: $(DEPS)

$(TARGET) : $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(LIBS)

alock.o : ../config.mk

%.d : %.c
	$(MAKEDEP) -o $@ $<




ifneq ($(MAKECMDGOALS),deps)
ifneq ($(MAKECMDGOALS),first)
ifneq ($(MAKECMDGOALS),distclean)
ifneq ($(MAKECMDGOALS),clean)
     -include $(DEPS)
endif
endif
endif
endif




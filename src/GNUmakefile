##########################
$(info check for ../config.mk ...)
ifeq ($(wildcard ../config.mk),../config.mk)
$(info ok.)
include ../config.mk
endif

##########################
define funcAddModule
	ifdef WITH_$(1)
		SRC += $$(SRC_$(1))
		CFLAGS += $$(CFLAGS_$(1)) -DHAVE_$(1)
		LDFLAGS += $$(LDFLAGS_$(1))
		LIBS += $$(LIBS_$(1))
	endif
endef

define funcStandalone
a$(1) : auth_$(1).c
	$$(CC) -o $$@ -DSTAND_ALONE $$(CFLAGS) $$<
endef

##########################
MAKEDEP ?= gcc -MM
CC ?= gcc
CFLAGS += -Wall -Wmissing-prototypes -Wmissing-declarations -DVERSION=$(VERSION)
LDFLAGS ?=
LIBS ?= -lX11
##########################

SRC = 			alock.c alock_utils.c \
				auth_none.c \
				bg_none.c bg_blank.c \
				cursor_none.c cursor_glyph.c cursor_theme.c
SRC_PAM = 		auth_pam.c
SRC_PASSWD = 	auth_passwd.c
SRC_HASH = 	auth_sha1.c \
				auth_sha2.c \
				auth_md5.c \
				auth_wpool.c
SRC_XRENDER = 	bg_shade.c
SRC_IMLIB2 = 	cursor_image.c \
			 	bg_image.c
SRC_XPM = 		cursor_image.c
SRC_XCURSOR = 	cursor_xcursor.c

######################

MODULES = PAM PASSWD HASH XRENDER XCURSOR IMLIB2 XPM
STAND_ALONES = $(subst auth_,,$(SRC_HASH:.c=))

$(foreach module,$(MODULES),$(eval $(call funcAddModule,$(module))))

#######################
SOURCES = $(sort $(SRC))
OBJECTS = $(SOURCES:.c=.o)
DEPS    = $(foreach d,$(SOURCES), .$(d:.c=.d))
TARGET  = alock
#######################


#######################

$(TARGET) : $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(LIBS)

standalones : $(foreach s,$(STAND_ALONES), a$(s))

$(foreach s,$(STAND_ALONES),$(eval $(call funcStandalone,$(s))))


clean:
	@rm -fv *.o

distclean:
	rm -fv $(TARGET) $(foreach binary,$(STAND_ALONES),a$(binary)) *.o .*.d

deps: $(DEPS)

.%.d : %.c
	$(MAKEDEP) $(CFLAGS) -o $@ $<
	@echo "$(<:.c=.o): ../config.mk" >> $@

#######################

ifneq ($(MAKECMDGOALS),deps)
ifneq ($(MAKECMDGOALS),first)
ifneq ($(MAKECMDGOALS),distclean)
ifneq ($(MAKECMDGOALS),clean)
     -include $(DEPS)
endif
endif
endif
endif

#######################
#######################


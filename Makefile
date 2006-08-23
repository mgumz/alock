
ifeq ($(wildcard config.mk),config.mk)
include config.mk
ifdef WITH_DOCS
first: alock docs
else
first: alock
endif
else
first:
	@echo "run './configure' first"
endif

clean:
	$(MAKE) -C src clean

distclean:
	$(MAKE) -C src distclean
	rm -f config.mk config.log tmp.c

install : alock
	mkdir -p $(DESTDIR)$(prefix)/bin/
	cp -fv src/alock $(DESTDIR)$(prefix)/bin/alock
	chmod 755 $(DESTDIR)$(prefix)/bin/alock
	mkdir -p $(DESTDIR)$(prefix)/man/man1/
	cp -fv alock.1 $(DESTDIR)$(prefix)/man/man1/alock.1
	chmod 444 $(DESTDIR)$(prefix)/man/man1/alock.1
	mkdir -p $(DESTDIR)$(prefix)/share/alock/xcursors
	cp -fv contrib/xcursor-alock contrib/xcursor-gentoo \
		contrib/xcursor-fluxbox contrib/xcursor-pekwm \
		$(DESTDIR)$(prefix)/share/alock/xcursors/
	mkdir -p $(DESTDIR)$(prefix)/share/alock/bitmaps
	cp -fv bitmaps/alock.xbm bitmaps/alock_mask.xbm \
		bitmaps/mini.xbm bitmaps/mini_mask.xbm \
		bitmaps/xtr.xbm bitmaps/xtr_mask.xbm \
		$(DESTDIR)$(prefix)/share/alock/bitmaps/
	chmod 444 $(DESTDIR)$(prefix)/share/alock/xcursors/*
	chmod 444 $(DESTDIR)$(prefix)/share/alock/bitmaps/*
	cp -fv README LICENSE CHANGELOG \
		$(DESTDIR)$(prefix)/share/alock/
	chmod 444 $(DESTDIR)$(prefix)/share/alock/README
	chmod 444 $(DESTDIR)$(prefix)/share/alock/LICENSE
	chmod 444 $(DESTDIR)$(prefix)/share/alock/CHANGELOG


alock :
	$(MAKE) -C src

ASCIIDOC := asciidoc
XMLTO := xmlto
FOP := fop

docs: alock.1 alock.html

alock.xml : alock.txt
	$(ASCIIDOC) -d manpage -b docbook -o $@ $<

alock.1 : alock.xml
	$(XMLTO) man $<

alock.html : alock.txt
	$(ASCIIDOC) -d manpage -b xhtml11 -o $@ $<

alock.pdf : alock.xml
	$(XMLTO) fo $< && $(FOP) $(@:.pdf=.fo) $@ 



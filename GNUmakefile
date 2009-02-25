
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

ifdef WITH_DOCS
install : alock docs
else
install : alock
endif
	install -d \
		$(DESTDIR)$(prefix)/bin \
		$(DESTDIR)$(prefix)/share/alock/xcursors \
		$(DESTDIR)$(prefix)/share/alock/bitmaps
	install -m755 \
		src/alock \
		$(DESTDIR)$(prefix)/bin/alock
	if [ -f alock.1 ]; then \
		install -d \
			$(DESTDIR)$(prefix)/share/man/man1 ; \
		install -m444 \
			alock.1 \
			$(DESTDIR)$(prefix)/share/man/man1/alock.1 ; \
	else \
		echo ",-----------------------------------------------------------,"; \
		echo "| not installing the documentation because it was not built |"; \
		echo "| install 'asciidoc' and run './configure && make' again    |"; \
		echo "\`-----------------------------------------------------------'"; \
	fi
	install -m444 \
		contrib/xcursor-alock contrib/xcursor-gentoo \
		contrib/xcursor-fluxbox contrib/xcursor-pekwm \
		$(DESTDIR)$(prefix)/share/alock/xcursors/
	install -m444 \
		bitmaps/alock.xbm bitmaps/alock_mask.xbm \
		bitmaps/mini.xbm bitmaps/mini_mask.xbm \
		bitmaps/xtr.xbm bitmaps/xtr_mask.xbm \
		$(DESTDIR)$(prefix)/share/alock/bitmaps/
	install -m444 \
		README.txt LICENSE.txt CHANGELOG.txt \
		$(DESTDIR)$(prefix)/share/alock/
	@if `./src/alock -auth list | grep passwd > /dev/null`; then      \
		echo ",-------------------------------------------------,"; \
		echo "| it seems that you have compiled 'alock' with    |"; \
		echo "| 'shadow' support. to use that binary you have   |"; \
		echo "| set the 'suid' - bit, something like:           |"; \
		echo "|                                                 |"; \
		echo "|    $$> chown root:root \$$prefix/bin/alock         |";\
		echo "|    $$> chmod 4111 \$$prefix/bin/alock              |";\
		echo "|                                                 |"; \
		echo "| if not you ll get 'permission denied' problems. |"; \
		echo "\`-------------------------------------------------'";\
	fi

alock :
	$(MAKE) -C src

ifdef WITH_DOCS

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

endif


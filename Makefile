
ifeq ($(wildcard config.mk),config.mk)
include config.mk
first: alock docs
else
first:
	@echo "run './configure' first"
endif

clean:
	$(MAKE) -C src clean

distclean:
	$(MAKE) -C src distclean
	rm -f config.mk config.log tmp.c


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


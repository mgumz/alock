In order to regenerate manual page from the txt source file, it is required
to have a recent asciidoc package (http://www.methods.co.nz/asciidoc/) and
xmlto (https://fedorahosted.org/xmlto/browser). The result man page and
altered .txt file should be checked into the git repository.

man:

  $> asciidoc -b docbook -d manpage alock.txt
  $> xmlto man alock.xml

pdf:

  $> asciidoc -b docbook -d manpage alock.txt
  $> docbook2pdf alock.xml

docbook:

  $> asciidoc -b docbook-sgml -d manpage alock.txt

html:

  $> asciidoc -b xhtml -d manpage alock.txt

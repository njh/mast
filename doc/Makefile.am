
ASCIIDOC = @ASCIIDOC@
XMLTO = @XMLTO@

man_MANS = mastcast.1 mastclient.1


HTMLMANPAGES = $(man_MANS:%=%.html)
XMLMANPAGES = $(man_MANS:%=%.xml)
ASCMANPAGES = $(man_MANS:%=%.asc)


noinst_DATA = $(HTMLMANPAGES) $(XMLMANPAGES) README.html INSTALL.html

README.html: ../README
	$(ASCIIDOC) -d article -b html -o $@ $<
	
INSTALL.html: ../INSTALL
	$(ASCIIDOC) -d article -b html -o $@ $<


$(HTMLMANPAGES): %.html: %.asc
	$(ASCIIDOC) -d manpage -b html -o $@ $<

$(XMLMANPAGES): %.xml: %.asc
	$(ASCIIDOC) -d manpage -b docbook -o $@ $<

$(man_MANS): %: %.xml
	$(XMLTO) man $<
	
CLEANFILES = $(man_MANS) $(HTMLMANPAGES) $(XMLMANPAGES) README.html INSTALL.html \
			 manpage.links manpage.refs

EXTRA_DIST = $(ASCMANPAGES)

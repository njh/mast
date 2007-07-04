
man_MANS = mast_cast.1 mast_client.1

HTMLPAGES =  README.html INSTALL.html
HTMLMANPAGES = $(man_MANS:%=%.html)
XMLMANPAGES = $(man_MANS:%=%.xml)
ASCMANPAGES = $(man_MANS:%=%.asc)


## Tool locations
ASCIIDOC = @ASCIIDOC@
XMLTO = @XMLTO@


## Only re-build documentation if tools are available
if BUILD_DOC 

INSTALL.html: ../INSTALL
	$(ASCIIDOC) -d article -b xhtml11 -o $@ $<

README.html: ../README
	$(ASCIIDOC) -d article -b xhtml11 -o $@ $<

$(HTMLMANPAGES): %.html: %.asc
	$(ASCIIDOC) -d manpage -b xhtml11 -o $@ $<

$(XMLMANPAGES): %.xml: %.asc
	$(ASCIIDOC) -d manpage -b docbook -o $@ $<

$(man_MANS): %: %.xml
	$(XMLTO) man $<

endif



dist_doc_DATA = $(HTMLPAGES) $(HTMLMANPAGES)
DISTCLEANFILES = $(man_MANS) $(HTMLMANPAGES)
CLEANFILES = $(XMLMANPAGES) manpage.links manpage.refs
EXTRA_DIST = $(ASCMANPAGES) $(man_MANS) $(HTMLPAGES) $(HTMLMANPAGES)

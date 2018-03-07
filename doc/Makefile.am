
## Manual pages
man_MANS = \
	mast_info.1 \
	mast_cast.1 \
	mast_filecast.1 \
	mast_rawcast.1 \
	mast_record.1 \
	mast_rawrecord.1
	

HTMLPAGES =  README.html INSTALL.html
HTMLMANPAGES = $(man_MANS:%=%.html)
XMLMANPAGES = $(man_MANS:%=%.xml)
ASCMANPAGES = $(man_MANS:%=%.asc)


## Tool Configuration
ASCIIDOC = @ASCIIDOC@ \
	-a revision="@PACKAGE_VERSION@" \
	-a theme=mast \
	-a linkcss \
	-a stylesdir=. 
XMLTO = @XMLTO@


## Only re-build documentation if tools are available
if BUILD_DOC 

INSTALL.html: ../INSTALL
	$(ASCIIDOC) -d article -o $@ $<

README.html: ../README.md
	$(ASCIIDOC) -d article -o $@ $<

$(HTMLMANPAGES): %.html: %.asc
	$(ASCIIDOC) -d manpage -o $@ $<

$(XMLMANPAGES): %.xml: %.asc
	$(ASCIIDOC) -d manpage -b docbook -o $@ $<

$(man_MANS): %: %.xml
	$(XMLTO) man --skip-validation $<

endif



dist_doc_DATA = $(HTMLPAGES) $(HTMLMANPAGES)
DISTCLEANFILES = $(man_MANS) $(HTMLPAGES) $(HTMLMANPAGES)
CLEANFILES = $(XMLMANPAGES) manpage.links manpage.refs
EXTRA_DIST = $(ASCMANPAGES) $(man_MANS) $(HTMLPAGES) $(HTMLMANPAGES)


ASCIIDOC = asciidoc
SGML2MAN = docbook2man

man_MANS = pcm6cast.1 pcm6client.1 pcm6archive.1 \
           pcm6shout.1 pcm6shout.conf.5


HTMLMANPAGES = $(man_MANS:%=%.html)
SGMLMANPAGES = $(man_MANS:%=%.sgml)
ASCMANPAGES = $(man_MANS:%=%.asc)


doc: $(man_MANS) $(HTMLMANPAGES) $(SGMLMANPAGES) README.html INSTALL.html

README.html: ../README
	$(ASCIIDOC) -d article -b html -o $@ $<
	
INSTALL.html: ../INSTALL
	$(ASCIIDOC) -d article -b html -o $@ $<


$(HTMLMANPAGES): %.html: %.asc
	$(ASCIIDOC) -d manpage -b html -o $@ $<

$(SGMLMANPAGES): %.sgml: %.asc
	$(ASCIIDOC) -d manpage -b docbook-sgml -o $@ $<

$(man_MANS): %: %.sgml
	$(SGML2MAN) $<
	
doc-clean:
	rm -f $(man_MANS) $(HTMLMANPAGES) $(SGMLMANPAGES) 
	rm -f README.html INSTALL.html
	rm -f manpage.links manpage.refs


EXTRA_DIST = $(man_MANS) $(ASCMANPAGES) $(SGMLMANPAGES) \
             $(HTMLMANPAGES) README.html INSTALL.html


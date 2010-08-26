.PHONY: idehelp

REL=$(shell source ../tools/docconfig_clovis.sh; echo $$RELEASE)

DOCID=ideguide
DOCTITLE=IDE User Guide
DOCURL=Doc:sdk_$(REL)\/ideguide
PARTNUMBER=000106
INPUT_WIKI_FORMAT=singlepage
OUTPUT_HTML_FORMAT=multipage1

DOCIDHELP=idehelp
OUTPUT_HTML_FORMAT_HELP=multipage3

# Makefile for generating html and pdf doc for IDE Guide

include ../tools/Makefile.in

idehelp: 
	mkdir -p ../html/$(DOCIDHELP)
	sed -e "s/DOCID=/DOCID=$(DOCID)/g" \
        -e "s/DOCTITLE=/DOCTITLE=\"$(DOCTITLE)\"/g" \
        -e "s/WIKI_URL=/WIKI_URL=$(DOCURL)/g" \
        -e "s/INPUT_WIKI_FORMAT=/INPUT_WIKI_FORMAT=$(INPUT_WIKI_FORMAT)/g" \
        -e "s/HTMLDIR=/HTMLDIR=\.\.\/html\/$(DOCIDHELP)/g" \
        -e "s/PARTNUMBER=/PARTNUMBER=$(PARTNUMBER)/g" ../tools/docconfig_clovis.sh > docconfig_clovis.sh
	source docconfig_clovis.sh; \
	cp ../tools/Doxyfile_clovis . ; \
	grep -v "$(DOCID)" Doxyfile_clovis | \
	   	sed -e "s/[$$]docname[$$]/$(DOCIDHELP)/g" \
	            -e "s/clovis_header.html//g" \
	            -e "s/OpenClovis SDK/OpenClovis IDE/g" > Doxyfile; \
	PROJECT_NUMBER="Release $$RELEASE - rev$$REVISION"; \
	if [ "$(BUILD_NUMBER)" ]; then \
	    PROJECT_NUMBER="$$PROJECT_NUMBER - build $(BUILD_NUMBER)"; \
	fi; \
	sed -e "s/^PROJECT_NUMBER\\s*=/PROJECT_NUMBER = \"$$PROJECT_NUMBER\"/g" Doxyfile > Doxyfile.mod
	sed -e "s/OUTPUT_HTML_FORMAT=/OUTPUT_HTML_FORMAT=$(OUTPUT_HTML_FORMAT_HELP)/g" \
	        docconfig_clovis.sh > docconfig.sh    
	../tools/convert_wiki_to_html_pdf.sh -c docconfig.sh -p
	grep -v "bg9.png" ../tools/clovisdoc.css > ../html/$(DOCIDHELP)/clovisdoc.css; 
	cp ../html/$(DOCIDHELP)/index.html ../html/$(DOCIDHELP)/toc.xml
	./create_toc_xml.sh ../html/$(DOCIDHELP)/toc.xml; \
	./reformat_idehelp_html_pages.sh

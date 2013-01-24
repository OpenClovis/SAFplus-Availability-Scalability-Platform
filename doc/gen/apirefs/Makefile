.PHONY: all html pdf

DOXYFILE?=Doxyfile
DOCID=apirefs
DOCTITLE=SAFplus Platform API Reference Guide
PARTNUMBER=000109

include ../docconfig.mk

HTMLOUTPUT := ../../html/$(DOCID)

# Makefile for generating html and pdf doc for API Reference Guide

all: html

html:
	mkdir -p $(HTMLOUTPUT)
	mkdir -p tmp
	sed "s/DOCTITLE/$$PRODUCT_NAME $(DOCTITLE)/g" clovis_header.html.in > tmp/clovis_header.html
	sed_cmd="s/^PROJECT_NUMBER\\s*=\\s*\"([^\\\"]*)\"/PROJECT_NUMBER = \"$$PROJECT_NUMBER\"/" && sed -r -e "$$sed_cmd" $(DOXYFILE) > tmp/Doxyfile;
	./create_error_codes_page.sh > errorcodes.txt
	doxygen tmp/Doxyfile


old_html:
	mkdir -p ../html/$(DOCID)
	cp $(DOCTOOLS_DIR)/docconfig_clovis.sh .      
	source docconfig_clovis.sh; \
	./create_error_codes_page.sh > errorcodes.txt; \
	sed "s/DOCTITLE/$$PRODUCT_NAME $(DOCTITLE)/g" $(DOCTOOLS_DIR)/clovis_header.html.in > clovis_header.html; \
	PROJECT_NUMBER="Release $$RELEASE - rev$$REVISION"; \
	if [ "$(BUILD_NUMBER)" ]; then \
	    PROJECT_NUMBER="$$PROJECT_NUMBER - build $(BUILD_NUMBER)"; \
	fi; \
	sed_cmd="s/^PROJECT_NUMBER\\s*=\\s*\"([^\\\"]*)\"/PROJECT_NUMBER = \"$$PROJECT_NUMBER\"/" && sed -r -e "$$sed_cmd" $(DOXYFILE) > Doxyfile.mod; \
	doxygen Doxyfile.mod  
# && ./reformat_apirefs_html_pages.sh; \
	cp $(DOCTOOLS_DIR)/OpenClovis_Logo.png $(DOCTOOLS_DIR)/bg9.png ../html/$(DOCID)
	cp latex/refman.tex latex/refman.orig
	cp latex/index.tex latex/index.orig


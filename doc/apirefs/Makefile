.PHONY: all html pdf

DOXYFILE?=	Doxyfile
DOCID=apirefs
DOCTITLE=API Reference Guide
PARTNUMBER=000109
DOCTOOLS_DIR=../tools

# Makefile for generating html and pdf doc for API Reference Guide

all: html pdf

html:
	mkdir -p ../html/$(DOCID)
	cp $(DOCTOOLS_DIR)/docconfig_clovis.sh .      
	source docconfig_clovis.sh; \
	./create_error_codes_page.sh > errorcodes.txt; \
	sed "s/DOCTITLE/$$PRODUCT_NAME $(DOCTITLE)/g" $(DOCTOOLS_DIR)/clovis_header.html.in > clovis_header.html; \
	PROJECT_NUMBER="Release $$RELEASE - rev$$REVISION"; \
	if [ "$(BUILD_NUMBER)" ]; then \
	    PROJECT_NUMBER="$$PROJECT_NUMBER - build $(BUILD_NUMBER)"; \
	fi; \
	sed_cmd="s/^PROJECT_NUMBER\\s*=\\s*\"([^\\\"]*)\"/PROJECT_NUMBER = \"$$PROJECT_NUMBER\"/" && \
		sed -r -e "$$sed_cmd" $(DOXYFILE) > Doxyfile.mod; \
	doxygen Doxyfile.mod && ./reformat_apirefs_html_pages.sh; \
	cp $(DOCTOOLS_DIR)/OpenClovis_Logo.png $(DOCTOOLS_DIR)/bg9.png ../html/$(DOCID)
	cp latex/refman.tex latex/refman.orig
	cp latex/index.tex latex/index.orig

pdf:    html
	mkdir -p ../pdf
	source docconfig_clovis.sh; \
	cp latex/refman.orig latex/refman.tex; \
	cp latex/index.orig latex/index.tex; \
	cp $(DOCTOOLS_DIR)/refman.tex latex/refman_head.tex; \
	cp $(DOCTOOLS_DIR)/docparams.tex latex/tmp.tex; \
	sed -e "s/DOCUMENT/$(DOCTITLE)/g" \
	    -e "s/PARTNUMBER/$(PARTNUMBER)/g" \
	    -e "s/PRODUCT_NAME/$$PRODUCT_NAME/g" \
	    -e "s/RELEASE/$$RELEASE/g" \
	    -e "s/REVISION/$$REVISION/g" \
            -e "s/DATE/`date +%Y-%B-%d`/g" latex/tmp.tex > latex/docparams.tex;  \
	cp $(DOCTOOLS_DIR)/clovisdoc.sty latex; \
	cp $(DOCTOOLS_DIR)/doxygen.sty latex; \
	cp $(DOCTOOLS_DIR)/openclovis_logo.png latex; \
	cp $(DOCTOOLS_DIR)/bg3.png latex; \
	(cd latex; ../../tools/reformat_apirefs_latex_pages.sh; make; make; make); \
	cp latex/refman.pdf ../pdf/$${COMMON_PREFIX}_$(DOCID)_$$RELEASE.pdf

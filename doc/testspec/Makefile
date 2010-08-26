.PHONY: all html pdf

DOCNAME=testspec
DOCTITLE=Test Specification
DOCURL=Doc:testspec
PARTNUMBER=000111
INPUT_FORMAT=multipage

# Makefile for generating html and pdf doc for Test Specification

all: html pdf

html:
	mkdir -p ../html/$(DOCNAME)
	cp ../config.sh .      
	source config.sh; \
        $$DOCTOOLS_DIR/create_doxygen_doc.sh $(DOCNAME) "$(DOCTITLE)" $(DOCURL) $(INPUT_FORMAT) $$OUTPUT_FORMAT; \
	grep -v $(DOCNAME) ../Doxyfile.wiki_style.in | \
	   	sed "s/[$$]docname[$$]/$(DOCNAME)/g" > Doxyfile ; \
	PROJECT_NUMBER="Release $$RELEASE - rev$$REVISION"; \
	if [ "$(BUILD_NUMBER)" ]; then \
	    PROJECT_NUMBER="$$PROJECT_NUMBER - build $(BUILD_NUMBER)"; \
	fi; \
	sed_cmd="s/^PROJECT_NUMBER\\s*=\\s*\"([^\\\"]*)\"/PROJECT_NUMBER = \"$$PROJECT_NUMBER\"/" && \
		sed -r -e "$$sed_cmd" Doxyfile > Doxyfile.mod; \
	doxygen Doxyfile.mod; \
	cp $$DOCTOOLS_DIR/OpenClovis_Logo.png $$DOCTOOLS_DIR/bg9.png ../html/$(DOCNAME); \
	cp OpenClovis_Note.png ../html/$(DOCNAME)/OpenClovis_Note.png; \
	cp OpenClovis_Caution.png ../html/$(DOCNAME)/OpenClovis_Caution.png; \
	cp OpenClovis_Info.png ../html/$(DOCNAME)/OpenClovis_Info.png; \
	cp latex/index.tex latex/index.orig
 
pdf:
	mkdir -p ../pdf
	source config.sh; \
	cp latex/index.orig latex/index.tex; \
        $$DOCTOOLS_DIR/reformat_wiki_latex_pages.awk latex/index.tex > latex/tmp.tex; \
        mv latex/tmp.tex latex/index.tex; \
	cp $$DOCTOOLS_DIR/refman.tex latex; \
	cp docparams.tex latex/tmp.tex; \
	sed -e "s/DOCUMENT/$(DOCTITLE)/g" \
	    -e "s/PARTNUMBER/$(PARTNUMBER)/g" \
            -e "s/DATE/`date +%Y-%B-%d`/g" latex/tmp.tex > latex/docparams.tex;  \
	cp clovisdoc.sty latex; \
	cp $$DOCTOOLS_DIR/doxygen.sty latex; \
	cp $$DOCTOOLS_DIR/openclovis_logo.png latex; \
	cp $$DOCTOOLS_DIR/bg3.png latex; \
	cp *.png latex; \
	(cd latex; make; make; make); \
	cp latex/refman.pdf ../pdf/$${COMMON_PREFIX}_$(DOCNAME)-$$RELEASE.pdf


REL=1.0
DOCID=ASP_Runtime_Director_Users_Guide
WIKI_IP=localhost:8080

ArdOverrides = DOCID=$(DOCID) DOCTITLE="ASP Runtime Director User's Guide" WIKI_URL="Doc:Sdk_4.0/awdguideCrawler" INPUT_WIKI_FORMAT=multipage HTMLDIR=../html/$(DOCID) PARTNUMBER=0002 WIKI_IP=$(WIKI_IP) RELEASE=1.0 REVISION=02 OUTPUT_HTML_FORMAT=multipage2

HtmlOverrides = HTMLDIR=../html/$(DOCID) OUTPUT_HTML_FORMAT=multipage2

#PdfOverrides = OUTPUT_HTML_FORMAT=singlepage PDFDIR=../pdf/ASP_Runtime_Director

all: html  #pdf


init: 
	mkdir -p ../html/$(DOCID)
	mkdir -p ../pdf/$(DOCID)

	cp ../tools/Doxyfile_clovis . ; \
	grep -v "$(DOCID)" Doxyfile_clovis | \
	   	sed "s/[$$]docname[$$]/$(DOCID)/g" > Doxyfile; \
	PROJECT_NUMBER="Release $$RELEASE - rev$$REVISION"; \
	if [ "$(BUILD_NUMBER)" ]; then \
	    PROJECT_NUMBER="$$PROJECT_NUMBER - build $(BUILD_NUMBER)"; \
	fi; \
	sed -e "s/^PROJECT_NUMBER\\s*=/PROJECT_NUMBER = \"$$PROJECT_NUMBER\"/g" Doxyfile > Doxyfile.mod

#pdf: init
#	mkdir -p ../pdf/$(DOCID)
#	touch nothing
#	(source ../tools/docconfig_clovis.sh; export $(ArdOverrides); export $(PdfOverrides); ../tools/convert_wiki_to_html_pdf.sh  -c nothing)


html: init
	mkdir -p ../html/$(DOCID)
	touch nothing
	(source ../tools/docconfig_clovis.sh; export $(ArdOverrides); ../tools/convert_wiki_to_html_pdf.sh -c nothing)

clean:
	rm -f *.wiki *.txt
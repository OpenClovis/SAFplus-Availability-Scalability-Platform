REL=$(shell source ../tools/docconfig_clovis.sh; echo $$RELEASE)

DOCID=DynamicHA
DOCTITLE=Dynamic HA Demo Release $(REL)
DOCURL=Doc:Sdk_3.1\/evalguide\/dhaDemo
PARTNUMBER=0001
INPUT_WIKI_FORMAT=singlepage
OUTPUT_HTML_FORMAT=singlepage

init:
	mkdir -p ../html/$(DOCID)
	mkdir -p ../pdf
	sed -e "s/DOCID=/DOCID=$(DOCID)/g" \
        -e "s/DOCTITLE=/DOCTITLE=\"$(DOCTITLE)\"/g" \
        -e "s/WIKI_URL=/WIKI_URL=$(DOCURL)/g" \
        -e "s/INPUT_WIKI_FORMAT=/INPUT_WIKI_FORMAT=$(INPUT_WIKI_FORMAT)/g" \
        -e "s/HTMLDIR=/HTMLDIR=\.\.\/html\/$(DOCID)/g" \
        -e "s/PARTNUMBER=/PARTNUMBER=$(PARTNUMBER)/g" ../tools/docconfig_clovis.sh > docconfig_clovis.sh
	source docconfig_clovis.sh; \
	cp ../tools/Doxyfile_clovis . ; \
	grep -v "$(DOCID)" Doxyfile_clovis | \
	   	sed "s/[$$]docname[$$]/$(DOCID)/g" > Doxyfile; \
	PROJECT_NUMBER="Release $$RELEASE - rev$$REVISION"; \
	if [ "$(BUILD_NUMBER)" ]; then \
	    PROJECT_NUMBER="$$PROJECT_NUMBER - build $(BUILD_NUMBER)"; \
	fi; \
	sed -e "s/^PROJECT_NUMBER\\s*=/PROJECT_NUMBER = \"$$PROJECT_NUMBER\"/g" Doxyfile > Doxyfile.mod

pdf: init
	sed -e "s/OUTPUT_HTML_FORMAT=/OUTPUT_HTML_FORMAT=singlepage/g" \
	        docconfig_clovis.sh > docconfig.sh    
	../tools/convert_wiki_to_html_pdf.sh  -c docconfig.sh

clean:
	rm -f *.wiki *.txt
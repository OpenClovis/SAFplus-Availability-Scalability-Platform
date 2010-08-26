# Makefile to generate html and pdf doc for wiki-based documents


SKIPDOWNLOAD= #-g

.PHONY: all html pdf init

all: pdf html

init:
	mkdir -p html
	mkdir -p pdf
	mkdir -p temp
	cp Doxyfile temp/Doxyfile.mod

#	source docconfig.sh;


pdf: init
	sed -e "s/OUTPUT_HTML_FORMAT=/OUTPUT_HTML_FORMAT=singlepage/g" docconfig.sh > temp_docconfig_pdf.sh    
	(cd temp; ../../tools/convert_wiki_to_html_pdf.sh $(SKIPDOWNLOAD) -c ../temp_docconfig_pdf.sh ) 

# Skip downloading: (cd temp; ../../tools/convert_wiki_to_html_pdf.sh -g -c ../temp_docconfig_2.sh ) 
html: init
	sed -e "s/OUTPUT_HTML_FORMAT=/OUTPUT_HTML_FORMAT=multipage2/g" docconfig.sh > temp_docconfig_2.sh    
	(cd temp; ../../tools/convert_wiki_to_html_pdf.sh $(SKIPDOWNLOAD) -p -c ../temp_docconfig_2.sh ) 
	(cd html; tar cvfz ../ArdGuide.tgz ArdGuide)
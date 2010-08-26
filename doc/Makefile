.PHONY: pkg install html pdf

PKG_DIR?=	../pkg
DOC_DIR=	$(PKG_DIR)/doc
HTML_DIR=	$(DOC_DIR)/html
PDF_DIR=	$(DOC_DIR)/pdf

DOXYFILE?=	Doxyfile

pkg: install

init:
	@echo "================================"
	@echo "First pass"
	@echo "================================"

	mkdir -p html
	source tools/docconfig_clovis.sh; \
	PROJECT_NUMBER="Release $$RELEASE"; \
	if [ "$(BUILD_NUMBER)" ]; then \
	    PROJECT_NUMBER="$$PROJECT_NUMBER - build $(BUILD_NUMBER)"; \
	fi; \
	sed_cmd="s/^PROJECT_NUMBER\\s*=\\s*\"([^\\\"]*)\"/PROJECT_NUMBER = \"$$PROJECT_NUMBER\"/" && \
		sed -r -e "$$sed_cmd" $(DOXYFILE) > Doxyfile.mod; \
	sed -r -e "s/DATE/on `date` /g" ./tools/clovis_footer.html.in > ./tools/clovis_footer.html
	sed -r -e "s/DOCTITLE/OpenClovis SDK/g" ./tools/clovis_header.html.in > ./tools/clovis_header.html
	doxygen Doxyfile.mod
	cp ./tools/OpenClovis_Logo.png ./tools/bg9.png ./html
	
pdf:
	mkdir -p pdf
	make -C aspconsole pdf
	make -C apirefs pdf
	make -C relnotes pdf
	make -C safcompliance pdf
	make -C installguide pdf
	make -C tutorial pdf
	make -C ideguide pdf
	make -C sdkguide pdf
	make -C evalguide pdf
	make -C logtoolguide pdf

html:
	make -C aspconsole html
	make -C apirefs html
	make -C relnotes html
	make -C safcompliance html
	make -C installguide html
	make -C tutorial html
	make -C ideguide html
	make -C sdkguide html
	make -C evalguide html
	make -C logtoolguide html
                
	@echo "================================"
	@echo "Second pass"
	@echo "================================"

	doxygen Doxyfile.mod
	make -C aspconsole html
	make -C apirefs html
	
	for docdir in relnotes safcompliance installguide tutorial ideguide sdkguide evalguide logtoolguide; do \
	   (cd $$docdir; doxygen Doxyfile.mod); \
	   (cd html/$$docdir; ../../tools/reformat_wiki_html_pages.sh); \
	done
                
idehelp:
	make -C ideguide idehelp
	rsync -avDH html/idehelp/ ../IDE/plugins/com.clovis.cw.help/contents/
	rsync -avDH html/idehelp/toc.xml ../IDE/plugins/com.clovis.cw.help/toc.xml
               
install: init pdf html idehelp
	mkdir -p $(HTML_DIR)
	rsync -avDH html/ $(HTML_DIR)
	mkdir -p $(PDF_DIR)
	rsync -avDH pdf/ $(PDF_DIR)
 

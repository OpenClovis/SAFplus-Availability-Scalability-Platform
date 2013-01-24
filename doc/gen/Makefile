.PHONY: pkg install html pdf

html:
	make -C apirefs html

idehelp:
	make -C ideguide idehelp
	rsync -avDH html/idehelp/ ../IDE/plugins/com.clovis.cw.help/contents/
	rsync -avDH html/idehelp/toc.xml ../IDE/plugins/com.clovis.cw.help/toc.xml

install: html
	mkdir -p $(HTML_DIR)
	rsync -avDH html/ $(HTML_DIR)
	mkdir -p $(PDF_DIR)
	rsync -avDH pdf/ $(PDF_DIR)

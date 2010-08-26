SHELL = /bin/sh
RM = /bin/rm -rf
RST2HTML = rst2html.py

STYLEDIR = .
WEBHOST = kid-templating.org
WEBPATH = /var/projects/doc/kid

version_file = .version
style_files = default.css custom.css
src_files = index.txt language.txt guide.txt notes.txt readme.txt
doc_files = index.html language.html guide.html notes.html readme.html
publish_files = $(src_files) $(doc_files) $(style_files)
clean_files = $(doc_files) $(version_file) readme.txt .*.html

VERSION = $(shell grep '__version__ = ' ../kid/__init__.py | perl -p -e 's/.*"(.{1,3}).*"/\1/')

all: $(version_file) $(doc_files)

$(style_files): ;

$(version_file):
				echo $(VERSION) > .version

%.html: %.txt
				$(RST2HTML) --exit-status=3 --stylesheet=custom.css --link-stylesheet $< $@

clean: FORCE
				-$(RM) $(clean_files)

publish: all
				-ssh $(WEBHOST) "mkdir -p $(WEBPATH)/$(VERSION)"
				scp -p $(publish_files) $(WEBHOST):$(WEBPATH)/$(VERSION)/

readme.txt:
				cp ../README readme.txt

FORCE:

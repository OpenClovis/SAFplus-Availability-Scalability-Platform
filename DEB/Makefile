deb_install:
	$(eval REQ_FILES:=$(filter-out $(PWD)/src/,$(filter-out $(PWD)/Makefile,$(filter-out $(PWD)/debian/, $(wildcard $(PWD)/*/)))))
	mkdir -p $(DESTDIR)/$(PREFIX)
	for value in $(REQ_FILES);do \
	cp -r $$value $(DESTDIR)/$(PREFIX); \
	done
	cp -r  $(PWD)/src $(DESTDIR)/$(PREFIX)/..
deb_clean:
	rm -rf debian/$(PACKAGENAME).*debhelper
	rm -rf debian/$(PACKAGENAME).substvars
deb_build:
	echo "Not required"


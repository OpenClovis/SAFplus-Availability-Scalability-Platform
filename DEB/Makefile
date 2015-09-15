deb_install:
	$(eval REQ_FILES:=$(filter-out $(PWD)/debian/, $(wildcard $(PWD)/*/)))
	mkdir -p $(DESTDIR)/$(PREFIX)
	for value in $(REQ_FILES);do \
        cp -r $$value $(DESTDIR)/$(PREFIX); \
        done
	# Need to uncomment the below line to include git repository in the SAFPlus Debian package.
	#cp -r $(PWD)/.git $(DESTDIR)/$(PREFIX)
deb_clean:
	rm -rf debian/$(PACKAGENAME).*debhelper
	rm -rf debian/$(PACKAGENAME).substvars
deb_build:
	echo "Not required"


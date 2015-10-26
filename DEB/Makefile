define pkg_install
$(info $1)
mkdir -p $(DESTDIR)/$(PREFIX)
for value in $1;do \
cp -r $$value $(DESTDIR)/$(PREFIX); \
done
cp -r  $(PWD)/src $(DESTDIR)/$(PREFIX)/../..
endef

deb_install:
	$(eval REQ_FILES:=$(filter-out $(PWD)/src/,$(filter-out $(PWD)/Makefile,$(filter-out $(PWD)/debian/, $(wildcard $(PWD)/*/)))))
	$(call pkg_install,$(REQ_FILES))
deb_clean:
	rm -rf debian/$(PACKAGENAME).*debhelper
	rm -rf debian/$(PACKAGENAME).substvars
deb_build:
	echo "Not required"
rpm_install:
	$(eval REQ_FILES:=$(filter-out $(PWD)/src/,$(filter-out $(PWD)/Makefile, $(wildcard $(PWD)/*/))))
	$(call pkg_install,$(REQ_FILES))

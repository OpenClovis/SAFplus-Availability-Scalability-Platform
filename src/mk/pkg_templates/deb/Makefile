
DIRECTORYS = $(filter-out $(PWD)/Makefile,$(filter-out $(PWD)/debian/, $(wildcard $(PWD)/*/)))

install:
	mkdir -p $(DESTDIR)/$(PREFIX)
	for dir in $(DESTDIR)/$(PREFIX);do \
	rm -rf dir;\
	done
	echo $(DIRECTORYS)
	for value in $(DIRECTORYS);do \
	cp -r $$value $(DESTDIR)/$(PREFIX); \
	done
	
uninstall:
	for dir in $(DIRECTORYS);do \
	rm -rf $(DESTDIR)/$(PREFIX)/$$dir; \
	done
clean:
	rm -rf debian/$(PACKAGENAME).*debhelper
	rm -rf debian/$(PACKAGENAME).substvars

PACKAGE = kid
PYTHON = python2.4

PY_MODULE = $(PACKAGE)
SVN_ROOT = svn://www.kid-templating.org/
DIST_DEST = kid-templating.org:/srv/kid-templating.org/www/htdocs/dist/$(PACKAGE)/
CLEANFILES = MANIFEST build dist export daily release ChangeLog.bak \
             *.pyc kid/*.pyc bin/*.pyc \
             test/*.pyc test/*.out test/*test*.py \
             test/templates.py test/serialization.py test/context.py \
             .*.html doc/*.html doc/.*.html \
             examples/*/*.html examples/basic/*.py

RM = /bin/rm -rf

# pudge documentation utility
PUDGE = KID_OUTPUT_PY=1 $(PYTHON) ../pudge/bin/pudge

# browser open support
BROWSER = open

VERSION = $(shell $(PYTHON) -c 'import $(PY_MODULE); print $(PY_MODULE).__version__')
DATE    = $(shell $(PYTHON) -c 'import $(PY_MODULE); print $(PY_MODULE).__date__[7:17]')
SVN_TAG = $(SVN_ROOT)/tags/release-$(VERSION)
SVN_BRANCH = $(shell svn info | grep '^URL:' | cut -f2 -d' ')
PYTHON23 = $(shell /usr/bin/which python2.3 2>/dev/null | egrep -v '^no ')
PYTHON24 = $(shell /usr/bin/which python2.4 2>/dev/null | egrep -v '^no ')
PYTHON25 = $(shell /usr/bin/which python2.5 2>/dev/null | egrep -v '^no ')
CVS2CL   = $(shell /usr/bin/which cvs2cl 2>/dev/null | egrep -v '^no ')
TESTPYTHONS = $(PYTHON23) $(PYTHON24) $(PYTHON25)

default:
	@echo TARGETS: ChangeLog daily release clean test doc

ChangeLog: FORCE
	$(CVS2CL) -S --utc

dist: FORCE
	scp -p release/$(PACKAGE)-* $(DIST_DEST)

release: FORCE pre-release-test
	svn copy "$(SVN_BRANCH)" "$(SVN_TAG)" -m "Release $(VERSION) Tag"
	$(RM) export
	mkdir export
	-mkdir release
	cd export; svn export $(SVN_TAG) $(PACKAGE)
	cd export/$(PACKAGE); $(MAKE) doc
	cd export/$(PACKAGE); \
		$(PYTHON) setup.py sdist --force-manifest ; \
		$(PYTHON) setup.py bdist_rpm ; \
		$(PYTHON) setup.py bdist_wininst
	mv export/$(PACKAGE)/dist/* release/

pre-release-test: FORCE
	@echo "version = $(VERSION), date = $(DATE), tag = $(SVN_TAG)"
	#test $(DATE) = `date +'%Y-%m-%d'` # verify release date is set to today
	! svn status | egrep '^M ' | egrep -v -e '__init__.py' -e 'ChangeLog' -e 'makefile'

daily: FORCE doc
	$(RM) export
	mkdir export
	-mkdir release
	cd export; svn export $(SVN_BRANCH) $(PACKAGE)

	NOW_DATE=`date +'%Y\\/%m\\/%d'`; \
	NOW_VERSION=`date +'%Y%m%d'`; \
	echo $$NOW_DATE $$NOW_VERSION; \
	OLD=export/$(PACKAGE)/$(PY_MODULE)/__init__.py.old; \
	NEW=export/$(PACKAGE)/$(PY_MODULE)/__init__.py; \
	mv $$NEW $$OLD; \
	sed -e "s/__version__.*/__version__ = '$$NOW_VERSION'/" \
            -e "s/__date__.*/__date__ = '$$NOW_DATE'/" < $$OLD > $$NEW;
	cd export/$(PACKAGE); $(MAKE) doc
	cd export/$(PACKAGE); \
		$(PYTHON) setup.py sdist --force-manifest ; \
		$(PYTHON) setup.py bdist_rpm ; \
		$(PYTHON) setup.py bdist_wininst
	mv export/$(PACKAGE)/dist/* release/

clean: FORCE
	cd doc ; $(MAKE) clean
	$(RM) $(CLEANFILES)

fullclean: FORCE clean
	$(RM) release

test: FORCE
	@for PYTHONBIN in $(TESTPYTHONS); do \
		echo "Testing with $$PYTHONBIN"; \
		$$PYTHONBIN run_tests.py; \
		echo ""; \
	done

doc: FORCE
	cd doc ; $(MAKE) all

pudge: FORCE
	-mkdir -p build/doc
	$(PUDGE) --dest=build/doc $(PACKAGE)
	-$(BROWSER) build/doc/index.html

publish: FORCE
	cd doc ; $(MAKE) publish

FORCE:

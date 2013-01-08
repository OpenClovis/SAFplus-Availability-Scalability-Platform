"""Unit Tests for the import extensions and path functionality."""

__revision__ = "$Rev: 455 $"
__author__ = "David Stanek <dstanek@dstanek.com>"
__copyright__ = "Copyright 2005, David Stanek"

import sys
from os.path import join as joinpath
from tempfile import mkdtemp
from shutil import rmtree, copyfile

import kid
from kid.test.util import raises


def setup_module(module):
    global tmpdir, tfile
    tmpdir = mkdtemp(prefix='kid_test_suffixes_')
    kid.path.insert(tmpdir)
    tfile = joinpath(tmpdir, 'test_suffixes0.kid')
    open(tfile, 'w').write("""\
        <html xmlns:py="http://purl.org/kid/ns#">
        <body>
            <p>my content</p>
        </body>
        </html>""")

def teardown_module(module):
    kid.path.remove(tmpdir)
    rmtree(tmpdir)

def test_enable_import_empty():
    """By default *.kid files are imported."""
    sys.path.insert(0, tmpdir)
    try:
        kid.disable_import()
        raises(ImportError, "import test_suffixes0")
        kid.enable_import()
        import test_suffixes0
        raises(ImportError, "import test_suffixes1")
        kid.disable_import()
    finally:
        sys.path.remove(tmpdir)

def test_enable_import_with_ext():
    """Using exts any file extension can be importable."""
    ext = ".html,.kid.html"
    sys.path.insert(0, tmpdir)
    try:
        raises(ImportError, "import test_suffixes1")
        raises(ImportError, "import test_suffixes2")
        raises(ImportError, "import test_suffixes3")
        kid.enable_import(ext=ext)
        dest = joinpath(tmpdir, "test_suffixes1.kid")
        copyfile(tfile, dest)
        import test_suffixes1 # *.kid files are always importable
        dest = joinpath(tmpdir, "test_suffixes2.html")
        copyfile(tfile, dest)
        import test_suffixes2
        dest = joinpath(tmpdir, "test_suffixes3.kid.html")
        copyfile(tfile, dest)
        import test_suffixes3
        dest = joinpath(tmpdir, "test_suffixes4.xhtml")
        copyfile(tfile, dest)
        raises(ImportError, "import test_suffixes4")
        kid.disable_import()
    finally:
        sys.path.remove(tmpdir)

def test_enable_import_with_path():
    """Using path any template directory can be importable."""
    assert tmpdir not in sys.path
    raises(ImportError, "import test_suffixes4")
    kid.enable_import(path=tmpdir)
    dest = joinpath(tmpdir, "test_suffixes4.kid")
    copyfile(tfile, dest)
    import test_suffixes4
    kid.disable_import(path=tmpdir)
    dest = joinpath(tmpdir, "test_suffixes5.kid")
    copyfile(tfile, dest)
    raises(ImportError, "import test_suffixes5")

"""Unit Tests for Python scope."""

__revision__ = "$Rev: 421 $"
__author__ = "David Stanek <dstanek@dstanek.com>"
__copyright__ = "Copyright 2005, David Stanek"

from os.path import join as joinpath
from tempfile import mkdtemp
from shutil import rmtree

import kid

def setup_module(module):
    global tmpdir
    tmpdir = mkdtemp(prefix='kid_test_scope_')
    kid.path.insert(tmpdir)

def teardown_module(module):
    kid.path.remove(tmpdir)
    rmtree(tmpdir)

def test_scope_1():
    """Test for scoping issue reported in ticket #101.

    Parameters passed into the Template constructor override the
    parameters of functions created with py:def.
    """
    open(joinpath(tmpdir, "scope.kid"), 'w').write("""\
        <foo xmlns:py="http://purl.org/kid/ns#">
            <bar py:def="foo(bar)" py:content="bar"/>
            <bar py:replace="foo(0)" />
        </foo>
    """)
    s = kid.Template(file="scope.kid", bar=1).serialize()
    assert "<bar>0</bar>" in s

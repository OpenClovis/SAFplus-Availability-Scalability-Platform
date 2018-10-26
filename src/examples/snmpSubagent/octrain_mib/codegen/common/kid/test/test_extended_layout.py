"""Unit Tests for Extended Layouts."""

__revision__ = "$Rev: 455 $"
__author__ = "Christoph Zwerschke <cito@online.de>"
__copyright__ = "Copyright 2006, Christoph Zwerschke"

from os.path import join as joinpath
from tempfile import mkdtemp
from shutil import rmtree

import kid

def setup_module(module):
    global tmpdir
    tmpdir = mkdtemp(prefix='kid_test_extended_layout_')
    kid.path.insert(tmpdir)

def teardown_module(module):
    kid.path.remove(tmpdir)
    rmtree(tmpdir)

def test_extended_layout():
    """Test layout template extended by another template."""
    open(joinpath(tmpdir, 'master.kid'), 'w').write("""\
        <html xmlns="http://www.w3.org/1999/xhtml"
            xmlns:py="http://purl.org/kid/ns#">
        <head py:match="item.tag == '{http://www.w3.org/1999/xhtml}head'">
            <div py:replace="item[:]"/>
            <script src="master.js"></script>
        </head>
        <body py:match="item.tag == '{http://www.w3.org/1999/xhtml}body'">
            <h1>Master Title</h1>
            <div py:replace="item[:]"/>
        </body>
        </html>""")
    t = kid.Template(file="master.kid")
    rslt = t.serialize(output='xhtml')
    assert '<html xmlns="http://www.w3.org/1999/xhtml">' in rslt
    assert '</html>' in rslt
    assert 'head' not in rslt
    assert 'master.js' not in rslt
    assert 'body' not in rslt
    assert 'Title' not in rslt
    open(joinpath(tmpdir, 'section.kid'), 'w').write("""\
        <html xmlns="http://www.w3.org/1999/xhtml"
            xmlns:py="http://purl.org/kid/ns#" py:extends="'master.kid'">
        <head>
        <title>Section Title</title>
        </head>
        <body>
            <h2>Section Title</h2>
            <div id="content">Section content</div>
        </body>
        </html>""")
    t = kid.Template(file="section.kid")
    rslt = t.serialize(output='xhtml')
    assert '<html xmlns="http://www.w3.org/1999/xhtml">' in rslt
    assert '<head>' in rslt
    assert '<title>Section Title</title>' in rslt
    assert '<script src="master.js">' in rslt
    assert '<body>' in rslt
    assert '<h1>Master Title</h1>' in rslt
    assert '<h2>Section Title</h2>' in rslt
    assert '<h3>Subsection Title</h3>' not in rslt
    assert '<div id="content">Section content</div>' in rslt
    open(joinpath(tmpdir, 'subsection.kid'), 'w').write("""\
        <html xmlns="http://www.w3.org/1999/xhtml"
            xmlns:py="http://purl.org/kid/ns#" py:layout="'section.kid'">
        <div py:match="item.get('id') == 'content'">
            <h3>Subsection Title</h3>
            <div>Subsection content</div>
        </div>
        </html>""")
    t = kid.Template(file="subsection.kid")
    rslt = t.serialize(output='xhtml')
    assert '<html xmlns="http://www.w3.org/1999/xhtml">' in rslt
    assert '<head>' in rslt
    assert '<title>Section Title</title>' in rslt
    assert '<script src="master.js">' in rslt
    assert '<body>' in rslt
    assert '<h1>Master Title</h1>' in rslt
    assert '<h2>Section Title</h2>' in rslt
    assert '<h3>Subsection Title</h3>' in rslt
    assert 'id="content"' not in rslt

"""Unit Tests for Template Reuse."""

__revision__ = "$Rev: 455 $"
__author__ = "Christoph Zwerschke <cito@online.de>"
__copyright__ = "Copyright 2006, Christoph Zwerschke"

from os.path import join as joinpath
from tempfile import mkdtemp
from shutil import rmtree

import kid

def setup_module(module):
    global tmpdir
    tmpdir = mkdtemp(prefix='kid_test_extends_')
    kid.path.insert(tmpdir)
    open(joinpath(tmpdir, 'layout.kid'), 'w').write("""\
        <html xmlns:py="http://purl.org/kid/ns#">
            <body py:match="item.tag == 'body'">
                <p>my header</p>
                <div py:replace="item[:]" />
                <p>my footer</p>
            </body>
        </html>""")

def teardown_module(module):
    kid.path.remove(tmpdir)
    rmtree(tmpdir)

def test_extends():
    """Test the basic template reuse functionality."""
    page = """\
        <html py:extends="%s" xmlns:py="http://purl.org/kid/ns#">
            <body>
                <p>my content</p>
            </body>
        </html>"""
    for extends in ("'layout.kid'", "layout.kid",
        "'layout'", "layout"):
        source = page % extends
        rslt = kid.Template(source=source).serialize()
        assert 'my header' in rslt
        assert 'my content' in rslt
        assert 'my footer' in rslt
    source = page % "layout_module"
    from kid.template_util import TemplateExtendsError
    try:
        rslt = kid.Template(source=source).serialize()
    except TemplateExtendsError, e:
        e = str(e)
    except Exception:
        e = 'wrong error'
    else:
        e = 'silent'
    assert "'layout_module'" in e
    assert 'not defined' in e
    assert 'while processing extends=' in e
    source = """<?python
        layout_module = kid.load_template(
        kid.path.find('layout.kid')) ?>
        """ + source
    for extends in ("layout_module", "layout_module.Template"):
        rslt = kid.Template(source=source).serialize()
        assert 'my header' in rslt
        assert 'my content' in rslt
        assert 'my footer' in rslt

def test_comments_in_extends():
    """Test for the bug that was reported in ticket #66."""
    open(joinpath(tmpdir, 'layout2.kid'), 'w').write("""\
        <!-- layout -->
        <html xmlns:py="http://purl.org/kid/ns#">
            <head><title>layout</title></head>
            <body py:match="item.tag == 'body'">
                <div>header</div>
                <!-- comment 1 -->
                <p align="center" py:replace="item[:]">
                    ... content will be inserted here ...
                </p>
                <!-- comment 2 -->
                <div>footer</div>
            </body>
        </html>""")
    open(joinpath(tmpdir, 'page2.kid'), 'w').write("""\
        <!-- page -->
        <html xmlns:py="http://purl.org/kid/ns#"
                py:extends="'layout2.kid'">
            <head><title>page</title></head>
            <body>
                <!-- comment 3 -->
                <p>my content</p>
                <!-- comment 4 -->
            </body>
        </html>""")
    t = kid.Template(file="page2.kid")
    rslt = t.serialize(output='xhtml')
    expected = """\
        <!-- page -->
        <html>
            <head>
            <title>page</title></head>
            <body>
                <div>header</div>
                <!-- comment 1 -->
                <!-- comment 3 -->
                <p>my content</p>
                <!-- comment 4 -->
                <!-- comment 2 -->
                <div>footer</div>
            </body>
        </html>"""
    i = 0
    for line in expected.splitlines():
        line = line.strip()
        i = rslt.find(line, i)
        assert i >= 0, 'Missing or misplaced: ' + line

def test_layout_and_extends():
    """Test for the bug that was reported in ticket #194."""
    open(joinpath(tmpdir, 'page3.kid'), 'w').write("""\
        <html xmlns:py="http://purl.org/kid/ns#"
            py:layout="'layout3.kid'"
            py:extends="'page3e.kid'">
        <title>Welcome to the test</title>
        <body>
            <div py:def="insertContent()">
                Welcome <span py:replace="pageString()" />
            </div>
        </body>
        </html>""")
    open(joinpath(tmpdir, 'page3e.kid'), 'w').write("""\
        <html xmlns:py="http://purl.org/kid/ns#">
        <head><title>Extend Page</title></head>
        <body>
            <b py:def="pageString()">page</b>
        </body>
        </html>""")
    open(joinpath(tmpdir, 'layout3.kid'), 'w').write("""\
        <html xmlns:py="http://purl.org/kid/ns#"
            py:extends="'layout3e.kid'">
        <head><title>Layout Title</title></head>
        <body>
            <h1 py:content="layoutString()" />
            <div py:replace="insertContent()" />
        </body>
        </html>""")
    open(joinpath(tmpdir, 'layout3e.kid'), 'w').write("""\
        <html xmlns:py="http://purl.org/kid/ns#">
        <head><title>Extend Layout</title></head>
        <body>
            <b py:def="layoutString()">layout</b>
        </body>
        </html>""")
    t = kid.Template(file="page3.kid")
    rslt = t.serialize(output='xhtml')
    expected = """\
        <html>
        <head>
            <title>Layout Title</title></head>
        <body>
            <h1><b>layout</b></h1>
            <div>
                Welcome <b>page</b>
            </div>
        </body>
        </html>"""
    i = 0
    for line in expected.splitlines():
        line = line.strip()
        i = rslt.find(line, i)
        assert i >= 0, 'Missing or misplaced: ' + line

def test_pudge_layout():
    """This is how Pudge implements layouts.

    This will cause a generator to be sent to template_util.generate_content.
    For each of the (ev, item) pairs yielded from the generator will be
    fed through generate content. Before the fix the tuples were treated as
    text for the output.
    """
    open(joinpath(tmpdir, 'pudge_layout.kid'), 'w').write("""
        <?xml version="1.0"?>
        <div xmlns="http://www.w3.org/1999/xhtml"
            xmlns:py="http://purl.org/kid/ns#"
            py:extends="'testlayout.kid'"
            py:strip="1">
        <span>Interesting text here</span>
        </div>
    """.strip())
    open(joinpath(tmpdir, 'testlayout.kid'), 'w').write("""\
        <?xml version="1.0"?>
        <html xmlns="http://www.w3.org/1999/xhtml"
            xmlns:py="http://purl.org/kid/ns#"
            py:def="layout">
        <body>
        <div id="main-content" py:content="content()"/>
        </body>
        </html>
    """.strip())
    t = kid.Template(file="pudge_layout.kid")
    rslt = t.serialize(output='xml')
    print rslt
    expected = """\
        <?xml version="1.0" encoding="utf-8"?>
        <html xmlns="http://www.w3.org/1999/xhtml">
        <body>
        <div id="main-content">
        <span>Interesting text here</span>
        </div>
        </body>
        </html>
    """
    rslt = [x.strip() for x in rslt.splitlines() if x.strip()]
    expected = [x.strip() for x in expected.splitlines() if x.strip()]
    assert expected == rslt

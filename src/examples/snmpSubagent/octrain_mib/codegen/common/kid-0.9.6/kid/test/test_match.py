"""Unit Tests for the template matching."""

__revision__ = "$Rev: 455 $"
__author__ = "David Stanek <dstanek@dstanek.com>"
__copyright__ = "Copyright 2005, David Stanek"

from os.path import join as joinpath
from tempfile import mkdtemp
from shutil import rmtree

import kid

def setup_module(module):
    global tmpdir
    tmpdir = mkdtemp(prefix='kid_test_match_')
    kid.path.insert(tmpdir)

def teardown_module(module):
    kid.path.remove(tmpdir)
    rmtree(tmpdir)

def test_match0():
    open(joinpath(tmpdir, "match0_base.kid"), 'w').write("""\
    <!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
        "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
    <html xmlns="http://www.w3.org/1999/xhtml"
        xmlns:py="http://purl.org/kid/ns#">

        <head py:match="item.tag=='{http://www.w3.org/1999/xhtml}head'">
            <meta content="text/html; charset=UTF-8"
                http-equiv="content-type" py:replace="''" />
            <title py:replace="''">Your title goes here</title>
            <meta py:replace="item[:]" />
        </head>

        <body py:match="item.tag=='{http://www.w3.org/1999/xhtml}body'">
            <p align="center">
                <img src="http://www.turbogears.org/tgheader.png" />
            </p>
            <div py:replace="item[:]" />
        </body>
    </html>""")
    open(joinpath(tmpdir, "match0_page.kid"), 'w').write("""\
    <!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
        "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
    <html xmlns="http://www.w3.org/1999/xhtml"
        xmlns:py="http://purl.org/kid/ns#" py:extends="'match0_base.kid'">

        <head>
            <meta content="text/html; charset=UTF-8"
                http-equiv="content-type" py:replace="''" />
            <title>Welcome to TurboGears</title>
        </head>

        <body>
            <strong py:match="item.tag == '{http://www.w3.org/1999/xhtml}b'"
                py:content="item.text.upper()" />

            <p>My Main page with <b>bold</b> text</p>
        </body>
    </html>""")
    html = kid.Template(file="match0_page.kid").serialize()
    assert '<title>Welcome to TurboGears</title>' in html
    assert '<strong>BOLD</strong>' in html

def test_match1():
    open(joinpath(tmpdir, "match1_base.kid"), 'w').write("""\
    <!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
        "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
    <html xmlns:py="http://purl.org/kid/ns#"
         xmlns="http://www.w3.org/1999/xhtml" lang="en">

        <head py:match="item.tag == '{http://www.w3.org/1999/xhtml}head'">
            <title>Some title here</title>
        </head>

        <span py:match="item.tag == '{http://www.w3.org/1999/xhtml}a' and item.get('href').startswith('http://') and 'noicon' not in str(item.get('class')).split(' ')" class="link-external" py:content="item"></span>
        <span py:match="item.tag == '{http://www.w3.org/1999/xhtml}a' and item.get('href').startswith('mailto:') and 'noicon' not in str(item.get('class')).split(' ')" class="link-mailto" py:content="item"></span>
        <span py:match="item.tag == '{http://www.w3.org/1999/xhtml}a' and item.get('href').startswith('/members/') and item.get('href').count('/') == 2 and 'noicon' not in str(item.get('class')).split(' ')" class="link-person" py:content="item"></span>

        <body py:match="item.tag == '{http://www.w3.org/1999/xhtml}body'">
            <div id="header">...</div>
            <div py:replace="item[:]">Real content would go here.</div>
            <div id="footer">...</div>
        </body>
    </html>""")
    open(joinpath(tmpdir, "match1_page.kid"), 'w').write("""\
    <!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
        "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
    <html xmlns="http://www.w3.org/1999/xhtml"
        xmlns:py="http://purl.org/kid/ns#" py:extends="'match1_base.kid'">
        <head />
        <body>
            <p>This is a <a href="http://www.google.ca/">test link</a>,
            or an <a href="mailto:foo@bar.baz">e-mail address</a>.</p>
        </body>
    </html>""")
    html = kid.Template(file="match1_page.kid").serialize()
    assert '<div id="header">...</div>' in html
    assert '<span class="link-external">' \
        '<a href="http://www.google.ca/">test link</a></span>' in html
    assert '<span class="link-mailto">' \
        '<a href="mailto:foo@bar.baz">e-mail address</a></span>' in html
    assert '<div id="footer">...</div>' in html
    assert '<div>Real content would go here.</div>' not in html

def test_match_2():
    """Test for a know bad case in the apply_matches function (ticket # 142)."""
    open(joinpath(tmpdir, "match2_master.kid"), 'w').write("""\
        <html xmlns="http://www.w3.org/1999/xhtml"
            xmlns:py="http://purl.org/kid/ns#">
            <body py:match="item.tag=='{http://www.w3.org/1999/xhtml}body'">
                <div py:replace="item[:]"/>
                <!-- MASTER MATCH -->
            </body>
        </html>""")
    open(joinpath(tmpdir, "match2_userform.kid"), 'w').write("""\
        <html xmlns="http://www.w3.org/1999/xhtml"
            xmlns:py="http://purl.org/kid/ns#">
            <body>
            <!-- THE INFAMOUS PY:MATCH   -->
                <div py:match="item.tag=='{http://testing.seasources.net/ns#}userform'"
                    py:strip="True">
                    <form py:attrs="action=action,method='post'" id="usereditform" />
                </div>
            </body>
        </html>""")
    extends = ('master', 'userform')
    for i in range(2):
        file = "match2_%s_%s.kid" % extends
        open(joinpath(tmpdir, file), 'w').write("""\
            <html xmlns="http://www.w3.org/1999/xhtml"
                xmlns:py="http://purl.org/kid/ns#"
                xmlns:seasources="http://testing.seasources.net/ns#"
                py:extends="'match2_%s.kid','match2_%s.kid'">
                <body>
                    <!-- THIS IS THE TAG I WANT TO PY:MATCH ON -->
                    <seasources:userform></seasources:userform>
                </body>
            </html>""" % extends)
        t = kid.Template(file=file)
        t.action = file
        html = t.serialize()
        assert 'THIS IS THE TAG' in html # text from  main template
        assert 'MASTER MATCH' in html # text from master template
        assert 'seasources:userform' not in html # text from userform template
        extends = list(extends)
        extends.reverse()
        extends = tuple(extends)

def test_match_3():
    """Check for an issue with additional blank lines (ticket #131)."""
    template = """\
        <html xmlns="http://www.w3.org/1999/xhtml"
                xmlns:py="http://purl.org/kid/ns#">
        <span>one</span>
        <p py:match="item.tag == 'hello'">
            hello world!
        </p>
        <span>two</span>
        </html>"""
    t = kid.Template(source=template)
    rslt = t.serialize(output="html")
    expect = """<html>
        <span>one</span>
        <span>two</span>
        </html>"""
    print rslt
    print expect
    assert rslt.endswith(expect)

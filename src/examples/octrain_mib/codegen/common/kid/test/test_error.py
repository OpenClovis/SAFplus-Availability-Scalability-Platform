"""Unit Tests for error reporting."""

__revision__ = "$Rev: 496 $"
__author__ = "Christoph Zwerschke <cito@online.de>"
__copyright__ = "Copyright 2006, Christoph Zwerschke"

import sys
from os.path import join as joinpath
from tempfile import mkdtemp
from shutil import rmtree

import kid
from kid.test.util import raises


# Error tracking is only fully supported in Python 2.4
# because we need the CurrentLineNumber attribute of Expat
python24 = sys.version_info[:2] >= (2, 4)

def setup_module(module):
    global tmpdir
    tmpdir = mkdtemp(prefix='kid_test_error_')
    kid.path.insert(tmpdir)

def teardown_module(module):
    kid.path.remove(tmpdir)
    rmtree(tmpdir)

class KidFileWriter:
    test_number = 0
    def __init__(self):
        KidFileWriter.test_number += 1
        self.file_number = 0
    def name(self):
        return 'test_error_%d_%d' % (self.test_number, self.file_number)
    def filename(self, full=False):
        filename = '%s.kid' % self.name()
        if full:
            filename = joinpath(tmpdir, filename)
        return filename
    def write(self, xml):
        self.file_number += 1
        open(self.filename(True), 'w').write(xml)

def test_xml_error():
    """Check that erroneous XML is reported."""
    from xml.parsers.expat import ExpatError
    page = """\
        <html>
            <h1>title</h1>
            <a><b>oops</b></a>
            <p>That's all, folks.</p>
        </html>"""
    kid.Template(page)
    page = page.replace('</b></a>', '</a></b>')
    e = str(raises(ExpatError, kid.Template, source=page))
    assert 'Error parsing XML' in e
    assert 'mismatched tag: line 3, column 24' in e
    assert page.splitlines()[2] in e # erroneous line
    assert "\n%25s\n" % "^" in e # offset pointer
    assert 'html' not in e
    assert 'title' not in e
    assert 'folks' not in e
    page = """\
        <html xmlns:py="http://purl.org/kid/ns#">
            <h1 py:replace="XML(xml)">title</h1>
        </html>"""
    t = kid.Template(source=page, xml="<h1>ok</h1>")
    from xml.parsers.expat import ExpatError
    content = """\
        <h1>start</h1>
        this &is wrong
        <p>end</p>"""
    t = kid.Template(source=page, xml=content)
    e = str(raises(ExpatError, t.serialize))
    assert 'Error parsing XML' in e
    assert 'not well-formed (invalid token): line 2, column 16' in e
    assert content.splitlines()[1] in e
    assert "\n%17s\n" % "^" in e
    assert 'html' not in e
    assert 'start' not in e
    assert 'end' not in e

def test_xml_long_line():
    """Check intelligent truncating of long XML error lines."""
    from xml.parsers.expat import ExpatError
    page = '<a>x</b>' + 9999*'x'
    e = str(raises(ExpatError, kid.Template, page))
    assert 'Error parsing XML' in e
    assert 'mismatched tag: line 1, column 6' in e
    assert ('\n<a>x</b>' + 68*'x') in e
    assert "\n%7s\n" % "^" in e
    page = '<a>' + 9999*'x' + '</b>'
    e = str(raises(ExpatError, kid.Template, page))
    assert 'Error parsing XML' in e
    assert 'mismatched tag: line 1, column 10004' in e
    assert ('\n' + 72*'x' + '</b>') in e
    assert "\n%75s\n" % "^" in e
    page = '<a>' + 9999*'x' + '</b>' + 9999*'x'
    e = str(raises(ExpatError, kid.Template, page))
    assert 'Error parsing XML' in e
    assert 'mismatched tag: line 1, column 10004' in e
    assert ('\n' + 36*'x' + '</b>' + 36*'x') in e
    assert "\n%39s\n" % "^" in e

def test_xml_filename_error():
    """Check that erroneous XML filename is reported."""
    f = KidFileWriter()
    f.write("<xml>This is XML</xml>")
    t = kid.Template(file=f.filename())
    f.write("This is not XML")
    from xml.parsers.expat import ExpatError
    e = str(raises(ExpatError, kid.Template, file=f.filename()))
    assert 'Error parsing XML' in e
    assert f.filename() in e
    assert 'This is not XML' in e
    assert 'syntax error: line 1, column 0' in e
    assert '\n^\n' in e

def test_layout_error():
    """Check that erroneous py:layout expressions are reported."""
    page = '<html xmlns:py="http://purl.org/kid/ns#" py:layout="no_layout" />'
    # because py:layout is dynamic, the template can be created
    # but the error should show up when we try to serialize the template
    t = kid.Template(source=page)
    from kid.template_util import TemplateLayoutError
    e = str(raises(TemplateLayoutError, t.serialize))
    assert 'not defined' in e
    assert 'while processing layout=' in e
    assert 'no_layout' in e

def test_extends_error():
    """Check that erroneous py:extends expressions are reported."""
    page = '<html xmlns:py="http://purl.org/kid/ns#" py:extends="no_extends" />'
    # because py:extends is not dynamic, the template cannot be created
    from kid.template_util import TemplateExtendsError
    e = str(raises(TemplateExtendsError, kid.Template, source=page))
    assert 'not defined' in e
    assert 'while processing extends=' in e
    assert 'no_extends' in e

def test_attr_error():
    """Check that erroneous py:attrs expressions are reported."""
    page = """\
        <html xmlns:py="http://purl.org/kid/ns#">
            <p py:attrs="%s" />
        </html>"""
    t = kid.Template(source=page % "abc=123, def=789")
    s = t.serialize()
    assert 'abc="123"' in s and 'def="789"' in s
    from kid.template_util import TemplateAttrsError
    t = kid.Template(source=page % "abc=123, 456=789")
    e = str(raises(TemplateAttrsError, t.serialize))
    assert 'invalid' in e
    assert 'while processing attrs=' in e
    assert 'abc=123, 456=789' in e
    t = kid.Template(source=page % "{'mickey':'mouse'}")
    s = t.serialize()
    assert 'mickey="mouse"' in s
    t = kid.Template(source=page % "mickey mouse")
    e = str(raises(TemplateAttrsError, t.serialize))
    assert 'while processing attrs=' in e
    assert 'mickey mouse' in e
    t = kid.Template(source=page % "{mickey:mouse}")
    e = str(raises(TemplateAttrsError, t.serialize))
    assert 'not defined' in e
    assert 'mickey' in e and 'mouse' in e

def test_tracking_1():
    """Check error tracking when compiling a Kid template."""
    from kid.compiler import compile_file
    f = KidFileWriter()
    xml = """<xml>
        <title>compilation fails</title>
        <p1>the expression ${1/0} can be compiled</p1>
        <p2>the expression ${1//0} can be compiled</p2>
        <p3>the expression ${1///0} cannot be compiled</p3>
        <p4>the expression ${1+1} can be compiled</p4>
        </xml>"""
    for call in (compile_file, kid.load_template, kid.Template):
        f.write(xml)
        e = str(raises(SyntaxError,
            call, file=f.filename(call == compile_file)))
        assert 'invalid syntax (%s.py, line ' % f.name() in e
        assert ' 1///0' in e and ' ^\n' in e
        assert '1/0' not in e and '1//0' not in e and '1+1' not in e
        assert 'can be compiled' not in e
        if python24:
            assert 'Error location in template file ' in e
            assert f.filename() in e
            assert 'on line 5 between columns 8 and 54:' in e
            assert 'the expression ${1///0} cannot be compiled' in e
        assert 'xml>' not in e and 'title>' not in e
        assert 'p1>' not in e and 'p2>' not in e and 'p4>' not in e
        assert 'compilation fails' not in e
    xml = """<!-- test1 -->
        <?python
            ok = 1/0
        ?>
        <!-- test2 -->
        <?python
            ok = 1//0
        ?>
        <!-- test3 -->
        <?python
            oops = 1///0
        ?>
        <!-- test4 -->
        <?python
            ok = 1+1
        ?>
        <xml>
        <title>compilation fails</title>
        </xml>"""
    for call in (compile_file, kid.load_template, kid.Template):
        f.write(xml)
        e = str(raises(SyntaxError,
            call, file=f.filename(call == compile_file)))
        assert 'invalid syntax (%s.py, line ' % f.name() in e
        assert 'oops = 1///0' in e and '              ^\n' in e
        assert 'ok =' not in e
        assert '1/0' not in e and '1//0' not in e and '1+1' not in e
        if python24:
            assert 'Error location in template file ' in e
            assert f.filename() in e
            assert 'between line 10, column 8 and line 13, column 8:' in e
            assert '            oops = 1///0' in e
        assert 'xml>' not in e and 'title>' not in e
        assert 'compilation fails' not in e

def test_tracking_2():
    """Check error tracking when importing a Kid template."""
    from kid.compiler import compile_file
    f = KidFileWriter()
    xml = """<!-- test1 -->
        <?python ok = 1/2 ?>
        <!-- test2 -->
        <?python oops = 1/0 ?>
        <xml>
        <title>import fails</title>
        </xml>"""
    f.write(xml)
    try:
        e = compile_file(file=f.filename(True))
    except Exception:
        e = None
    assert e == True, 'This file cannot be compiled properly.'
    for call in (kid.load_template, kid.Template):
        f.write(xml)
        e = str(raises(ZeroDivisionError, call, file=f.filename()))
        assert 'integer division or modulo by zero' in e
        if python24:
            assert 'Error location in template file ' in e
            assert f.filename() in e
            assert 'between line 4, column 8 and line 5, column 8:' in e
            assert '<?python oops = 1/0 ?>' in e
        assert 'xml>' not in e and 'title>' not in e
        assert 'import fails' not in e

def test_tracking_3():
    """Check error tracking when executing a Kid template."""
    f = KidFileWriter()
    xml = """<xml>
        <title>execution fails</title>
        <p1>the expression ${1/2} can be evaluated</p1>
        <p2>the expression ${1/0} cannot be evaluated</p2>
        </xml>"""
    f.write(xml)
    t = kid.Template(file=f.filename())
    def execute_template_method(t, n):
        if n == 1:
            import StringIO
            output = StringIO.StringIO()
            t.write(file=output)
            ret = output.getvalue()
            output.close()
        elif n == 2:
            ret = list(t.generate())
        else:
            ret = t.serialize()
        return ret
    for n in range(3):
        e = str(raises(ZeroDivisionError, execute_template_method, t, n))
        assert 'integer division or modulo by zero' in e
        if python24 and n < 2:
            assert 'Error location in template file ' in e
            assert f.filename() in e
            assert 'on line 4 between columns 8 and 53:' in e
            assert 'the expression ${1/0} cannot be evaluated' in e
        assert 'xml>' not in e and 'title>' not in e
        assert 'execution fails' not in e
    xml = """<xml>
        <title>execution fails</title>
        <?python ok = 1/2 ?>
        <?python oops = 1/0 ?>
        </xml>"""
    f.write(xml)
    t = kid.Template(file=f.filename())
    e = str(raises(ZeroDivisionError, t.serialize))
    assert 'integer division or modulo by zero' in e
    if python24:
        assert 'Error location in template file ' in e
        assert f.filename() in e
        assert 'between line 4, column 8 and line 5, column 8:' in e
        assert '<?python oops = 1/0 ' in e
        assert 'ok = 1/2' not in e
    assert 'xml>' not in e and 'title>' not in e
    assert 'execution fails' not in e
    xml = """<xml xmlns:py="http://purl.org/kid/ns#">
        <title>execution fails</title>
        <h1>test1</h1><p1 py:content="1/2" />
        <h2>test2</h2><p2 py:content="1/0" />
        </xml>"""
    f.write(xml)
    t = kid.Template(file=f.filename())
    e = str(raises(ZeroDivisionError, t.serialize))
    assert 'integer division or modulo by zero' in e
    if python24:
        assert 'Error location in template file ' in e
        assert f.filename() in e
        assert 'between line 4, column 22 and line 5, column 8:' in e
        assert ' py:content="1/0"' in e
    assert '"1/2"' not in e
    assert 'xml>' not in e and 'title>' not in e
    assert 'p1>' not in e and 'h1>' not in e
    assert 'h2>' not in e
    assert 'execution fails' not in e

def test_tracking_4():
    """Check error tracking when compiling a Kid template."""
    from kid.compiler import compile_file
    f = KidFileWriter()
    xml = """<xml xmlns:py="http://purl.org/kid/ns#">
        <title>execution fails</title>
        <title>test</title>
        <div py:content="'K\\xe4se'"/>
        </xml>"""
    f.write(xml)
    t = kid.Template(file=f.filename())
    e = raises(UnicodeDecodeError, t.serialize)
    assert e.__class__.__name__ == UnicodeDecodeError.__name__
    assert e.__class__.__module__ == UnicodeDecodeError.__module__
    assert hasattr(e, 'reason') \
        and e.reason == "ordinal not in range(128)"
    e = str(e)
    assert "can't decode byte 0xe4 in position 1" in e
    assert "ordinal not in range(128)" in e
    assert '\nError in code generated from template file ' in e
    assert f.filename() in e
    assert 'execution fails' not in e

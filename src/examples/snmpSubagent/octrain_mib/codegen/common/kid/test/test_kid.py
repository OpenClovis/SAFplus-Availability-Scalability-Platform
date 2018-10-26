"""kid package tests."""

__revision__ = "$Rev: 455 $"
__date__ = "$Date: 2006-12-21 01:42:34 -0500 (Thu, 21 Dec 2006) $"
__author__ = "Ryan Tomayko (rtomayko@gmail.com)"
__copyright__ = "Copyright 2004-2005, Ryan Tomayko"
__license__ = "MIT <http://www.opensource.org/licenses/mit-license.php>"

import os
import sys

from os.path import exists, join as joinpath, splitext
from glob import glob

import kid, kid.test
from kid.test import template_package, output_dir, template_dir

try:
    from xml.etree import ElementTree
except ImportError:
    from elementtree import ElementTree

keep_files = []

def cleanup():
    for f in glob(joinpath(output_dir, '*.out')):
        try:
            os.unlink(f)
        except OSError:
            pass
    for f in glob(joinpath(template_dir, '*.pyc')):
        try:
            os.unlink(f)
        except OSError:
            pass
    for f in glob(joinpath(template_dir, '*.py')):
        if exists(splitext(f)[0] + '.kid') and not f in keep_files:
            try:
                os.unlink(f)
            except OSError:
                pass

def setup_module(module):
    cleanup()

def teardown_module(module):
    if not os.environ.get('KID_NOCLEANUP'):
        cleanup()

def assert_template_interface(t):
    assert hasattr(t, 'pull')
    assert hasattr(t, 'generate')
    assert hasattr(t, 'write')
    assert hasattr(t, 'serialize')

def test_import_and_expand():
    try:
        del sys.modules['test.context']
    except KeyError:
        pass
    kid.disable_import()
    try:
        import test.context as c
    except ImportError:
        c = None
    assert c is None
    kid.enable_import()
    import test.context as c
    kid.disable_import()
    C = c.Template
    out = joinpath(output_dir, 'context.out')
    t = C(foo=10, bar='bla bla')
    it = t.pull()
    for e in it:
        pass
    t.write(out)
    check_xml_file(out)

def test_import_template_func():
    assert not sys.modules.has_key(template_package + 'test_def')
    t = kid.import_template(template_package + 'test_def')
    assert_template_interface(t)
    assert sys.modules.has_key(template_package + 'test_def')

def test_load_template_func():
    t = kid.load_template(joinpath(template_dir, 'test_if.kid'),
    	name='', cache=False)
    assert_template_interface(t)
    t2 = kid.load_template(joinpath(template_dir, 'test_if.kid'),
                           name=template_package + 'test_if',
                           cache=True)
    assert not t is t2
    t3 = kid.load_template(joinpath(template_dir, 'test_if.kid'),
                           name=template_package + 'test_if',
                           cache=True)
    assert t3 is t2

def test_load_template_func_with_ns():
    t = kid.load_template(joinpath(template_dir, 'templates.kid'),
    	name='', cache=False)
    s = """
    <docelement xmlns:py="http://purl.org/kid/ns#" py:extends="t">
    <element py:content="template1()" />
    </docelement>
    """
    t2 = kid.load_template(s, cache=False, ns={'t': t})
    xml = t2.serialize()
    assert "This is a test" in xml

def test_load_template_with_exec_module():
    s = """<?python Mickey = 'Mouse' ?>
    <html>Hello, World!</html>"""
    t= kid.load_template(s)
    assert t.Mickey == 'Mouse'
    assert not hasattr(t, 'Donald')
    def exec_module(mod, code):
        exec code in mod.__dict__
        mod.Donald = 'Duck'
    t = kid.load_template(s, exec_module=exec_module)
    assert t.Mickey == 'Mouse'
    assert t.Donald == 'Duck'

def test_template_func():
    t = kid.Template(name=template_package + 'test_if')
    assert_template_interface(t)
    assert isinstance(t, kid.BaseTemplate)

def test_generate_func():
    def run_test(o):
        for s in o.generate(encoding='ascii'):
            assert s is not None and len(s) > 0
    run_test(kid.Template(name=template_package + 'test_if'))
    run_test(kid.load_template(joinpath(template_dir, 'test_if.kid')))

def test_write_func():
    class FO:
        def write(self, text):
            pass
    kid.Template(name=template_package+'test_if').write(file=FO())
    m = kid.load_template(joinpath(template_dir, 'test_if.kid'))
    m.write(file=FO())

def test_serialize_func():
    def run_test(o):
        out = o.serialize(encoding='utf-8')
        assert out is not None and len(out) > 0
        out = o.serialize(encoding='ascii')
        assert out is not None and len(out) > 0
    run_test(kid.Template(name=template_package + 'test_if'))
    run_test(kid.load_template(joinpath(template_dir, 'test_if.kid')))

def test_short_form():
    # check that the serializer is outputting short-form elements when
    # no character data is present
    text = """<?xml version="1.0" encoding="utf-8"?>
<test><short /></test>"""
    template = kid.Template(file=text) #mod.Template()
    actual = template.serialize().strip()
    assert actual == text, '%r != %r' % (actual, text)

def test_XML_func_fragment():
    text = u"""some plain "text" with &amp; entities."""
    t = kid.Template(source="<p>${XML(text)}</p>", text=text)
    rslt = t.serialize(fragment=True)
    assert rslt == '''<p>some plain "text" with &amp; entities.</p>'''
    # another one
    text = """something <p>something else</p>"""
    t = kid.Template(source="<p>${XML(text)}</p>", text=text)
    actual = t.serialize(fragment=True)
    expected = '''<p>something <p>something else</p></p>'''
    assert actual == expected, '%r != %r' % (actual, expected)

def test_XML_ns_fragment():
    text = """something <p>something else</p>"""
    t = kid.Template(source='<p>${XML(text, xmlns="foo")}</p>', text=text)
    actual = t.serialize(fragment=True)
    expected = '''<p>something <p xmlns="foo">something else</p></p>'''
    assert actual == expected, '%r != %r' % (actual, expected)

def test_XML_func_unicode():
    s = u"""asdf \u2015 qwer"""
    t = kid.Template(source="""<p>${XML(s)}</p>""", s=s)
    print repr(t.serialize())

def test_dont_modify_trees():
    t = kid.Template(source="<a>$xml</a>")
    t.xml = ElementTree.fromstring("<b>some<c>nested</c>elements</b>")
    expected = "<a><b>some<c>nested</c>elements</b></a>"
    assert t.serialize(fragment=True) == expected
    print ElementTree.dump(t.xml)
    assert t.serialize(fragment=True) == expected

def test_comments():
    t = kid.Template(source="<doc><!-- bla --></doc>")
    rslt = t.serialize(fragment=True)
    assert rslt == "<doc><!-- bla --></doc>"

def test_kid_files(test='*'):
    test_files = glob(joinpath(template_dir, 'test_%s.kid' % test))
    for f in test_files:
        out = f[0:-4] + '.out'
        try:
            template = kid.Template(file=f, cache=True)
            template.assume_encoding = "utf-8"
            template.write(out)
            check_xml_file(out)
        except Exception:
            print '\nTemplate: %s' % f
            try:
                kid.compiler.compile_file(f, source=True,
                    force=True, encoding="utf-8")
            except:
                print "Corresponding source module could not be created."
            else:
                keep_files.append(splitext(f)[0] + '.py')
                print "The correspoding source module has been created."
            raise

def check_xml_file(filename):
    dot = kid.test.dot
    doc = ElementTree.parse(filename).getroot()
    assert doc.tag == 'testdoc'
    for t in doc.findall('test'):
        attempt = t.find('attempt')
        expect = t.find('expect')
        if expect.get('type') == 'text':
            doc = ElementTree.XML(expect.text)
            expect.append(doc)
            expect.text = ''
        try:
            diff_elm(attempt, expect, diff_this=False)
        except AssertionError:
            raise
        else:
            dot()
        kid.test.additional_tests += 1

def diff_elm(elm1, elm2, diff_this=True):
    for e in [elm1, elm2]:
        e.tail = e.tail and e.tail.strip() or None
        e.text = e.text and e.text.strip() or None
    if diff_this:
        assert elm1.tag == elm2.tag
        assert elm1.attrib == elm2.attrib
        assert elm1.tail == elm2.tail
    expected = elm2.text
    actual = elm1.text
    assert actual == expected, '%r != %r' % (actual, expected)
    ch1 = elm1.getchildren()
    ch2 = elm2.getchildren()
    assert len(ch1) == len(ch2)
    for elm1, elm2 in zip(ch1, ch2):
        diff_elm(elm1, elm2)

def test_string_templates():
    """Test for collisions in templates created with a string (ticket #70)."""
    t1 = kid.Template(source="<xx/>")
    t2 = kid.Template(source="<yy/>")
    assert str(t1) ==  '<?xml version="1.0" encoding="utf-8"?>\n<xx />'
    assert str(t2) ==  '<?xml version="1.0" encoding="utf-8"?>\n<yy />'

def test_reserved_names():
    """Check for reserved keyword arguments (ticket #181)."""
    from kid.template_util import TemplateAttrsError
    try:
        t = kid.Template('<div />', content=None)
    except TemplateAttrsError, e:
        e = str(e)
    else:
        e = 'reserved name was not detected'
    assert repr('content') in e
    assert 'reserved name' in e

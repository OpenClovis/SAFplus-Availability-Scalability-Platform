# -*- coding: utf-8 -*-

"""kid.serialization tests."""

__revision__ = "$Rev: 492 $"
__date__ = "$Date: 2007-07-06 21:38:45 -0400 (Fri, 06 Jul 2007) $"
__author__ = "Ryan Tomayko (rtomayko@gmail.com)"
__copyright__ = "Copyright 2004-2005, Ryan Tomayko"
__license__ = "MIT <http://www.opensource.org/licenses/mit-license.php>"

import kid
from kid.namespace import xhtml
from kid.serialization import serialize_doctype, doctypes

xhtml_namespace = str(xhtml)

def test_serialize_doctype():
    sd = serialize_doctype
    assert (sd(doctypes['xhtml-strict']) ==
        '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"'
        ' "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">')
    assert (sd(doctypes['html-quirks']) ==
        '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">')
    assert sd('a') == 'a'
    assert sd(('a',)) == '<!DOCTYPE a>'
    assert sd(('a', 'b')) == '<!DOCTYPE a PUBLIC "b">'
    assert sd(('a', 'b', 'c')) == '<!DOCTYPE a PUBLIC "b" "c">'

def test_html_output_method():
    t = kid.Template('<html><p>Test</p><br /></html>')
    for output in 'html', 'html-strict':
        rslt = t.serialize(output=output)
        expected = serialize_doctype(doctypes[output]) + \
                   '\n<html><p>Test</p><br></html>'
        assert rslt == expected

def test_HTML_output_method():
    t = kid.Template('<html><p>Test</p><br /></html>')
    for output in 'HTML', 'HTML-strict':
        rslt = t.serialize(output=output)
        expected = serialize_doctype(doctypes[output.lower()]) + \
                   '\n<HTML><P>Test</P><BR></HTML>'
        assert rslt == expected

def test_xhtml_output_method():
    t = kid.Template('<html xmlns="http://www.w3.org/1999/xhtml">'
            '<p>test</p><img src="some.gif" /><br /></html>')
    for output in 'xhtml', 'xhtml-strict':
        rslt = t.serialize(output=output)
        expected = serialize_doctype(doctypes[output]) \
                   + '\n<html xmlns="http://www.w3.org/1999/xhtml">' \
                   + '<p>test</p>' \
                   + '<img src="some.gif" /><br /></html>'
        assert rslt == expected

def test_html_strict_output_method():
    t = kid.Template('<html><p>test</p><br /></html>')
    rslt = t.serialize(output='HTML-strict')
    expected = serialize_doctype(doctypes['html-strict']) + \
               '\n<HTML><P>test</P><BR></HTML>'
    assert rslt == expected

def test_html_quirks_output_method():
    t = kid.Template('<html><p>test</p><br /></html>')
    rslt = t.serialize(output='HTML-quirks')
    expected = serialize_doctype(doctypes['html-quirks']) + \
               '\n<HTML><P>test</P><BR></HTML>'
    assert rslt == expected

def test_xml_output_method():
    t = kid.Template('<html><p>test</p><br/></html>')
    rslt = t.serialize(output='xml')
    expected = '<?xml version="1.0" encoding="utf-8"?>\n' + \
               '<html><p>test</p><br /></html>'
    assert rslt == expected

from kid.serialization import HTMLSerializer, XMLSerializer, XHTMLSerializer
serializer = HTMLSerializer()
serializer.doctype = None
serializer.inject_type = False

def HTMLTemplate(text, **kw):
    t = kid.Template(text, **kw)
    t.serializer = serializer
    return t

def test_html_transpose():
    t = kid.Template('<HtMl/>')
    t.serializer = HTMLSerializer()
    rslt = t.serialize()
    assert rslt.endswith('<html></html>')
    t.serializer = HTMLSerializer(transpose=False)
    rslt = t.serialize()
    assert rslt.endswith('<html></html>')
    t.serializer = HTMLSerializer(transpose=True)
    rslt = t.serialize()
    assert rslt.endswith('<HTML></HTML>')
    t.serializer = HTMLSerializer(transpose=None)
    rslt = t.serialize()
    assert rslt.endswith('<HtMl></HtMl>')
    def transpose(s):
        return s.capitalize()
    t.serializer = HTMLSerializer(transpose=transpose)
    rslt = t.serialize()
    assert rslt.endswith('<Html></Html>')

def test_html_empty_elements():
    close_tags = 'div p pre span script textarea'.split()
    noclose_tags = 'br hr img input link meta'.split()
    for tag in close_tags + noclose_tags:
        xml = '<html><%s/></html>' % tag
        t = HTMLTemplate(xml)
        rslt = t.serialize()
        if tag in noclose_tags:
            html = '<%s>' % tag
        else:
            html = '<%s></%s>' % (tag, tag)
        html = xml.replace('<%s/>' % tag, html)
        assert rslt == html

def test_xhtml_empty_elements():
    noempty_tags = 'div p pre textarea'.split()
    empty_tags = 'br hr img input'.split()
    for namespace in ('', ' xmlns="%s"' % xhtml_namespace):
        for tag in noempty_tags + empty_tags:
            xml = '<html%s><%s/></html>' % (namespace, tag)
            t = kid.Template(xml)
            rslt = t.serialize(output='xhtml')
            if tag in empty_tags:
                xhtml = '<%s />' % tag
            else:
                xhtml = '<%s></%s>' % (tag, tag)
            xhtml = xml.replace('<%s/>' % tag, xhtml)
            xhtml = serialize_doctype(doctypes['xhtml']) + '\n' + xhtml
            assert rslt == xhtml

def test_html_noescape_elements():
    t = HTMLTemplate("<html><head><script>" \
                     "<![CDATA[less than: < and amp: &]]>" \
                     "</script></head></html>")
    expected = '<html><head><script>' \
        'less than: < and amp: &</script></head></html>'
    rslt = t.serialize()
    assert rslt == expected

def test_html_boolean_attributes():
    t = HTMLTemplate('<html xmlns="%s">'
        '<option selected="1">Bla</option></html>' % xhtml_namespace)
    expected = '<html><option selected>Bla</option></html>'
    rslt = t.serialize()
    assert rslt == expected

def test_doctype_and_injection():
    serializer = HTMLSerializer(encoding='utf-8', transpose=True)
    serializer.doctype = doctypes['html-strict']
    serializer.inject_type = True
    source = "<html><head /></html>"
    t = kid.Template(source)
    t.serializer = serializer
    from kid.format import Format
    format=Format(no_empty_lines=True)
    rslt = t.serialize(format=format)
    rslt = rslt.replace('\n', '')
    expected = ('<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN"'
        ' "http://www.w3.org/TR/html4/strict.dtd">'
        '<HTML><HEAD>'
        '<META CONTENT="text/html; charset=utf-8" HTTP-EQUIV="content-type">'
        '</HEAD></HTML>')
    assert rslt == expected
    serializer = HTMLSerializer(encoding='ascii', transpose=False)
    serializer.doctype = None
    serializer.inject_type = True
    source = ('<html><head>'
        '<meta content="Reiner Wahnsinn" name="author"/>'
        '<meta content="nonsense" http-equiv="content-type"/>'
        '<meta content="garbage" name="keywords"/>'
        '<title>Content</title>'
        '</head><body><h1>Hello, World!</h1></body></html>')
    t = kid.Template(source)
    t.serializer = serializer
    rslt = t.serialize()
    expected = source.replace('/>', '>')
    assert rslt == expected
    source = source.replace('content-type', 'garbage-type')
    t = kid.Template(source)
    t.serializer = serializer
    rslt = t.serialize()
    rslt = rslt.replace('\n', '')
    expected = source.replace('/>', '>').replace('<title>',
        '<meta content="text/html; charset=ascii"'
        ' http-equiv="content-type"><title>')
    assert rslt == expected

def test_output_methods():
    xml = '<html xmlns="%s"><h1 />Hello<br />World</html>' % xhtml_namespace
    t = kid.Template(xml)
    assert len(kid.output_methods) >= 15
    for output in kid.output_methods:
        s = t.serialize(output=output)
        if 'html' in output.lower(): # html or xhtml
            assert s.startswith('<!DOCTYPE ')
            assert s.lower().startswith('<!doctype html public "')
            if output.startswith('xhtml'):
                assert 'DOCTYPE html PUBLIC' in s
                assert ' "-//W3C//DTD XHTML 1.0' in s
            else:
                assert 'DOCTYPE HTML PUBLIC' in s
                assert ' "-//W3C//DTD HTML 4.01' in s
            assert '//EN"' in s
            if 'strict' in output:
                assert 'transitional' not in s.lower()
                assert 'frameset' not in s.lower()
                assert 'loose' not in s.lower()
            else:
                assert 'strict' not in s.lower()
                if 'frameset' in output:
                    assert 'transitional' not in s.lower()
                    assert ' Frameset' in s
                else:
                    assert 'frameset' not in s.lower()
                    assert ' Transitional' in s
            if 'quirks' in output:
                assert 'http://' not in s
                assert 'dtd' not in s
            else:
                assert '"http://www.w3.org/TR/' in s
                assert '.dtd"' in s
                if output.startswith('xhtml'):
                    if 'strict' in output:
                        assert '/xhtml1/DTD/xhtml1-strict.dtd"' in s
                    elif 'frameset' in output:
                        assert '/xhtml1/DTD/xhtml1-frameset.dtd"' in s
                    else:
                        assert '/xhtml1/DTD/xhtml1-transitional.dtd"' in s
                else:
                    if 'strict' in output:
                        assert '/html4/strict.dtd"' in s
                    elif 'frameset' in output:
                        assert '/html4/frameset.dtd"' in s
                    else:
                        assert '/html4/loose.dtd"' in s
            r = xml
            r = r.replace('<h1 />', '<h1></h1>')
            if not output.startswith('x'):
                r = r.replace('<br />', '<br>')
                r = r.replace(' xmlns="%s"' % xhtml_namespace, '')
            if output.lower() != output:
                r = r.upper()
                r = r.replace('HELLO', 'Hello').replace('WORLD', 'World')
            r = '\n' + r
            assert s.endswith(r)
        elif output.endswith('ml'): # xml or wml
            assert s.startswith('<?xml version="1.0" encoding="utf-8"?>\n')
            assert s.endswith(xml)
        else:
            assert output == 'plain'
            assert s == 'HelloWorld'

def test_strip_lang():
    serializer = HTMLSerializer(transpose=True)
    serializer.doctype = doctypes['html-strict']
    t = kid.Template("<html xml:lang='en' lang='en' />")
    t.serializer = serializer
    expected = '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN"' \
        ' "http://www.w3.org/TR/html4/strict.dtd">\n' \
        '<HTML LANG="en"></HTML>'
    rslt = t.serialize()
    assert rslt == expected

import string
def test_transpose_lower():
    serializer = HTMLSerializer()
    serializer.doctype = None
    serializer.inject_type = False
    serializer.transpose = string.lower
    t = kid.Template("<HTML><HEAD /></HTML>")
    t.serializer = serializer
    expected = '<html><head></head></html>'
    rslt = t.serialize()
    assert rslt == expected

def test_transpose_off():
    serializer = HTMLSerializer()
    serializer.doctype = None
    serializer.inject_type = False
    serializer.transpose = None
    t = kid.Template("<HTML><HEAD /></HTML>")
    t.serializer = serializer
    expected = '<HTML><HEAD></HEAD></HTML>'
    rslt = t.serialize()
    assert rslt == expected

def test_whitespace():
    """Only real XML whitespace should be stripped away."""
    # These chars are whitespace in XML and Python:
    for char in '\x09\x0a\x0d\x20': # HT, LF, CR, SP
        t = kid.Template('<p>%s</p>' % char)
        rslt = t.serialize(encoding='latin-1')
        assert rslt.endswith('<p />')
    rslt = kid.Template('<p>&#32;</p>').serialize(output='xhtml')
    assert rslt.endswith('<p> </p>')
    # These chars are considered whitespace in Python, but not in XML:
    from xml.parsers.expat import ExpatError
    for char in '\x0b\x0c\xa0': # VT, FF, NBSP
        try:
            t = kid.Template('<p>%s</p>' % char)
            rslt = t.serialize(encoding='latin-1')
        except ExpatError, e:
            e = str(e)
            if 'not well-formed' in e and 'invalid token' in e:
                rslt = '<p>%s</p>' % char
            else:
                rslt = 'XML Error'
        assert rslt.endswith('<p>%s</p>' % char)
    rslt = kid.Template('<p>&#160;</p>').serialize(
        output='xhtml', encoding='latin-1')
    assert rslt.endswith('<p>\xa0</p>')
    t = kid.Template('<p>&nbsp;</p>')
    rslt = t.serialize(output='xhtml', encoding='latin-1')
    assert rslt.endswith('<p>\xa0</p>')

def test_whitespace2():
    """Keep nonbreakable space inside paragraph (ticket #140)."""
    rslt = kid.Template("""\
        <?python
        def nbsp1():
          return XML("nbsp")
        def nbsp2():
          return XML("&nbsp;")
        def nbsp3():
          return XML("&#160;")
        def nbsp4():
          return u'\u00a0'
        ?>
        <div xmlns:py="http://purl.org/kid/ns#">
        <h1>ticket #140</h1>
        <span py:content="nbsp1()"/>
        <span py:content="nbsp2()"/>
        <span py:content="nbsp3()"/>
        <span py:content="nbsp4()"/>
        </div>""").serialize(encoding='latin-1')
    rslt = rslt.replace(' ', '').replace('\n', '')
    assert rslt.endswith('<div><h1>ticket#140</h1><span>nbsp</span>'
        + 3*'<span>\xa0</span>' + '</div>')

def test_comment_whitespace():
    """Comments should not add an additional newline (ticket #107)."""
    expected = '<?xml version="1.0" encoding="utf-8"?>\n<html>\n' \
            '<!-- a comment -->\n<element />\n</html>'
    assert kid.Template(expected).serialize(output='xml') == expected
    expected = serialize_doctype(doctypes['html']) + '\n<HTML>\n' \
            '<!-- a comment -->\n<ELEMENT>\n</ELEMENT>\n</HTML>'
    assert kid.Template(expected).serialize(output='HTML') == expected
    expected = serialize_doctype(doctypes['xhtml']) + '\n<html>\n' \
            '<!-- a comment -->\n<element>\n</element>\n</html>'
    assert kid.Template(expected).serialize(output='xhtml') == expected

def test_empty_lines():
    """Handling of empty lines in templates.

    Empty lines between elements should be removed.
    We assume that balanced_blocks is enabled
    for both HTML and XHTML templates.
    Inline elements in template should remain so.
    Other elements should be indented.

    """
    t = kid.Template("""
        <html>
        <script>some
        lines

        and more lines</script><body>
         <a href="/"><img src="pic.jpg"/></a>
        </body>

          </html>""")
    expected = serialize_doctype(
            doctypes['html']) + """\n<HTML>
        <SCRIPT>some
        lines

        and more lines</SCRIPT><BODY>
         <A HREF="/"><IMG SRC="pic.jpg"></A>
        </BODY>
          </HTML>"""
    assert t.serialize(output='HTML') == expected
    expected = serialize_doctype(
            doctypes['xhtml']) + """\n<html>
        <script>some
        lines

        and more lines</script><body>
         <a href="/"><img src="pic.jpg" /></a>
        </body>
          </html>"""
    assert t.serialize(output='xhtml') == expected

def test_extra_indentation():
    """Check that no extra indentation is inserted (ticket #131)."""
    html = """<div>
            <a href="/"><img src="pic.jpg"/></a>
        </div>"""
    assert HTMLTemplate(html).serialize() == html.replace('/>', '>')

def test_textarea_indentation():
    """Check for extra indentation to textarea (ticket #83)."""
    widgetsrc = """
        <textarea xmlns:py="http://purl.org/kid/ns#" py:content="value" />
    """
    template = """
      <div xmlns:py="http://purl.org/kid/ns#">
      ${widget}
      </div>
    """
    t = kid.Template(widgetsrc, value='')
    tmpl = kid.Template(template, widget=t.transform())
    rslt = tmpl.serialize(output='xhtml', fragment=True)
    expected = """<div>
      <textarea></textarea>
      </div>"""
    assert rslt == expected
    template = """
        <div xmlns:py="http://purl.org/kid/ns#">${widget}</div>
    """
    t = kid.Template(widgetsrc, value='')
    tmpl = kid.Template(template, widget=t.transform())
    rslt = tmpl.serialize(output='xhtml', fragment=True)
    expected = """<div><textarea></textarea></div>"""
    assert rslt == expected
    template = """
      <div xmlns:py="http://purl.org/kid/ns#"> ${widget}</div>
    """
    assert rslt == expected

def test_br_namespace_issues():
    """Check problem with handling of br in XHTML (ticket #83)."""
    widgetsrc = '<div><br/></div>'
    template = """<div xmlns="http://www.w3.org/1999/xhtml"
                        xmlns:py="http://purl.org/kid/ns#">
        ${widget}<br/>
        </div>"""
    t = kid.Template(widgetsrc, value='')
    tmpl = kid.Template(template, widget=t.transform())
    rslt = tmpl.serialize(output='xhtml', fragment=True)
    expected = """<div xmlns="http://www.w3.org/1999/xhtml">
        <div><br /></div><br />
        </div>"""
    assert rslt == expected

def test_nbsp():
    """Check that &nbsp; is rendered correctly."""
    xml = '<p>Dr.&nbsp;Snuggles</p>'
    t = kid.Template(xml)
    for s in (XMLSerializer, HTMLSerializer, XHTMLSerializer):
        output = s()
        r = t.serialize(output=output, encoding='ascii')
        assert r.endswith(xml.replace('&nbsp;', '&#160;'))
        output = s(entity_map=True)
        r = t.serialize(output=output, encoding='ascii')
        assert r.endswith(xml)
        output = s(entity_map = {u'\xa0': ' Mooney '})
        r = t.serialize(output=output, encoding='ascii')
        assert r.endswith(xml.replace('&nbsp;', ' Mooney '))

def test_no_repeated_namespaces():
    """Check that namespaces are not repeated (ticket #144)."""
    div1 = """
        <div xmlns="http://www.w3.org/1999/xhtml"/>"""
    div2 = """
      <div xmlns="http://www.w3.org/1999/xhtml"
        xmlns:py="http://purl.org/kid/ns#"
        py:content="div1"/>"""
    t = kid.Template(div2, div1=kid.Template(div1).transform())
    s = t.serialize(output='xhtml', fragment=True)
    assert s == '<div xmlns="http://www.w3.org/1999/xhtml"><div></div></div>'

# -*- coding: utf-8 -*-

"""kid.parser tests"""

__revision__ = "$Rev: 492 $"
__date__ = "$Date: 2007-07-06 21:38:45 -0400 (Fri, 06 Jul 2007) $"
__author__ = "Ryan Tomayko (rtomayko@gmail.com)"
__copyright__ = "Copyright 2004-2005, Ryan Tomayko"
__license__ = "MIT <http://www.opensource.org/licenses/mit-license.php>"

from kid import Element, load_template, Template
from kid.parser import ElementStream, XML

def test_xml_type():
    doc = XML("<doc>hello world</doc>", fragment=False)
    assert type(doc) is ElementStream
    doc = XML("hello world", fragment=True)
    assert type(doc) is ElementStream

def test_expand():
    doc = XML("<doc><hello>world</hello></doc>", fragment=False)
    assert type(doc) is ElementStream
    doc = doc.expand()
    assert type(doc) == type(Element('doc'))
    assert doc.tag == 'doc'
    assert doc[0].tag == 'hello'
    assert doc[0].text == 'world'

def test_strip():
    xml = """<html>
        <head><title>The Title</title></head>
        <body>
        <h1>Header 1</h1>
        <div class="1">
        <h2>Header 1.1</h2>
        <h2>Header 1.2</h2>
        <div class="2">
        <h3>Header  1.2.1</h3>
        <h3>Header  1.2.2</h3>
        <div class="2">
        <p>Hello, World!</p>
        </div>
        <h3>Header  1.2.3</h3>
        </div>
        <h2>Header 1.3</h2>
        </div>
        </body>
        </html>"""
    xml_stream = XML(xml, fragment=False)
    assert type(xml_stream) is ElementStream
    xml_stream_stripped = xml_stream.strip(levels=6)
    assert type(xml_stream_stripped) is ElementStream
    from kid import XMLSerializer
    serializer = XMLSerializer(decl=False)
    xml_stripped = serializer.serialize(xml_stream_stripped, fragment=True)
    assert xml_stripped == 'Hello, World!'

def test_xml_with_entity_map():
    xml = '&nbsp;'
    assert list(XML(xml))[0][1] == u'\xa0'
    xml = '&codswallop;'
    try:
        e = list(XML(xml))
    except Exception, e:
        e = str(e)
    assert 'undefined entity &codswallop;' in e
    xml = '&nbsp;, &codswallop;!'
    entity_map = {'nbsp': u'Hello', 'codswallop': u'World'}
    assert list(XML(xml, entity_map=entity_map))[0][1] == u'Hello, World!'

def test_load_template_with_entity_map():
    xml = '<html>&nbsp;</html>'
    s = load_template(xml).serialize(encoding='ascii')
    assert s.endswith('<html>&#160;</html>')
    xml = '<html>&codswallop;</html>'
    try:
        e = load_template(xml).serialize(encoding='ascii')
    except Exception, e:
        e = str(e)
    assert 'undefined entity &codswallop;' in e
    xml = '<html>&nbsp;, &codswallop;!</html>'
    entity_map = {'nbsp': u'Hello', 'codswallop': u'World'}
    s = load_template(xml, entity_map=entity_map).serialize(encoding='ascii')
    assert s.endswith('<html>Hello, World!</html>')

def test_encoding():
    s = ('<?xml version="1.0" encoding="iso-8859-2"?>\n'
        '<e a="\xb5\xb9\xe8\xbb\xbe">'
        '\xb5\xb9\xe8\xbb\xbe\xfd\xe1\xed</e>')
    r = load_template(s).serialize(encoding='utf-8')
    s = s.decode('iso-8859-2').replace('iso-8859-2', 'utf-8')
    r = r.decode('utf-8')
    assert r == s

def test_expand_fragments():
    """Testcase for expanding XML fragments (ticket #145)."""
    template = """<div xmlns:py="http://purl.org/kid/ns#"
        py:replace="stream" />"""
    t = Template("""\
        <div xmlns:py="http://purl.org/kid/ns#">
            <div py:for="i in range(3)">
                <p>Hello World #$i</p>
            </div>
        </div>""")
    s = t.serialize(fragment=True)
    expected = """<div>
            <div>
                <p>Hello World #0</p>
            </div><div>
                <p>Hello World #1</p>
            </div><div>
                <p>Hello World #2</p>
            </div>
        </div>"""
    assert s == expected
    stream = ElementStream(t.transform()).expand()
    t2 = Template(source=template, stream=stream)
    s2 = t2.serialize(fragment=True)
    assert s2 == s
    t = Template("""\
        <div xmlns:py="http://purl.org/kid/ns#" py:for="i in range(3)">
            <p>Hello World #$i</p>
        </div>""")
    s = t.serialize(fragment=True)
    expected = """<div>
            <p>Hello World #0</p>
        </div><div>
            <p>Hello World #1</p>
        </div><div>
            <p>Hello World #2</p>
        </div>"""
    assert s == expected
    stream = ElementStream(t.transform()).expand()
    t2 = Template(source=template, stream=stream)
    s2 = t2.serialize(fragment=True)
    assert s2 == s
    t = Template("""\
        <div xmlns:py="http://purl.org/kid/ns#">
            <div py:strip="True">
                <p>Hello World</p>
            </div>
        </div>""")
    s = t.serialize(fragment=True)
    expected = """<div>
                <p>Hello World</p>
        </div>"""
    assert s == expected
    stream = ElementStream(t.transform()).expand()
    t2 = Template(source=template, stream=stream)
    s2 = t2.serialize(fragment=True)
    assert s2 == s
    t = Template("""\
        <div xmlns:py="http://purl.org/kid/ns#" py:strip="True">
            <p>Hello World</p>
        </div>""")
    s = t.serialize(fragment=True).strip()
    expected = """<p>Hello World</p>"""
    assert s == expected
    stream = ElementStream(t.transform()).expand()
    t2 = Template(source=template, stream=stream)
    s2 = t2.serialize(fragment=True).strip()
    assert s2 == s

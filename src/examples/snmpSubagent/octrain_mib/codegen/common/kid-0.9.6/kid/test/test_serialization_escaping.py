# -*- coding: utf-8 -*-

"""Tests exercising text escaping."""

__revision__ = "$Rev: 492 $"
__date__ = "$Date: 2007-07-06 21:38:45 -0400 (Fri, 06 Jul 2007) $"
__author__ = "David Stanek <dstanek@dstanek.com>"
__copyright__ = "Copyright 2006, David Stanek"
__license__ = "MIT <http://www.opensource.org/licenses/mit-license.php>"

from kid.serialization import XMLSerializer, XHTMLSerializer, HTMLSerializer
XML = XMLSerializer
XHTML = XHTMLSerializer
HTML = HTMLSerializer

TEST_CHARS = ('<', '>', '"', "'", '&',)
TEST_STRINGS = ('str', 'k\204se')
TEST_COMBO = ('str<"&">str', "k\204se<'&'>k\204se")

def escape_functions():
    """Generator producing escape functions."""
    for serializer in (HTMLSerializer, XMLSerializer, XHTMLSerializer):
        for escape in (serializer.escape_cdata, serializer.escape_attrib):
            yield serializer, escape

def do_escape(func, test_chars, result_chars, encoding=None):
    for x, char in enumerate(test_chars):
        assert func(char, encoding) == result_chars[x]

def test_escape():
    expected = {
        XML.escape_cdata: ('&lt;', '>', '"', "'", '&amp;',),
        XML.escape_attrib: ('&lt;', '>', '&quot;', "'", '&amp;',),
        XHTML.escape_cdata: ('&lt;', '>', '"', "'", '&amp;',),
        XHTML.escape_attrib: ('&lt;', '>', '&quot;', "'", '&amp;',),
        HTML.escape_cdata: ('&lt;', '>', '"', "'", '&amp;',),
        HTML.escape_attrib: ('<', '>', '&quot;', "'", '&amp;',),
    }
    for serializer, escape in escape_functions():
        do_escape(escape, TEST_CHARS, expected[escape])

def test_escape_encoding():
    """Test the encoding part of the escaping functions."""
    ascii_expected = ('str', 'k\204se')
    utf8_expected = ('str', 'k&#132;se')
    for serializer, escape in escape_functions():
        do_escape(escape, TEST_STRINGS, ascii_expected)
        do_escape(escape, TEST_STRINGS, utf8_expected, 'utf-8')

def test_escape_encoding_combo():
    ascii_expected = {
        XML.escape_cdata:
            ('str&lt;"&amp;">str', "k\204se&lt;'&amp;'>k\204se"),
        XML.escape_attrib:
            ('str&lt;&quot;&amp;&quot;>str', "k\204se&lt;'&amp;'>k\204se"),
        XHTML.escape_cdata:
            ('str&lt;"&amp;">str', "k\204se&lt;'&amp;'>k\204se"),
        XHTML.escape_attrib:
            ('str&lt;&quot;&amp;&quot;>str', "k\204se&lt;'&amp;'>k\204se"),
        HTML.escape_cdata:
            ('str&lt;"&amp;">str', "k\204se&lt;'&amp;'>k\204se"),
        HTML.escape_attrib:
            ('str<&quot;&amp;&quot;>str', "k\204se<'&amp;'>k\204se"),
    }
    utf8_expected = {
        XML.escape_cdata:
            ('str&lt;"&amp;"str', "1k&#132;se&lt;'&amp;'&gt;k&#132;se"),
        XML.escape_attrib:
            ('str&lt;&quot;&amp;&quot;>str',
             "k&#132;se&lt;'&amp;'&gt;k&#132;se"),
        XHTML.escape_cdata:
            ('str&lt;"&amp;">str', "k&#132;se&lt;'&amp;'&gt;k&#132;se"),
        XHTML.escape_attrib:
            ('str&lt;&quot;&amp;&quot;>str',
             "k&#132;se&lt;'&amp;'&gt;k&#132;se"),
        HTML.escape_cdata:
            ('str&lt;"&amp;">str', "k&#132;se&lt;'&amp;'&gt;k&#132;se"),
        HTML.escape_attrib:
            ('str<&quot;&amp;&quot;>str',
             "k&#132;se&lt;'&amp;'&gt;k&#132;se"),
    }
    for serializer, escape in escape_functions():
        do_escape(escape, TEST_COMBO, ascii_expected[escape])
        do_escape(escape, TEST_COMBO, utf8_expected[escape], 'utf-8')

def test_escaping_int():
    for serializer, escape in escape_functions():
        try:
            assert escape(1)
        except TypeError, e:
            assert str(e) == 'cannot serialize 1 (type int)'

def test_escaping_nbsp():
    for serializer, escape in escape_functions():
        assert escape('\xa0', 'ascii') == '&#160;'
        assert escape('\xa0', 'ascii', {'\xa0': 'bingo'}) == 'bingo'


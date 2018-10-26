"""Unit Tests for formatted output."""

__revision__ = "$Rev: 486 $"
__author__ = "Christoph Zwerschke <cito@online.de>"
__copyright__ = "Copyright 2006, Christoph Zwerschke"

import kid
from kid.format import Format

nbsp = u'\u00a0'
string1 = ' \t \n \t Hello, World \n \t '
string2 = '\t\tHello,  World \t \t '
string3 = '\n\n\nHello, \n\n\tWorld\n\t\n'
xml1 = '<p>%s</p>' % string1
xml2 = '<p>%s</p>' % string2
xml3 = '<p>%s</p>' % string3

def serialize(source, format=None, output='html', encoding='utf-8'):
    """Compile source template and serialize with given format."""
    t = kid.Template(source=source)
    return t.serialize(encoding=encoding, format=format, output=output,
        fragment=True) # so that output will not be stripped

def test_format_repr():
    from kid.format import output_formats
    repr_dict = lambda d: dict([(k, repr(v)) for k, v in d.items()])
    for format in output_formats.values():
        d = [repr_dict(f.__dict__) for f in (format, eval(repr(format)))]
        assert d[0] == d[1]

def test_lstrip():
    lstrip = Format.lstrip
    for c in ' \t\r\n':
        for d in '!~\f\xa0':
            assert lstrip(c + d) == d
    assert lstrip(string1) == 'Hello, World \n \t '
    assert '\n'.lstrip() == '' and nbsp.lstrip() == ''
    assert lstrip('\n') == '' and lstrip(nbsp) == nbsp

def test_rstrip():
    rstrip = Format.rstrip
    for c in ' \t\r\n':
        for d in '!~\v\f':
            assert rstrip(d + c) == d
    assert rstrip(string1) == ' \t \n \t Hello, World'
    assert '\n'.rstrip() == '' and nbsp.rstrip() == ''
    assert rstrip('\n') == '' and rstrip(nbsp) == nbsp

def test_strip():
    strip = Format.strip
    for c in ' \t\r\n':
        for d in '!~\v\f':
            assert strip(c + d + c) == d
    assert strip(string1) == 'Hello, World'
    assert '\n'.strip() == '' and nbsp.strip() == ''
    assert strip('\n') == '' and strip(nbsp) == nbsp

def test_lstrip_blanks():
    lstrip_blanks = Format.lstrip_blanks
    for c in ' \t':
        for d in '!~\v\f\r\n':
            assert lstrip_blanks(c + d) == d
    assert lstrip_blanks(string1) == '\n \t Hello, World \n \t '
    assert lstrip_blanks('\n') == '\n'
    assert lstrip_blanks(nbsp) == nbsp

def test_rstrip_blanks():
    rstrip_blanks = Format.rstrip_blanks
    for c in ' \t':
        for d in '!~\v\f\r\n':
            assert rstrip_blanks(d + c) == d
    assert rstrip_blanks(string1) == ' \t \n \t Hello, World \n'
    assert rstrip_blanks('\n') == '\n'
    assert rstrip_blanks(nbsp) == nbsp

def test_strip_blanks():
    strip_blanks = Format.strip_blanks
    for c in ' \t':
        for d in '!~\v\f\r\n':
            assert strip_blanks(c + d + c) == d
    assert strip_blanks(string1) == '\n \t Hello, World \n'
    assert strip_blanks('\n') == '\n'
    assert strip_blanks(nbsp) == nbsp

def test_lstrip_lines():
    lstrip_lines = Format.lstrip_lines
    assert lstrip_lines(string1) == '\nHello, World \n'
    assert lstrip_lines(string2) == 'Hello,  World \t \t '
    assert lstrip_lines(string3) == '\n\n\nHello, \n\nWorld\n\n'
    assert lstrip_lines(nbsp) == nbsp

def test_rstrip_lines():
    rstrip_lines = Format.rstrip_lines
    assert rstrip_lines(string1) == '\n \t Hello, World\n'
    assert rstrip_lines(string2) == '\t\tHello,  World'
    assert rstrip_lines(string3) == '\n\n\nHello,\n\n\tWorld\n\n'
    assert rstrip_lines(nbsp) == nbsp

def test_strip_lines():
    strip_lines = Format.strip_lines
    assert strip_lines(string1) == '\nHello, World\n'
    assert strip_lines(string2) == 'Hello,  World'
    assert strip_lines(string3) == '\n\n\nHello,\n\nWorld\n\n'
    assert strip_lines(nbsp) == nbsp

def test_simple_blanks():
    simple_blanks = Format.simple_blanks
    assert simple_blanks('   ') == ' '
    assert simple_blanks('\t\t\t') == ' '
    assert simple_blanks(' \t \t ') == ' '
    assert simple_blanks('\n\n\n') == '\n\n\n'
    assert simple_blanks(' \n ') == ' \n '
    assert simple_blanks(string1) == ' \n Hello, World \n '
    assert simple_blanks(string2) == ' Hello, World '
    assert simple_blanks(string3) == string3
    assert simple_blanks(nbsp + nbsp) == nbsp + nbsp

def test_simple_newlines():
    simple_newlines = Format.simple_newlines
    assert simple_newlines('   ') == '   '
    assert simple_newlines('\n\n\n') == '\n'
    assert simple_newlines(' \n \t \n ') == ' \n '
    assert simple_newlines(string1) == string1
    assert simple_newlines(string2) == string2
    assert simple_newlines(string3) == '\nHello, \n\tWorld\n'
    assert simple_newlines(nbsp + nbsp) == nbsp + nbsp

def test_simple_newline_whitespace():
    simple_newline_whitespace = Format.simple_newline_whitespace
    assert simple_newline_whitespace('   ') == '   '
    assert simple_newline_whitespace('\t\t\t') == '\t\t\t'
    assert simple_newline_whitespace(' \t \t ') == ' \t \t '
    assert simple_newline_whitespace('\n\n\n') == '\n'
    assert simple_newline_whitespace(' \n ') == '\n'
    assert simple_newline_whitespace(' \n \t \n ') == '\n'
    assert simple_newline_whitespace(' \n \t \n * \t *  ') == '\n* \t *  '
    assert simple_newline_whitespace(string1) == '\nHello, World\n'
    assert simple_newline_whitespace(string2) == string2
    assert simple_newline_whitespace(string3) == '\nHello,\nWorld\n'
    assert simple_newline_whitespace(nbsp + nbsp) == nbsp + nbsp

def test_simple_whitespace():
    simple_whitespace = Format.simple_whitespace
    assert simple_whitespace('   ') == ' '
    assert simple_whitespace('\t\t\t') == ' '
    assert simple_whitespace(' \t \t ') == ' '
    assert simple_whitespace('\n\n\n') == '\n'
    assert simple_whitespace(' \n ') == '\n'
    assert simple_whitespace(' \n \t \n ') == '\n'
    assert simple_whitespace(' \n \t \n * \t *  ') == '\n* * '
    assert simple_whitespace(string1) == '\nHello, World\n'
    assert simple_whitespace(string2) == ' Hello, World '
    assert simple_whitespace(string3) == '\nHello,\nWorld\n'
    assert simple_whitespace(nbsp + nbsp) == nbsp + nbsp

def test_clean_whitespace():
    clean_whitespace = Format.clean_whitespace
    assert clean_whitespace('   ') == ''
    assert clean_whitespace('\t\t\t') == ''
    assert clean_whitespace(' \t \t ') == ''
    assert clean_whitespace('\n\n\n') == ''
    assert clean_whitespace(' \n ') == ''
    assert clean_whitespace(' \n \t \n ') == ''
    assert clean_whitespace(' \n \t \n * \t *  ') == '* *'
    assert clean_whitespace(string1) == 'Hello, World'
    assert clean_whitespace(string2) == 'Hello, World'
    assert clean_whitespace(string3) == 'Hello,\nWorld'
    assert clean_whitespace(nbsp + nbsp) == nbsp + nbsp

def test_educate_quotes():
    educate_quotes = Format().educate_quotes
    assert (educate_quotes("'Hello' \"World\"")
        == u"\u2018Hello\u2019 \u201cWorld\u201d")
    assert (educate_quotes("'Hello', \"World\"!")
        == u"\u2018Hello\u2019, \u201cWorld\u201d!")
    assert (educate_quotes("'Hello,' \"World!\"")
        == u"\u2018Hello,\u2019 \u201cWorld!\u201d")
    assert (educate_quotes("('Hello World')")
        == u"(\u2018Hello World\u2019)")
    assert (educate_quotes("'(Hello World)'")
        == u"\u2018(Hello World)\u2019")
    assert (educate_quotes('"Isn\'t this fun?"')
        == u"\u201cIsn\u2019t this fun?\u201d")
    assert (educate_quotes("The 70's and '80s weren't fun.")
        == u"The 70\u2019s and \u201980s weren\u2019t fun.")

def test_educate_backticks():
    educate_backticks = Format().educate_backticks
    assert (educate_backticks("`Hello' ``World\"")
        == u"\u2018Hello\u2019 \u201cWorld\u201d")

def test_educate_dashes():
    educate_dashes = Format().educate_dashes
    assert educate_dashes("Hello--World") == u"Hello\u2013World"
    assert educate_dashes("Hello---World") == u"Hello\u2014World"
    assert educate_dashes("----") == "----"

def test_educate_ellipses():
    educate_ellipses = Format().educate_ellipses
    assert educate_ellipses("Hello... World. . .") == u"Hello\u2026 World\u2026"
    assert educate_ellipses("..... --- . . . . .") == "..... --- . . . . ."

def test_stupefy():
    stupefy = Format().stupefy
    assert (stupefy(u"\u2018Hello\u2019\u2014\u201cWorld\u201d\u2026")
        == "'Hello'---\"World\"...")

def test_intent_lines():
    indent = Format(indent='\t').indent_lines
    assert indent(string1, '') == ' \t \nHello, World \n'
    assert indent(string2, '') == string2
    assert indent(string3, '') == '\n\n\nHello, \n\nWorld\n\n'
    assert indent(string1) == ' \t \n\tHello, World \n\t'
    assert indent(string2) == string2
    assert indent(string3) == '\n\t\n\t\n\tHello, \n\t\n\tWorld\n\t\n\t'
    assert indent(string1) == indent(string1, '\t')
    assert indent(string2) == indent(string2, '\t')
    assert indent(string3) == indent(string3, '\t')
    assert indent(string1, '*') == ' \t \n*Hello, World \n*'
    assert indent(string2, '*') == string2
    assert indent(string3, ' ') == '\n \n \n Hello, \n \n World\n \n '
    assert indent('\nprint "Hello"', ' 10 ') == '\n 10 print "Hello"'

def test_wrap_lines():
    wrap = Format(wrap=80).wrap_lines
    s = 'Hello, World!'
    assert wrap(s) == s
    assert wrap(s, 13) == s
    assert wrap(s, 12) == 'Hello,\nWorld!'
    assert wrap(s, 6) == 'Hello,\nWorld!'
    assert wrap(s, 0) == 'Hello,\nWorld!'
    assert wrap(s, 13, 1) == 'Hello,\nWorld!'
    s = ' 1234567890'
    assert wrap(s*9) == s*7 + s.replace(' ', '\n') + s
    assert wrap(s*9, 80) == wrap(s*9)
    assert wrap(s*9, 40) != wrap(s*9)
    assert wrap(s*9, 20) == s + 8*s.replace(' ', '\n')
    assert wrap(s*9, 21) != wrap(s*9, 20)
    assert wrap(s*9, 11) == wrap(s*9, 20)
    assert wrap(s*9, 10) == '\n' + wrap(s*9, 20).lstrip()
    assert wrap(s*9, 0) == wrap(s*9, 10)
    s = 'a ab abc'
    assert wrap(s) == s
    assert wrap(s, 8) == s
    assert wrap(s, 7) == 'a ab\nabc'
    assert wrap(s, 6) == wrap(s, 7)
    assert wrap(s, 5) == wrap(s, 7)
    assert wrap(s, 4) == wrap(s, 7)
    assert wrap(s, 3) == 'a\nab\nabc'
    assert wrap(s, 2) == wrap(s, 3)
    assert wrap(s, 1) == wrap(s, 3)
    assert wrap(s, 0) == wrap(s, 3)
    assert wrap(s, 4, 1) == wrap(s, 3)
    assert wrap(s, 80, 79) == 'a\nab abc'
    assert wrap(s, 80, 80) == '\n' + s

def test_indent_width():
    indent_width = Format.indent_width
    assert indent_width('12345') == 5
    assert indent_width('\t') == 8
    assert indent_width('\t\t\t12345') == 29

def test_new_offset():
    new_offset = Format.new_offset
    s = 'Hello, World!'
    assert new_offset(s) == 13
    assert new_offset(s, 0) == 13
    assert new_offset(s, 1) == 14
    assert new_offset(s, 123) == 136
    s = 'Hello,\nWorld!'
    assert new_offset(s) == 6
    assert new_offset(s, 0) == 6
    assert new_offset(s, 1) == 6
    assert new_offset(s, 123) == 6
    s = 'Hello,\n\t\tWorld!'
    assert new_offset(s, 0) == 22
    assert new_offset(s, 1) == 22
    s = '\tHello,\nWorld!'
    assert new_offset(s, 0) == 6
    assert new_offset(s, 1) == 6

def test_format_false():
    assert serialize(xml1, False).endswith(xml1)
    assert serialize(xml2, False).endswith(xml2)
    assert serialize(xml3, False).endswith(xml3)

def test_format_none():
    assert serialize(xml1).endswith(xml1)
    assert serialize(xml2).endswith(xml2)
    assert serialize(xml3).endswith('<p>\nHello, \n\tWorld\n</p>')

def test_format_strip():
    format_strip = Format(strip=True)
    format_lstrip = Format(lstrip=True)
    format_rstrip = Format(rstrip=True)
    format_lrstrip = Format(lstrip=True, rstrip=True)
    assert repr(format_strip) != repr(format_lstrip)
    assert repr(format_strip) != repr(format_rstrip)
    assert repr(format_strip) == repr(format_lrstrip)
    s = serialize(xml1, format_strip)
    assert s.endswith('<p>Hello, World</p>')
    s2 = serialize(xml1, format=format_lrstrip)
    assert s2 == s
    s = serialize(xml1, format_lstrip)
    assert s.endswith('<p>Hello, World \n \t </p>')
    s = serialize(xml1, format_rstrip)
    assert s.endswith('<p> \t \n \t Hello, World</p>')
    xml = '<body><p> \t </p><p> \n </p><p>n n</p><p>\t \t</p></body>'
    for format in (format_strip, format_lstrip, format_rstrip):
        s = serialize(xml, format, 'xml')
        assert s.endswith('<body><p /><p /><p>n n</p><p /></body>')
    s = serialize(xml1, format_strip, 'xml')
    assert s.endswith('<p>Hello, World</p>')
    s = serialize(xml1, format_strip, 'xhtml')
    assert s.endswith('<p>Hello, World</p>')
    s = serialize(xml1, format_strip)
    assert s.endswith('<p>Hello, World</p>')
    s = serialize(xml1, format_strip, 'HTML')
    assert s.endswith('<P>Hello, World</P>')
    s = serialize(xml1, format_strip, 'plain')
    assert s == 'Hello, World'

def test_format_strip_blanks():
    format_strip_blanks = Format(strip_blanks=True)
    format_lstrip_blanks = Format(lstrip_blanks=True)
    format_rstrip_blanks = Format(rstrip_blanks=True)
    format_lrstrip_blanks = Format(lstrip_blanks=True, rstrip_blanks=True)
    assert repr(format_strip_blanks) != repr(format_lstrip_blanks)
    assert repr(format_strip_blanks) != repr(format_rstrip_blanks)
    assert repr(format_strip_blanks) == repr(format_lrstrip_blanks)
    s = serialize(xml1, format_strip_blanks)
    assert s.endswith('<p>\n \t Hello, World \n</p>')
    s2 = serialize(xml1, format_lrstrip_blanks)
    assert s2 == s
    s = serialize(xml1, format=format_lstrip_blanks)
    assert s.endswith('<p>\n \t Hello, World \n \t </p>')
    s = serialize(xml1, format_rstrip_blanks)
    assert s.endswith('<p> \t \n \t Hello, World \n</p>')
    xml = '<body><p> \t </p><p>\n n</p><p>n \n</p><p>\t \t</p></body>'
    for format in (format_strip_blanks, format_lrstrip_blanks,
            format_lstrip_blanks, format_rstrip_blanks):
        s = serialize(xml, format, 'xml')
        assert s.endswith('<body><p /><p>\n n</p><p>n \n</p><p /></body>')
    s = serialize(xml1, format_strip_blanks, 'plain')
    assert s == '\n \t Hello, World \n'

def test_format_strip_lines():
    format_strip_lines = Format(strip_lines=True)
    format_lstrip_lines = Format(lstrip_lines=True)
    format_rstrip_lines = Format(rstrip_lines=True)
    format_lrstrip_lines = Format(lstrip_lines=True, rstrip_lines=True)
    assert repr(format_strip_lines) != repr(format_lstrip_lines)
    assert repr(format_strip_lines) != repr(format_rstrip_lines)
    assert repr(format_strip_lines) == repr(format_lrstrip_lines)
    s = serialize(xml1, format_strip_lines)
    assert s.endswith('<p>\nHello, World\n</p>')
    s2 = serialize(xml1, format_lrstrip_lines)
    assert s2 == s
    s = serialize(xml1, format_lstrip_lines)
    assert s.endswith('<p>\nHello, World \n</p>')
    s = serialize(xml1, format_rstrip_lines)
    assert s.endswith('<p>\n \t Hello, World\n</p>')
    xml = '<body><p> \t </p><p> \nn\n\t\n </p><p>\t \t</p></body>'
    for format in (format_strip_lines, format_lrstrip_lines,
            format_lstrip_lines, format_rstrip_lines):
        s = serialize(xml, format, 'xml')
        assert s.endswith('<body><p /><p>\nn\n\n</p><p /></body>')
    s = serialize(xml1, format_strip_lines, 'plain')
    assert s == '\nHello, World\n'

def test_format_simple_blanks():
    format = Format(simple_blanks=True)
    assert repr(format) != repr(Format())
    s = serialize(xml1, format)
    assert s.endswith('<p> \n Hello, World \n </p>')
    s = serialize(xml2, format)
    assert s.endswith('<p> Hello, World </p>')
    s = serialize(xml3, format)
    assert s.endswith(xml3)

def test_format_simple_newlines():
    format = Format(simple_newlines=True)
    format2 = Format(no_empty_lines=True)
    assert repr(format) != repr(Format())
    assert repr(format) == repr(format2)
    s = serialize(xml1, format)
    assert s.endswith(xml1)
    s = serialize(xml2, format)
    assert s.endswith(xml2)
    s = serialize(xml3, format)
    assert s.endswith('<p>\nHello, \n\tWorld\n</p>')

def test_format_simple_whitespace():
    format = Format(simple_whitespace=True)
    assert repr(format) != repr(Format())
    s = serialize(xml1, format)
    assert s.endswith('<p>\nHello, World\n</p>')
    s = serialize(xml2, format)
    assert s.endswith('<p> Hello, World </p>')
    s = serialize(xml3, format)
    assert s.endswith('<p>\nHello,\nWorld\n</p>')

def test_format_with_clean_whitespace():
    clean_whitespace = Format.clean_whitespace
    format = Format(clean_whitespace)
    assert repr(format) != repr(Format())
    s = serialize(xml1, format)
    assert s.endswith('<p>Hello, World</p>')
    s = serialize(xml2, format)
    assert s.endswith('<p>Hello, World</p>')
    s = serialize(xml3, format)
    assert s.endswith('<p>Hello,\nWorld</p>')

def test_format_indent():
    xml = ('<html><body><h1>Hello, World</h1><div>'
        '<p>Hello, <b>Kids</b>!</p></div></body></html>')
    format = Format(indent=True)
    s = serialize(xml, format)
    assert s.endswith('<html>\n<body>\n\t<h1>Hello, World</h1>\n\t<div>\n'
        '\t\t<p>Hello, <b>Kids</b>!</p>\n\t</div>\n</body>\n</html>')
    format = Format(indent='')
    s = serialize(xml, format)
    assert s.endswith('<html>\n<body>\n<h1>Hello, World</h1>\n<div>\n'
        '<p>Hello, <b>Kids</b>!</p>\n</div>\n</body>\n</html>')
    format = Format(indent='   ')
    s = serialize(xml, format)
    assert s.endswith('<html>\n<body>\n   <h1>Hello, World</h1>\n   <div>\n'
        '      <p>Hello, <b>Kids</b>!</p>\n   </div>\n</body>\n</html>')
    format = Format(indent=2)
    s2 = serialize(xml, format)
    assert s2 != s
    format = Format(indent=3)
    s3 = serialize(xml, format)
    assert s3 == s
    xml = ('<html><body><h1> Hello </h1></body></html>')
    format = Format(indent=True, min_level=0)
    s = serialize(xml, format)
    assert s.endswith(
        '<html>\n\t<body>\n\t\t<h1> Hello </h1>\n\t</body>\n</html>')
    format = Format(indent=True, min_level=3)
    s = serialize(xml, format)
    assert s.endswith('<html>\n<body>\n<h1> Hello </h1>\n</body>\n</html>')
    format = Format(indent=True, max_level=2)
    s = serialize(xml, format)
    assert s.endswith(
        '<html>\n<body>\n\t<h1> Hello </h1>\n</body>\n</html>')
    format = Format(indent=True, min_level=0, max_level=1)
    s = serialize(xml, format)
    assert s.endswith(
        '<html>\n\t<body>\n\t<h1> Hello </h1>\n\t</body>\n</html>')
    xml = '<html><body><pre><div><h1>Hello</h1></div></pre></body></html>'
    format = Format(indent=True)
    s = serialize(xml, format)
    assert s.endswith('<html>\n<body>\n\t<pre><div><h1>Hello'
        '</h1></div></pre>\n</body>\n</html>')
    s = serialize(xml, format, 'xml')
    assert s.endswith('<html>\n<body>\n\t<pre>\n\t\t<div>\n\t\t\t'
        '<h1>Hello</h1>\n\t\t</div>\n\t</pre>\n</body>\n</html>')
    s = serialize(xml, format, 'plain')
    assert '\n\t\t\tHello\n' in s
    xml = '<p><em>Hello</em> wonderful <em>World</em>.</p>'
    format = Format(indent=True)
    s = serialize(xml, format)
    assert xml in s
    xml = ("""<body py:strip="" xmlns:py="http://purl.org/kid/ns#">
        <ul><li py:for="s in ('Hello', 'World')" py:content="s" /></ul></body>""")
    format = Format(indent=True, min_level=0, no_empty_lines=True)
    s = serialize(xml, format)
    assert '<ul>\n\t<li>Hello</li>\n\t<li>World</li>\n</ul>' in s

def test_format_wrap():
    xml = """<body>
        It's a long way to Tipperary,
        It's a long way to go.
        It's a long way to Tipperary
        To the sweetest girl I know.
        </body>"""
    format = Format(wrap=True)
    s = serialize(xml, format)
    assert s.endswith("<body> "
        "It's a long way to Tipperary, It's a long way to go. "
        "It's a long way to\nTipperary To the sweetest girl I know. "
        "</body>")
    format = Format(wrap=80)
    s2 = serialize(xml, format)
    format = Format(wrap=32)
    s = serialize(xml, format)
    assert s.endswith("<body> It's a long way to Tipperary,\n"
        "It's a long way to go. It's a\n"
        "long way to Tipperary To the\n"
        "sweetest girl I know. </body>")
    xml = '<html>%s</html>' % xml
    format = Format(wrap=32, indent=True, min_level=0)
    s = serialize(xml, format)
    assert s.endswith("\t<body> It's a long way to\n"
        "\t\tTipperary, It's\n\t\ta long way to\n"
        "\t\tgo. It's a long\n\t\tway to Tipperary\n"
        "\t\tTo the sweetest\n\t\tgirl I know.\n"
        "\t</body>\n</html>")

def test_format_custom():
    strip = Format.strip
    xml = """<body>
            Sometimes you need to be British to understand Monty Python.
            </body>"""
    f1 = lambda s: s.replace('Monty ', '')
    f2 = lambda s: s.replace('British', 'Dutch')
    format = Format(strip, f1, f2)
    s = serialize(xml, format)
    assert s.endswith("<body>Sometimes "
        "you need to be Dutch to understand Python.</body>")
    format = Format(f2, f1, strip)
    s2 = serialize(xml, format)
    assert s2 == s
    format = Format(f1, f2)
    s2 = serialize(xml, format)
    assert s2 != s
    f3 = lambda s: s.replace('Dutch', 'a Python')
    f4 = lambda s: s.replace('Monty', 'a')
    format = Format(strip, f4, f1, f2, f3)
    s = serialize(xml, format)
    assert s.endswith("<body>Sometimes "
        "you need to be a Python to understand a Python.</body>")
    format = Format(strip, f4, f1, f2, f3, wrap=10)
    s = serialize(xml, format)
    assert s.endswith("<body>Sometimes\nyou need\nto be a\nPython to\n"
        "understand\na Python.</body>")
    xml = '<b>kId\t</b>'
    f = lambda s: s.capitalize()
    assert serialize(xml, Format(f), 'plain') == 'Kid\t'
    f = lambda s: s.lower()
    assert serialize(xml, Format(f, rstrip=True), 'plain') == 'kid'

def test_predefined_formats():
    xml = """<html>
        <head><title>Test Page</title></head>
        <body>
        <h1>Hello,   World!</h1>
        <div>\n\n\n
        <p>This is a test page only.</p>
        <p><b>To be, or not to be:</b> that is the question:
        Whether 'tis nobler in the mind to suffer
        The slings and arrows of outrageous fortune,
        Or to take arms against a sea of troubles,
        And by opposing <em>end</em> them?</p>
        <pre><div>Hello</div>\n\n<div>World</div></pre>
        </div></body></html>"""
    s = serialize(xml, 'default')
    assert s == serialize(xml)
    xml1 = xml.split('</head>', 1)[1].replace('\n\n\n', '')
    assert s.endswith(xml1)
    s = serialize(xml, 'compact')
    xml2 = '\n'.join([line.strip() for line in xml1.splitlines()])
    xml2 = xml2.replace('  ', '')
    assert s.endswith(xml2)
    s = serialize(xml, 'pretty')
    assert s.endswith("<body>\n"
        "\t<h1>Hello, World!</h1>\n"
        "\t<div>\n"
        "\t\t<p>This is a test page only.</p>\n"
        "\t\t<p><b>To be, or not to be:</b> that is the question:\n"
        "\t\t\tWhether 'tis nobler in the mind to suffer\n"
        "\t\t\tThe slings and arrows of outrageous fortune,\n"
        "\t\t\tOr to take arms against a sea of troubles,\n"
        "\t\t\tAnd by opposing <em>end</em> them?\n"
        "\t\t</p>\n"
        "\t\t<pre><div>Hello</div>\n\n<div>World</div></pre>\n"
        "\t</div>\n</body>\n</html>")
    s = serialize(xml, 'wrap')
    assert s.endswith("<body>\n"
        "<h1>Hello, World!</h1>\n"
        "<div>\n"
        "<p>This is a test page only.</p>\n"
        "<p><b>To be, or not to be:</b> that is the question: "
        "Whether 'tis nobler in the mind to\nsuffer "
        "The slings and arrows of outrageous fortune, "
        "Or to take arms against a\nsea of troubles, "
        "And by opposing <em>end</em> them?\n"
        "</p>\n"
        "<pre><div>Hello</div>\n\n<div>World</div></pre>\n"
        "</div>\n</body>\n</html>")
    xml = """<html><body>
        <ul>
        <li>"'Hello' 'World'"</li>
        <li>'"Hello" "World"'</li>
        <li>The 1920's and '30s</li>
        <li>World Tour Soccer '06</li>
        <li>Die '68er-Generation</li>
        <li>Die "'68er-Generation"</li>
        <li>The 70's and '80s weren't all fun.</li>
        <li>"Isn't this fun?"</li>
        <li>`Isn't this fun?'</li>
        <li>``Isn't this fun?"</li>
        <li>'Jack thought we shouldn't.'</li>
        <li>"Jack thought we shouldn't."</li>
        <li>"Jack thought we shouldn't".</li>
        <li><b>"Hello"</b> "<i>World</i>"</li>
        <li><b>'Hello'</b> '<i>World</i>'</li>
        </ul>
        <p>Miss Watson would say,
        "Don't put your feet up there, Huckleberry;"
        and "Don't scrunch up like that,
        Huckleberry--set up straight;"
        and pretty soon she would say,
        "Don't gap and stretch like that,
        Huckleberry---why don't you try to behave?"</p>
        </body></html>"""
    xml_nice = """<html><body>
        <ul>
        <li>&ldquo;&lsquo;Hello&rsquo; &lsquo;World&rsquo;&rdquo;</li>
        <li>&lsquo;&ldquo;Hello&rdquo; &ldquo;World&rdquo;&rsquo;</li>
        <li>The 1920&rsquo;s and &rsquo;30s</li>
        <li>World Tour Soccer &rsquo;06</li>
        <li>Die &rsquo;68er-Generation</li>
        <li>Die &ldquo;&rsquo;68er-Generation&rdquo;</li>
        <li>The 70&rsquo;s and &rsquo;80s weren&rsquo;t all fun.</li>
        <li>&ldquo;Isn&rsquo;t this fun?&rdquo;</li>
        <li>&lsquo;Isn&rsquo;t this fun?&rsquo;</li>
        <li>&ldquo;Isn&rsquo;t this fun?&rdquo;</li>
        <li>&lsquo;Jack thought we shouldn&rsquo;t.&rsquo;</li>
        <li>&ldquo;Jack thought we shouldn&rsquo;t.&rdquo;</li>
        <li>&ldquo;Jack thought we shouldn&rsquo;t&rdquo;.</li>
        <li><b>&ldquo;Hello&rdquo;</b> &ldquo;<i>World</i>&rdquo;</li>
        <li><b>&lsquo;Hello&rsquo;</b> &lsquo;<i>World</i>&rsquo;</li>
        </ul>
        <p>Miss Watson would say,
        &ldquo;Don&rsquo;t put your feet up there, Huckleberry;&rdquo;
        and &ldquo;Don&rsquo;t scrunch up like that,
        Huckleberry&ndash;set up straight;&rdquo;
        and pretty soon she would say,
        &ldquo;Don&rsquo;t gap and stretch like that,
        Huckleberry&mdash;why don&rsquo;t you try to behave?&rdquo;</p>
        </body></html>"""
    for output in ('xml', 'html', 'xhtml'):
        s1 = serialize(xml, None, output)
        assert s1.endswith(xml[6:])
        s2 = serialize(xml_nice, None, output)
        assert not s2.endswith(xml_nice[6:])
        s3 = serialize(xml_nice, 'named', output, 'ascii')
        assert s3.endswith(xml_nice[6:])
        s4 = serialize(xml_nice, 'named', output)
        assert s4 == s2
        s5 = serialize(xml,'nice+named', output, 'ascii')

def test_custom_quotes():
    xml = """<html><body>
    <p>"Hello" --- 'World'</p>
    <p>That's all, folks...</p>
    </body></html>"""
    format = Format(educate=True,
        squotes='{}', dquotes=['<<', '>>'],
        apostrophe='`', dashes='-~', ellipsis='++')
    xml_strange = """<html><body>
    <p>&lt;&lt;Hello&gt;&gt; ~ {World}</p>
    <p>That`s all, folks++</p>
    </body></html>"""
    assert serialize(xml, format) == serialize(xml_strange)
    format = Format(educate=True,
        squotes=u'\u201a\u2018', dquotes=u'\u201e\u201c',
        apostrophe="'", dashes=u'\u2013\u2013', ellipsis=u'\u2014')
    xml_german = """<html><body>
    <p>&bdquo;Hello&ldquo; &ndash; &sbquo;World&lsquo;</p>
    <p>That's all, folks&mdash;</p>
    </body></html>"""
    assert serialize(xml, format) == serialize(xml_german)

def test_not_formatted():
    tags = 'pre script textarea'.split()
    for tag in tags:
        formatted = """<%s><h2>Subtitle</h2>   <h3> Subsubtitle </h3>
            <p>This is a <b>test</b>.</p>\t<p>Will the whitespace be <b>
            preserved? </b></p>\n\n\nWe'll hope so.</%s>""" % (tag, tag)
        xml = ('<html><body><h1>Title</h1>'
            '%s<p>The\t\t\tend</p></body></html>' % formatted)
        s = serialize(xml)
        assert formatted in s
        s = serialize(xml, format='compact')
        assert formatted in s
        assert '<p>The end</p>' in s
        s = serialize(xml, format='pretty')
        assert formatted in s
        assert '\n\t<p>The end</p>\n' in s

def test_nbsp():
    """Check that &nbsp; is rendered correctly."""
    xml = '<p>Dr.&nbsp;Snuggles</p>'
    t = kid.Template(source=xml)
    for output in 'xml', 'html', 'xhtml':
        format = Format()
        r = t.serialize(output=output, format=format, encoding='ascii')
        assert r.endswith(xml.replace('&nbsp;', '&#160;'))
        format = Format(entity_map=True)
        r = t.serialize(output=output, format=format, encoding='ascii')
        assert r.endswith(xml)
        format = Format(entity_map={u'\xa0': ' Mooney '})
        r = t.serialize(output=output, format=format, encoding='ascii')
        assert r.endswith(xml.replace('&nbsp;', ' Mooney '))

def test_noformat_tags():
    """Check that the content of some tags is not formatted."""
    format = Format(lambda s: s.lower())
    xml = '<%s>Hello, World!</%s>'
    format_tags = 'address div h1 p quote span'.split()
    noformat_tags = 'code kbd math pre script textarea'.split()
    for tag in format_tags + noformat_tags:
        x = xml % (tag, tag)
        s = serialize(x, format)
        if tag in format_tags:
            x = x.lower()
        assert s.endswith(x)

def test_for_loops_with_formatting():
    """Proper output for img and li tags in for loops (ticket #59)."""
    xml = """<html xmlns:py="http://purl.org/kid/ns#">
        <?python
            images = ['img%d.png' %n for n in range(1,4)]
            items = ['Item %d' %n for n in range(1,5)]
        ?>
        <head>
        <title py:content="'Ticket 59'"/>
        </head>
        <body>
            <div><img py:for="img in images" src="${img}"/></div>
            <ul><li py:for="item in items" py:content="item"/></ul>
        </body>
        </html>"""
    result = serialize(xml, 'pretty')
    expected = ('<html>\n<head>\n'
        '\t<meta content="text/html; charset=utf-8"'
        ' http-equiv="content-type">\n'
        '\t<title>Ticket 59</title>\n'
        '</head>\n<body>\n'
        '\t<div><img src="img1.png">'
        '<img src="img2.png"><img src="img3.png"></div>\n'
        '\t<ul>\n'
        '\t\t<li>Item 1</li>\n'
        '\t\t<li>Item 2</li>\n'
        '\t\t<li>Item 3</li>\n'
        '\t\t<li>Item 4</li>\n'
        '\t</ul>\n'
        '</body>\n</html>')
    assert result.endswith(expected)
    result = serialize(xml, 'newlines')
    expected = expected.replace('\t', '')
    assert result.endswith(expected)

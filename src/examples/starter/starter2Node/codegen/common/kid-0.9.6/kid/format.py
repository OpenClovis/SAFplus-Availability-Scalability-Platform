# -*- coding: utf-8 -*-

"""Infoset serialization format styles.

This modules provides methods assisting the serialization module
in formatting the text content of serialized infosets.

The methods for "educating" and "stupefying" typographic characters
have been inspired by John Gruber's "SmartyPants" project
(http://daringfireball.net/projects/smartypants/,
see also http://web.chad.org/projects/smartypants.py/).

"""

__revision__ = "$Rev: 492 $"
__date__ = "$Date: 2007-07-06 21:38:45 -0400 (Fri, 06 Jul 2007) $"
__author__ = "Christoph Zwerschke (cito@online.de)"
__copyright__ = "Copyright 2006, Christoph Zwerschke"
__license__ = "MIT <http://www.opensource.org/licenses/mit-license.php>"

import re

__all__ = ['Format', 'output_formats']


class Format(object):
    """Formatting details for Serializers."""

    # Default values for some parameters:

    wrap = 80
    indent = '\t'
    min_level, max_level = 1, 8
    tabsize = 8

    apostrophe = u'\u2019'
    squotes = u'\u2018\u2019'
    dquotes = u'\u201c\u201d'
    dashes = u'\u2013\u2014'
    ellipsis = u'\u2026'

    # Regular expressions used by the Format class:

    re_whitespace = re.compile(r'[ \t\n\r]+')
    re_leading_blanks = re.compile(r'^[ \t]+', re.MULTILINE)
    re_trailing_blanks = re.compile(r'[ \t]+$', re.MULTILINE)
    re_duplicate_blanks = re.compile(r'[ \t]{2,}')
    re_duplicate_newlines = re.compile(r'\n[ \t\n\r]*\n')
    re_whitespace_with_newline = re.compile(r'[ \t]*\n[ \t\n\r]*')
    re_indentation = re.compile(r'\n[ \t]*')
    re_squotes = re.compile(r"'")
    re_dquotes = re.compile(r'"')
    re_sbackticks = re.compile(r"`")
    re_dbackticks = re.compile(r"(?<![\w`])``(?!`)")
    re_squote_decade = re.compile(r"(?<=\W)'(?=\d\d\D)")
    re_squote_left = re.compile(
        r"((?<=\W)'(?=\w))|((?<=\s)'(?=\S))", re.UNICODE)
    re_squote_right = re.compile(
        r"((?<=\w)'(?=\W))|((?<=\S)'(?=\s))|((?<=\W)')", re.UNICODE)
    re_dquote_left = re.compile(
        r'((?<=\W)"(?=\w))|((?<=\s)"(?=\S))', re.UNICODE)
    re_dquote_right = re.compile(
        r'((?<=\w)"(?=\W))|((?<=\S)"(?=\s))|((?<=\W)")', re.UNICODE)
    re_endash = re.compile(r'(?<!-)--(?!-)')
    re_emdash = re.compile(r'(?<!-)---(?!-)')
    re_hyphen_between_blanks = re.compile(r'(?<!\S)-(?!\S)', re.UNICODE)
    re_ellipses = re.compile(
        r'((?<!\.)\.\.\.(?!\.\.))|((?<!\. )\. \. \.(?! \. \.))')

    def __init__(self, *args, **kw):
        """Create an output format with given parameters.

        You can pass one or more text filter functions
        for processing text content in the output stream.

        You can also set keyword parameters for using some
        standard text filter operations. The following parameters
        must be set to True to activate the operation:

        strip_lines: strip blanks in all text lines
        lstrip_lines: left strip blanks in all text lines
        rstrip_lines: right strip blanks in all text lines
        simple_blanks: remove all duplicate blanks
        no_empty_lines: remove all empty lines
        simple_whitespace: remove all duplicate whitespace
        wrap: wrap text lines to a maximum width

        You can also specify the exact width using the wrap parameter.

        There are some more operations which you should use
        with caution, since they may remove significant whitespace:

        strip: strip whitespace
        lstrip: left strip whitespace
        rstrip: right strip whitespace
        strip_blanks: strip blanks
        lstrip_blanks: left strip blanks
        rstrip_blanks: right strip blanks

        The following parameters control typographic punctuation.

        educate_quotes: use typographic quotes
        educate_backticks: replace backticks with opening quotes
        educate_dashes: replace en-dashes and em-dashes
        educate_ellipses: replace ellipses
        educate (or nice): all of the above
        stupefy (or ugly): reverse operation of educate

        apostrophe: character to be used for the apostrophe
        squotes: left and right single quote characters
        dquotes: left and right double quote characters
        dashes: characters to be used for en-dash and em-dash
        ellipsis: character to be used for the ellipsis

        The following parameters control indentation.
        This will insert newlines and level-dependent indentation,
        paying regard to inline and whitespace senstive tags:

        indent: string or number of blanks for indentation
        min_level: minimum level for indentation
        max_level: maximum level for indentation

        Note that this formatting has some limitations since
        it processes only text content in a stream (no look-ahead,
        no paying regard to the format of the serialized tags).

        The following parameters are passed to the Serializer
        (see there for the possible values of these parameters):

        decl: add xml declaration at the beginning
        doctype: add document type at the beginning
        entity_map (or named): entity map for named entities
        transpose: how to transpose html tags
        inject_type: inject meta tag with content-type

        """
        wrap = kw.get('wrap')
        if wrap:
            if isinstance(wrap, bool):
                wrap = wrap and self.wrap or None
            elif isinstance(wrap, int):
                wrap = wrap or None
            else:
                wrap = None
        self.wrap = wrap
        if wrap:
            simple_whitespace = None
            simple_blanks = simple_newlines = None
        else:
            simple_whitespace = kw.get('simple_whitespace')
            if simple_whitespace:
                simple_blanks = simple_newlines = None
            else:
                simple_blanks = kw.get('simple_blanks')
                simple_newlines = kw.get('simple_newlines',
                    kw.get('no_empty_lines'))
                if simple_blanks and simple_newlines:
                    simple_whitespace = True
                    simple_blanks = simple_newlines = None
        strip = kw.get('strip')
        if strip:
            lstrip = rstrip = None
        else:
            lstrip = kw.get('lstrip')
            rstrip = kw.get('rstrip')
            if lstrip and rstrip:
                strip = True
                lstrip = rstrip = None
        strip_lines = kw.get('strip_lines')
        if strip_lines:
            lstrip_lines = rstrip_lines = None
        else:
            lstrip_lines = kw.get('lstrip_lines')
            rstrip_lines = kw.get('rstrip_lines')
            if lstrip_lines and rstrip_lines:
                strip_lines = True
                lstrip_lines = rstrip_lines = None
        if strip or strip_lines:
            lstrip_blanks = rstrip_blanks \
                = strip_blanks = None
        else:
            strip_blanks = kw.get('strip_blanks')
            if strip_blanks:
                lstrip_blanks = rstrip_blanks = None
            else:
                if lstrip or lstrip_lines:
                    lstrip_blanks = None
                else:
                    lstrip_blanks = kw.get('lstrip_blanks')
                if rstrip or rstrip_lines:
                    rstrip_blanks = None
                else:
                    rstrip_blanks = kw.get('rstrip_blanks')
                if lstrip_blanks and rstrip_blanks:
                    strip_blanks = True
                    lstrip_blanks = rstrip_blanks = None
        indent = kw.get('indent')
        if indent:
            if isinstance(indent, bool):
                indent = indent and self.indent or None
            elif isinstance(indent, int):
                indent = ' ' * indent
            elif isinstance(indent, basestring):
                pass
            else:
                indent = None
            min_level = kw.get('min_level', self.min_level)
            max_level = kw.get('max_level', self.max_level)
        else:
            min_level = max_level = None
        self.indent = indent
        self.min_level, self.max_level = min_level, max_level

        stupefy = kw.get('stupefy', kw.get('ugly'))
        educate = not stupefy and kw.get('educate',
            kw.get('educated', kw.get('nice')))
        educate_quotes = kw.get('educate_quotes', educate)
        educate_backticks = kw.get('educate_backticks', educate)
        self.with_backticks = bool(educate_backticks)
        educate_dashes = kw.get('educate_dashes', educate)
        educate_ellipses = kw.get('educate_ellipses', educate)
        self.apostrophe = kw.get('apostrophe', self.apostrophe)
        self.squotes = kw.get('squotes', self.squotes)
        self.dquotes = kw.get('dquotes', self.dquotes)
        self.dashes = kw.get('dashes', self.dashes)
        self.ellipsis = kw.get('ellipsis', self.ellipsis)

        filters = []
        if simple_whitespace:
            filters.append(self.simple_whitespace)
        else:
            if simple_blanks:
                filters.append(self.simple_blanks)
            elif simple_newlines:
                filters.append(self.simple_newlines)
        if strip:
            filters.append(self.strip)
        elif lstrip:
            filters.append(self.lstrip)
        elif rstrip:
            filters.append(self.rstrip)
        if strip_lines:
            filters.append(self.strip_lines)
        elif lstrip_lines:
            filters.append(self.lstrip_lines)
        elif rstrip_lines:
            filters.append(self.rstrip_lines)
        if strip_blanks:
            filters.append(self.strip_blanks)
        elif lstrip_blanks:
            filters.append(self.lstrip_blanks)
        elif rstrip_blanks:
            filters.append(self.rstrip_blanks)
        if stupefy:
            filters.append(self.stupefy)
        if educate_backticks and not educate_quotes:
            filters.append(self.educate_backticks)
        if educate_dashes:
            filters.append(self.educate_dashes)
        if educate_ellipses:
            filters.append(self.educate_ellipses)

        self.custom_filters = []
        for f in args:
            if callable(f):
                if f not in self.custom_filters and f not in filters:
                    self.custom_filters.append(f)
        self.text_filters = self.custom_filters + filters

        self.context_filters = []
        if educate_quotes:
            self.context_filters.append(self.educate_quotes)

        self.decl = kw.get('decl')
        self.doctype = kw.get('doctype')
        self.entity_map = kw.get('entity_map',
            kw.get('named_entities', kw.get('named')))
        self.transpose = kw.get('transpose')
        self.inject_type = kw.get('inject_type')

    def __repr__(self):
        args = {}
        attrs = self.__dict__.keys()
        for attr in attrs:
            if attr.endswith('_filters'):
                continue
            value = getattr(self, attr)
            if value is not None:
                try:
                    if value == getattr(Format, attr):
                        value = attr in ('wrap', 'indent') and True or None
                except AttributeError:
                    pass
                if value is not None:
                    args[attr] = value
        for f in self.text_filters + self.context_filters:
            attr = f.__name__
            if attr == 'indent_lines':
                if 'indent' not in args:
                    args['indent'] = True
            elif attr == 'wrap_lines':
                if 'wrap' not in args:
                    args['wrap'] = True
            else:
                args[attr] = True
        if 'with_backticks' in args:
            if args['with_backticks']:
                args['educate_backticks'] = True
            del args['with_backticks']
        if ('educate_quotes' in args
            and 'educate_backticks' in args
            and 'educate_dashes' in args
            and 'educate_ellipses' in args):
            del args['educate_quotes']
            del args['educate_backticks']
            del args['educate_dashes']
            del args['educate_ellipses']
            args['educate'] = True
        attrs = args.keys()
        attrs.sort()
        args = [f.__name__ for f in self.custom_filters] \
            + ['%s=%r' % (attr, args[attr]) for attr in attrs]
        return "%s(%s)" % (self.__class__.__name__, ', '.join(args))

    def filter(self, s, last_char=None, next_char=None):
        """Run all filters."""
        return self.context_filter(self.text_filter(s),
            last_char, next_char)

    # Text filters without context

    def text_filter(self, s):
        """Run all filters which do not need a context."""
        for f in self.text_filters:
            s = f(s)
        return s

    # Whitespace filtering
    # (note that XML whitespace is different from Python whitespace)

    def lstrip(s):
        """Left strip XML whitespace from string."""
        return s.lstrip(' \t\n\r')
    lstrip = staticmethod(lstrip)

    def rstrip(s):
        """Right strip XML whitespace from string."""
        return s.rstrip(' \t\n\r')
    rstrip = staticmethod(rstrip)

    def strip(s):
        """Strip XML whitespace from string."""
        return s.strip(' \t\n\r')
    strip = staticmethod(strip)

    def lstrip_blanks(s):
        """Left strip only blanks and tabs from string."""
        return s.lstrip(' \t')
    lstrip_blanks = staticmethod(lstrip_blanks)

    def rstrip_blanks(s):
        """Right strip only blanks and tabs from string."""
        return s.rstrip(' \t')
    rstrip_blanks = staticmethod(rstrip_blanks)

    def strip_blanks(s):
        """Strip only blanks and tabs from string."""
        return s.strip(' \t')
    strip_blanks = staticmethod(strip_blanks)

    def lstrip_lines(cls, s):
        """Left strip XML whitespace from all lines in string."""
        return cls.re_leading_blanks.sub('', s)
    lstrip_lines = classmethod(lstrip_lines)

    def rstrip_lines(cls, s):
        """Right strip XML whitespace from all lines in string."""
        return cls.re_trailing_blanks.sub('', s)
    rstrip_lines = classmethod(rstrip_lines)

    def strip_lines(cls, s):
        """Right strip XML whitespace from all lines in string."""
        return cls.lstrip_lines(cls.rstrip_lines(s))
    strip_lines = classmethod(strip_lines)

    def simple_blanks(cls, s):
        """Remove all duplicate blanks in string."""
        return cls.re_duplicate_blanks.sub(' ', s)
    simple_blanks = classmethod(simple_blanks)

    def simple_newlines(cls, s):
        """Remove all duplicate newlines in string."""
        return cls.re_duplicate_newlines.sub('\n', s)
    simple_newlines = classmethod(simple_newlines)

    def simple_newline_whitespace(cls, s):
        """Simplify all whitespace containing newlines in string."""
        return cls.re_whitespace_with_newline.sub('\n', s)
    simple_newline_whitespace = classmethod(simple_newline_whitespace)

    def simple_whitespace(cls, s):
        """Simplify all whitespace in string."""
        return cls.simple_blanks(cls.simple_newline_whitespace(s))
    simple_whitespace = classmethod(simple_whitespace)

    def clean_whitespace(cls, s):
        """Simplify and strip all whitespace in string."""
        return cls.strip(cls.simple_whitespace(s))
    clean_whitespace = classmethod(clean_whitespace)

    # Typographic formatting

    def educate_backticks(self, s):
        """Replace backticks (`) with opening quotes."""
        # Double backticks become opening quotes:
        s = self.re_dbackticks.sub(self.dquotes[0], s)
        # Any remaining double quotes should be closing ones:
        s = self.re_dquotes.sub(self.dquotes[1], s)
        # Single backticks become opening quotes:
        s = self.re_sbackticks.sub(self.squotes[0], s)
        # Special case for decade abbreviations (the '80s):
        s = self.re_squote_decade.sub(self.apostrophe, s)
        # Closing single quotes:
        s = self.re_squote_right.sub(self.squotes[1], s)
        # Any remaining single quotes should be apostrophes:
        s = self.re_squotes.sub(self.apostrophe, s)
        return s

    def educate_dashes(self, s):
        """Replace en-dashes (--) and em-dashes (---)."""
        s = self.re_hyphen_between_blanks.sub(self.dashes[0], s)
        s = self.re_emdash.sub(self.dashes[1], s)
        s = self.re_endash.sub(self.dashes[0], s)
        return s

    def educate_ellipses(self, s):
        """Replace ellipses (...)."""
        return self.re_ellipses.sub(self.ellipsis, s)

    def stupefy(self, s):
        """Replace typographic with simple punctuation."""
        s = s.replace(self.squotes[0], "'").replace(self.squotes[1], "'")
        s = s.replace(self.dquotes[0], '"').replace(self.dquotes[1], '"')
        s = s.replace(self.dashes[0], '--').replace(self.dashes[1], '---')
        return s.replace(self.ellipsis, '...')

    # Text filters with context character

    def context_filter(self, s, last_char=None, next_char=None):
        """Run all filters which need context characters."""
        for f in self.context_filters:
            s = f(s, last_char, next_char)
        return s

    # Typographic formatting with context character

    def educate_quotes(self, s, last_char=None, next_char=None):
        """Use proper typographic quotes in the text.

        You should at least pass the last character
        of the text content if used as a stream filter.

        """
        if self.with_backticks:
            # backticks become opening quotes:
            s = self.re_dbackticks.sub(self.dquotes[0], s)
            s = self.re_sbackticks.sub(self.squotes[0], s)
        s = '%c%s%c' % (last_char or ' ', s, next_char or '.')
        # Special case for decade abbreviations (the '80s):
        s = self.re_squote_decade.sub(self.apostrophe, s)
        # Opening and closing single quotes:
        s = self.re_squote_left.sub(self.squotes[0], s)
        s = self.re_squote_right.sub(self.squotes[1], s)
        # Any remaining single quotes should be apostrophes:
        s = self.re_squotes.sub(self.apostrophe, s)
        # Opening and closing double quotes:
        s = self.re_dquote_left.sub(self.dquotes[0], s)
        s = self.re_dquote_right.sub(self.dquotes[1], s)
        # Any remaining double quotes should be opening ones:
        s = self.re_dquotes.sub(self.dquotes[0], s)
        return s[1:-1]

    # Special filters for indentation and word wrapping

    def indent_lines(self, s, indent=None):
        """Ident all lines in string."""
        if indent is None:
            indent = self.indent
        return self.re_indentation.sub('\n' + indent, s)

    def wrap_lines(self, s, width=None, offset=0, indent=0):
        """Wrap words in text lines with offset to a maximum width."""
        if width is None:
            width = self.wrap
        s = self.re_whitespace.split(s)
        t = []
        if s:
            word = s.pop(0)
            offset += len(word)
            if offset > width > 0:
                t.append('\n')
                offset = indent + len(word)
            t.append(word)
            for word in s:
                offset += len(word) + 1
                if offset <= width:
                    t.append(' ')
                else:
                    t.append('\n')
                    offset = indent + len(word)
                t.append(word)
        return ''.join(t)

    # Auxiliary functions for indentation and word wrapping

    def indent_width(indent, tabsize=tabsize):
        """Calculate width of indentation."""
        if indent.startswith('\t'):
            width = len(indent)
            indent = indent.lstrip('\t')
            width -= len(indent)
            width *= tabsize
            width += len(indent)
        else:
            width = len(indent)
        return width
    indent_width = staticmethod(indent_width)

    def new_offset(s, offset=0):
        """Calculate new offset after appending a string."""
        n = s.rfind('\n')
        if n < 0:
            offset += len(s)
        else:
            offset = Format.indent_width(s[n+1:])
        return offset
    new_offset = staticmethod(new_offset)


# create some predefined serialization formats...
output_formats = {
    'default':
        Format(no_empty_lines=True),
    'straight':
        Format(),
    'compact':
        Format(simple_whitespace=True),
    'newlines':
        Format(simple_whitespace=True, indent=''),
    'pretty':
        Format(simple_whitespace=True, indent='\t'),
    'wrap':
        Format(wrap=True, indent=''),
    'nice':
        Format(no_empty_lines=True, nice=True),
    'ugly':
        Format(no_empty_lines=True, ugly=True),
    'named':
        Format(no_empty_lines=True, named=True),
    'compact+named':
        Format(simple_whitespace=True, named=True),
    'newlines+named':
        Format(simple_whitespace=True, indent='', named=True),
    'pretty+named':
        Format(simple_whitespace=True, indent='\t', named=True),
    'wrap+named':
        Format(wrap=True, indent='', named=True),
    'compact+nice':
        Format(simple_whitespace=True, nice=True),
    'newlines+nice':
        Format(simple_whitespace=True, indent='', nice=True),
    'pretty+nice':
        Format(simple_whitespace=True, indent='\t', nice=True),
    'wrap+nice':
        Format(wrap=True, indent='', nice=True),
    'nice+named':
        Format(no_empty_lines=True, nice=True, named=True),
    'compact+named+nice':
        Format(simple_whitespace=True, nice=True, named=True),
    'newlines+named+nice':
        Format(simple_whitespace=True, indent='', nice=True),
    'pretty+named+nice':
        Format(simple_whitespace=True, indent='\t', nice=True, named=True),
    'wrap+named+nice':
        Format(wrap=True, indent='', nice=True, named=True),
    }

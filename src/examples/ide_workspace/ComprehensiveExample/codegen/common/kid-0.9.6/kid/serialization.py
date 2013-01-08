# -*- coding: utf-8 -*-

"""Infoset serialization formats (XML, XHTML, HTML, etc)"""

__revision__ = "$Rev: 492 $"
__date__ = "$Date: 2007-07-06 21:38:45 -0400 (Fri, 06 Jul 2007) $"
__author__ = "Ryan Tomayko (rtomayko@gmail.com)"
__copyright__ = "Copyright 2004-2005, Ryan Tomayko"
__license__ = "MIT <http://www.opensource.org/licenses/mit-license.php>"

import htmlentitydefs

try:
    set
except NameError: # fallback for Python 2.3
    from sets import Set as set

from kid.element import Element, Comment, ProcessingInstruction, \
    Fragment, QName, namespaces, encode_entity, raise_serialization_error
import kid.namespace as namespace
from kid.parser import START, END, TEXT, COMMENT, PI, _coalesce
from kid.format import Format, output_formats

__all__ = ['doctypes', 'Serializer', 'XMLSerializer', 'HTMLSerializer']

# bring in well known namespaces
xml_uri = namespace.xml.uri
xhtml_uri = namespace.xhtml.uri

# This is the default entity map:
default_entity_map = {}
for k, v in htmlentitydefs.codepoint2name.items():
    default_entity_map[unichr(k)] = "&%s;" % v

# Some common doctypes.
# You can pass doctype strings from here or doctype tuples to Serializers.
doctypes = {
    'wml': ('wml', "-//WAPFORUM//DTD WML 1.1//EN",
        "http://www.wapforum.org/DTD/wml_1.1.xml"),
    'xhtml-strict': ('html', "-//W3C//DTD XHTML 1.0 Strict//EN",
        "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd"),
    'xhtml': ('html', "-//W3C//DTD XHTML 1.0 Transitional//EN",
        "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"),
    'xhtml-frameset': ('html', "-//W3C//DTD XHTML 1.0 Frameset//EN",
        "http://www.w3.org/TR/xhtml1/DTD/xhtml1-frameset.dtd"),
    'html-strict': ('HTML', "-//W3C//DTD HTML 4.01//EN",
        "http://www.w3.org/TR/html4/strict.dtd"),
    'html': ('HTML', "-//W3C//DTD HTML 4.01 Transitional//EN",
        "http://www.w3.org/TR/html4/loose.dtd"),
    'html-frameset': ('HTML', "-//W3C//DTD HTML 4.01 Frameset//EN",
        "http://www.w3.org/TR/html4/frameset.dtd"),
    'html-quirks': ('HTML', '-//W3C//DTD HTML 4.01 Transitional//EN'),
    'html-frameset-quirks': ('HTML', "-//W3C//DTD HTML 4.01 Frameset//EN")
    }


class Serializer(object):

    namespaces = namespace.namespaces
    src_encoding = encoding = 'utf-8'
    format = output_formats['default']
    formatted = False
    inline = False

    def __init__(self, encoding=None, src_encoding=None,
            formatted=None, inline=None, format=None):
        """Initialize Serializer.

        You can change the following parameters:

        encoding: the output encoding
        src_encoding: the source encoding
        formatted: whether all tags should be considered formatted
        inline: whether all tags should be considered inline
        format: format to be applied (string or instance of Format)

        """
        if encoding is not None:
            self.encoding = encoding
        if src_encoding is not None:
            self.src_encoding = src_encoding
        if formatted is not None:
            self.formatted = formatted
        if inline is not None:
            self.inline = inline
        if format is not None:
            self.format = format
        self.format = self._get_format(format)

    def _get_format(self, format):
        if format is None:
            return self.format
        elif isinstance(format, basestring):
            return output_formats[format]
        else:
            return format

    def is_formatted(self, tagname):
        return self.formatted

    def is_inline(self, tagname):
        return self.inline

    def serialize(self, stream, encoding=None,
            fragment=False, format=None):
        try:
            text = ''.join(self.generate(stream, encoding, fragment, format))
        except TypeError: # workaround for bug 905389 in Python < 2.5
            text = ''.join(tuple(
                self.generate(stream, encoding, fragment, format)))
        if not fragment:
            text = Format.strip(text)
        return text

    def write(self, stream, file, encoding=None,
            fragment=False, format=None):
        needs_closed = False
        if not hasattr(file, 'write'):
            needs_closed = True
            file = open(file, 'wb')
        try:
            write = file.write
            for text in self.generate(stream, encoding, fragment, format):
                write(text)
        finally:
            # only close a file if it was opened locally
            if needs_closed:
                file.close()

    def generate(self, stream, encoding=None,
            fragment=False, format=None):
        pass

    def apply_filters(self, stream, format=None):
        stream = _coalesce(stream, self.src_encoding)
        if format:
            stream = self.format_stream(stream, format)
        return stream

    def format_stream(self, stream, format):
        """Apply format to stream.

        Note that this method is unaware of the serialization of the tags
        and does only take into account the text inside the stream. So the
        results may sometimes differ from what you expect when formatting
        the complete serialized output.

        """
        filter_text = format.filter
        indent, wrap = format.indent, format.wrap
        if indent is not None:
            indent_lines = format.indent_lines
            lstrip_blanks = format.lstrip_blanks
            rstrip_blanks = format.rstrip_blanks
            lstrip_lines = format.lstrip_lines
            min_level, max_level = format.min_level, format.max_level
            indent_level = []
            new_line = False
        if wrap is not None:
            wrap_lines = format.wrap_lines
            indent_width, new_offset = format.indent_width, format.new_offset
            offset = 0
        formatted = 0
        text = last_char = ''
        for ev, item in stream:
            if ev == TEXT:
                text += item
            else:
                if ev in (START, END):
                    tag = item.tag
                    if not formatted:
                        text = filter_text(text, last_char)
                        if indent is None:
                            if wrap is not None:
                                text = wrap_lines(text, wrap, offset)
                        else:
                            level = len(indent_level)
                            if max_level and level > max_level:
                                level = max_level
                            if min_level:
                                level -= min_level
                                if level < 0:
                                    level = 0
                            if wrap is not None:
                                text = wrap_lines(text, wrap, offset,
                                    indent_width(level*indent))
                            if '\n' in text and indent_level:
                                indent_level[-1] = True
                            if new_line:
                                if lstrip_blanks(text)[:1] != '\n':
                                    text = '\n' + lstrip_blanks(text)
                                    offset = 0
                                new_line = False
                            if tag == Comment or not self.is_inline(tag):
                                if ev == START:
                                    if indent_level:
                                        if rstrip_blanks(text)[-1:] != '\n':
                                            text = rstrip_blanks(text) + '\n'
                                        text = indent_lines(text, level*indent)
                                        indent_level[-1] = True
                                    elif text:
                                        text = lstrip_lines(text)
                                    if tag != Comment \
                                            and not self.is_formatted(tag):
                                        indent_level.append(False)
                                else:
                                    if indent_level:
                                        if indent_level.pop():
                                            if rstrip_blanks(text)[-1:] == '\n':
                                                text = rstrip_blanks(text)[:-1]
                                            text = indent_lines(text,
                                                level*indent)
                                            text = rstrip_blanks(text) + '\n'
                                            level = len(indent_level)
                                            if max_level and level > max_level:
                                                level = max_level
                                            if min_level:
                                                level -= min_level
                                                if level < 0:
                                                    level = 0
                                            text += level*indent
                                    elif text:
                                        text = lstrip_lines(text)
                                    new_line = True
                            elif text:
                                text = indent_lines(text, level*indent)
                    if tag == Comment or self.is_formatted(tag):
                        if ev == START:
                            formatted += 1
                        elif formatted:
                            formatted -= 1
                            new_line = True
                    yield TEXT, text
                    if wrap is not None:
                        offset = new_offset(text, offset)
                    last_char = text[-1:]
                    text = ''
                yield ev, item
        if text:
            if not formatted:
                text = filter_text(text, last_char)
                if wrap is not None:
                    text = wrap_lines(text, wrap, offset)
                if indent is None:
                    if wrap is not None:
                        text = wrap_lines(text, wrap, offset)
                else:
                    level = len(indent_level)
                    if max_level and level > max_level:
                        level = max_level
                    if min_level:
                        level -= min_level
                        if level < 0:
                            level = 0
                    if wrap is not None:
                        text = wrap_lines(text, wrap, offset,
                            indent_width(level*indent))
                    if rstrip_blanks(text)[-1:] == '\n':
                        text = text[:-1]
                    text = indent_lines(text, level*indent)
            yield TEXT, text


class XMLSerializer(Serializer):

    decl = True
    doctype = None
    entity_map = None

    def __init__(self, encoding=None,
            decl=None, doctype=None, entity_map=None, namespaces=None,
            formatted=None, inline=None, format=None):
        """Initialize XMLSerializer.

        You can change the following parameters:

        encoding: the output encoding
        decl: add xml declaration at the beginning (True/False)
        doctype: add doctype (None, string, tuple)
        entity_map: use named entities for output (True/False or mapping)
        namespaces: mapping of namespaces
        formatted: whether all tags should be considered formatted
        inline: whether all tags should be considered inline
        format: format to be applied (string or instance of Format)

        """
        Serializer.__init__(self, encoding=encoding,
            format=format, formatted=formatted, inline=inline)
        if decl is not None:
            self.decl = decl
        if doctype is not None:
            self.doctype = doctype
        if entity_map is not None:
            self.entity_map = entity_map
        if namespaces is not None:
            self.namespaces = namespaces

    def can_be_empty_element(self, item_name):
        return True

    def generate(self, stream, encoding=None,
            fragment=False, format=None):
        """Serializes an event stream to bytes of the specified encoding.

        This function yields an encoded string over and over until the
        stream is exhausted.

        """
        decl = self.decl
        doctype = self.doctype
        encoding = encoding or self.encoding or 'utf-8'
        entity_map = self.entity_map
        format = self._get_format(format)
        if format:
            if format.decl is not None:
                decl = format.decl
            if format.doctype is not None:
                doctype = format.doctype
            if format.entity_map is not None:
                entity_map = format.entity_map
        if entity_map == True:
            # if True, use default HTML entity map
            entity_map = default_entity_map
        elif entity_map == False:
            entity_map = None
        if isinstance(doctype, basestring):
            # allow doctype strings
            doctype = doctypes[self.doctype]

        escape_cdata = XMLSerializer.escape_cdata
        escape_attrib = XMLSerializer.escape_attrib

        lastev = None
        stream = iter(stream)
        names = NamespaceStack(self.namespaces)
        if not fragment:
            if decl:
                yield '<?xml version="1.0" encoding="%s"?>\n' % encoding
            if doctype is not None:
                yield serialize_doctype(doctype) + '\n'
        text = None
        for ev, item in self.apply_filters(stream, format):
            if ev in (START, END) and item.tag == Fragment:
                continue
            elif ev == TEXT:
                if text is not None:
                    text = u''.join([text, item])
                else:
                    text = item
                continue
            if lastev == START:
                if ev == END and (not text or not (Format.strip(text)
                        or self.is_formatted(item.tag))) \
                        and self.can_be_empty_element(item.tag):
                    yield ' />'
                    lastev = END
                    text = None
                    names.pop()
                    continue
                yield ">"
            if text:
                yield escape_cdata(text, encoding, entity_map)
                text = None
            if ev == START:
                if item.tag == Comment:
                    yield "<!--%s-->" % item.text.encode(encoding)
                    lastev = COMMENT
                    continue
                elif item.tag == ProcessingInstruction:
                    yield "<?%s?>" % item.text.encode(encoding)
                    lastev = PI
                    continue
                else:
                    current_names = names.current
                    names.push(namespaces(item, remove=True))
                    qname = names.qname(item.tag, default=True)
                    yield "<" + qname.encode(encoding)
                    for k, v in item.attrib.items():
                        k = names.qname(k, default=False).encode(encoding)
                        v = escape_attrib(v, encoding)
                        yield ' %s="%s"' % (k, v)
                    for prefix, uri in names.current.items():
                        if prefix not in current_names \
                                or current_names[prefix] != uri:
                            v = escape_attrib(uri, encoding)
                            if prefix:
                                k = 'xmlns:' + prefix.encode(encoding)
                            else:
                                k = 'xmlns'
                            yield ' %s="%s"' % (k, v)
            elif ev == END and item.tag not in (
                    Comment, ProcessingInstruction):
                qname = names.qname(item.tag, default=True)
                yield "</%s>" % qname.encode(encoding)
                names.pop()
            lastev = ev
        if fragment and text:
            yield escape_cdata(text, encoding, entity_map)
        return

    def escape_cdata(text, encoding=None, entity_map=None):
        """Escape character data."""
        try:
            if encoding:
                try:
                    text = text.encode(encoding)
                except UnicodeError:
                    return encode_entity(text, entities=entity_map)
            text = text.replace("&", "&amp;")
            text = text.replace("<", "&lt;")
        except (TypeError, AttributeError):
            raise_serialization_error(text)
        return text
    escape_cdata = staticmethod(escape_cdata)

    def escape_attrib(text, encoding=None, entity_map=None):
        """Escape attribute value."""
        try:
            if encoding:
                try:
                    text = text.encode(encoding)
                except UnicodeError:
                    return encode_entity(text, entities=entity_map)
            text = text.replace("&", "&amp;")
            text = text.replace("<", "&lt;")
            text = text.replace("\"", "&quot;")
        except (TypeError, AttributeError):
            raise_serialization_error(text)
        return text
    escape_attrib = staticmethod(escape_attrib)



class HTMLBased(object):
    """Mixin class for HTML based serializers."""

    inject_type = None

    empty_elements = set([
        'area', 'base', 'basefont', 'br', 'col', 'frame', 'hr',
        'img', 'input', 'isindex', 'link', 'meta', 'param'])
    formatted_elements = set([
        'code', 'kbd', 'math', 'pre', 'script', 'textarea'])
    inline_elements = set(['a', 'abbr', 'acronym', 'b', 'basefont',
        'bdo', 'big', 'br', 'cite', 'code', 'dfn', 'em', 'font',
        'i', 'img', 'input', 'kbd', 'label', 'q', 's', 'samp',
        'select', 'small', 'span', 'strike', 'strong', 'sub', 'sup',
        'textarea', 'tt', 'u', 'var'])

    def can_be_empty_element(self, tag):
        return self.tagname(tag) in self.empty_elements

    def is_formatted(self, tag):
        return self.tagname(tag) in self.formatted_elements

    def is_inline(self, tag):
        return self.tagname(tag) in self.inline_elements

    def is_escape(self, tag):
        return self.tagname(tag) not in self.noescape_elements

    def inject_meta_content_type(self, stream, encoding):
        """Injects a meta tag for the content-type."""
        return self.inject_meta_tags(stream,
            [{'http-equiv': 'content-type',
                'content': 'text/html; charset=%s' % encoding}])

    def inject_meta_tags(self, stream, taglist):
        """Injects meta tags at the start of the head section.

        If meta tags already exist at that position, they are kept.
        Expects a list of meta-tag attributes with content keys.
        The attribute names and values must be given in lower case.
        """
        done = False
        meta_tag = None
        for ev, item in stream:
            if not done:
                if ev in (START, END):
                    tag = self.tagname(item.tag)
                    if meta_tag:
                        if item.tag == meta_tag:
                            if ev == START:
                                for attributes in taglist:
                                    for attrib, value in item.items():
                                        attrib = attrib.lower()
                                        if attrib == 'content':
                                            continue
                                        if attrib not in attributes:
                                            break
                                        value = value.lower()
                                        if attributes[attrib] != value:
                                            break
                                    else:
                                        # that meta tag exists already
                                        attributes['content'] = None
                                        break
                        else:
                            for attributes in taglist:
                                if attributes['content'] is None:
                                    continue
                                meta_item = Element(meta_tag, **attributes)
                                yield START, meta_item
                                yield END, meta_item
                                yield TEXT, '\n'
                            done = True
                    elif tag == 'head' and ev == START:
                        meta_tag = item.tag[:-4] + 'meta'
            yield ev, item


class HTMLSerializer(HTMLBased, Serializer):

    doctype = doctypes['html']
    transpose = False
    inject_type = True
    entity_map = None

    noescape_elements = set([
        'script', 'style'])
    boolean_attributes = set(
        ['selected', 'checked', 'compact', 'declare',
        'defer', 'disabled', 'ismap', 'multiple', 'nohref',
        'noresize', 'noshade', 'nowrap'])

    def __init__(self, encoding='utf-8', doctype=None, transpose=False,
            inject_type=None, entity_map=None, format=None):
        """Initialize HTMLSerializer.

        You can change the following parameters:

        encoding: the output encoding
        doctype: add doctype (None, string, tuple)
        transpose: alter tag names (None, True/False, callable)
        inject_type: inject content type (True/False)
        entity_map: use named entities for output (True/False or mapping)
        format: format to be applied (string or instance of Format)

        """
        Serializer.__init__(self, encoding=encoding, format=format)
        if doctype is not None:
            self.doctype = doctype
        self.transpose = transpose
        if inject_type is not None:
            self.inject_type = inject_type
        if entity_map is not None:
            self.entity_map = entity_map

    def tagname(tag):
        """Remove namespace from tag and make it lowercase."""
        if isinstance(tag, basestring):
            if tag.startswith('{'):
                tag = tag.split('}', 1)[-1]
            tag = tag.lower()
        return tag
    tagname = staticmethod(tagname)

    def is_escape(self, tag):
        return self.tagname(tag) not in self.noescape_elements

    def is_boolean_attribute(self, attribute):
        return attribute in self.boolean_attributes

    def generate(self, stream, encoding=None,
            fragment=False, format=None):
        """Serializes an event stream to bytes of the specified encoding.

        This function yields an encoded string over and over until the
        stream is exhausted.

        """
        doctype = self.doctype
        encoding = encoding or self.encoding or 'utf-8'
        entity_map = self.entity_map
        transpose = self.transpose
        inject_type = self.inject_type
        format = self._get_format(format)
        if format:
            if format.doctype is not None:
                doctype = format.doctype
            if format.entity_map is not None:
                entity_map = format.entity_map
            if format.transpose is not None:
                transpose = format.transpose
            if format.inject_type is not None:
                inject_type = format.inject_type
        if entity_map == True:
            # if True, use default HTML entity map
            entity_map = default_entity_map
        elif entity_map == False:
            entity_map = None
        if isinstance(doctype, basestring):
            # allow doctype strings
            doctype = doctypes[self.doctype]
        if transpose is not None:
            if not callable(transpose):
                if transpose:
                    transpose = lambda s: s.upper()
                else:
                    transpose = lambda s: s.lower()

        escape_cdata = HTMLSerializer.escape_cdata
        escape_attrib = HTMLSerializer.escape_attrib
        names = NamespaceStack(self.namespaces)

        def grok_name(tag):
            if tag[0] == '{':
                uri, localname = tag[1:].split('}', 1)
            else:
                uri, localname = None, tag
            if uri and uri != xhtml_uri:
                qname = names.qname(tag, default=False)
            else:
                qname = localname
                if transpose:
                    qname = transpose(qname)
            return uri, localname, qname

        current = None
        stack = [current]
        stream = iter(stream)

        if not fragment and doctype is not None:
            yield serialize_doctype(doctype) + '\n'

        if inject_type and encoding:
            stream = self.inject_meta_content_type(stream, encoding)

        for ev, item in self.apply_filters(stream, format):
            if ev == TEXT and item:
                escape = self.is_escape(current)
                yield escape_cdata(item, encoding, entity_map, escape)
            elif ev == START:
                if item.tag == Comment:
                    yield "<!--%s-->" % item.text.encode(encoding)
                    continue
                elif item.tag == ProcessingInstruction:
                    yield "<?%s>" % item.text.encode(encoding)
                    continue
                elif item.tag == Fragment:
                    continue
                else:
                    names.push(namespaces(item, remove=True))
                    tag = item.tag
                    qname = grok_name(tag)[2]
                    # push this name on the stack so we know where we are
                    current = qname.lower()
                    stack.append(current)
                    yield "<" + qname.encode(encoding)
                    attrs = item.attrib.items()
                    if attrs:
                        for k, v in attrs:
                            q = grok_name(k)[2]
                            lq = q.lower()
                            if lq == 'xml:lang':
                                continue
                            if self.is_boolean_attribute(lq):
                                # XXX: what if v is 0, false, or no.
                                #      should we omit the attribute?
                                yield ' %s' % q.encode(encoding)
                            else:
                                yield ' %s="%s"' % (
                                    q.encode(encoding),
                                    escape_attrib(v, encoding, entity_map))
                    yield ">"
            elif ev == END and item.tag not in (
                    Comment, ProcessingInstruction, Fragment):
                current = stack.pop()
                if not self.can_be_empty_element(current):
                    tag = item.tag
                    qname = grok_name(tag)[2]
                    yield "</%s>" % qname.encode(encoding)
                current = stack[-1]
                names.pop()

    def escape_cdata(text, encoding=None, entity_map=None, escape=True):
        """Escape character data."""
        try:
            if encoding:
                try:
                    text = text.encode(encoding)
                except UnicodeError:
                    return encode_entity(text, entities=entity_map)
            if escape:
                text = text.replace("&", "&amp;")
                text = text.replace("<", "&lt;")
        except (TypeError, AttributeError):
            raise_serialization_error(text)
        return text
    escape_cdata = staticmethod(escape_cdata)

    def escape_attrib(text, encoding=None, entity_map=None):
        """Escape attribute value."""
        try:
            if encoding:
                try:
                    text = text.encode(encoding)
                except UnicodeError:
                    return encode_entity(text, entities=entity_map)
            text = text.replace("&", "&amp;")
            text = text.replace("\"", "&quot;")
        except (TypeError, AttributeError):
            raise_serialization_error(text)
        return text
    escape_attrib = staticmethod(escape_attrib)


class XHTMLSerializer(HTMLBased, XMLSerializer):

    doctype = doctypes['xhtml']
    inject_type = True

    def __init__(self, encoding='utf-8', decl=None, doctype=None,
            inject_type=None, entity_map=None, namespaces=None, format=None):
        """Initialize XHTMLSerializer."""
        XMLSerializer.__init__(self, encoding=encoding,
            decl=decl, doctype=doctype, entity_map=entity_map,
            namespaces=namespaces, format=format)
        if inject_type is not None:
            self.inject_type = inject_type

    def tagname(tag):
        """Remove namespace from tag."""
        if isinstance(tag, basestring) and tag.startswith('{%s}' % xhtml_uri):
            tag = tag.split('}', 1)[-1]
        return tag
    tagname = staticmethod(tagname)

    def generate(self, stream, encoding=None,
            fragment=False, format=None):
        encoding = encoding or self.encoding or 'utf-8'
        inject_type = self.inject_type
        format = self._get_format(format)
        if format:
            if format.inject_type is not None:
                inject_type = format.inject_type
        if inject_type and encoding:
            stream = self.inject_meta_content_type(stream, encoding)
        return XMLSerializer.generate(self, stream, encoding=encoding,
            fragment=fragment, format=format)


class PlainSerializer(Serializer):

    def generate(self, stream, encoding=None,
            fragment=False, format=None):
        """Generate only the text content."""
        encoding = encoding or self.encoding or 'utf-8'
        for ev, item in self.apply_filters(stream, format):
            if ev == TEXT:
                yield item


class NamespaceStack:
    """Maintains a stack of namespace prefix to URI mappings."""

    def __init__(self, default_map=namespace.namespaces):
        self.stack = []
        self.default_map = default_map
        self.push()
        self.ns_count = 0

    def push(self, names=None):
        if names is None:
            names = {}
        self.current = names
        self.stack.insert(0, self.current)

    def pop(self):
        names = self.stack.pop(0)
        if self.stack:
            self.current = self.stack[0]
        return names

    def resolve_prefix(self, uri, default=True):
        """Figure out prefix given a URI."""
        if uri == xml_uri:
            return 'xml'
        # first check if the default is correct
        is_default = None
        prefix = None
        for names in self.stack:
            for p, u in names.items():
                if default and is_default is None and not p:
                    # this is the current default namespace
                    is_default = (u == uri)
                    if (default and is_default) or prefix:
                        break
                if u == uri and p:
                    prefix = p
                    if is_default is not None:
                        break
        if default and is_default == True:
            return ''
        elif prefix:
            return prefix
        else:
            return None

    def resolve_uri(self, prefix):
        """Figure out URI given a prefix."""
        if prefix == 'xml':
            return xml_uri
        for names in self.stack:
            uri = names.get(prefix)
            if uri:
                return uri
        return None

    def qname(self, cname, default=False):
        if isinstance(cname, QName):
            cname = cname.text
        if cname[0] != '{':
            # XXX: need to make sure default namespace is "no-namespace"
            return cname
        uri, localname = cname[1:].split('}', 1)
        prefix = self.resolve_prefix(uri, default)
        if prefix is None:
            # see if we have it in our default map
            prefix = self.default_map.get(uri)
            if prefix is not None:
                self.current[prefix] = uri
            else:
                if default and not self.current.has_key(''):
                    prefix = ''
                    self.current[prefix] = uri
                else:
                    self.ns_count += 1
                    # XXX : need to check for collisions here.
                    prefix = 'ns%d' % self.ns_count
                    self.current[prefix] = uri
        if prefix != '':
            return '%s:%s' % (prefix, localname)
        else:
            return localname


def serialize_doctype(doctype):
    if isinstance(doctype, basestring):
        return doctype
    elif len(doctype) == 1:
        return '<!DOCTYPE %s>' % doctype
    elif len(doctype) == 2:
        return '<!DOCTYPE %s PUBLIC "%s">' % doctype
    else:
        return '<!DOCTYPE %s PUBLIC "%s" "%s">' % doctype

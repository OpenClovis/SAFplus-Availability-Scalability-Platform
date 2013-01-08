# -*- coding: utf-8 -*-

"""Pull-style interface for ElementTree."""

__revision__ = "$Rev: 492 $"
__date__ = "$Date: 2007-07-06 21:38:45 -0400 (Fri, 06 Jul 2007) $"
__author__ = "Ryan Tomayko (rtomayko@gmail.com)"
__copyright__ = "Copyright 2004-2005, Ryan Tomayko"
__license__ = "MIT <http://www.opensource.org/licenses/mit-license.php>"

import htmlentitydefs
from xml.parsers import expat

from kid.element import Element, Comment, ProcessingInstruction
from kid.util import open_resource, QuickTextReader, get_expat_error

__all__ = ['ElementStream', 'XML', 'document', 'Parser', 'ExpatParser',
    'START', 'END', 'TEXT', 'COMMENT', 'PI', 'XML_DECL', 'DOCTYPE', 'LOCATION']


# This is the default entity map:
default_entity_map = {}
default_external_dtd = []
for k, v in htmlentitydefs.name2codepoint.items():
    default_entity_map[k] = unichr(v)
    default_external_dtd.append('<!ENTITY %s "&#%d;">' % (k, v))
default_external_dtd = '\n'.join(default_external_dtd)


class InvalidStreamState(Exception):
    def __init__(self, msg="Invalid stream state."):
        Exception.__init__(self, msg)


def XML(text, fragment=True, encoding=None, xmlns=None, entity_map=None):
    """Convert XML string into an ElementStream."""
    if text.startswith('<?xml ') or text.startswith('<!DOCTYPE '):
        fragment = False
    if fragment:
        # allow XML fragments
        if xmlns:
            text = '<xml xmlns="%s">%s</xml>' % (xmlns, text)
        else:
            text = '<xml>%s</xml>' % text
        if isinstance(text, unicode):
            encoding = 'utf-16'
            text = text.encode(encoding)
        p = Parser(QuickTextReader(text), encoding, entity_map)
        return ElementStream(_coalesce(p, encoding=encoding)).strip()
    else:
        if isinstance(text, unicode):
            encoding = 'utf-16'
            text = text.encode(encoding)
        p = Parser(QuickTextReader(text), encoding, entity_map)
        return ElementStream(_coalesce(p, encoding=encoding))


def document(file, encoding=None, filename=None, entity_map=None, debug=False):
    """Convert XML document into an Element stream."""
    if not hasattr(file, 'read'):
        if filename is None:
            filename = file
        file = open_resource(file, 'rb')
    else:
        if filename is None:
            filename = '<string>'
    p = Parser(file, encoding, entity_map, debug)
    p._filename = filename
    return ElementStream(_coalesce(p, encoding=encoding))


class ElementStream(object):
    """Provides a pull/streaming interface to ElementTree.

    Instances of this class are iterable. Most methods of the class act on
    the Element that is currently being visited.

    """

    def __init__(self, stream, current=None):
        """Create an ElementStream.

        stream - an iterator that returns ElementStream events.
        current - If an Element is provided in this parameter than
                  it is yielded as the first element in the stream.

        """
        if hasattr(stream, 'tag') and hasattr(stream, 'attrib'):
            stream = self._pull(stream, tail=True)
        self.current = None
        self._iter = self._track(iter(stream), current)

    def __iter__(self):
        return self._iter

    def expand(self):
        """Expand the current item in the stream as an Element.

        In the case where there is no current element and no single
        root element, this will return a list of Elements.

        """
        current = self.current
        if current is None:
            current = []
        stack = [current]
        this, last = current, None
        for ev, item in self._iter:
            if ev == START:
                current, last = item, None
                stack[-1].append(current)
                stack.append(current)
            elif ev == END:
                last = stack.pop()
                assert last is item
                if not stack:
                    break
            elif ev == TEXT:
                if last is not None:
                    last.tail = item
                elif not isinstance(current, list):
                    current.text = item
        if isinstance(this, list) and len(this) == 1:
            this = this[0]
        return this

    def strip(self, levels=1):
        """Return new ElementStream with first element level(s) stripped."""
        def strip(depth):
            for ev, item in self._iter:
                if ev == END:
                    depth -= 1
                    if depth == 0:
                        break
                    elif depth < 0:
                        raise InvalidStreamState()
                if depth >= levels:
                    yield ev, item
                if ev == START:
                    depth += 1
        depth = self.current is not None and 1 or 0
        return ElementStream(strip(depth))

    def eat(self):
        """Eat the current element and all decendant items."""
        depth = self.current is not None and 1 or 0
        for ev, item in self._iter:
            if ev == START:
                depth += 1
            elif ev == END:
                depth -= 1
                if depth == 0:
                    break
        return self

    def _pull(self, elem, tail=False):
        """Make a stream from an Element."""
        orig = elem
        elem = Element(orig.tag, dict(orig.attrib))
        ## XXX: find a better way
        if elem.tag in (Comment, ProcessingInstruction):
            elem.text = orig.text
            orig.text = None
        yield START, elem
        if orig.text:
            yield TEXT, orig.text
        for child in orig.getchildren():
            for event in self._pull(child, tail=True):
                yield event
        yield END, elem
        if tail and orig.tail:
            yield TEXT, orig.tail

    def _track(self, stream, current=None):
        """Track current item in stream."""
        if current is not None:
            self.current = current
            yield START, current
        for p in stream:
            ev, item = p
            if ev == START:
                self.current = item
            elif ev == END:
                self.current = None
            yield ev, item

    def ensure(cls, stream, current=None):
        """Ensure the stream is an ElementStream."""
        if isinstance(stream, cls):
            return stream
        else:
            return cls(stream, current)
    ensure = classmethod(ensure)


def to_unicode(value, encoding):
    if isinstance(value, unicode):
        return value

    if hasattr(value, '__unicode__'):
        return unicode(value)

    if not isinstance(value, str):
        value = str(value)

    return unicode(value, encoding)


def _coalesce(stream, encoding):
    """Coalesces TEXT events and namespace events.

    Fold multiple sequential TEXT events into a single event.

    The 'encoding' attribute is for the source strings.

    """
    textbuf = []
    namespaces = []
    last_ev = None
    stack = [None]
    for ev, item in stream:
        if ev == TEXT:
            textbuf.append(item)
            last_ev = TEXT
            continue
        if last_ev == TEXT:
            text = u""
            for value in textbuf:
                text += to_unicode(value, encoding)
            textbuf = []
            if text:
                yield TEXT, text
        if ev == START:
            attrib = item.attrib
            for prefix, uri in namespaces:
                if prefix:
                    attrib['xmlns:%s' % prefix] = uri
                else:
                    attrib['xmlns'] = uri
            namespaces = []
            current = item
            stack.append(item)
        elif ev == END:
            current = stack.pop()
        elif ev == START_NS:
            prefix, uri = item
            namespaces.append((prefix, uri))
            continue
        elif ev == END_NS:
            continue
        yield ev, item
    if last_ev == TEXT:
        text = u""
        for value in textbuf:
            text += to_unicode(value, encoding)
        if text:
            yield TEXT, text


# Common Events
START = 1
END = 2
TEXT = 3
DOCTYPE = 4
XML_DECL = 5

# These events aren't used after initial parsing
START_NS = 10
END_NS = 11
PI = 12
COMMENT = 13

# This is for forwarding the location in the XML document
LOCATION = 20


def Parser(source, encoding=None, entity_map=None, debug=False):
    return ExpatParser(source, encoding, entity_map, debug)


# Most of the following copied from ElementTree.XMLTreeBuilder.
# Differences from ElementTree implementation:
#
#   * Specialized for generator based processing. Elements are built
#     using a iterator approach instead of the TreeBuilder approach.
#
#   * Support for DOCTYPE, Comment, and Processing Instruction nodes.

class ExpatParser(object):

    def __init__(self, source, encoding=None, entity_map=None, debug=False):
        if hasattr(source, 'read'):
            filename = '<string>'
        else:
            filename = source
            source = open(source, 'rb')
        self._filename = filename
        self._source = source
        self._encoding = encoding
        self._parser = parser = expat.ParserCreate(encoding, "}")
        self._queue = []

        try:
            self._parser.CurrentLineNumber
        except AttributeError:
            # Expat in Python < 2.4 does not provide the location,
            # silently switch off the debug mode in this case
            debug = False
        if debug:
            self.push = self._push_location
        else:
            self.push = self._push
        self.debug = debug

        # callbacks
        parser.DefaultHandler = self._default
        parser.StartElementHandler = self._start
        parser.EndElementHandler = self._end
        parser.CharacterDataHandler = self._data
        parser.ProcessingInstructionHandler = self._pi
        parser.CommentHandler = self._comment
        parser.StartNamespaceDeclHandler = self._start_ns
        parser.EndNamespaceDeclHandler = self._end_ns
        parser.XmlDeclHandler = self._xmldecl_handler
        parser.StartDoctypeDeclHandler = self._doctype_handler

        # attributes
        # (these should all become customizable at some point)
        parser.buffer_text = True
        parser.ordered_attributes = False
        parser.specified_attributes = True
        self._doctype = None
        if entity_map:
            self.entity = entity_map
        else:
            self.entity = default_entity_map
        self.external_dtd = default_external_dtd
        # setup entity handling
        parser.SetParamEntityParsing(expat.XML_PARAM_ENTITY_PARSING_ALWAYS)
        parser.ExternalEntityRefHandler = self._buildForeign
        parser.UseForeignDTD()

    def _buildForeign(self, context, base, systemId, publicId):
        import StringIO
        parseableFile = StringIO.StringIO(default_external_dtd)
        original_parser = self._parser
        self._parser = self._parser.ExternalEntityParserCreate(context)
        self._parser.ParseFile(parseableFile)
        self._parser = original_parser
        return True

    def _push(self, ev, stuff):
        self._queue.append((ev, stuff))

    def _push_location(self, ev, stuff):
        self._push(LOCATION, (self._parser.CurrentLineNumber,
            self._parser.CurrentColumnNumber))
        self._push(ev, stuff)

    def _expat_stream(self):
        bufsize = 4 * 1024 # 4K
        feed = self.feed
        read = self._source.read
        more = True
        while more:
            while more and not self._queue:
                data = read(bufsize)
                if data == '':
                    self.close()
                    more = False
                else:
                    feed(data)
            for item in self._queue:
                yield item
            self._queue = []

    def __iter__(self):

        clarknames = {}
        def clarkname(key):
            try:
                name = clarknames[key]
            except KeyError:
                if "}" in key:
                    name = "{" + key
                else:
                    name = key
                clarknames[key] = name
            return name

        stack = []
        current = None
        for ev, stuff in self._expat_stream():
            if ev == TEXT:
                yield TEXT, stuff
            elif ev == START:
                tag, attrib = stuff
                tag = clarkname(tag)
                attrib = dict([(clarkname(key), value)
                    for key, value in attrib.items()])
                parent = current
                current = Element(tag, attrib)
                stack.append(current)
                yield START, current
            elif ev == END:
                current = stack.pop()
                assert clarkname(stuff) == current.tag
                parent = len(stack) and stack[-1] or None
                yield END, current
            elif ev == COMMENT:
                current = Comment(stuff)
                yield START, current
                yield END, current
            elif ev == PI:
                current = ProcessingInstruction(*stuff)
                yield START, current
                yield END, current
            else:
                yield ev, stuff

    def feed(self, data, isfinal=False):
        try:
            self._parser.Parse(data, isfinal)
        except expat.ExpatError, e:
            e.filename = self._filename
            e.source = self._source
            try:
                e = 'Error parsing XML%s:\n%s%s' % (
                    e.filename and e.filename != '<string>'
                    and (' file %r' % e.filename) or '',
                    get_expat_error(e), str(e))
            except Exception:
                pass
            raise expat.ExpatError(e)

    def close(self):
        if hasattr(self, '_parser'):
            try:
                self.feed('', True)  # end of data
            finally:
                del self._parser # get rid of circular references

    def _start(self, tag, attrib):
        self.push(START, (tag, attrib))

    def _data(self, text):
        self.push(TEXT, text)

    def _end(self, tag):
        self.push(END, tag)

    def _default(self, text):
        if text.startswith('&'):
            # deal with undefined entities
            try:
                self.push(TEXT, self.entity[text[1:-1]])
            except KeyError:
                raise expat.error("undefined entity %s: line %d, column %d"
                    % (text, self._parser.ErrorLineNumber,
                        self._parser.ErrorColumnNumber))
        else:
            # XXX not sure what should happen here.
            # This gets: \n at the end of documents?, <![CDATA[, etc..
            pass

    def _pi(self, target, data):
        self.push(PI, (target, data))

    def _comment(self, text):
        self.push(COMMENT, text)

    def _start_ns(self, prefix, uri):
        self.push(START_NS, (prefix or '', uri))

    def _end_ns(self, prefix):
        self.push(END_NS, prefix or '')

    def _xmldecl_handler(self, version, encoding, standalone):
        self.push(XML_DECL, (version, encoding, standalone))

    def _doctype_handler(self, name, sysid, pubid, has_internal_subset):
        self.push(DOCTYPE, (name, pubid, sysid))

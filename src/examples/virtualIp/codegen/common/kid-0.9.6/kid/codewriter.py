# -*- coding: utf-8 -*-

"""KidWriter

Write Python source code from XML.

"""

__revision__ = "$Rev: 496 $"
__date__ = "$Date: 2007-07-15 18:14:35 -0400 (Sun, 15 Jul 2007) $"
__author__ = "Ryan Tomayko (rtomayko@gmail.com)"
__copyright__ = "Copyright 2004-2005, Ryan Tomayko"
__license__ = "MIT <http://www.opensource.org/licenses/mit-license.php>"

import sys, re
from os.path import splitext
from traceback import extract_tb, format_exception_only

from kid import __version__, Namespace
from kid.parser import document, START, END, TEXT, XML_DECL, DOCTYPE, LOCATION
from kid.element import namespaces, Comment, ProcessingInstruction

__all__ = ['KID_XMLNS', 'KID_PREFIX', 'kidns', 'raise_template_error']

# the Kid XML namespace
KID_XMLNS = "http://purl.org/kid/ns#"
KID_PREFIX = 'py'
kidns = Namespace(KID_XMLNS)
QNAME_FOR = kidns['for']
QNAME_IF = kidns['if']
QNAME_DEF = kidns['def']
QNAME_SLOT = kidns['slot']
QNAME_CONTENT = kidns['content']
QNAME_REPLACE = kidns['replace']
QNAME_MATCH = kidns['match']
QNAME_STRIP = kidns['strip']
QNAME_ATTRIBUTES = kidns['attrs']
QNAME_EXTENDS = kidns['extends']
QNAME_LAYOUT = kidns['layout']

# deprectaed
QNAME_OMIT = kidns['omit']
QNAME_REPEAT = kidns['repeat']

# the Kid processing instruction name
KID_PI = 'python'
KID_ALT_PI = 'py'
KID_OLD_PI = 'kid'


def parse(source, encoding=None, filename=None, entity_map=None):
    doc = document(source, encoding=encoding,
        filename=filename, entity_map=entity_map)
    return KidWriter(doc, encoding, filename).parse()

def parse_file(filename, encoding=None, entity_map=None):
    """Parse the file specified.

    filename -- the name of a file.
    fp       -- an optional file like object to read from. If not specified,
                filename is opened.

    """
    source = open(filename, 'rb')
    try:
        return parse(source, encoding, filename, entity_map)
    finally:
        source.close()

def error_location(filename, encoding=None, entity_map=None, lineno=None):
    if lineno:
        try:
            source = open(filename, 'rb')
            try:
                doc = document(source, encoding=encoding,
                    filename=filename, entity_map=entity_map, debug=True)
                writer = KidWriter(doc, encoding, filename, lineno)
                return writer.parse()
            finally:
                source.close()
        except Exception:
            pass

def TemplateExceptionError(error, add_message):
    """Get exception with additional error message."""
    Error = error.__class__
    class TemplateExceptionError(Error):
        def __init__(self):
            for arg in dir(error):
                if not arg.startswith('_'):
                    setattr(self, arg, getattr(error, arg))
        def __str__(self):
            return str(error) + '\n' + add_message
    TemplateExceptionError.__name__ = Error.__name__
    TemplateExceptionError.__module__ = Error.__module__
    return TemplateExceptionError()

def raise_template_error(module=None, filename=None, encoding=None):
    """Raise template error along with additional context information.

    If the module containing the erroneous code has been compiled from
    a Kid template, try to compile that template with additional debug
    information, and display the location in the Kid template file
    corresponding to the erroneous code.

    """
    if module and not (filename and encoding):
        if module in sys.modules:
            mod = sys.modules[module]
            if hasattr(mod, 'encoding'):
                encoding = mod.encoding
            if hasattr(mod, 'kid_file'):
                filename = mod.kid_file
    if not filename or filename == '<string>':
        raise
    if not encoding:
        encoding = 'utf-8'
    py_file = splitext(filename)[0] + '.py'
    exc_type, exc_value = sys.exc_info()[:2]
    if exc_type == SyntaxError:
        tb = [(py_file, exc_value.lineno)]
    else:
        tb = extract_tb(sys.exc_info()[2])
        tb.reverse()
    for t in tb:
        if py_file != t[0] or not t[1]:
            continue
        location = error_location(filename, encoding, lineno=t[1])
        if not location:
            continue
        (start_line, start_col), (end_line, end_col) = location
        if start_line > end_line:
            continue
        s = []
        if not end_col and end_line > start_line:
            end_line -= 1
            end_col = -1
        if start_line == end_line:
            s.append('on line %d' % start_line)
            if start_col == end_col:
                s.append(', column %d' % start_col)
            elif start_col:
                if end_col > start_col:
                    s.append(' between columns %d and %d'
                        % (start_col, end_col))
                else:
                    s.append(' after column %d' % start_col)
            elif end_col > 0:
                s.append(' before column %d' % end_col)
        else:
            s.append('between line %d' % start_line)
            if start_col:
                s.append(', column %d' % start_col)
            s.append(' and line %d' % end_line)
            if end_col > 0:
                s.append(', column %d' % end_col)
        if s:
            s = ''.join(s)
            try:
                start_line -= 1
                end_line -= 1
                error_line, error_text = [], []
                for line, text in enumerate(open(filename)):
                    if line < start_line:
                        continue
                    text = text.rstrip()
                    if text:
                        if line == start_line and start_col:
                            if text[:start_col].rstrip():
                                text = text[start_col:].lstrip()
                                if text:
                                    text = '... ' + text
                        if line == end_line and end_col > 0:
                            if text[end_col:].lstrip():
                                if end_col > 75:
                                    end_col = 75
                                text = text[:end_col].rstrip()
                                if text:
                                    text += ' ...'
                            else:
                                text = text[:end_col].rstrip()
                        if len(text) > 79:
                            text = text[:75].rstrip() + ' ...'
                        if text:
                            if len(error_line) < 3:
                                error_line.append(line)
                                error_text.append(text)
                            else:
                                error_line[2] = line
                                error_text[2] = text
                    if line >= end_line:
                        break
                if not error_line:
                    raise LookupError, 'error line not found'
                if len(error_line) == 2:
                    if error_line[1] - error_line[0] > 1:
                        error_text.insert(1, '...')
                elif len(error_line) == 3:
                    if error_line[2] - error_line[0] > 2:
                        error_text[1] = '...'
                s = [s + ':'] + error_text
            except Exception, e:
                s = [s, '(cannot acquire source text: %s)' % str(e)]
            s.insert(0, 'Error location in template file %r' % filename)
            break
    else:
        s = ['Error in code generated from template file %r' % filename]
    s = ''.join(format_exception_only(exc_type, exc_value)[:-1]) + '\n'.join(s)
    if isinstance(exc_type, str):
        exc_type += '\n' + s
    else:
        exc_value = TemplateExceptionError(exc_value, s)
        exc_type = exc_value.__class__
    raise exc_type, exc_value, sys.exc_info()[2]


class KidWriter(object):

    def __init__(self, stream, encoding=None, filename=None, lineno=None):
        self.stream = stream
        self.encoding = encoding or 'utf-8'
        self.filename = filename
        self.depth = 0
        self.lineno = lineno
        self.location = None
        self.locations = []
        self.module_code = self.codegen()
        self.class_code = self.codegen()
        self.expand_code = self.codegen(level=1)
        self.end_module_code = self.codegen()
        self.module_defs = []
        self.inst_defs = []

    def codegen(self, code=None, level=0, tab='\t'):
        if self.lineno:
            return LocationGenerator(code, self.getloc)
        else:
            return CodeGenerator(code, level, tab)

    def getloc(self):
        return self.location

    def parse(self):
        self.begin()
        self.proc_stream(self.module_code)
        self.end()
        parts = []
        parts += self.module_code.code
        for c in self.module_defs:
            parts += c.code
        parts += self.class_code.code
        parts += self.expand_code.code
        for c in self.inst_defs:
            parts += c.code
        parts += self.end_module_code.code
        if self.lineno:
            lineno = self.lineno - 1
            if not 0 <= lineno < len(parts):
                return None
            pos = parts[lineno]
            if not pos:
                return None
            pos, is_start = pos
            if not 0 <= pos < len(self.locations):
                return None
            start_loc = self.locations[pos]
            pos += is_start and 1 or -1
            if 0 <= pos < len(self.locations):
                end_loc = self.locations[pos]
            else:
                end_line = start_loc[0]
                if is_start:
                    end_line += 1
                end_loc = (end_line, 0)
            if not is_start:
                start_loc, end_loc = end_loc, start_loc
            return start_loc, end_loc
        return '\n'.join(parts)

    def begin(self):
        code = self.module_code
        # Start with PEP 0263 encoding declaration
        code.line('# -*- coding: %s -*-' % self.encoding,
            '# Kid template module',
            # version against which the file has been compiled
            'kid_version = %r' % __version__,
            # source from which the file has been compiled
            'kid_file = %r' % self.filename,
            # imports
            'import kid',
            'from kid.template_util import *',
            'import kid.template_util as template_util',
            '_def_names = []',
            # default variables (can be overridden by template)
            'encoding = "%s"' % self.encoding,
            'doctype = None',
            'omit_namespaces = [kid.KID_XMLNS]',
            'layout_params = {}',
            # module methods
            'def pull(**kw): return Template(**kw).pull()',
            "def generate(encoding=encoding, fragment=False,"
                " output=None, format=None, **kw):"
                " return Template(**kw).generate(encoding=encoding,"
                " fragment=fragment, output=output, format=format)",
            "def serialize(encoding=encoding, fragment=False,"
                " output=None, format=None, **kw):"
                " return Template(**kw).serialize(encoding=encoding,"
                " fragment=fragment, output=output, format=format)",
            "def write(file, encoding=encoding, fragment=False,"
                " output=None, format=None, **kw):"
                " return Template(**kw).write(file, encoding=encoding,"
                " fragment=fragment, output=output, format=format)",
            'def initialize(template): pass',
            'BaseTemplate = kid.BaseTemplate')
        # expand code
        code = self.expand_code
        code.start_block('def initialize(self):')
        code.line('rslt = initialize(self)',
            'if rslt != 0: super(Template, self).initialize()')
        code.end_block()
        code.start_block('def _pull(self):')
        # XXX hack: nasty kluge for making kwargs locals
        code.line("exec template_util.get_locals(self, locals())",
            'current, ancestors = None, []',
            'if doctype: yield DOCTYPE, doctype')
        code = self.end_module_code
        code.line('')

    def end(self):
        self.expand_code.end_block()

    def proc_stream(self, code):
        for ev, item in self.stream:
            if ev == START:
                if item.tag == Comment:
                    text = item.text.strip()
                    if text.startswith('!'):
                        continue # swallow comment
                    if code is self.module_code:
                        line = self.expand_code.line
                    else:
                        line = code.line
                    if text.startswith('[') or text.startswith('<![') \
                            or text.endswith('//'):
                        sub = interpolate(item.text)
                        if isinstance(sub, list):
                            text = "''.join([unicode(o) for o in %r])" % sub
                        else:
                            text = repr(sub)
                    else:
                        text = repr(item.text)
                    line('_e = Comment(%s)' % text,
                        'yield START, _e; yield END, _e; del _e')
                elif item.tag == ProcessingInstruction:
                    if ' ' in item.text.strip():
                        name, data = item.text.split(' ', 1)
                    else:
                        name, data = item.text, ''
                    if name in (KID_PI, KID_ALT_PI, KID_OLD_PI):
                        if data:
                            code.insert_block(data)
                    else:
                        c = self.depth and code or self.expand_code
                        c.line('_e = ProcessingInstruction(%r, %r)'
                                % (name, data),
                            'yield START, _e; yield END, _e; del _e')
                        del c
                else:
                    layout = None
                    if code is self.module_code:
                        layout = item.get(QNAME_LAYOUT)
                        if layout is not None:
                            del item.attrib[QNAME_LAYOUT]
                            layout = str(layout)
                        base_classes = []
                        extends = item.get(QNAME_EXTENDS)
                        if extends is not None:
                            del item.attrib[QNAME_EXTENDS]
                            extends = str(extends)
                            for c in extends.split(','):
                                base_classes.append('BaseTemplate%d'
                                    % (len(base_classes) + 1))
                                code.line('%s = template_util'
                                    '.base_class_extends(%r, globals(), {}, %r)'
                                    % (base_classes[-1], c.strip(), extends))
                            code.end_block()
                        base_classes.append('BaseTemplate')
                        code = self.class_code
                        code.start_block('class Template(%s):'
                            % ', '.join(base_classes))
                        code.line('_match_templates = []')
                        code = self.expand_code
                    self.def_proc(item, item.attrib, code)
                    if layout is not None:
                        old_code = code
                        code = self.codegen(level=1)
                        code.start_block('def _pull(self):')
                        # Note: the following hack could be avoided
                        # if template args were not stored in self.__dict__
                        code.line(
                            'exec template_util.get_locals(self, locals())',
                            'kw = dict(layout_params)',
                            'kw.update(dict([(name, getattr(self, name))'
                                ' for name in _def_names]))',
                            'kw.update(self.__dict__)',
                            # Note: these names cannot be passed
                            # to the layout template via layout_params
                            'kw.pop("assume_encoding", None)',
                            'kw.pop("_layout_classes", None)',
                            't = template_util.base_class_layout('
                                '%r, globals(), locals())(**kw)' % layout,
                            't._match_templates += self._match_templates',
                            'bases = [b for b in t.__class__.__bases__'
                                ' if b not in self.__class__.__bases__]',
                            'self.__class__.__bases__ = tuple(bases)'
                                ' + self.__class__.__bases__',
                            'return t._pull()')
                        code.end_block()
                        self.inst_defs.append(code)
                        code = old_code
                if self.location:
                    # code is inside the current tag:
                    self.location[1] = True
            elif ev == END and item.tag not in (
                    ProcessingInstruction, Comment):
                break
            elif ev == TEXT:
                self.text_interpolate(item, code)
            elif ev == XML_DECL and item[1] is not None:
                encoding = str(item[1])
                if encoding != self.encoding:
                    self.module_code.line('encoding = %r' % encoding)
            elif ev == DOCTYPE:
                self.module_code.line('doctype = (%r, %r, %r)' % item)
            elif ev == LOCATION:
                # store all different locations in the XML file in a list
                n = len(self.locations) - 1
                if n < 0 or item != self.locations[n]:
                    self.locations.append(item)
                    n += 1
                # store current location from this list for every code line
                # along with a flag indicating this is the starting location
                self.location = [n, False]

    def def_proc(self, item, attrib, code):
        attr_name = QNAME_DEF
        decl = attrib.get(attr_name)
        if decl is None:
            attr_name = QNAME_SLOT
            decl = attrib.get(attr_name)
        if decl is not None:
            del attrib[attr_name]
            old_code = code
            if '(' not in decl:
                decl += '()'
            name, args = decl.split('(', 1)
            args = args.lstrip()
            if not args.startswith(')'):
                args = ', ' + args
            class_decl = '(self'.join((name, args))

            # module function code
            code = self.codegen()
            code.start_block('def %s(*args, **kw):' % name)
            code.line('return Template().%s(*args, **kw)' % name)
            code.end_block()
            code.line('_def_names.append("%s")' % name)
            self.module_defs.append(code)

            # instance method code
            code = self.codegen(level=1)
            code.start_block('def __%s:' % class_decl)
            code.line('exec template_util.get_locals(self, locals())',
                'current, ancestors = None, []')
            self.inst_defs.append(code)
            self.match_proc(item, attrib, code)
            code.end_block()
            code.start_block('def %s(self, *args, **kw):' % name)
            code.line('return ElementStream(self.__%s(*args, **kw))' % name)
            code.end_block()
            if attr_name == QNAME_SLOT:
                old_code.line('for _e in template_util'
                    '.generate_content(self.%s()): yield _e' % name)
        else:
            self.match_proc(item, attrib, code)

    def match_proc(self, item, attrib, code):
        expr = attrib.get(QNAME_MATCH)
        if expr is not None:
            del attrib[QNAME_MATCH]
            code = self.codegen(level=1)
            code.start_block('def _match_func(self, item, apply):')
            code.line('exec template_util.get_locals(self, locals())',
                'current, ancestors = None, []')
            self.for_proc(item, attrib, code)
            code.end_block()
            code.line('_match_templates.append('
                '(lambda item: %s, _match_func))' % expr)
            self.inst_defs.append(code)
        else:
            self.for_proc(item, attrib, code)

    def for_proc(self, item, attrib, code):
        expr = attrib.get(QNAME_FOR)
        if expr is not None:
            code.start_block('for %s:' % expr)
            del attrib[QNAME_FOR]
            self.if_proc(item, attrib, code)
            code.end_block()
        else:
            self.if_proc(item, attrib, code)

    def if_proc(self, item, attrib, code):
        expr = attrib.get(QNAME_IF)
        if expr is not None:
            code.start_block('if %s:' % expr)
            del attrib[QNAME_IF]
            self.replace_proc(item, attrib, code)
            code.end_block()
        else:
            self.replace_proc(item, attrib, code)

    def replace_proc(self, item, attrib, code):
        expr = attrib.get(QNAME_REPLACE)
        if expr is not None:
            del attrib[QNAME_REPLACE]
            attrib[QNAME_STRIP] = ""
            attrib[QNAME_CONTENT] = expr
        self.strip_proc(item, attrib, code)

    def strip_proc(self, item, attrib, code):
        has_content = self.content_proc(attrib, code)
        expr, attr = attrib.get(QNAME_STRIP), QNAME_STRIP
        if expr is None:
            # XXX py:omit is deprecated equivalent of py:strip
            expr, attr = attrib.get(QNAME_OMIT), QNAME_OMIT
        start_block, end_block = code.start_block, code.end_block
        line = code.line
        if expr is not None:
            del attrib[attr]
            if expr != '':
                start_block("if not (%s):" % expr)
                self.attrib_proc(item, attrib, code)
                end_block()
            else:
                pass # element is always stripped
        else:
            self.attrib_proc(item, attrib, code)
        if has_content:
            code.start_block(
                'for _e in template_util.generate_content(_cont):')
            line('yield _e', 'del _e')
            code.end_block()
            self.stream.eat()
        else:
            self.depth += 1
            self.proc_stream(code)
            self.depth -= 1
        if expr:
            start_block("if not (%s):" % expr)
            line('yield END, current',
                'current = ancestors.pop(0)')
            end_block()
        elif expr != '':
            line('yield END, current',
                'current = ancestors.pop(0)')

    def attrib_proc(self, item, attrib, code):
        line = code.line
        need_interpolation = False
        names = namespaces(item, remove=True)
        for k, v in attrib.items():
            sub = interpolate(v)
            if id(sub) != id(v):
                attrib[k] = sub
                if isinstance(sub, list):
                    need_interpolation = True
        expr = attrib.get(QNAME_ATTRIBUTES)
        if expr is not None:
            del attrib[QNAME_ATTRIBUTES]
            attr_text = ('template_util.make_updated_attrib('
                '%r, "%s", globals(), locals(), self._get_assume_encoding())'
                % (attrib, expr.replace('"', '\\\"')))
        else:
            if attrib:
                if need_interpolation:
                    attr_text = ('template_util.make_attrib('
                        '%r, self._get_assume_encoding())' % attrib)
                else:
                    attr_text = repr(attrib)
            else:
                attr_text = '{}'
        line('ancestors.insert(0, current)',
            'current = Element(%r, %s)' % (item.tag, attr_text))
        if len(names):
            code.start_block('for _p, _u in %r.items():' % names)
            line('if not _u in omit_namespaces: yield START_NS, (_p,_u)')
            code.end_block()
        line('yield START, current')

    def content_proc(self, attrib, code):
        expr = attrib.get(QNAME_CONTENT)
        if expr is not None:
            del attrib[QNAME_CONTENT]
            if expr:
                code.line('_cont = %s' % expr)
            else:
                code.line('_cont = None')
            return True

    def text_interpolate(self, text, code):
        line = code.line
        sub = interpolate(text)
        if isinstance(sub, list):
            code.start_block('for _e in %r:' % sub)
            code.line('for _e2 in template_util'
                '.generate_content(_e): yield _e2')
            code.end_block()
        else:
            line('yield TEXT, %r' % sub)


class SubExpression(list):
    """Collecting and representing expressions."""
    def __repr__(self):
        return "[%s]" % ', '.join(map(_ascii_encode, self))

# regular expression for expression substitution
_sub_expr = re.compile( # $$ | $expr | ${expr}
    r"\$(\$|[a-zA-Z][a-zA-Z0-9_\.]*|\{.*?\})",
        re.DOTALL) # this is for multi-line expressions

def interpolate(text):
    """Perform expression substitution on text."""
    text = _sub_expr.split(text)
    if len(text) == 1:
        # shortcut for standard case
        return text[0]
    # build subexpression list,
    # making it as small as possible (coalesce pieces)
    parts = SubExpression()
    plain_parts = [] # collect plaintext parts here
    plain = True # first part is plaintext
    for part in text:
        if part: # skip empty text
            if plain or part == '$': # plaintext
                plain_parts.append(part)
            else: # code expression
                if part.startswith('{'):
                    # long form
                    part = part[1:-1].strip()
                if part: # skip empty expressions
                    plain_parts = ''.join(plain_parts)
                    if plain_parts:
                        parts.append(repr(plain_parts))
                    plain_parts = []
                    parts.append(part)
        # plaintext and code parts take turns
        plain = not plain
    plain_parts = ''.join(plain_parts)
    if parts:
        if plain_parts:
            parts.append(repr(plain_parts))
        return parts
    else:
        return plain_parts


class CodeGenerator(object):
    """A simple Python code generator."""

    level = 0
    tab = '\t'

    def __init__(self, code=None, level=0, tab='\t'):
        self.code = code or []
        if level != self.level:
            self.level = level
        if tab != self.tab:
            self.tab = tab
        self.pad = self.tab * self.level

    def line(self, *lines):
        for text in lines:
            self.code.append(self.pad + text)

    def start_block(self, text):
        self.line(text)
        self.level += 1
        self.pad += self.tab

    def end_block(self, nblocks=1, with_pass=False):
        for n in range(nblocks):
            if with_pass:
                self.line('pass')
            self.level -= 1
            self.pad = self.pad[:-len(self.tab)]

    def insert_block(self, block):
        lines = block.splitlines()
        if len(lines) == 1:
            # special case single lines
            self.line(lines[0].strip())
        else:
            # adjust the block
            for line in _adjust_python_block(lines, self.tab):
                self.line(line)

    def __str__(self):
        return '\n'.join(self.code + [''])


class LocationGenerator(object):
    """A simple location generator for debugging."""

    def __init__(self, code=None, getloc=None):
        self.code = code or []
        if getloc:
            self.getloc = getloc

    def getloc(self):
        return len(self.code)

    def line(self, *lines):
        n = 0
        for text in lines:
            n += len(text.splitlines())
        self.code.extend(n*[self.getloc()])

    def start_block(self, text):
        self.line(text)

    def end_block(self, nblocks=1, with_pass=False):
        if with_pass:
            self.line(['pass']*len(nblocks))

    def insert_block(self, block):
        self.line(block)

    def __str__(self):
        return 'Current location: %r' % self.getloc()


# Auxiliary functions

def _ascii_encode(s):
    return s. encode('ascii', 'backslashreplace')

def _adjust_python_block(lines, tab='\t'):
    """Adjust the indentation of a Python block."""
    lines = [lines[0].strip()] + [line.rstrip() for line in lines[1:]]
    ind = None # find least index
    for line in lines[1:]:
        if line != '':
            s = line.lstrip()
            if s[0] != '#':
                i = len(line) - len(s)
                if ind is None or i < ind:
                    ind = i
                    if i == 0:
                        break
    if ind is not None or ind != 0: # remove indentation
        lines[1:] = [line[:ind].lstrip() + line[ind:]
            for line in lines[1:]]
    if lines[0] and not lines[0][0] == '#':
        # the first line contains code
        try: # try to compile it
            compile(lines[0], '<string>', 'exec')
            # if it works, line does not start new block
        except SyntaxError: # unexpected EOF while parsing?
            try: # try to compile the whole block
                block = '\n'.join(lines) + '\n'
                compile(block, '<string>', 'exec')
                # if it works, line does not start new block
            except IndentationError: # expected an indented block?
                # so try to add some indentation:
                lines2 = lines[:1] + [tab + line for line in lines[1:]]
                block = '\n'.join(lines2) + '\n'
                # try again to compile the whole block:
                compile(block, '<string>', 'exec')
                lines = lines2 # if it works, keep the indentation
            except:
                pass # leave it as it is
        except:
            pass # leave it as it is
    return lines

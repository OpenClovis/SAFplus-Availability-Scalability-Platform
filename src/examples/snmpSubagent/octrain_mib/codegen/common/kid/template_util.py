# -*- coding: utf-8 -*-

"""Utility functions used by generated kid modules."""

__revision__ = "$Rev: 492 $"
__date__ = "$Date: 2007-07-06 21:38:45 -0400 (Fri, 06 Jul 2007) $"
__author__ = "Ryan Tomayko (rtomayko@gmail.com)"
__copyright__ = "Copyright 2004-2005, Ryan Tomayko"
__license__ = "MIT <http://www.opensource.org/licenses/mit-license.php>"

import inspect
import sys
from types import TypeType, ModuleType, ClassType, GeneratorType
import itertools

# these are for use by template code
import kid
from kid.parser import XML, document, ElementStream, START, END, TEXT, \
    START_NS, COMMENT, PI, DOCTYPE, XML_DECL, to_unicode
from kid.element import Element, SubElement, Comment, ProcessingInstruction

__all__ = ['XML', 'document', 'ElementStream',
    'Element', 'SubElement', 'Comment', 'ProcessingInstruction',
    'START', 'END', 'TEXT', 'START_NS', 'COMMENT', 'PI',
    'DOCTYPE', 'XML_DECL']


class TemplateError(Exception): pass
class TemplateNotFound(TemplateError): pass
class TemplateImportError(TemplateError): pass
class TemplateDictError(TemplateError): pass
class TemplateAttrsError(TemplateError): pass
class TemplateExtendsError(TemplateError): pass
class TemplateLayoutError(TemplateError): pass


_local_excludes = ['generate', 'module', 'parser', 'serialize', 'transform', 'write']
def get_locals(inst, _locals=None):
    if _locals is None:
        _locals = {}
    ls = []
    local_excludes = _local_excludes # local copy
    for var, value in inspect.getmembers(inst):
        if not var.startswith('_') and not var in local_excludes \
                and var not in _locals:
            ls.append('%s=self.%s' % (var, var))
    return ';'.join(ls)

def get_base_class(thing, from_file=None, arg=None):
    """Get template base class for thing, raising an exception on error."""
    if thing is None:
        return kid.BaseTemplate
    if isinstance(thing, TypeType):
        return thing
    elif isinstance(thing, ModuleType):
        try:
            cls = thing.Template
        except AttributeError:
            cls = None
        if (isinstance(cls, TypeType)
                and issubclass(cls, kid.BaseTemplate)
                and cls != kid.Template):
            return cls
        thing = repr(thing)
        if arg:
            thing = arg
        else:
            try:
                thing = thing.__name__
            except AttributeError:
                thing = repr(thing)
        raise TemplateNotFound(
            '%s is a module without Template class' % thing)
    elif isinstance(thing, basestring):
        try:
            path = kid.path.find(thing, from_file)
        except Exception:
            path = None
        if not path:
            if arg:
                thing = arg
            raise TemplateNotFound('Template file %r not found' % thing)
        try:
            mod = kid.load_template(path)
        except Exception:
            mod = None
        if not mod:
            raise TemplateNotFound('Could not open %r' % path)
        try:
            cls = mod.Template
        except AttributeError:
            cls = None
        if (isinstance(cls, TypeType)
                and issubclass(cls, kid.BaseTemplate)
                and cls != kid.Template):
            return cls
        raise TemplateNotFound('%r does not contain a template class' % path)
    thing = repr(thing)
    if arg:
        thing = '%s (%s)' % (arg, thing)
    raise TemplateNotFound('%s is not a Template class' % thing)

def base_class(arg, globals, locals):
    """Get base class for argument with graceful exception handling."""
    try:
        from_file = globals['__file__']
        thing = eval(arg, globals, locals)
        return get_base_class(thing, from_file, arg)
    except Exception, e:
        errors = [str(e)]
        # try again without evaluating the argument (forgotten quotes etc.)
        try:
            return get_base_class(arg, from_file, arg)
        except Exception, e:
            errors.append(str(e))
        # reraise the original problem when we tried to evaluate the thing
        errors = '\n'.join(filter(bool, errors)) or arg
        raise TemplateNotFound, errors

def base_class_extends(extends, globals, locals, all_extends=None):
    """Get Template base class for 'extends'."""
    try:
        return base_class(extends, globals, locals)
    except Exception, e:
        raise TemplateExtendsError((str(e)
            + '\nwhile processing extends=%r'
                % (all_extends or extends)).lstrip())

def base_class_layout(layout, globals, locals):
    """Get Template base class for 'layout'."""
    try:
        return base_class(layout, globals, locals)
    except Exception, e:
        raise TemplateLayoutError((str(e)
            + '\nwhile processing layout=%r' % layout).lstrip())

def make_attrib(attrib, encoding=None):
    """Generate unicode strings in dictionary."""
    if attrib is None:
        return {}
    if encoding is None:
        encoding = sys.getdefaultencoding()
    for (k, v) in attrib.items():
        if v is not None:
            try:
                v = generate_attrib(v, encoding)
            except TemplateAttrsError:
                raise TemplateAttrsError('Illegal value for attribute "%s"'
                    % k.encode('raw_unicode_escape'))
        if v is None:
            del attrib[k]
        else:
            attrib[k] = v
    return attrib

def generate_attrib(attrib, encoding):
    """Generate unicode string from attribute."""
    if attrib is None:
        return None
    elif isinstance(attrib, basestring):
        return to_unicode(attrib, encoding)
    elif isinstance(attrib, ElementStream):
        text = []
        for ev, item in attrib:
            if ev == TEXT:
                text.append(to_unicode(item, encoding))
            else:
                raise TemplateAttrsError
        if text:
            return ''.join(text)
        else:
            return None
    elif hasattr(attrib, '__iter__'):
        # if we get any other iterable, join the strings together:
        text = []
        for item in attrib:
            if item is not None:
                item = generate_attrib(item, encoding)
                if item is not None:
                    text.append(item)
        if text:
            return ''.join(text)
        else:
            return None
    else:
        return to_unicode(attrib, encoding)

def generate_content(content):
    """Generate ElementStream from content."""
    if content is None:
        return []
    elif isinstance(content, basestring):
        return [(TEXT, content)]
    elif isinstance(content, (ElementStream, kid.BaseTemplate)):
        return content
    elif isinstance(content, GeneratorType):
        return ElementStream(content)
    elif hasattr(content, 'tag') and hasattr(content, 'attrib'):
        # if we get an Element back, make it an ElementStream
        return ElementStream(content)
    elif hasattr(content, '__iter__'):
        # if we get any other iterable, chain the contents together:
        return itertools.chain(*itertools.imap(generate_content, content))
    else:
        return [(TEXT, unicode(content))]

def filter_names(names, omit_list):
    for ns in names.keys():
        if ns in omit_list:
            del names[ns]
    return names

def update_dict(a, args, globals, locals):
    """Update dictionary a from keyword argument string args."""
    try:
        b = eval('%s' % args, globals, locals)
        if not isinstance(b, dict):
            b = dict(b)
    except Exception:
        try:
            b = eval('dict(%s)' % args, globals, locals)
        except SyntaxError:
            # TypeErrror could happen with Python versions < 2.3, because
            # building dictionaries from keyword arguments was not supported.
            # Kid requires a newer Python version, so we do not catch this.
            # SyntaxError can happen if one of the keyword arguments is
            # the same as a Python keyword (e.g. "class") or if it is
            # a qualified name containing a namespace prefixed with a colon.
            # In these cases we parse the keyword arguments manually:
            try:
                try:
                    from cStringIO import StringIO
                except ImportError:
                    from StringIO import StringIO
                from tokenize import generate_tokens
                from token import NAME, OP
                depth, types, parts = 0, [], []
                for token in generate_tokens(StringIO(args).readline):
                    type_, string = token[:2]
                    if type_ == OP:
                        if string == '=':
                            if depth == 0:
                                if len(types) > 0 \
                                    and types[-1] == NAME and parts[-1]:
                                    if len(types) > 2 \
                                        and types[-2] == OP and parts[-2] == ':' \
                                        and types[-3] == NAME and parts[-3]:
                                        parts[-3:] = ["'%s'" % ''.join(parts[-3:])]
                                    else:
                                        parts[-1] = "'%s'" % parts[-1]
                                    string = ':'
                        elif string in '([{':
                            depth += 1
                        elif depth > 0 and string in ')]}':
                            depth -= 1
                    types.append(type_)
                    parts.append(string)
                b = eval('{%s}' % ''.join(parts), globals, locals)
            except Exception:
                b = None
        if not isinstance(b, dict):
            raise
    for k in b.keys():
        if b[k] is None:
            del b[k]
            if k in a:
                del a[k]
    a.update(b)
    return a

def update_attrs(attrib, attrs, globals, locals):
    """Update attributes from attrs string args."""
    try:
        return update_dict(attrib, attrs, globals, locals)
    except Exception, e:
        raise TemplateAttrsError((str(e)
            + '\nwhile processing attrs=%r' % attrs).lstrip())

def make_updated_attrib(attrib, attrs, globals, locals, encoding=None):
    """"Generate unicode strings in updated dictionary."""
    return make_attrib(update_attrs(attrib, attrs, globals, locals), encoding)

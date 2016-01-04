# -*- coding: utf-8 -*-

"""Pythonic, XML Templating

Kid is a simple, Python-based template language for generating and
transforming XML vocabularies. Kid was spawned as a result of a kinky love
triangle between XSLT, TAL, and PHP. We believe many of the best features
of these languages live on in Kid with much of the limitations and
complexity stamped out (well, eventually :).

"""

__revision__ = "$Rev: 492 $"
__date__ = "$Date: 2007-07-06 21:38:45 -0400 (Fri, 06 Jul 2007) $"

from kid import release

__version__ = release.version
__author__ = release.author
__email__ = release.email
__copyright__ = release.copyright
__license__ = release.license

import sys, os

assert sys.hexversion >= 0x02030000, "Kid templates need Python 2.3 or later"

from kid.util import xml_sniff, QuickTextReader
from kid.namespace import Namespace
from kid.codewriter import KID_XMLNS, raise_template_error
from kid.compiler import KID_EXT
from kid.element import Element, SubElement, Comment, \
    ProcessingInstruction, Fragment
from kid.parser import ElementStream, XML, document, _coalesce
from kid.filter import transform_filter
from kid.serialization import Serializer, PlainSerializer, \
    XMLSerializer, HTMLSerializer, XHTMLSerializer
from kid.format import Format, output_formats
import kid.template_util as template_util

__all__ = ['KID_XMLNS', 'BaseTemplate', 'Template',
           'enable_import', 'import_template', 'load_template',
           'Element', 'SubElement', 'XML', 'document', 'Namespace',
           'Serializer', 'XMLSerializer', 'HTMLSerializer', 'XHTMLSerializer',
           'output_methods', 'Format', 'output_formats',
           'filter', 'format', 'namespace', 'serialization', 'util']

assume_encoding = sys.getdefaultencoding()


def enable_import(ext=None, path=None):
    """Enable the kid module loader and import hooks.

    This function must be called before importing kid templates if templates
    are not pre-compiled.

    """
    import kid.importer
    kid.importer.install(ext, path)


def disable_import(path=None):
    """Disable the kid module loader and import hooks again."""
    import kid.importer
    kid.importer.uninstall(path)


# Turn on import hooks if KID_IMPORT variables are set
if os.environ.get('KID_IMPORT'):
    enable_import(os.environ.get('KID_IMPORT_EXT'))
if os.environ.get('KID_IMPORT_PATH'):
    enable_import(os.environ.get('KID_IMPORT_EXT'),
        os.environ['KID_IMPORT_PATH'])


def import_template(name, encoding=None):
    """Import template by name.

    This is identical to calling `enable_import` followed by an import
    statement. For example, importing a template named foo using the normal
    import mechanism looks like this::

        import kid
        kid.enable_import()
        import foo

    This function can be used to achieve the same result as follows::

        import kid
        foo = kid.import_template('foo')

    This is sometimes useful when the name of the template is available only
    as a string.
    """
    enable_import()
    mod = __import__(name)
    components = name.split('.')
    for comp in components[1:]:
        mod = getattr(mod, comp)
    if encoding:
        mod.encoding = encoding
    return mod


def load_template(file, name='', cache=True, encoding=None, ns={},
        entity_map=None, exec_module=None):
    """Bypass import machinery and load a template module directly.

    This can be used as an alternative to accessing templates using the
    native python import mechanisms.

    file
      Can be a filename, a kid template string, or an open file object.
    name
      Optionally specifies the module name to use for this template. This
      is a hack to enable relative imports in templates.
    cache
      Whether to look for a byte-compiled version of the template. If
      no byte-compiled version is found, an attempt is made to dump a
      byte-compiled version after compiling. This argument is ignored if
      file is not a filename.
    entity_map
      Entity map to be used when parsing the template.
    exec_module
      If you want full control over how the template module is executed,
      you can provide this callable that will be called with the template
      module and the code to be executed as parameters, after the code has
      been compiled and the module has been created.

    """
    if isinstance(file, basestring):
        if xml_sniff(file):
            fo = QuickTextReader(file)
            filename = '<string>'
        else:
            fo = None
            filename = file
    else:
        fo = file
        filename = '<string>'
    import kid.importer as importer
    if filename != '<string>':
        abs_filename = path.find(filename)
        if not abs_filename:
            raise template_util.TemplateNotFound(
                "%s (in %s)" % (filename, ', '.join(path.paths)))
        filename = abs_filename
        name = importer.get_template_name(name, filename)
        if sys.modules.has_key(name):
            return sys.modules.get(name)
    import kid.compiler as compiler
    if filename == '<string>':
        code = compiler.compile(fo, filename, encoding, entity_map)
    else:
        template = compiler.KidFile(filename, force=False,
            encoding=encoding, entity_map=entity_map)
        code = template.compile(dump_code=cache,
            dump_source=os.environ.get('KID_OUTPUT_PY'))
    mod = importer._create_module(code, name, filename,
        store=cache, ns=ns, exec_module=exec_module)
    return mod


# create some default serializers...
output_methods = {
    'xml': XMLSerializer(decl=True),
    'wml': XMLSerializer(decl=True, doctype='wml'),
    'xhtml-strict': XHTMLSerializer(decl=False, doctype='xhtml-strict'),
    'xhtml': XHTMLSerializer(decl=False, doctype='xhtml'),
    'xhtml-frameset': XHTMLSerializer(decl=False, doctype='xhtml-frameset'),
    'html-strict': HTMLSerializer(doctype='html-strict'),
    'html': HTMLSerializer(doctype='html'),
    'html-frameset': HTMLSerializer(doctype='html-frameset'),
    'html-quirks': HTMLSerializer(doctype='html-quirks'),
    'html-frameset-quirks': HTMLSerializer(doctype='html-frameset-quirks'),
    'HTML-strict': HTMLSerializer(doctype='html-strict', transpose=True),
    'HTML': HTMLSerializer(doctype='html', transpose=True),
    'HTML-frameset': HTMLSerializer(doctype='html-frameset', transpose=True),
    'HTML-quirks': HTMLSerializer(doctype='html-quirks', transpose=True),
    'HTML-frameset-quirks': HTMLSerializer(doctype='html-frameset-quirks', transpose=True),
    'plain': PlainSerializer()}


def Template(file=None, source=None, name=None, encoding=None, **kw):
    """Get a Template class quickly given a module name, file, or string.

    This is a convenience function for getting a template in a variety of
    ways. One and only one of the arguments name or file must be specified.

    file:string
      The template module is loaded by calling
      ``load_template(file, name='', cache=True)``
    name:string
      The kid import hook is enabled and the template module is located
      using the normal Python import mechanisms.
    source:string
      string containing the templates source.

    Once the template module is obtained, a new instance of the module's
    Template class is created with the keyword arguments passed to this
    function.
    """
    if name:
        mod = import_template(name, encoding=encoding)
    elif file is not None:
        mod = load_template(file, name=name, encoding=encoding)
    elif source is not None:
        mod = load_template(QuickTextReader(source),
            name=name or hex(id(source)), encoding=encoding)
    else:
        raise template_util.TemplateError(
            "Must specify one of name, file, or source.")
    try:
        mod.Template.module = mod
    except Exception:
        raise template_util.TemplateImportError(
            "Template could not be initialized.")
    return mod.Template(**kw)


class BaseTemplate(object):

    """Base class for compiled Templates.

    All kid template modules expose a class named ``Template`` that
    extends from this class making the methods defined here available on
    all Template subclasses.

    This class should not be instantiated directly.
    """

    # the serializer to use when writing output
    serializer = output_methods['xml']

    def __init__(self, *args, **kw):
        """
        Initialize a template with instance attributes specified by
        keyword arguments.

        Keyword arguments are available to the template using self.var
        notation.
        """
        for k in kw:
            # check that reserved keywords such as 'content' are not used
            if hasattr(BaseTemplate, k):
                raise template_util.TemplateAttrsError(
                    "Keyword argument %r is a reserved name." % k)
        self.__dict__.update(kw)
        self._filters = [transform_filter]
        self._layout_classes = []

    def write(self, file, encoding=None,
            fragment=False, output=None, format=None):
        """
        Execute template and write output to file.

        file:file
          A filename or a file like object (must support write()).
        encoding:string
          The output encoding. Default: utf-8.
        fragment:bool
          Controls whether prologue information (such as <?xml?>
          declaration and DOCTYPE should be written). Set to True
          when generating fragments meant to be inserted into
          existing XML documents.
        output:string,`Serializer`
          A string specifying an output method ('xml', 'html',
          'xhtml') or a Serializer object.
        """
        serializer = self._get_serializer(output)
        try:
            return serializer.write(self, file, encoding, fragment, format)
        except Exception:
            raise_template_error(module=self.__module__)

    def serialize(self, encoding=None,
            fragment=False, output=None, format=None):
        """
        Execute a template and return a single string.

        encoding
          The output encoding. Default: utf-8.
        fragment
          Controls whether prologue information (such as <?xml?>
          declaration and DOCTYPE should be written). Set to True
          when generating fragments meant to be inserted into
          existing XML documents.
        output
          A string specifying an output method ('xml', 'html',
          'xhtml') or a Serializer object.

        This is a convienence method, roughly equivalent to::

          ''.join([x for x in obj.generate(encoding, fragment, output)]

        """
        serializer = self._get_serializer(output)
        try:
            return serializer.serialize(self, encoding, fragment, format)
        except Exception:
            raise_template_error(module=self.__module__)

    def generate(self, encoding=None,
            fragment=False, output=None, format=None):
        """
        Execute template and generate serialized output incrementally.

        This method returns an iterator that yields an encoded string
        for each iteration. The iteration ends when the template is done
        executing.

        encoding
          The output encoding. Default: utf-8.
        fragment
          Controls whether prologue information (such as <?xml?>
          declaration and DOCTYPE should be written). Set to True
          when generating fragments meant to be inserted into
          existing XML documents.
        output
          A string specifying an output method ('xml', 'html',
          'xhtml') or a Serializer object.
        """
        serializer = self._get_serializer(output)
        try:
            return serializer.generate(self, encoding, fragment, format)
        except Exception:
            raise_template_error(module=self.__module__)

    def __iter__(self):
        return iter(self.transform())

    def __str__(self):
        return self.serialize()

    def __unicode__(self):
        return unicode(self.serialize(encoding='utf-16'), 'utf-16')

    def initialize(self):
        pass

    def pull(self):
        """Returns an iterator over the items in this template."""
        # create stream and apply filters
        self.initialize()
        stream = ElementStream(_coalesce(self.content(),
            self._get_assume_encoding()))
        return stream

    def _pull(self):
        """Generate events for this template.

        Compiled templates implement this method.
        """
        return []

    def content(self):
        from inspect import getmro
        visited = self._layout_classes
        mro = list(getmro(self.__class__))
        mro.reverse()
        for c in mro:
            if c.__dict__.has_key('layout') and c not in visited:
                visited.insert(0, c)
                return c.__dict__['layout'](self)
        return self._pull()

    def transform(self, stream=None, filters=[]):
        """
        Execute the template and apply any match transformations.

        If stream is specified, it must be one of the following:

        Element
          A kid.Element.
        ElementStream
          An `pull.ElementStream` instance or other iterator that yields
          stream events.
        string
          A file or URL unless the string starts with
          '<' in which case it is considered an XML document
          and processed as if it had been an Element.

        By default, the `pull` method is called to obtain the stream.
        """
        if stream is None:
            stream = self.pull()
        elif isinstance(stream, basestring):
            if xml_sniff(stream):
                stream = XML(stream, fragment=False)
            else:
                stream = document(stream)
        elif hasattr(stream, 'tag'):
            stream = ElementStream(stream)
        else:
            stream = ElementStream.ensure(stream)
        for f in filters + self._filters:
            stream = f(stream, self)
        return stream

    def _get_match_templates(self):
        # XXX: use inspect instead of accessing __mro__ directly
        try:
            rslt = self._match_templates_cached
        except AttributeError:
            rslt = []
            mro = self.__class__.__mro__
            for C in mro:
                try:
                    templates = C._match_templates
                except AttributeError:
                    continue
                rslt += templates
            self._match_templates_cached = rslt
        return rslt

    def _get_serializer(self, serializer):
        if serializer is None:
            return self.serializer
        elif isinstance(serializer, basestring):
            return output_methods[serializer]
        else:
            return serializer

    def _get_assume_encoding(self):
        global assume_encoding

        if hasattr(self, "assume_encoding"):
            return self.assume_encoding
        else:
            return assume_encoding

    def defined(self, name):
        return hasattr(self, name)

    def value_of(self, name, default=None):
        return getattr(self, name, default)


class TemplatePath(object):
    """Finding templates on a list of paths."""

    def __init__(self, paths=None):
        """Initialize with path list."""
        if isinstance(paths, basestring):
            paths = paths.split(os.pathsep)
        elif paths is None:
            paths = []
        paths.append(os.getcwd())
        self.paths = []
        for path in paths:
            self.append(path)

    def _cleanse_path(self, path):
        """Normalize path."""
        return os.path.abspath(os.path.normpath(os.path.expanduser(path)))

    def insert(self, path, pos=0):
        """Insert path to list if not already there."""
        path = self._cleanse_path(path)
        if path not in self.paths:
            self.paths.insert(pos, path)

    def append(self, path):
        """Append path to list if not already there."""
        path = self._cleanse_path(path)
        if path not in self.paths:
            self.paths.append(path)

    def remove(self, path):
        """Remove path from list."""
        path = self._cleanse_path(path)
        self.paths = [p for p in self.paths if p != path]

    def find(self, path, rel=None):
        """Find file relative to path list and rel."""
        path = os.path.normpath(path)
        if rel:
            rel = [os.path.dirname(rel)]
        else:
            rel = []
        for p in self.paths + rel:
            p = os.path.join(p, path)
            if os.path.exists(p):
                return p
            if not p.endswith(KID_EXT):
                p += KID_EXT
                if os.path.exists(p):
                    return p


path = TemplatePath()

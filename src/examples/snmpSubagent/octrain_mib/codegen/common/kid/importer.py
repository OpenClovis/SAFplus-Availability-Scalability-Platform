# -*- coding: utf-8 -*-

"""Kid Import Hooks.

When installed, these hooks allow importing .kid files as if they were
Python modules.

Notes:

We use new import hooks instead of the old ihooks module, because ihooks is
incompatible with Python eggs. We allow importing from one or more specified
paths for Kid templates, or importing from sys.path. In the latter case, we
use an importer on meta_path because importers on path_hook do not fall back
to the built-in import in Python >= 2.5 (this worked in Python 2.3 and 2.4).

"""

__revision__ = "$Rev: 492 $"
__date__ = "$Date: 2007-07-06 21:38:45 -0400 (Fri, 06 Jul 2007) $"
__author__ = "Ryan Tomayko (rtomayko@gmail.com); Christoph Zwerschke (cito@online.de)"
__copyright__ = "Copyright 2004-2005, Ryan Tomayko; 2006 Christoph Zwerschke"
__license__ = "MIT <http://www.opensource.org/licenses/mit-license.php>"

import sys
import time
import new
from os import environ, extsep, pathsep
from os.path import exists, join as joinpath, isdir

from kid import __version__
from kid.codewriter import raise_template_error
from kid.compiler import KidFile, KID_EXT
from kid.template_util import TemplateImportError

__all__ = ['install', 'uninstall', 'import_template', 'get_template_name']


def install(ext=None, path=None):
    """Install importer for Kid templates.

    ext can be one or more extensions as list or comma separated string
    path can be one or more paths as list or pathsep separated string

    """
    if ext:
        if isinstance(ext, basestring):
            exts = ext.split(',')
        else:
            exts = list(ext)
        for ext in exts:
            if not ext.startswith(extsep):
                raise Exception, "Illegal exception: " + ext
        if KID_EXT in exts:
            exts.remove(KID_EXT)
    else:
        exts = []
    exts.insert(0, KID_EXT)
    if path: # install path hook
        if isinstance(path, basestring):
            paths = path.split(pathsep)
        else:
            paths = list(path)
        # Install special Kid template paths, because since Python 2.5,
        # path hooks do not fall back to the built-in import any more.
        ext = ','.join(exts)
        kidpaths = []
        syspath = sys.path
        for path in paths:
            kidpath = 'kid::%s::' % path
            syspath = [path for path in syspath
                if not path.startswith(kidpath)]
            kidpaths.append(kidpath + ext)
        sys.path = kidpaths + syspath
        if kidpaths:
            if not KidImporter in sys.path_hooks:
                sys.path_hooks.insert(0, KidImporter)
    else: # install meta hook for all sys paths
        for importer in sys.meta_path:
            if isinstance(importer, KidImporter):
                importer.exts = exts
                break
        else:
            importer = KidImporter(ext=exts)
            sys.meta_path.insert(0, importer)

def uninstall(path=None):
    """Uninstall importer for Kid templates.

    path can be one or more paths as list or pathsep separated string

    """
    if path: # uninstall path hook
        if isinstance(path, basestring):
            paths = path.split(pathsep)
        else:
            paths = list(path)
        syspath = []
        remove_hook = True
        for path in sys.path:
            p = path.split(':')
            if len(p) >= 5 and \
                    p[0] == 'kid' and not p[1] and not p[-2]:
                if ':'.join(p[2:-2]) in paths:
                    continue
                remove_hook = False
            syspath.append(path)
        sys.path = syspath
        if remove_hook:
            if KidImporter in sys.path_hooks:
                sys.path_hooks = [hook for hook in sys.path_hooks
                    if hook != KidImporter]
                sys.path_importer_cache.clear()
    else: # uninstall meta hook for all sys paths
        sys.meta_path = [importer for importer in sys.meta_path
            if not isinstance(importer, KidImporter)]

def import_template(name, filename, force=False):
    if not force and name and sys.modules.has_key(name):
        return sys.modules[name]
    template = KidFile(filename)
    code = template.compile(dump_source=environ.get('KID_OUTPUT_PY'))
    module = _create_module(code, name, filename)
    return module

def get_template_name(name, filename):
    if name:
        return name
    else:
        return 'kid.util.template_%x' % (hash(filename) + sys.maxint + 1)

def _create_module(code, name, filename,
        store=True, ns={}, exec_module=None):
    for recompiled in range(2):
        name = get_template_name(name, filename)
        mod = new.module(name)
        mod.__file__ = filename
        mod.__ctime__ = time.time()
        mod.__dict__.update(ns)
        try:
            if exec_module:
                exec_module(mod, code)
            else:
                exec code in mod.__dict__
        except Exception:
            if store:
                sys.modules[name] = mod
            raise_template_error(module=name, filename=filename)
        if getattr(mod, 'kid_version', None) == __version__:
            break
        # the module has been compiled against an old Kid version,
        # recompile to ensure compatibility and best performance
        if recompiled: # already tried recompiling, to no avail
            raise TemplateImportError('Cannot recompile template file'
                ' %r for Kid version %s' % (filename, __version__))
        template = KidFile(filename)
        template.stale = True
        template._python = template._code = None
        code = template.compile(dump_source=environ.get('KID_OUTPUT_PY'))
    if store:
        sys.modules[name] = mod
    return mod


class KidImporter(object):
    """Importer for Kid templates via sys.path_hooks or sys.meta_path."""

    def __init__(self, path=None, ext=None):
        if path: # initialized via sys.path_hooks
            # check for special path format:
            # path = kid::/path/to/templates::.ext1,.ext2
            p = path.split(':')
            if len(p) >= 5 and \
                    p[0] == 'kid' and not p[1] and not p[-2]:
                path = ':'.join(p[2:-2])
                exts = p[-1].split(',')
                if exts:
                    for ext in exts:
                        if not ext.startswith(extsep):
                            break
                    else:
                        if isdir(path):
                            self.path = path
                            self.exts = exts
                            return
            raise ImportError
        else: # initialize for use via sys.meta_path
            if ext:
                if isinstance(ext, basestring):
                    exts = ext.split(',')
                else:
                    exts = list(ext)
                for ext in exts:
                    if not ext.startswith(extsep):
                        raise ImportError
            else:
                raise ImportError
            self.path = None
            self.exts = exts

    def find_module(self, fullname, path=None):
        name = fullname.split('.')[-1]
        if self.path:
            if path:
                raise ImportError
            else:
                paths = [self.path]
        else:
            paths = sys.path
            if path:
                paths = path + paths
        for path in paths:
            if isdir(path):
                path = joinpath(path, name)
                for ext in self.exts:
                    if exists(path + ext):
                        self.filename = path + ext
                        return self
        return None

    def load_module(self, fullname):
        return import_template(fullname, self.filename, force=True)

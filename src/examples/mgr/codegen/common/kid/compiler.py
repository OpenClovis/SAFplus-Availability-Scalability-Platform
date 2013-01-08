# -*- coding: utf-8 -*-

"""Kid Compiler

Compile XML to Python byte-code.

"""

__revision__ = "$Rev: 492 $"
__date__ = "$Date: 2007-07-06 21:38:45 -0400 (Fri, 06 Jul 2007) $"
__author__ = "Ryan Tomayko (rtomayko@gmail.com)"
__copyright__ = "Copyright 2004-2005, Ryan Tomayko"
__license__ = "MIT <http://www.opensource.org/licenses/mit-license.php>"

import os
import os.path
import imp
import stat
import struct
import marshal

import kid
from kid.codewriter import raise_template_error

__all__ = ['KID_EXT', 'compile', 'compile_file', 'compile_dir']

# kid filename extension
KID_EXT = ".kid"

def actualize(code, dict=None):
    """Run code with variables in dict, updating the dict."""
    if dict is None:
        dict = {}
    exec code in dict
    return dict

_py_compile = compile
def py_compile(code, filename='<string>', kind='exec'):
    """The Python built-in compile function with safeguard."""
    if type(code) == unicode:
        # unicode strings may not have a PEP 0263 encoding declaration
        if code.startswith('# -*- coding: '):
            # we want the line numbering to match with the source file,
            # so we only remove the magic word in the comment line:
            code = '# -*-' + code[13:]
    return _py_compile(code, filename, 'exec')

def compile(source, filename='<string>', encoding=None, entity_map=None):
    """Compiles Kid XML source to a Python code object.

    source   -- A file like object - must support read.
    filename -- An optional filename that is used

    """
    # XXX all kinds of work to do here catching syntax errors and
    #     adjusting line numbers...
    py = kid.codewriter.parse(source, encoding, filename, entity_map)
    return py_compile(py, filename)


_timestamp = lambda filename : os.stat(filename)[stat.ST_MTIME]

class KidFile(object):
    magic = imp.get_magic()

    def __init__(self, kid_file, force=False,
            encoding=None, strip_dest_dir=None, entity_map=None):
        self.kid_file = kid_file
        self.py_file = os.path.splitext(kid_file)[0] + '.py'
        self.strip_dest_dir = strip_dest_dir
        self.pyc_file = self.py_file + 'c'
        self.encoding = encoding
        self.entity_map = entity_map
        fp = None
        if force:
            stale = True
        else:
            stale = False
            try:
                fp = open(self.pyc_file, "rb")
            except IOError:
                stale = True
            else:
                if fp.read(4) != self.magic:
                    stale = True
                else:
                    mtime = struct.unpack('<I', fp.read(4))[0]
                    kid_mtime = _timestamp(kid_file)
                    if kid_mtime is None or mtime < kid_mtime:
                        stale = True
        self.stale = stale
        self._pyc_fp = fp
        self._python = None
        self._code = None

    def compile(self, dump_code=True, dump_source=False):
        if dump_source:
            self.dump_source()
        code = self.code
        if dump_code and self.stale:
            self.dump_code()
        return code

    def code(self):
        """Get the compiled Python code for the template."""
        if self._code is None:
            if self.stale:
                pyfile = self.py_file
                if self.strip_dest_dir and \
                   self.py_file.startswith(self.strip_dest_dir):
                    pyfile = os.path.normpath(
                        self.py_file[len(self.strip_dest_dir):])
                try:
                    self._code = py_compile(self.python, pyfile)
                except Exception:
                    raise_template_error(filename=self.kid_file,
                        encoding=self.encoding)
            else:
                self._code = marshal.load(self._pyc_fp)
        return self._code
    code = property(code)

    def python(self):
        """Get the Python source for the template."""
        if self._python is None:
            py = kid.codewriter.parse_file(self.kid_file,
                self.encoding, self.entity_map)
            self._python = py
        return self._python
    python = property(python)

    def dump_source(self, file=None):
        py = self.python
        encoding = self.encoding or 'utf-8'
        file = file or self.py_file
        fp = _maybe_open(file, 'wb')
        if fp:
            try:
                try:
                    fp.write(py.encode(encoding))
                finally:
                    fp.close()
            except IOError:
                _maybe_remove(file)
            else:
                return True
        return False

    def dump_code(self, file=None):
        code = self.code
        file = file or self.pyc_file
        fp = _maybe_open(file, 'wb')
        if fp:
            try:
                try:
                    if self.kid_file:
                        mtime = os.stat(self.kid_file)[stat.ST_MTIME]
                    else:
                        mtime = 0
                    fp.write('\0\0\0\0')
                    fp.write(struct.pack('<I', mtime))
                    marshal.dump(code, fp)
                    fp.flush()
                    fp.seek(0)
                    fp.write(self.magic)
                finally:
                    fp.close()
            except IOError:
                _maybe_remove(file)
            else:
                return True
        return False

def _maybe_open(f, mode):
    if isinstance(f, basestring):
        try:
            f = open(f, mode)
        except IOError:
            f = None
    return f

def _maybe_remove(f):
    if isinstance(f, basestring):
        try:
            os.remove(f)
        except OSError:
            pass

#
# functions for compiling files directly and the kidc utility
#

def compile_file(file, force=False, source=False, encoding=None,
        strip_dest_dir=None, entity_map=None):
    """Compile the file specified.

    Return True if the file was compiled, False if the compiled file already
    exists and is up-to-date.

    """
    template = KidFile(file, force, encoding, strip_dest_dir, entity_map)
    if template.stale:
        template.compile(dump_source=source)
        return True
    else:
        return False

def compile_dir(dir, maxlevels=10, force=False, source=False,
        encoding=None, strip_dest_dir=None, entity_map=None):
    """Byte-compile all kid modules in the given directory tree.

    Keyword Arguments: (only dir is required)
    dir       -- the directory to byte-compile
    maxlevels -- maximum recursion level (default 10)
    force     -- if True, force compilation, even if timestamps are up-to-date.
    source    -- if True, dump python source (.py) files along with .pyc files.

    Yields tuples (stat, filename) where stat is either an error message,
    True if the file was compiled or False if the file did not need to be compiled.

    """
    names = os.listdir(dir)
    names.sort()
    ext_len = len(KID_EXT)
    for name in names:
        fullname = os.path.join(dir, name)
        if os.path.isfile(fullname):
            ext = name[-ext_len:]
            if ext == KID_EXT:
                try:
                    stat = compile_file(fullname, force, source,
                        encoding, strip_dest_dir, entity_map)
                except Exception, e:
                    # TODO: grab the traceback and yield it with the other stuff
                    stat = e
                yield stat, fullname
        elif maxlevels > 0 and name != os.curdir and name != os.pardir \
                and os.path.isdir(fullname) and not os.path.islink(fullname):
            for res in compile_dir(fullname, maxlevels - 1, force, source,
                    encoding, strip_dest_dir, entity_map):
                yield res

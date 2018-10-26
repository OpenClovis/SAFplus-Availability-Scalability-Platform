#!/usr/bin/env python
# -*- coding: utf-8 -*-

# This module provides the "kidc" command

"""Usage: kidc [OPTIONS] [file...]
Compile kid templates into Python byte-code (.pyc) files.

OPTIONS:

  -f, --force
          Force compilation even if .pyc file already exists.
  -s, --source
          Generate .py source files along with .pyc files.
          This is sometimes useful for debugging.
  -d, --strip-dest-dir <destdir>
          Strips the supplied path from the beginning of source
          filenames stored for error messages in the generated
          .pyc files

The file list may have files and/or directories. If a directory is specified,
all .kid files found in the directory and any sub-directories are compiled.
"""

__revision__ = "$Rev: 492 $"
__date__ = "$Date: 2007-07-06 21:38:45 -0400 (Fri, 06 Jul 2007) $"
__author__ = "Ryan Tomayko (rtomayko@gmail.com)"
__copyright__ = "Copyright 2004-2005, Ryan Tomayko"
__license__ = "MIT <http://www.opensource.org/licenses/mit-license.php>"

import sys
from os.path import isdir
from getopt import getopt, GetoptError as gerror

try:
    from os import EX_OK, EX_DATAERR, EX_USAGE
except ImportError:
    EX_OK, EX_DATAERR, EX_USAGE = 0, 1, 2

import kid.compiler

def main():
    # get options
    try:
        opts, args = getopt(sys.argv[1:],
            'fshd=', ['force', 'source', 'help', 'strip-dest-dir='])
    except gerror, e:
        sys.stderr.write(str(e) + '\n')
        sys.stdout.write(__doc__)
        sys.exit(EX_USAGE)
    force = source = False
    strip_dest_dir = None
    for o, a in opts:
        if o in ('-f', '--force'):
            force = True
        elif o in ('-s', '--source'):
            source = True
        elif o in ('-h', '--help'):
            sys.stdout.write(__doc__)
            sys.exit(EX_OK)
        elif o in ('-d', '--strip-dest-dir'):
            strip_dest_dir = a
    files = args

    if not files:
        sys.stderr.write('kidc: No kid template specified.\n')
        sys.stderr.write("      Try 'kidc --help' for usage information.\n")
        sys.exit(EX_USAGE)

    # a quick function for printing results
    def print_result(res):
        stat, filename = res
        if stat == True:
            msg = 'compile: %s\n' % filename
        elif stat == False:
            msg = 'fresh: %s\n' % filename
        else:
            msg = 'error: %s (%s)\n' % (filename, stat)
        sys.stderr.write(msg)

    # run through files and compile
    err = False
    for f in files:
        if isdir(f):
            for res in kid.compiler.compile_dir(f, force=force,
                    source=source, strip_dest_dir=strip_dest_dir):
                if res[0] not in (True, False):
                    err = True
                print_result(res)
        else:
            try:
                stat = kid.compiler.compile_file(f, force=force,
                    source=source, strip_dest_dir=strip_dest_dir)
            except Exception, e:
                stat, err = e, True
            print_result((stat, f))

    # exit with error status if one compilation failed
    sys.exit(err and EX_DATAERR or EX_OK)

if __name__ == '__main__':
    main()

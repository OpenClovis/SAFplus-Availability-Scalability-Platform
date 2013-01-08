#!/usr/bin/env python
# -*- coding: utf-8 -*-

# This module provides the "kid" command

"""Usage: kid [options] file [args]
Expand a Kid template file.

OPTIONS:

  -e enc, --encoding=enc
          Specify the output character encoding.
          Default: utf-8
  -o outfile, --output=outfile
          Specify the output file.
          Default: standard output
  -s host:port, --server=host:port
          Specify the server address if
          you want to start the HTTP server.
          Instead of the Kid template,
          you can specify a base directory.
  -h, --help
          Print this help message and exit.
  -V, --version
          Print the Kid version number and exit.

file:
  filename of the Kid template to be processed
  or "-" for reading the template from stdin.

args:
  key=value or other arguments passed to the template.
"""

__revision__ = "$Rev: 492 $"
__date__ = "$Date: 2007-07-06 21:38:45 -0400 (Fri, 06 Jul 2007) $"
__author__ = "Ryan Tomayko (rtomayko@gmail.com)"
__copyright__ = "Copyright 2004-2005, Ryan Tomayko"
__license__ = "MIT <http://www.opensource.org/licenses/mit-license.php>"

import sys
from os.path import dirname, abspath
from getopt import getopt, GetoptError as gerror

try:
    from os import EX_OK, EX_DATAERR, EX_USAGE
except ImportError:
    EX_OK, EX_DATAERR, EX_USAGE = 0, 1, 2

import kid

def main():
    # get options
    try:
        opts, args = getopt(sys.argv[1:], 'e:o:s:hV',
            ['encoding=', 'output=', 'server=', 'help', 'version'])
    except gerror, e:
        sys.stderr.write(str(e) + '\n')
        sys.stdout.write(__doc__)
        sys.exit(EX_USAGE)
    enc = 'utf-8'
    outfile = server = None
    for o, a in opts:
        if o in ('-e', '--encoding'):
            enc = a
        elif o in ('-o', '--output'):
            outfile = a
        elif o in ('-s', '--server'):
            server = a
        elif o in ('-h', '--help'):
            sys.stdout.write(__doc__)
            sys.exit(EX_OK)
        elif o in ('-V', '--version'):
            from kid import __version__
            sys.stdout.write('Kid %s\n' % __version__)
            sys.exit(EX_OK)
    if server is None:
        if args:
            # get template file
            f = args.pop(0)
            sys.argv = [f]
            if f != '-':
                # make sure template dir is on sys.path
                path = abspath(dirname(f))
                if not path in sys.path:
                    sys.path.insert(0, path)
            else:
                f = sys.stdin.read()
            # get arguments for the template file
            kw = {}
            while args:
                a = args.pop(0).split('=', 1)
                if len(a) > 1:
                    kw[a[0]] = a[1]
                else:
                    sys.argv.append(a[0])
            # do not run as __main__ module
            sys.modules['__kid_main__'] = sys.modules['__main__']
            __name__ = '__kid_main__'
            del sys.modules['__main__']
            # load kid template as __main__ module
            module = kid.load_template(f, name='__main__', cache=False)
            # execute the template and write output
            if not outfile:
                outfile = sys.stdout
            module.write(outfile, encoding=enc, **kw)
        else:
            sys.stderr.write('kid: No template file specified.\n')
            sys.stderr.write("     Try 'kid --help' for usage information.\n")
            sys.exit(EX_USAGE)
    else:
        if len(args) < 2:
            if outfile:
                stderr = file(outfile, 'a', 1)
                sys.stderr = stderr
            sys.stdout.write('Starting HTTP server ...\n')
            if args:
                # get base directory
                basedir = args.pop(0)
                from os import chdir
                chdir(basedir)
            from os import getcwd
            basedir = getcwd()
            sys.stdout.write('Base directory: %s\n' % basedir)
            if outfile:
                sys.stdout.write('Server log: %s\n' % outfile)
            if server == '-':
                server = 'localhost'
            sys.argv[1:] = [server]
            from kid.server import main
            main()
            if outfile:
                sys.stderr = sys.__stderr__
                stderr.close()
        else:
            sys.stderr.write('kid: Server does not need additional arguments.\n')
            sys.stderr.write("     Try 'kid --help' for usage information.\n")
            sys.exit(EX_USAGE)

if __name__ == '__main__':
    main()

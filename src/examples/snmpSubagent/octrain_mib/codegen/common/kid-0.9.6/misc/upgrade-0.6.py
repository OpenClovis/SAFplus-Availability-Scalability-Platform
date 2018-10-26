#!/usr/bin/env python

"""usage: %s file...

Upgrades a kid template from version 0.5 to 0.6. The previous version of each
file is backed up to "file-0.5". It is recommended that

"""

import sys
import re
from os.path import exists
from shutil import move, copymode

ns_exp = re.compile(r'http://naeblis.cx/ns/kid#')
omit_exp = re.compile(r'py:omit=')
brace_exp = re.compile(r"""
                        (?<!\{)\{(?!\{)      # match single {
                        (.+?)                # anything
                        (?<!\})\}(?!\})      # match single }
                        """,
                        re.VERBOSE)

def upgrade_file(fin):
    fout = fin
    fin = fout + '-0.5'
    if exists(fin):
        print 'Backup file: %s already exists - not stomping.'
        print '  Do something with the backup and try again.'
        return
    move(fout, fin)
    fo = open(fin, 'rb')
    text = fo.read()
    fo.close()
    text = ns_exp.sub('http://purl.org/kid/ns#', text)
    text = omit_exp.sub('py:strip=', text)
    parts = brace_exp.split(text)
    for (i, part) in zip(range(len(parts)), parts):
        if (i % 2) == 1:
            parts[i] = '${%s}' % part
    text = ''.join(parts)
    del parts
    fo = open(fout, 'wb')
    fo.write(text)
    fo.close()
    copymode(fin, fout)
    print 'Upgraded: %s...' % fout

if len(sys.argv) == 1:
    print __doc__
    sys.exit(0)

for f in sys.argv[1:]:
    upgrade_file(f)

# -*- coding: utf-8 -*-

"""Utility functions for Kid."""

__revision__ = "$Rev: 492 $"
__date__ = "$Date: 2007-07-06 21:38:45 -0400 (Fri, 06 Jul 2007) $"
__author__ = "Ryan Tomayko (rtomayko@gmail.com)"
__copyright__ = "Copyright 2004-2005, Ryan Tomayko"
__license__ = "MIT <http://www.opensource.org/licenses/mit-license.php>"


from urllib import splittype


class QuickTextReader(object):

    def __init__(self, text):
        self.text = text
        self.len = len(self.text)
        self.pos = 0
        self.lines = None

    def __iter__(self):
        while 1:
            if self.lines is None:
                self.lines = self.text.splitlines(True)
            if not self.lines:
                break
            yield self.lines.pop(0)

    def close(self):
        self.text = None
        self.pos = self.len = 0
        self.lines = None

    def read(self, size=None):
        if size is not None:
            try:
                size = int(size)
            except:
                size = None
            else:
                if not 0 <= size < self.len:
                    size = None
        pos = self.pos
        if size is None:
            self.pos = self.len
            return self.text[pos:]
        else:
            self.pos += size
            if self.pos > self.len:
                self.pos = self.len
            return self.text[pos:self.pos]

    def seek(self, offset, whence=0):
        if whence:
            if whence == 2:
                self.pos = self.len - offset
            else:
                self.pos += offset
        else:
            self.pos = offset
            self.lines = None
        if self.pos < 0:
            self.pos = 0
        elif self.pos > self.len:
            self.pos = self.len

        def tell(self):
            return self.pos

        def next(self):
            if not self.lineno:
                self.lines = self.splitlines(True)
            self.lineno += 1
            if not self.lines:
                raise StopIteration
            return self.lines.pop(0)

def xml_sniff(text):
    """Sniff text to see if it looks like XML.

    Return True if text looks like XML, otherwise return False.

    """
    for x in text:
        if x in '\t\r\n ':
            continue
        elif x == '<':
            return True
        else:
            return False

def open_resource(uri, mode='rb'):
    """Generic resource opener."""
    scheme, rest = splittype(uri)
    if not scheme or (len(scheme) == 1 and rest.startswith('\\')):
        return open(uri, mode)
    else:
        import urllib2
        return urllib2.urlopen(uri)

def get_expat_error(error):
    """Return text showing the position of an Expat error."""
    source, lineno, offset = error.source, error.lineno, error.offset
    if lineno < 1:
        lineno = 1
        offset = 0
    source.seek(0)
    nlines = 0
    for line in source:
        lineno -= 1
        nlines += 1
        if not lineno:
            break
    else:
        offset = 0
    if nlines:
        if nlines == 1:
            if line.startswith('\xef\xbb\xbf'):
                line = line[3:]
        if line:
            if offset < 0:
                offset = 0
            elif offset > len(line):
                offset = len(line)
            if offset > 75:
                if len(line) - offset > 37:
                    line = line[offset-38:offset+38]
                    offset = 38
                else:
                    offset -= len(line) - 76
                    line = line[-76:]
            else:
                line = line[:76]
            if line:
                line = '%s\n%%%ds\n' % (line.rstrip(), offset + 1)
                line = line % '^'
    return line

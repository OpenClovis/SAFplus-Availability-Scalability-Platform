#!/usr/bin/env python
"""
Grabs a Wiki page from a Mediawiki Wiki, such as the one used as Clovis Intranet

Run the script with no arguments for usage info.
"""

import os, sys, re, pdb
import cookielib
import urllib, urllib2
from xml.dom import minidom

cj = cookielib.LWPCookieJar()
opener = urllib2.build_opener(urllib2.HTTPCookieProcessor(cj))
urllib2.install_opener(opener)

# fake a user agent, some websites (like google) don't like automated (robot)
# exploration
TXTHEADER =  {'User-agent' : 'Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.8.1.2) Gecko/20070328 Firefox/2.0.0.2'}

def my_urlopen(url, data=None):
    """
    urlopen wrapper to handle errors and TXTHEADER
    """
    if data:
        data = urllib.urlencode(data)
        
    try:
        req = urllib2.Request(url, data, TXTHEADER)
        handle = urllib2.urlopen(req)

    except IOError, e:
        print 'We failed to open "%s".' % url
        if hasattr(e, 'code'):
            print >> sys.stderr, 'We failed with error code - %s.' % e.code
        elif hasattr(e, 'reason'):
            print >> sys.stderr, "The error object has the following 'reason' attribute :"
            print >> sys.stderr, e.reason
            print >> sys.stderr, "This usually means the server doesn't exist,",
            print >> sys.stderr, "is down, or we don't have an internet connection."
        sys.exit(1)

    else:
        pass
        # print >> sys.stderr, 'Here are the headers of the page :'
        # print >> sys.stderr, handle.info()
        # handle.read() returns the page
        # handle.geturl() returns the true url of the page fetched
        # (in case urlopen has followed any redirects, which it sometimes does)
    
    return handle
    
def wiki_login(base_url, user, passwd):
    """
    Authenticates with wiki. This is typically needed before any page can
    be accessed in Wiki. The handling of this is very Wikimedia specific.
    """
    url = base_url+ \
        '/index.php?title=Special:Userlogin&returnto=Special:Userlogout'
        
    data = {'wpName':user,
            'wpPassword':passwd,
            'wpLoginattempt':'Log+in'}

    handle = my_urlopen(url, data)
    
    print >> sys.stderr, 'Logged in...'

def wiki_getpage(base_url, pagename, format='HTML'):
    """
    Get an arbitrary page from Wiki. This assumes authentication has already
    happened and all session info is stored in cookies
    """
    print >> sys.stderr, "Getting page %s/%s" % (str(base_url),str(pagename))

        # now we need to make raw_content to appear as a file object
        # (i.e., make readline() working
    class FakeFile:
            def __init__(self, lines):
                self.lines = lines
                self.next = 0
            def readline(self):
                if self.next < len(self.lines):
                    self.next+=1
                    return self.lines[self.next-1]+'\n'
                else:
                    return None
    # get rid of any spaces in page name
    pagename = pagename.replace(' ', '')
    
    if format == 'HTML':
        url = base_url+'/index.php/%s' % pagename
    else: # XML format needed for both XML and RAW
        url = base_url+'/index.php/Special:Export/%s' % pagename
    
    page_handler = my_urlopen(url, None)

    if format == 'RAW':
        
        dom = minidom.parse(page_handler)
        text_node = dom.getElementsByTagName('text')
        if len(text_node)==0:
          # This page has no text; it is probably unimplemented
          return FakeFile("")

        assert(len(text_node)==1)
        text_node = text_node[0]
        assert(len(text_node.childNodes)==1)
        text_node = text_node.childNodes[0]
        assert(text_node.nodeType==dom.TEXT_NODE)
        raw_content = text_node.nodeValue

        ff = FakeFile(raw_content.splitlines())
        page_handler = ff

    return page_handler

def wikigrab(base_url, user, passwd, pagename, format='HTML'):
    """
    Grabs a named wiki page and returns a handle that works like a file handle.
    If format is 'XML', the result will be in Wiki XML format.
    """
    wiki_login(base_url, user, passwd)
    return wiki_getpage(base_url, pagename, format)

img = re.compile('\[\[(Image: *([^ |\]]+)).*\]')
# We are looking for lines like this:
# </ul><div class="fullImageLink" id="file"><img border="0" src="/wiki/images/f/fc/Modtut-3.gif" width="25" height="26" alt="" /></div><div class="fullMedia"><p><a href="/wiki/images/f/fc/Modtut-3.gif" class="internal" title="Modtut-3.gif">Modtut-3.gif</a>
# </ul><div class="fullImageLink" id="file"><a href="/wiki/images/0/01/GettingStartedIDE_GenericFlow.gif"><img border="0" src="/wiki/images/thumb/0/01/GettingStartedIDE_GenericFlow.gif/55px-GettingStartedIDE_GenericFlow.gif" width="55" height="238" alt="" /></a><br />
imgline = re.compile('class="fullImageLink" id="file"><[^>]*(href|src)="([^"]+)"')
img_list = [] # cache of image names already downloaded.
def grab_images(base_url, line):
    """ Check line for [[Image:<filename>]] pattern. If found, get image <filename> """
    ms = img.findall(line)
    for m in ms:
        print >> sys.stderr, 'Found image reference [%s] (filename [%s])...' % \
                    (m[0], m[1]),
        sys.stderr.flush()
        pagename = m[0]
        img_fname = m[1]
        if img_fname in img_list:
            print >> sys.stderr, 'already downloaded'
            return
        # grab image
        image_page_handler = wiki_getpage(base_url, pagename, 'HTML')
        line = image_page_handler.readline()
        while line:
            m = imgline.findall(line)
            if not m:
                line = image_page_handler.readline()
                continue
            else:
                assert(len(m)==1)
                m = m[0][1]
                # m has the form '/wiki/images/b/b7/Modtut-74.gif'
                # need to chop '/wiki' part
                url = base_url + '/' + '/'.join(m.split('/')[2:])
                image_file = my_urlopen(url, None)
                imgdata = image_file.read()
                f_img = file(img_fname, 'w')
                f_img.write(imgdata)
                f_img.close()
                print >> sys.stderr, 'saved file as [%s]' % img_fname
                img_list.append(img_fname)
                break
            

if __name__ == "__main__":
    """
    Allows running wikigrab() as a command line tool
    """

    usage = """
Usage:
    %s [-x|-r|-rg] <wiki-base-url> <username> <password> <page-name>

Example:
    %s http://intranet:8080/wiki clovis clovis Engineering
    
    With -x option, the page will be in Wiki XML format.
    With -r option, the page will be in Raw Wiki format.
    With -rg option, in addition to the Raw Wiki page, all embedded
    images will be downloaded from Wiki (with the file name used in
    Wiki).
    
"""
    if not len(sys.argv) in [5, 6]:
        print >> sys.stderr, usage % (sys.argv[0], sys.argv[0])
        sys.exit(1)

    format = 'HTML'
    need_images = False
    if len(sys.argv) == 6:
        if sys.argv[1] == '-x':
            format='XML'
            del(sys.argv[1])
        elif sys.argv[1] in ['-r', '-rg']:
            format='RAW'
            need_images = (len(sys.argv[1])==3)
            del(sys.argv[1])
        else:
            print >> sys.stderr, usage % (sys.argv[0], sys.argv[0])
            sys.exit(1)
    
    base_url = sys.argv[1]
    user = sys.argv[2]
    passwd = sys.argv[3]
    pagename = sys.argv[4]
    
    page_handle = wikigrab(base_url, user, passwd, pagename, format)
    
    line = page_handle.readline()
    while line:
        print line.encode('utf8'),
        if need_images:
            grab_images(base_url, line)
        line = page_handle.readline()
        
    sys.exit(0)

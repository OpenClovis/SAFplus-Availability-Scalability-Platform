#!/usr/bin/env python

"""A very very early attempt at a CGI based handler for Kid templates."""

import sys
# make sure the kid modules are available
# sys.path.append(os.path.dirname(__file__))

import os
import cgi
import cgitb; cgitb.enable()

import kid
f = os.environ['PATH_TRANSLATED']
template = kid.Template(file=f)

def default_cgi_main():
  dic = template.__dict__
  media_type = dic.get('media_type', 'text/html')
  charset = dic.get('encoding', 'utf-8')
  print 'Content-Type: %s;charset=%s' % (media_type, charset)
  print ''

try:
  cgi_main = module.cgi_main
except AttributeError:
  cgi_main = default_cgi_main

cgi_main()
form = cgi.FieldStorage()
for part in template.generate(output=sys.stdout, context=form):
  sys.stdout.write(part)

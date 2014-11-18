import sys
import os
import re
import pdb
from xml.dom import minidom
from string import Template

def componentIcon(iconbase = "component.svg", name = '', commandLine = ''):
  templateComponentIcon = ''
  try:
    fd = open(iconbase)
    templateComponentIcon = Template(fd.read())
  except IOError as ex:
    pass

  return templateComponentIcon.substitute({'name': "%s" %name, 'commandLine': "%s" %commandLine})

if __name__ == '__main__':
  print componentIcon(name="abc", commandLine="def")
import sys
import os
import re
import pdb
from xml.dom import minidom
from string import Template

myDir = os.path.dirname(__file__)
if not myDir: myDir = os.getcwd()

def loadSvgIcon(iconType, config):
  iconTypeMap = { 1 : nodeIcon,
              2 : appIcon,
              3 : sgIcon,
              4 : suIcon,
              5 : siIcon,
              6 : compIcon,
              7 : csiIcon,
              8 : clusterIcon
            }

  return iconTypeMap[iconType](config)

def nodeIcon(nodeConfig):
  iconbase = "node.svg"
  return buildIcon(iconbase, nodeConfig)

def appIcon(appConfig):
  iconbase = "app.svg"
  return buildIcon(iconbase, appConfig)

def sgIcon(sgConfig):
  iconbase = "sg.svg"
  return buildIcon(iconbase, sgConfig)

def suIcon(suConfig):
  iconbase = "su.svg"
  return buildIcon(iconbase, suConfig)

def siIcon(siConfig):
  iconbase = "si.svg"
  return buildIcon(iconbase, siConfig)

def compIcon(compConfig):
  iconbase = "component.svg"
  return buildIcon(iconbase, compConfig)

def csiIcon(csiConfig):
  iconbase = "csi.svg"
  return buildIcon(iconbase, csiConfig)

def clusterIcon(clusterConfig):
  iconbase = "cluster.svg"
  return buildIcon(iconbase, clusterConfig)

def buildIcon(iconbase, config):
  templateSvgIcon = ''
  try:
    fd = open("%s/%s" %(myDir,iconbase))
    templateSvgIcon = Template(fd.read())
  except IOError as ex:
    return templateSvgIcon

  return templateSvgIcon.safe_substitute(**config)

if __name__ == '__main__':
  print loadSvgIcon(6, {'name':"myComponentName", 'commandLine':"myCommandLine"})
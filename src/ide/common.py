import os
import re
userWorkspace = None

class Log():
  def write(self,string):
    print string

log = Log()

class GuiPlaces:
  def __init__(self,menubar, toolbar, statusbar, menu, projectTree):
    self.menubar = menubar
    self.toolbar = toolbar
    self.statusbar = statusbar
    self.menu = menu
    self.prjTree = projectTree

def str2Tuple(s):
  return tuple(float(x) for x in re.findall("[0-9\.]+",s))

def fileResolver(filename):
  """Look in standard places for a file"""
  #if filename[0] == os.sep:  # Filename is already an absolute path
  #  if os.path.exists(filename): return filename

  if os.path.exists(filename): return filename # File exists right where specified, will also handle absolute path of course

  candidate = "resources" + os.sep + filename     # look in resources
  if os.path.exists(candidate): return candidate  
  candidate = "resources" + os.sep + "images" + os.sep + filename # look in media
  if os.path.exists(candidate): return candidate
  candidate = os.path.dirname(__file__) + os.sep + candidate # look in media of this relative path 
  if os.path.exists(candidate): return candidate
  candidate = os.path.dirname(__file__) + os.sep + filename
  if os.path.exists(candidate): return candidate
  if userWorkspace:
    candidate = userWorkspace + os.sep + filename     # look in the user's directory
    if os.path.exists(candidate): return candidate
  # print "No such file: %s" % filename
  raise IOError(2,'No such file')

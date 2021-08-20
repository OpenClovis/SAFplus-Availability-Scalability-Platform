import pdb
import inspect
import os
import re
import types
import pwd
import wx
userWorkspace = None
programDirectory = None

# the background color of toobars.  I have to use opaque -- transparent actually just turns black
BAR_GREY = (210,210,210,wx.ALPHA_OPAQUE)
DISABLED_GREY = (160,160,160,wx.ALPHA_OPAQUE)

class Log():
  def write(self,string):
    print(string)

log = Log()
numRecentProjects = 5 # define the maximum number of recent projects

class Output:
  """This abstract class defines how the generated output is written to the file system
     or tarball.

     In these routines if you tell it to write a file or template to a nonexistant path
     then that path is created.  So you no longer have to worry about creating directories.
     If you call writes("/foo/bar/yarh/myfile.txt","contents") then the directory hierarchy /foo/bar/yarh is automatically created.  The file "myfile.txt" is created and written with "contents".
  """
  def write(self, filename, data):
    if type(data) in (str,):
      self.writes(filename, data)
    else: self.writet(filename,data)
        
  def writet(self, filename, template):
    pass
  def writes(self, filename, template):
    pass
   
class FilesystemOutput(Output):
  """Write to a normal file system"""

  def writet(self, filename, template):
    if not os.path.exists(os.path.dirname(filename)):
      os.makedirs(os.path.dirname(filename))
    template.write(filename, "utf-8",False,"xml")
    
  def writes(self, filename, string):
    if not os.path.exists(os.path.dirname(filename)):
      os.makedirs(os.path.dirname(filename))
    with open(filename,"w") as f:
      f.write(string)

class GuiPlaces:
  """This class identifies all the important services/locations in the GUI so that entities can use these services"""
  def __init__(self,frame, menubar, toolbar, statusbar, menu, projectTree):
    self.frame = frame
    self.menubar = menubar
    self.toolbar = toolbar
    self.statusbar = statusbar
    self.menu = menu
    self.prjTree = projectTree

def elemsInSet(lst, st):
  ret = []
  for l in lst:
    if l in st:
      ret.append(l)
  return ret

def splitByThisAndWhitespace(deliminators, s):
  for delim in deliminators:
    s = s.replace(delim, ' ')
  return s.split()

def str2Tuple(s):
  return tuple(float(x) for x in re.findall("[0-9\.]+",s))

def fileResolver(filename):
  """Look in standard places for a file"""
  global programDirectory
  if not programDirectory: 
    programDirectory = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))) 
  #if filename[0] == os.sep:  # Filename is already an absolute path
  #  if os.path.exists(filename): return filename

  if os.path.exists(filename): return filename # File exists right where specified, will also handle absolute path of course

  candidate = "resources" + os.sep + filename     # look in resources
  if os.path.exists(candidate): return candidate  
  candidate = "resources" + os.sep + "images" + os.sep + filename # look in media
  if os.path.exists(candidate): return candidate

  candidate = programDirectory + os.sep + filename
  if os.path.exists(candidate): return candidate
  candidate = programDirectory + os.sep + "resources" + os.sep + filename 
  if os.path.exists(candidate): return candidate
  candidate = programDirectory + os.sep + "resources" + os.sep + "images" + os.sep + filename 
  if os.path.exists(candidate): return candidate

  if userWorkspace:
    candidate = userWorkspace + os.sep + filename     # look in the user's directory
    if os.path.exists(candidate): return candidate

  # print "No such file: %s" % filename
  pdb.set_trace()
  raise IOError(2,'No such file')

def getRecentPrjFile(force_create=False):
  currentUsername = pwd.getpwuid(os.getuid())[0]
  path = '/home/'+currentUsername+'/.safplus_ide'
  if force_create:
    if not os.path.exists(path):
      os.makedirs(path)
  return path 

recentPrjFile = getRecentPrjFile(True)+'/recent_projects'

def getRecentPrjs():
  if not os.path.exists(recentPrjFile):
    return []
  with open(recentPrjFile) as f:
    contents = f.readlines()
  return contents

def getMostRecentPrj():
  prjs = getRecentPrjs()
  if len(prjs)==0:
    return None
  return prjs[-1]

def getMostRecentPrjDir():  
  t = getMostRecentPrj()
  if t:
    p = os.path.dirname(t)
    return os.path.dirname(p)  
  # t is None, return the default dir /home/`$user`
  currentUsername = pwd.getpwuid(os.getuid())[0]
  return '/home/'+currentUsername

def addRecentPrj(prjPath):
  t = getRecentPrjs()  
  l=len(t)
  p = prjPath+'\n'
  if l<numRecentProjects and not p in t:    
    with open(recentPrjFile,"a+") as f:
      f.write(p)
  else:
    os.remove(recentPrjFile)
    with open(recentPrjFile, "a+") as f:
      t2 = t[l-numRecentProjects+1:] # excerpt last n-1 elements from the original list
      for i in t2:
        if i!=p:
          f.write(i)
      f.write(p)

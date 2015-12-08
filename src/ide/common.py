import pdb
import inspect
import os
import re
import types
userWorkspace = None
programDirectory = None

class Log():
  def write(self,string):
    print string

log = Log()

class Output:
  """This abstract class defines how the generated output is written to the file system
     or tarball.

     In these routines if you tell it to write a file or template to a nonexistant path
     then that path is created.  So you no longer have to worry about creating directories.
     If you call writes("/foo/bar/yarh/myfile.txt","contents") then the directory hierarchy /foo/bar/yarh is automatically created.  The file "myfile.txt" is created and written with "contents".
  """
  def write(self, filename, data):
    if type(data) in types.StringTypes:
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
    f = open(filename,"w")
    f.write(string)
    f.close()

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

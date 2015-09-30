import safplus
import pdb

mgtGet = safplus.mgtGet

def isValidDirectory(path):
  return True

class Commands:
  def __init__(self):
    self.commands = {}
    self.context = None

  def do_set(self, location, value):
    """syntax: 
        set (location) (value)  
        Sets the leaf at the specified locations to the specified value
    """
    loc = self.context.curdir + "/" + location
    safplus.mgtSet(str(loc),str(value))
    return ""

  def do_create(self,location):
    """syntax: 
         create (location)  
         Creates a new object at the specified location.  The type of the object is implied by its location in the tree.
    """
    loc = self.context.curdir + "/" + location
    safplus.mgtCreate(str(loc))
    return ""

  def do_delete(self,location):
    """syntax: 
         delete (location)  
         deletes the object at (location) and all children
    """
    loc = self.context.curdir + "/" + location
    safplus.mgtDelete(str(loc))
    return ""

  def setContext(self,context):
    """Sets the context (environment) object so commands can access it while they are executing"""
    self.context = context


def Initialize():
  svcs = safplus.Libraries.MSG | safplus.Libraries.GRP | safplus.Libraries.MGT_ACCESS
  sic = safplus.SafplusInitializationConfiguration()
  sic.port = 51
  safplus.Initialize(svcs, sic)
  return (Commands(),{})

def isListElem(elem,path):
  """Return the name of the list if this is an item in a list"""
  if elem.attrib.has_key("listkey"):
    return elem.tag
  return None

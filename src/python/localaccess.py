import atexit
import safplus
import pdb

mgtGet = safplus.mgtGet

def isValidDirectory(path):
  data = mgtGet(str("{d=0}"+path))
  if not data: return False  # TODO, what about an empty list?
  # print "isValidDir: ", str(data)
  # pdb.set_trace()
  return True

class Commands:
  def __init__(self):
    self.commands = {}
    self.context = None

  def canonicalPath(self,location):
    print "cur: ", self.context.curdir, " loc: ", location
    if location == "." or location == "" or location == None:
      return self.context.curdir
    if location[0] == "/": # not a relative path
      return location
    if self.context.curdir == "/":
      return "/" + location
    elif self.context.curdir:
      return self.context.curdir + "/" + location
    return location
 

  def do_set(self, location, value):
    """syntax: 
        set (location) (value)  
        Sets the leaf at the specified locations to the specified value
    """
    loc = self.canonicalPath(location)
    ret = safplus.mgtSet(str(loc),str(value))
    if ret == 4:  # CL_ERROR_NOT_EXIST
      return "<error>location [%s] does not exist</error>" % location
    elif ret == 0:
      return ""
    return str(ret)

  def do_create(self,location):
    """syntax: 
         create (location)  
         Creates a new object at the specified location.  The type of the object is implied by its location in the tree.
    """
    loc = self.canonicalPath(location)
    print "create " + loc
    safplus.mgtCreate(str(loc))
    return ""

  def do_delete(self,*locations):
    """syntax: 
         delete (location)  
         deletes the object at (location) and all children
    """
    for l in locations:
      loc = self.canonicalPath(l)
      safplus.mgtDelete(str(loc))
    return ""

  def setContext(self,context):
    """Sets the context (environment) object so commands can access it while they are executing"""
    self.context = context


def Initialize():
  svcs = safplus.Libraries.MSG | safplus.Libraries.GRP | safplus.Libraries.MGT_ACCESS
  sic = safplus.SafplusInitializationConfiguration()
  sic.port = 51
  atexit.register(safplus.Finalize)  # Register a clean up function so SAFplus does not assert on a dead mutex when quitting
  safplus.Initialize(svcs, sic)
  return (Commands(),{})

def isListElem(elem,path):
  """Return the name of the list if this is an item in a list"""
  if elem.attrib.has_key("listkey"):
    return elem.tag
  return None

import safplus

mgtGet = safplus.mgtGet

class Commands:
  def do_set(self, location, value):
    """?  syntax: 
        set (location) (value)  
        Sets the leaf at the specified locations to the specified value
    """
    loc = self.context.curdir + "/" + location
    safplus.mgtSet(str(loc),str(value))
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
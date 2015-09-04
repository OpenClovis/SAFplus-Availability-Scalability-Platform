import safplus

mgtGet = safplus.mgtGet

def Initialize():
  svcs = safplus.Libraries.MSG | safplus.Libraries.GRP | safplus.Libraries.MGT_ACCESS
  sic = safplus.SafplusInitializationConfiguration()
  sic.port = 51
  safplus.Initialize(svcs, sic)
  return None

def isListElem(elem,path):
  """Return the name of the list if this is an item in a list"""
  print path
  if elem.attrib.has_key("listkey"):
    return elem.tag
  return None

import safplus

mgtGet = safplus.mgtGet

def Initialize():
  svcs = safplus.Libraries.MSG | safplus.Libraries.GRP | safplus.Libraries.MGT_ACCESS
  sic = safplus.SafplusInitializationConfiguration()
  sic.port = 51
  safplus.Initialize(svcs, sic)

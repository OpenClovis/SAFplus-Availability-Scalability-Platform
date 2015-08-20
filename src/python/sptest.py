import pdb
import safplus as sp



svcs = sp.Libraries.MSG | sp.Libraries.GRP | sp.Libraries.MGT_ACCESS
sic = sp.SafplusInitializationConfiguration()
sic.port = 51
sp.Initialize(svcs, sic)

sp.logMsgWrite(sp.APP_LOG,sp.LogSeverity.ERROR,1,"LIB","SCN","file",300,"test")

#sg0xml = sp.mgtGet("/")
#print "/:\n",sg0xml

sg0xml = sp.mgtGet("{d=1}/safplusAmf")
print "SG0:\n",sg0xml
sg0xml = sp.mgtGet("{d=2}/safplusAmf")
print "SG0:\n",sg0xml

sg0xml = sp.mgtGet("{d=1}/safplusAmf/ServiceGroup/sg0")
print "SG0:\n",sg0xml
pdb.set_trace()
print "Getting su0:"
su0xml = sp.mgtGet("/safplusAmf/ServiceUnit[name='su0']")
print su0xml

def ckptTest():
  hdl = sp.Handle.create(0)  # sp.Handle(sp.HandleType.Transient,1,0,0,0)
  txn = sp.Transaction()
  ckpt = sp.Checkpoint(hdl,sp.Checkpoint.REPLICATED | sp.Checkpoint.SHARED,64000,500)

  ckpt.write("testkey","testval",txn)
  val = ckpt.read("testkey")
  print str(val)
  try:
    val = ckpt.read("foo")
  except KeyError, e:
    print "Error works!"

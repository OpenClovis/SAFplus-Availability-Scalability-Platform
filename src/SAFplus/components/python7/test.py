import pySAFplus as sp

sp.logMsgWrite(sp.APP_LOG,sp.Severity.ERROR,1,"LIB","SCN","file",300,"test")
hdl = sp.Handle(sp.HandleType.Transient,1,0,0,0)
txn = sp.Transaction()
ckpt = sp.Checkpoint(hdl,sp.Checkpoint.REPLICATED | sp.Checkpoint.SHARED,64000,500)

ckpt.write("testkey","testval",txn)
val = ckpt.read("testkey")
print str(val)
try:
  val = ckpt.read("foo")
except KeyError, e:
  print "Error works!"

import gdb
import gdb.types

VERSION = "7.0.0"

# frame = gdb.selected_frame()
# gdb.newest_frame ()

if 0:
  (sym, methField) = gdb.lookup_symbol("origMsg")
  gdb.write(" " + str(sym.type) + str(sym.line) + str(sym.value(frame).dereference().type.fields()))

  msgPtr = sym.value(frame)
#msgPtr = msgPtr.dereference()["nextMsg"]
  if msgPtr == 0:
    print("ZERO")
    gdb.write("ZZZZZZ")

  gdb.write(" " + str(msgPtr))

  count = 0
  while msgPtr != 0 and count < 1500:
    count+=1
    obj = msgPtr.dereference()
    gdb.write(str(msgPtr) + ": " + str(obj) + "\n")
    msgPtr = obj["nextMsg"]

def dumpFrag(fragPtr, indent = 0):
  count = 0
  while fragPtr != 0 and count < 1500:
    count+=1
    obj = fragPtr.dereference()
    gdb.write(" "*indent + str(fragPtr) + ": " + str(obj) + "\n")
    fragPtr = obj["nextFragment"]


def dumpMsg(msgPtr,indent = 0):
    count = 0
    while msgPtr != 0 and count < 1500:
      count+=1
      obj = msgPtr.dereference()
      gdb.write(" "*indent + str(msgPtr) + ": " + str(obj) + "\n")
      dumpFrag(obj["firstFragment"],2)
      msgPtr = obj["nextMsg"]

class SpHelp(gdb.Command):
    def __init__(self,cmds):
        super(SpHelp, self).__init__("sp-help", gdb.COMMAND_DATA, gdb.COMPLETE_SYMBOL)
        self.cmds = cmds

    def help(self):
      return "sp-help: This help"

    def invoke(self, argument, from_tty):
        args = gdb.string_to_argv(argument)
        if len(args)>0:
          expr = args[0]
          arg = gdb.parse_and_eval(expr)
          # todo find this command and print help on only it
        for c in self.cmds:
          gdb.write(c.help() + "\n")
        
 
  
class SpDumpMsg(gdb.Command):
    def __init__(self):
        super(SpDumpMsg, self).__init__("sp-dump-msg", gdb.COMMAND_DATA, gdb.COMPLETE_SYMBOL)

    def help(self):
      return "sp-dump-msg SAFplus::Message*: Iterate through this list of messages printing all information and fragment information"
    def invoke(self, argument, from_tty):
        args = gdb.string_to_argv(argument)
        if len(args)!=1:
          gdb.write("args: pointer to the message\n")
          return
        expr = args[0]
        msgPtr = gdb.parse_and_eval(expr)
        dumpMsg(msgPtr)
        
        

sd = SpDumpMsg()

SpHelp([sd])

gdb.write("Loaded SAFplus GDB extensions %s\n" % VERSION)

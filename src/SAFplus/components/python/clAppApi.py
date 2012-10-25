import sys
import amfpy
from aspLog import Log

app = None

class ClAppApi:
  def __init__(self):
    global app
    # self.Log("App constructor")
    app = self

  def Start(self):
    """Start yourself (and keep this thread until termination)"""
    amfpy.initializeAmf()

  #def __del__(self):
  #  """Destructor"""
  #  amfpy.finalizeAmf()

  def Stop(self):
    """Stop yourself, all threads you have created, and return from the "Start" function."""
    self.Log("Executing unimplemented function Stop")
    pass

  def HealthCheck(self): 
    """Override (optional) and return True if your app is ok, otherwise return any error"""
    pass

  
  # WORK ASSIGNMENT CALLBACKS

  def ActivateWorkAssignment (self, workDescriptor): 
    """Override and activate the passed work (SAForum CSI) assignment
     * @param workDescriptor A dictionary detailing the work item.
    """
    self.Log("Executing unimplemented function ActivateWorkAssignment")
    pass

  def StandbyWorkAssignment  (self, workDescriptor):
    """Override and set to standby the passed work (SAForum CSI) assignment
     * @param workDescriptor A dictionary detailing the work item.
    """
    self.Log("Executing unimplemented function StandbyWorkAssignment")
    pass

  def QuiesceWorkAssignment  (self, workDescriptor):
    """Override and cleanly stop the passed work (SAForum CSI) assignment
     * @param workDescriptor A structure detailing the work item.
    """
    self.Log("Executing unimplemented function QuiesceWorkAssignment")
    pass

  def AbortWorkAssignment    (self, workDescriptor):
    """Override and halt the work (SAForum CSI) assignment
     * @param workDescriptor A structure detailing the work item.
    """
    self.Log("Executing unimplemented function AbortWorkAssignment")
    pass

  def RemoveWorkAssignment (self, workName, all):
    """Override and remove the passed work (SAForum CSI) assignment from your component.
     * @param workName The name of the work item (previously passed in an Activate call)
     * @param all      If True, remove ALL matching work items; otherwise, just remove one.
    """
    self.Log("Executing unimplemented function RemoveWorkAssignment")
    pass

  def Log(self, levelOrStr,s=None):
    """ This function takes logs of the format:
    Log(LEVEL,STRING) example: Log(safplus.LogLevelError,'Big problem')
    Log(STRING) example: Log('informational log')
    Log levels are defined in the 'safplus' module as: LogLevelError, LogLevelWarn, LogLevelInfo, LogLevelDebug
    """
    if s is None:
      s = levelOrStr
      levelOrStr = None
    Log(s,levelOrStr,logunwind=2)
    
    #sys.stderr.write(s)
    #sys.stderr.flush()

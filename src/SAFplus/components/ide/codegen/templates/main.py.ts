# Standard Python Imports
import os, sys
import pdb
import time
import os.path
import thread

# SAFplus Imports
try:
  import safplus
except ImportError:
  print "\nSAFplus Python library not found.  Did you configure SAFplus with the --with-python-build option?\n"
  raise

# Your application's imports


# The SAFplus application class

class App(safplus.SafApp):
  """This class contains your implementation of the SAF AMF control callbacks"""
  
  def __init__(self):
    safplus.SafApp.__init__(self)
    self.Log(safplus.LogLevelInfo, "Python-based SAF app %s is created running from %s" % (safplus.COMP_NAME,safplus.DEPLOY_DIR))
      
  def Start(self):
    """Start yourself"""
    safplus.SafApp.Start(self) # call parent's function when you are ready to connect to the SAF AMF
    pass
    
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
    # This is just an example of what you might do when an active comes in.
    # In this case, I spawn a function in a thread.   
    thread.start_new_thread(ExampleActiveFunction,(self,workDescriptor))

    # You must return within the configured timeout to release this thread back to the AMF
    return

  def StandbyWorkAssignment  (self, workDescriptor):
    """Override and set to standby the passed work (SAForum CSI) assignment
     * @param workDescriptor A dictionary detailing the work item.
    """
    # This is just an example of what you might do when an active comes in.
    # In this case, I spawn a function in a thread.   
    thread.start_new_thread(ExampleStandbyFunction,(self,workDescriptor))

    # You must return within the configured timeout to release this thread back to the AMF
    return 

  def QuiesceWorkAssignment  (self, workDescriptor):
    """Override and cleanly stop the passed work (SAForum CSI) assignment
     * @param workDescriptor A structure detailing the work item.
    """
    pass

  def AbortWorkAssignment    (self, workDescriptor):
    """Override and halt the work (SAForum CSI) assignment
     * @param workDescriptor A structure detailing the work item.
    """
    pass

  def RemoveWorkAssignment (self, workName, all):
    """Override and remove the passed work (SAForum CSI) assignment from your component.
     * @param workName The name of the work item (previously passed in an Activate call)
     * @param all      If True, remove ALL matching work items; otherwise, just remove one.
    """
    pass


# These are example active and standby functions...

def ExampleActiveFunction(app,work):
  "This function is called in a thread when the application becomes active"
  app.Log(safplus.LogLevelInfo,"Component '%s' is assigned active for the work '%s'" % (safplus.COMP_NAME,work['csiName']))

def ExampleStandbyFunction(app,work):
  "This function is called in a thread when the application becomes standby"
  app.Log(safplus.LogLevelInfo,"Component '%s' is assigned standby for the work '%s'" % (safplus.COMP_NAME,work['csiName']))



# Instantiate the app
app = App() 

def main(argv):
  # Connect this app to the SAFplus AMF. You must do this within the
  # configured time or the SAFplus AMF will fail your process. 
  app.Start()
  
  # If you started this app from the command line the SAFplus AMF is not going
  # to manage it.
  if safplus.MANUAL_START:
    # So I'll manually fake a call to "activate" to get things going
    app.ActivateWorkAssignment({'csiName':'fakeWork'}) # You should change dictionary to valid work for your app
  
  print "%s: main is done. Quit or drop to interactive prompt" % safplus.COMP_NAME
      
if __name__ == "__main__":
  main(sys.argv)

def Test():
  # You can implement unit test code here
  pass

import thread
import sys
import os
import time
import pdb
import traceback
from string import Template

import clAppApi

#csi = None

plugins = None
thisEnv = None

ACTIVE  = "active"
STANDBY = "standby"
STOPPED = "stopped"
NOTEXIST = None


# These are functions that are injected into the plugin environment
cliImport ="""
def cliImport():
  global cliApp
  global cliShell
  if 1:
    try:
      import pysh
      clShellCapable = True
      import thread

      cliApp = pysh.App(redirect=False)
      cliShell = cliApp.frame

      thread.start_new_thread(cliInstantiate,())
    except Exception, e:  # if wxwindows is not installed
      print str(e)
      clShellCapable = False 
  else:
      clShellCapable = False
  return clShellCapable
"""

cliInstantiate ="""
def cliInstantiate():
  global cliApp
  cliApp.MainLoop()
"""

def pluginImport(plugin,pluginObj):
  return """import %s
%s = %s.clGetPlugin()
plugins[%s.VirtProduct]=(thisEnv,'%s')""" % (plugin, pluginObj, plugin,pluginObj,pluginObj)


class ClEoApp(clAppApi.ClAppApi):
  def __init__(self, interactiveDialog=True):
    self.log = open("eoapp.log","w+")
    self.plugins = {}
    self.csi     = {}
    self.interactiveDialog = interactiveDialog
    self.pluginLst = {}  
    self.InitializePlugins()


  def InitializePluginEnv(self, env=None):
    if env is None:
      if self.interactiveDialog: env = globals()
      else: env = {}

      env.update({ "cliApp":None, "cliShell":None,"plugins": self.plugins, "thisEnv":env})
      # thisEnv is nicely twisted, huh?  This is just so I can populate the self.plugins from the exec env 

    if self.interactiveDialog:
      exec cliImport in env
      exec cliInstantiate in env
      eval("cliImport()",env)
      # Once all of the exec envs are started, wait until they are up.
      # pyshell has a bug that requires that it come up before an event comes in
      time.sleep(3)


    return env
    

  def InitializePlugins(self):
    # Get the plugin path
    path = os.getenv("ASP_BINDIR")
    if not path: 
      #path = "../../bin"
      path = "."
    pluginDir = os.path.realpath(path)

    # Get the plugin imports
    walk = os.walk(pluginDir)
    for (dirpath,dirnames,filenames) in walk:  break  # Just get the first one, I am not recursing
    files = filter(lambda x: ("Plugin" in x) and (os.path.splitext(x)[1] == ".py"), filenames)
    files = filter(lambda x: not ("clVmPluginApi" in x), files)
    imports = map(lambda x: os.path.splitext(x)[0],files)

    envs = []
    for i in imports:
      env = self.InitializePluginEnv()
      pluginVar = i + "Obj"
      # import the plugin to our environment
      if env['cliShell']:
        # BUG: Note, the order of execution is lost when you "RunCmd"
        eval("cliShell.RunCmd(%s)" % pluginImport(i,pluginVar).split("\n"), env)
      else: exec pluginImport(i,pluginVar) in env
   
      envs.append((i, env,pluginVar))

    # I must return from Python so that other C libraries have a chance to run (like wxwindows)
    # and load up the module.  So I spawn a thread that monitors when the load is done.  YUCK!
    # thread.start_new_thread(ClEoApp.MonitorPluginReady, (self,envs))

  def MonitorPluginReady(self,allenvs):
    self.Log(str(allenvs))
    for (plugin, env,pluginVar) in allenvs:
      pi = None
      while pi is None:
        try:
          pi = self.pluginLst[pluginVar]
        except KeyError:
          self.Log(str(env))
          self.Log("still waiting for plugin %s, variable %s" % (plugin, pluginVar))
          time.sleep(5)
      # Each plugin is stored by name and contains 2 fields: the environment it runs in, and the name of the plugin object within that environment.
      self.plugins[pi.VirtProduct] = (env, pluginVar) 

      self.Log("Plugin [%s] loaded.  Handles the [%s] virtualization product." % (plugin,pi.VirtProduct))
    

      
  def EvalInAllPlugins(self,cmd):
    ret = []
    t = Template(cmd)
    for (prod,(env, var)) in self.plugins.items():
      s = t.substitute(plugin=var)
      self.Log("Evaluating: %s" % s)
      if env['cliShell']:
        # BUG: Note, the order of execution is lost when you "RunCmd"
        eval("cliShell.RunCmd(\"\"\"%s\"\"\")" % s, env)
      else:
        ret.append(eval(s,env))
    return ret

  def EvalInPlugin(self,prod,cmd):
    ret = None
    t = Template(cmd)
    (env,var) = self.plugins.get(prod,(None,None))
    if env:
      s = t.substitute(plugin=var)
      self.Log("Evaluating: %s" % s)
      if env['cliShell']:
        # BUG: Note, the order of execution is lost when you "RunCmd"
        eval("cliShell.RunCmd(\"\"\"%s\"\"\")" % s, env)
      else:
        ret = eval(s,env)
    return ret


  def Start(self):
    """Start yourself (and keep this thread until termination)"""
    self.Log("Stage 3.  ClEoApp Start called")
    self.EvalInAllPlugins("$plugin.appApi.Start()")
    pass

  def Stop(self):
    """Stop yourself, all threads you have created, and return from the "Start" function."""
    self.Log("Stage 3.  ClEoApp Stop called")
    self.EvalInAllPlugins("$plugin.appApi.Stop()")

    # TODO: Bug, should eliminate common envs before running this
    for (prod,(env, var)) in self.plugins.items():
      if env["cliApp"] is not None:
        try:
          eval("cliApp.Exit()", env)
        except KeyError, e:
          pass


  def HealthCheck(self): 
    """Override (optional) and return True if your app is ok, otherwise return any error"""
    self.EvalInAllPlugins("$plugin.appApi.HealthCheck()")    
    pass

  
  # WORK ASSIGNMENT CALLBACKS

  def ActivateWorkAssignment (self, workDescriptor): 
    """Override and activate the passed work (SAForum CSI) assignment
     * @param workDescriptor A dictionary detailing the work item.
    """
    self.Log("Stage 3.  ClEoApp ActivateWorkAssignment called: %s" % str(workDescriptor))
    if workDescriptor.has_key('csiFlags') and workDescriptor['csiFlags'] == 'Target All':
      return self.EvalInAllPlugins(self,"$plugin.appApi.ActivateWorkAssignment(%s)" % str(workDescriptor))
    else:
      # We do this in case the AMF calls us without passing the CSI.
      workName = workDescriptor['csiName']
      wd = self.csi.get(workName, (0,{}))[1]  # Get the existing descriptor if there is one
      wd.update(workDescriptor) # Overwrite with new key/values

      self.csi[workName] = (ACTIVE, wd)
      return self.EvalInPlugin(wd['VirtProduct'], "$plugin.appApi.ActivateWorkAssignment(%s)" % str(wd))


  def StandbyWorkAssignment  (self, workDescriptor):
    """Override and set to standby the passed work (SAForum CSI) assignment
     * @param workDescriptor A dictionary detailing the work item.
    """
    self.Log("Stage 3.  ClEoApp StandbyWorkAssignment called: %s" % str(workDescriptor))

    # We do this in case the AMF calls us without passing the CSI.
    workName = workDescriptor['csiName']
    wd = self.csi.get(workName, (0,{}))[1]  # Get the existing descriptor if there is one
    wd.update(workDescriptor) # Overwrite with new key/values
    
    self.csi[workName] = (STANDBY, wd)
    return self.EvalInPlugin(wd['VirtProduct'], "$plugin.appApi.StandbyWorkAssignment(%s)" % str(wd))


  def QuiesceWorkAssignment  (self, assignment):
    """Override and cleanly stop the passed work (SAForum CSI) assignment
     * @param workDescriptor A structure detailing the work item.
    """
    self.Log("Stage 3.  ClEoApp QuiesceWorkAssignment called: %s" % str(assignment))

    if assignment.has_key('csiFlags') and assignment['csiFlags'] == 'Target All':
      return self.EvalInAllPlugins(self,"$plugin.appApi.QuiesceWorkAssignment(%s)" % str(assignment))
    else:
      workName = assignment['csiName']
      wd = self.csi.get(workName, (0,{}))[1]  # Get the existing descriptor if there is one
      wd.update(assignment) # Overwrite with new key/values

      self.csi[workName] = (STOPPED, wd)
      return self.EvalInPlugin(wd['VirtProduct'], "$plugin.appApi.QuiesceWorkAssignment(%s)" % str(wd))


  def AbortWorkAssignment    (self, assignment):
    """Override and halt the work (SAForum CSI) assignment
     * @param workDescriptor A structure detailing the work item.
    """
    self.Log("Stage 3.  ClEoApp AbortWorkAssignment called: %s" % str(assignment))

    if assignment.has_key('csiFlags') and assignment['csiFlags'] == 'Target All':
      return self.EvalInAllPlugins(self,"$plugin.appApi.AbortWorkAssignment(%s)" % str(wd))
    else:
      workName = assignment['csiName']
      wd = self.csi.get(workName, (0,{}))[1]  # Get the existing descriptor if there is one
      wd.update(assignment) # Overwrite with new key/values

      self.csi[workName] = (STOPPED, wd)
      return self.EvalInPlugin(wd['VirtProduct'], "$plugin.appApi.AbortWorkAssignment(%s)" % str(wd))


  def RemoveWorkAssignment (self, csiName, all):
    """Override and remove the passed work (SAForum CSI) assignment from your component.
     * @param csiName The name of the work item (previously passed in an Activate call)
     * @param all      If True, remove ALL matching work items; otherwise, just remove one.
    """
    self.Log("Stage 3.  ClEoApp RemoveWorkAssignment called: workName: %s all? %s" % (str(csiName),str(all)))
    cmd = "$plugin.appApi.RemoveWorkAssignment('%s',%s)" % (str(csiName),str(all))
    if all:
      #self.csi = {}
      return self.EvalInAllPlugins(self,cmd)
    else:
      workDescriptor = self.csi[csiName][1]
      self.Log("Removing: %s" % workDescriptor)
      #del self.csi[csiName]
      return self.EvalInPlugin(workDescriptor['VirtProduct'], cmd)


  def Log(self, s):
    sys.stderr.write(str(s) + "\n")
    sys.stderr.flush()
    self.log.write(str(s) + "\n")
    self.log.flush()

    if 0:
     try:
      if self.userSpace['cliShell']:
        self.userSpace['cliShell'].RunCmd("# %s" % s)
      sys.stderr.write("%s\n" % str(s))
      sys.stderr.flush()
     except Exception, e:
       sys.stderr.write("Logging Exception: %s" % str(e))

  def RunCmd(self,cmd):
    self.log.write(cmd + "\n")
    self.log.flush()
    try:
      if "Start" in cmd or "Stop" in cmd:
        eval(cmd)
    except Exception, e:
      sys.stderr.write("exception during local cmd evaluation: %s, %s" % (str(e), cmd))
      sys.stderr.flush()
   
    try:
      if self.userSpace['cliShell']:
        self.userSpace['cliShell'].RunCmd(cmd + "\n")
      else:
        sys.stderr.write("%s\n" % str(cmd))
        sys.stderr.flush()
        ret = eval(cmd,self.userSpace)
    except Exception, e:
      sys.stderr.write("Command Exception: %s" % str(e))


def evalOrExec(cmd,globl,local):
    """I don't care whether it returns a value or not; I just want it to work!"""
    try:
      ret = eval(cmd,globl,local)
      return ret
    except SyntaxError, e:  # You get a syntax error when you try to eval a statement
      exec cmd in globl,local
      return None
    except Exception, e:
      sys.stderr.write("Command: %s failed.  Error %s (dictionary %s)\n" % (str(cmd),str(e),str(e.__dict__)))
      traceback.print_exc(file=sys.stderr)
      sys.stderr.flush()
      
  



# A debugging log of all commands that came from the ASP
fromAspLog = None #open("fromasp.log","w+")

# The basic EO app implementation
baseEoApp = ClEoApp(False) 

def clCmdFromAsp(cmd):
  """This is the entry function for all commands coming from the ASP infrastructure"""
  if fromAspLog:
    fromAspLog.write(str(baseEoApp) + " " + cmd + "\n")
    fromAspLog.flush()

  sys.stderr.write("Stage 2. Command is in clEoApp.clCmdFromAsp: %s\n" % str(cmd))
  sys.stderr.flush()

   
  return evalOrExec(cmd,globals(),{"eoApp":baseEoApp})



def Test():
  eoapp = ClEoApp(True)
  eoapp = ClEoApp(False)
  time.sleep(10000)
  pdb.set_trace()

def Test1():
  clCmdFromAsp("""eoApp.Start()""")

  clCmdFromAsp("""eoApp.ActivateWorkAssignment({ 'csiFlags':'Add One', 'csiHaState':'Active' , 'csiName':'GuestMgrCSII11' , 'name':'xen1' , 'configFile':'/etc/xen/xen1.cfg' , 'csiTransitionDescriptor':'1' , 'csiActiveComponent':'GuestMgrI0' })""")
  clCmdFromAsp("""csi = { 'csiFlags':'Add One', 'csiHaState':'Active' , 'csiName':'GuestMgrCSII11' , 'name':'xen1' , 'configFile':'/etc/xen/xen1.cfg' , 'csiTransitionDescriptor':'1' , 'csiActiveComponent':'GuestMgrI0' }""")
  clCmdFromAsp("""eoApp.ActivateWorkAssignment(csi)""")

  time.sleep(30)
  clCmdFromAsp("""eoApp.Stop()""")
 
def Test2():
  clCmdFromAsp("eoApp.Start()")
#  pdb.set_trace()  
  clCmdFromAsp("""eoApp.ActivateWorkAssignment({ 'csiFlags':'Add One', 'csiHaState':'Active' , 'csiName':'GuestMgrCSII11' , 'name':'xen1' , 'configFile':'/etc/xen/xen1.cfg' , 'csiTransitionDescriptor':'1' , 'csiActiveComponent':'GuestMgrI0', 'VirtProduct':'Xen','VirtProductVersion':'3.1' })""")

  time.sleep(60)  
  clCmdFromAsp("eoApp.Stop()")


if __name__ == "__main__":
  Test2()

#else:
#  eoApp = ClEoApp(True)

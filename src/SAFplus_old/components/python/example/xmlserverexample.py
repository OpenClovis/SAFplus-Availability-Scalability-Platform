"""@namespace examples.xml

This example shows a xml-rpc protocol for program-to-program
communication.  For example, your EMS could use this protocol (extended,
of course) to control the cluster.

For more information about the XML RPC standard, see: http://www.xmlrpc.com/

By default, this server listens to port 8001.  The following URLs are available:

The companion program @ref xmlclientexample.py can be used to query this server.
"""


import os
import time
import threading

from SimpleXMLRPCServer import SimpleXMLRPCServer
from SimpleXMLRPCServer import SimpleXMLRPCRequestHandler


XMLRPC_PORT = 8001

# Boilerplate ASP setup code

# Connect to ASP  *MUST BE DONE FIRST*
import amfpy; amfpy.initializeAmf()

# Import Python ASP class layers
import clusterinfo; import aspApp; import aspAmf; import upgrade; import appdeploy

# Hook into the various services
ci    = clusterinfo.ci                                       # The clusterinfo.ci objects reflect the entire state of the cluster
amf   = aspAmf.Session()                                     # The aspAmf.Session class lets you modify the cluster state
appDb = aspApp.AppDb(os.getenv("ASP_DIR","/tmp") + "/apps")  # application database
umgr  = upgrade.UpgradeMgr()                                 # Upgrade manager

ci.setAppDb(appDb)                                           # connect clusterinfo to application versions

# Optional add some application bundles
if 0:
  appDb.NewAppFile("/code/vipapp1.4.0.0.tgz")  # Of course these would need to be changed to valid locations in your system
  appDb.NewAppFile("/code/vipapp1.4.0.1.tgz")


# Refresh the clusterinfo database periodically.
# since ci.load is an expensive operation, this is done outside of request processing so that web requests are served rapidly.
def reloadDb():
  while(1):
    ci.load()
    time.sleep(10)

refreshThread = threading.Thread(None,reloadDb,"clusterinfo_refresh")
refreshThread.daemon = True
refreshThread.start()


# Implement the functions that should be exposed by XML RPC
class ServerFunctions:

  def GetCluster(self):
    """Return node, su, and component hierarchy"""
    ret = []
    for node in ci.nodeList:
        retsu = []
        for su in node.su:
          retcomp = []
          for comp in su.comp:
            if comp.isRunning(): status = "running"
            else: status = "stopped"
            retcomp.append((comp.name, status, comp.command()))
          retsu.append((su.name,su.haRole(),retcomp))
        ret.append((node.name,node.slot,retsu))
    return ret
 
  def GetNodes(self):
    """Show the list of nodes"""
    return [x.name for x in ci.nodeList]

  def GetServiceUnits(self):
    """Show a list of Service Units"""
    return [x.name for x in ci.suList]

  def GetServiceGroups(self):
    """Show a list of Service Groups"""
    return [x.name for x in ci.sgList]

  def GetApplications(self):
    """Show a list of applications"""
    ret = []
    for app in appDb.apps.values():
      versions = []
      for ver in app.version.keys():
        versions.append(ver)
      ret.append((app.name,versions))
    return ret

  def NewApp(self,newfile):
     """Add a new application into the system.
        Note that the new application bundle file must exist on the system.
        The expectation is that it has been uploaded using some standard interface like scp or ftp.
        However, you can always extend this protocol to add file transfer...
     """
     af = appDb.NewAppFile(newfile)
     return (af.archive, af.appName, af.version)

  def Deploy(self,app,ver,nodeNames):
     """Deploy the new application"""
     nodes = [ci.entities[x] for x in nodeNames]  # Convert names to objects
     appFile = appDb.apps[app].version[ver]
     (sgList,(errors,notes)) = appdeploy.deploy(appFile.dir,appFile.cfg,nodes,abortIfCantAccess=True,copy=True,deploy=True)
     return ([x.name for x in sgList], errors, notes)

  def Upgrade(self,sgname,ver):
     """Upgrade an service group"""
     ci.load()
     sg = ci.entities[sgname]
     app = sg.app
     appFile = app.version[ver]
     umgr.add(sg)
     upSg = umgr.entities[sg.name]
     upSg.Upgrade(appFile)

     return upSg.upStatus


def RunServer():
  """Create server and run it forever"""
  server = SimpleXMLRPCServer(("", XMLRPC_PORT))
  server.register_introspection_functions()
  server.register_instance(ServerFunctions())
  server.serve_forever()

if __name__=="__main__":
  RunServer()

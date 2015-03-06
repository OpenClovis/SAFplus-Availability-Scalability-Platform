"""@namespace examples.http

This example shows a simple http server protocol for program-to-program
communication.  For example, your EMS could use this protocol (extended,
of course) to control the cluster.

For understandability, many of the server's responses have been enhanced 
with human-readable annotation in this simple example.  Of course, a real
implementation would return data more easily parsed by the client software
(such as XML formatted data).

By default, the server listens to port 8000.  The following URLs are available:

Show nodes, Service Units and Components:  URL: /

Show the list of nodes                     URL: /nodes

Show a list of Service Units               URL: /serviceUnits

Show a list of Service Groups              URL: /serviceGroups

Show a list of applications                URL: /apps

Add a new application into the system:     URL: /newapp/<filename>  
                                   For example: /newapp/home/stone/myApp.tgz

Deploy a new application                   URL: /deploy/<app>/<ver>/<nodes>  
                                   For example: /deploy/virtualIp/1.4.0.0/ctrlI0

Upgrade an service group              URL: /upgrade/<servicegroup>/<new version>
                                   For example: /upgrade/virtualIpi0/1.4.0.1
"""


import os
import time
import threading

import SocketServer
import BaseHTTPServer
import urlparse

HTTP_PORT = 8000

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


# Implement a simple HTTP request handler
class MyHandler(BaseHTTPServer.BaseHTTPRequestHandler):

    def do_GET(self):
      try:        
        parsed_path = urlparse.urlparse(self.path)

        # Show the Node->Service Unit->Component tree
        if self.path == "/":
          message = "Cluster:\n"
          for node in ci.nodeList:
            message += "  Node: " + node.name + " Slot: " + str(node.slot) + "\n"
            for su in node.su:
              message += "    Service Unit: " + su.name + " Role: " + su.haRole() + "\n"
              for comp in su.comp:
                if comp.isRunning(): status = "running"
                else: status = "stopped"
                message += "      Component: " + comp.name + " Status: " + status + " Executable: " + comp.command() + "\n"

        # Show the list of nodes
        elif self.path == "/nodes":
          message = [x.name for x in ci.nodeList]

        # Show a list of Service Units
        elif self.path == "/serviceUnits":
          message = [x.name for x in ci.suList]

        # Show a list of Service Groups
        elif self.path == "/serviceGroups":
          message = [x.name for x in ci.sgList]

        # Show a list of applications
        elif self.path == "/apps":
          message = "Applications:\n"
          for app in appDb.apps.values():
            message += app.name + "\n"
            for ver in app.version.keys():
              message += "  " + ver + "\n"

        # Add a new application into the system: /newapp/<filename>  For example: /newapp/home/stone/myApp.tgz
        # Note that the new application bundle file must exist on the system.
        # The expectation is that it has been uploaded using some standard interface like scp or ftp.
        # However, you can always extend this protocol to add file transfer...
        elif self.path.startswith("/newapp"):
          newfile = self.path[7:]  # Get everything after /newapp
          af = appDb.NewAppFile(newfile)
          message = "Application Bundle file %s registered: Application Name: %s Version: %s" % (af.archive, af.appName, af.version)  
        # Deploy the new application:  /deploy/<app>/<ver>/<nodes>  For example: /deploy/virtualIp/1.4.0.0/ctrlI0
        elif "deploy" in self.path:  
          components = self.path.split("/")
          if len(components) != 5: message = "Improper format: %s. Length is %d" % (components,len(components))
          else:
            app = components[2]
            ver = components[3]
            nodeNames = components[4].split("_")
            nodes = [ci.entities[x] for x in nodeNames]  # Convert names to objects
            appFile = appDb.apps[app].version[ver]
            (sgList,(errors,notes)) = appdeploy.deploy(appFile.dir,appFile.cfg,nodes,abortIfCantAccess=True,copy=True,deploy=True)
            message = "Executed Deploy %s %s %s.\nResult: SG: %s Errors: %s Notes:%s" % (app, ver, nodeNames, [x.name for x in sgList], errors, notes)

        # Upgrade an service group: /upgrade/<servicegroup>/<new version>  For example: /upgrade/virtualIpi0/1.4.0.1
        elif "upgrade" in self.path:  # /upgrade/sg/ver
          ci.load()
          components = self.path.split("/")
          if len(components) != 4: message = "Improper format: %s. Length is %d" % (components,len(components))
          else:
            sg  = ci.entities[components[2]]
            app = sg.app
            ver = components[3]
            appFile = app.version[ver]
            print sg, appFile
            umgr.add(sg)
            upSg = umgr.entities[sg.name]
            upSg.Upgrade(appFile)
            message = "Executed Upgrade of %s to application %s version %s.\nResult: %s" % (sg.name, app.name, ver, upSg.upStatus)
        else:  message = "Request '%s' not understood." % self.path

      except Exception, e:
        message = "Problem: %s" % str(e)
        self.end_headers()
        self.wfile.write(message)
        raise

        #self.send_response(200)
      self.end_headers()
      self.wfile.write(message)
      return  

httpserver = BaseHTTPServer.HTTPServer(("",HTTP_PORT),MyHandler)
httpserver.serve_forever()

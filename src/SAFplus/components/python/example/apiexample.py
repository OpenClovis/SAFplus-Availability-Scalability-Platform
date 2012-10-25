"""@namespace examples.interactive

This example showcases some the the most powerful APIs in the ARD library.
It is intended to not be run as a program, but for you to cut-and-paste
each line into an interactive python shell.  This will allow you to explore
the APIs interactively.

@verbatim
To run:
If ARD web server is running you can use the  browse to /shell in a web browser
# OR at the bash shell prompt:
# export PYTHONPATH=<ardDir>/lib:<ardDir>/bin
# cd <ArdDir>/bin
# <ASP_DIR>/bin/asp_run python

# A simple way to do this is to cd to <ardDir> (this directory) and then do:
#  export PYTHONPATH=`pwd`/lib:`pwd`/bin
#  export PATH=`pwd`/bin:$PATH
#  export LD_LIBRARY_PATH=`pwd`/lib:`pwd`/bin:$LD_LIBRARY_PATH


please see the file apiexample.py for details.

"""

# Catch people who are not reading this...
note = "This example is not meant to be run directly, but rather for you to\ncut-and-paste each line into a python buffer.  It will not work if run directly\nsince it contains some model and directory specific items."
print "\n**********\n", note, "\n**********\n"
raise Exception(note)

# Ok, the example starts now:

## @verbatim

# If within the WEB shell, do NOT cut and paste this LINE (it is already done)
# Paste it in ONLY if using python directly.
import amfpy; amfpy.initializeAmf()


# Standard Python modules
import os
import time

# ARD Python API example code, including deployment and upgrade:
import upgrade; import clusterinfo; import aspApp; import aspAmf

# Hook into the various services
ci = clusterinfo.ci     # The clusterinfo.ci objects reflect the entire state of the cluster
amf = aspAmf.Session()  # The aspAmf.Session class lets you modify the cluster state

# Example of getting the node names
print [x.name for x in ci.nodeList]

# Get the name of the node in slot 1
print ci.nodes[1].name

# Example of getting the sg started/stopped status
print "Running Service Groups:"
for x in clusterinfo.ci.sgList:
  if x.isRunning():
    print x.name

# Example of getting SU status and of traversing the entity hierarchy
print "Active Service Units:"
for x in clusterinfo.ci.suList:
  if x.isActive():
    print "%s is active for service group %s and is running on node %s (slot: %d)" % (x.name,x.sg.name,x.node.name,x.node.slot)

# Example of stopping and restarting an entity
su = ci.suList[0]
print "Stopping %s" % su.name
amf.Shutdown(su)
# Now we must reload the clusterinfo to reflect changes.
ci.load()
su = ci.entities[su.name]  # and reload our temporary.  Its best to access your entities by name in case the list order changes
if su.isRunning():
  print "SU %s is still running" % su.name
else:
  print "SU %s is stopped" % su.name

amf.Startup(su)

ci.load()
su = ci.entities[su.name]  # its best to access your entities by name in case the list order changes
if su.isRunning():
  print "SU %s is running" % su.name
else:
  print "SU %s is still stopped" % su.name

# Initialize the Application bundle DB & pass the bundle repository directory.  Note that you should only create 1 of these per bundle repository directory or you will have 2 entities managing the same data.
appDb = aspApp.AppDb(os.getenv("ASP_DIR","/tmp") + "/apps")

ci.setAppDb(appDb)  # Hook them up (so clusterinfo knows about application versions)

myNode = "ctrlI0"
# Set additional data that ASP may not know into the node
# This info is necessary to deploy software
# (it also can be entered via the GUI, or via etc/clustercfg.xml file)

# You need to change these values to correctly reflect your setup
if ci.entities[myNode].localUser == 'unknown':
  ci.entities[myNode].setIntraclusterAccess("192.168.66.130","root","clovis","/root/ocmsdemo7")

# The above example sets 1 node.  For the application deployment example
# below to work, you must set all of the nodes to the correct values.


# Add 2 bundle files into the system.  Of course you must modify the path to
# point to valid bundle files.
appDb.NewAppFile("/code/vipapp1.4.0.0.tgz")
appDb.NewAppFile("/code/vipapp1.4.0.1.tgz")

# Note that if you want AppDb to automatically find them again, 
# you must copy them into the repository (the directory you specified
# when creating "appDb").


# Show all bundles and versions
print [(x.name,x.version) for x in appDb.AppList()]

# Find all "live" nodes
# Of course, you can't deploy to nodes that aren't powered up!
liveNodes = filter(lambda x: x.isRunning(),ci.nodeList)
print [x.name for x in liveNodes]

# Get an application
appFile = appDb.apps['virtualIp'].version['1.4.0.0']
print "Name: ", appFile.name, "Located At: ", appFile.dir, "Version: ", appFile.version  
# print appFile.cfg

# Deploy the application:

# Tweak the configuration for just this deployment
# If I didn't copy the configuration, I would be modifying it in RAM
# which would affect other deployments I do within this Python session.
import copy
newCfg = copy.deepcopy(appFile.cfg)
# For example, set failback parameter to True:
newCfg.virtualIp.modifiers.failBack = True

# Really Deploy
import appdeploy
(sgList,(errors,notes)) = appdeploy.deploy(appFile.dir,newCfg,liveNodes,abortIfCantAccess=True,copy=True,deploy=True)
# Did you get a pexpect exception?  You probably forgot to specify the node 
# intracluster access data as shown above.  Or you may have specified
# incorrect information.

myNewSg  = sgList[0]
mySgName = myNewSg.name 
print "Deployed application and created SG %s" % mySgName

# Reload the AMF information model
ci.load()

# Reaccess via ci global to get refreshed data.
mySg = ci.entities[mySgName]

# Let's start it up
amf.Startup(mySg)

# Give it time to come up
# I reaccess through ci each time to get refreshed data.
while not ci.entities[mySgName].isRunning():
  time.sleep(5)
  ci.refresh()

print "It is running"

# Let's do an upgrade!

# Start the upgrade manager...
umgr = upgrade.UpgradeMgr()

umgr.add(mySg)  # Add my Service Group to it.

upSg = umgr.entities[mySgName]  # Get my sg's upgrade entity 
upSg.upMethod = upgrade.RollingUpgrade

print mySg.appVer.version
# Start the upgrade...
upSg.Upgrade(appDb.apps['virtualIp'].version['1.4.0.1'])

ci.load() # Reload the database.
mySg = ci.entities[mySg.name]  # Get the new SG object
print mySg.appVer.version

## @endverbatim

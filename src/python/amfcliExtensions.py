import pdb
import types, os, sys, time,types, signal, random, datetime, errno
import xml.etree.ElementTree as ET
import amfctrl

def AmfCreateApp(prefix, compList, nodes):
  sg = "%sSg" % prefix
  numActiveSus = (len(nodes)+1)/2
  numStandbySus = len(nodes) -  numActiveSus
  
  entities = { 
    # "Component/dynComp0" : { "maxActiveAssignments":4, "maxStandbyAssignments":2, "serviceUnit": "dynSu0", "instantiate": { "command" : "../test/exampleSafApp dynComp", "timeout":60*1000 }},   
    "ServiceGroup/%s" % sg : { "adminState":"off", "preferredNumActiveServiceUnits":numActiveSus,"preferredNumStandbyServiceUnits":numStandbySus, "serviceInstances" :  "safplusAmf/ServiceInstance/%sSi" % prefix },
    "ServiceInstance/%sSi" % prefix : { "serviceGroup": sg, "componentServiceInstances" :"safplusAmf/ComponentServiceInstance/%sCsi" % prefix },
    "ComponentServiceInstance/%sCsi" % prefix : { "serviceInstance": "safplusAmf/ServiceInstance/%sSi" % prefix, "name/testKey" : "testVal" }
  }

  count = 0
  sus = []
  for n in nodes:
    suname = "%sSu%d" % (prefix,count)
    comps = []
    for c in compList:
      name = os.path.basename(c.split(" ")[0])
      cname = "%s_in_%s" % (name,suname)
      idx=1
      while cname in comps:
        cname = "%s_in_%s_%d" % (name,suname,idx)
        idx+=1

      entities["Component/%s" % cname] = { "maxActiveAssignments":1, "maxStandbyAssignments":1, "serviceUnit": suname, "instantiate": { "command" : c, "timeout":15*1000 }}
      comps.append(cname)   

    entities["ServiceUnit/%s" % suname] = { "components":",".join(comps),"node":n, "serviceGroup": sg }
    sus.append(suname)
    count += 1

  entities["ServiceGroup/%s" % sg]["serviceUnits"] = ",".join(sus)
  amfctrl.commit(entities)

# This section of the example defines how commands can be added to the CLI

class ExampleCommands:
  # Boilerplate code:  Initalization 
  def __init__(self):
    self.context = None
    self.commands = { "show" : (self.show, None),
                    "create" : { "app" : (self.createApp, None) },
                    "delete" : { "app" : (self.deleteApp, None) },
                    "start" : { "app" : (self.startApp, None) },
                    "stop" : { "app" : (self.stopApp, None) }
    }

  # Boilerplate code:  The CLI calls your command class back with the CLI context.  But you won't need to use this context variable unless you are doing complex stuff...
  def setContext(self,context):
    self.context = context

  def startApp(self, appPrefix):
    sgName = "/safplusAmf/ServiceGroup/%sSg" % appPrefix
    # TDO cli.run("set %s/operState 1" % sgName)  # reenable the comps if faulted
    suList = cli.getList(sgName + "/serviceUnits")
    for su in suList:
      cli.set("%s/adminState" % su, "on")
    cli.run("set %s/adminState on" % sgName)  # turn app on
    return "%s started" % appPrefix

  def stopApp(self, appPrefix):
    sgName = "/safplusAmf/ServiceGroup/%sSg" % appPrefix
    cli.run("set %s/adminState off" % sgName)
    return "%s stopped" % appPrefix

  def deleteApp(self, appName):
    sg = None
    sus = []
    comps = []
    sgName = None
    suNames = []
    compNames = []
    siNames = []
    csiNames = []
    try:
      sgName = "/safplusAmf/ServiceGroup/%s" % appName
      sg = ET.fromstring(cli.run("ls " + sgName))
      if len(sg) == 0: 
        sgName = sgName + "Sg"
        sg = ET.fromstring(cli.run("ls " + sgName))       
      if len(sg) == 0: 
        return None
      else: sg=sg[0]
    except cli.Error, e:
      pass

    if sg:
      siNameText = sg.find("serviceInstances").text
      siNames = [ str(x) for x in siNameText.split(",")]
      suNameText = sg.find("serviceUnits").text
      suNames = [ str(x) for x in suNameText.split(",")]
      print suNames
      for suName in suNames:
        if suName[0] != "?":
          su = ET.fromstring(cli.run("ls %s" % suName))
          if len(su) != 0:
            su=su[0]
            sus.append(su)
            compN = su.find("components").text
            compN = compN.split(",")
            for compName in compN:
              if compName[0] != "?":
                comp = ET.fromstring(cli.run("ls %s" % compName))
                if len(comp) != 0:  # if it is valid then add it to the delete list
                  compNames.append(str(compName))
      if siNames:
       for siName in siNames:
        if siName[0] != "?":
          si = ET.fromstring(cli.run("ls %s" % siName))
          if len(si) != 0:
            si=si[0] # remove the "top"
            csiNameStr = si.find("componentServiceInstances").text
            tmp = [ str(x) for x in csiNameStr.split(",")]
            csiNames += tmp

    if sgName:
      cli.run("delete " + str(sgName))
      allNames = [sgName]
      if suNames: 
        for su in suNames: cli.run("delete " + str(su))
        allNames += suNames
      if compNames: 
        for compName in compNames: cli.run("delete " + str(compName))
        allNames += compNames
      if siNames: 
        for si in siNames: cli.run("delete " + str(si))
        allNames += siNames
      if csiNames: 
        for csi in csiNames: cli.run("delete " + str(csi))
        allNames += csiNames

      return "Deleted %s" % ", ".join(allNames)
    return "No such application"
   

  def createApp(self, appName, appCommandLine, *nodes):
    """Create an application running on the specified nodes
    Argument 1: application name prefix -- an arbitrary name given to all management elements created for this application
    Argument 2: application command line.  If spaces are needed, enclose in quotes.
    Argument 3: list of node names on which the application should be run
    """
    errors = []
    for n in nodes:
      nodeData = ET.fromstring(cli.run("ls /safplusAmf/Node/%s" % n))
      if len(nodeData) == 0:
        errors.append("Node %s does not exist." % n)
    if errors:
      # cli.display("""<text size="24" fore="#108010">""" + "\n".join(errors) + "</text>")
      return "\n".join(errors)

    AmfCreateApp(appName, [appCommandLine], nodes)
    # sgName = appName + "Sg"
    # cli.run("create /safplusAmf/ServiceGroup/%s" % sgName)
    #cli.run("set /safplusAmf/ServiceGroup/%s/")

    return "Ok"

  # All commands are defined as do_<command>.  So this function defines the "basicStatus" command.
  def show(self,element):
    """Display the status of the Basic application    
    """
    # This demonstrates an API called "get" to access a single element.  This API is a wrapper around
    # run("ls ...") which subsequently parses the XML and grabs the text of the first element in the response.
    # It will throw an exception of the element cannot be accessed
    sg = None
    sus = []
    comps = []
    try:
      sg = ET.fromstring(cli.run("ls /safplusAmf/ServiceGroup/%s" % element))
      if len(sg) == 0: 
        sg = ET.fromstring(cli.run("ls /safplusAmf/ServiceGroup/%sSg" % element))
      if len(sg) == 0: 
        return None
      else: sg=sg[0]
    except cli.Error, e:
      pass

    if sg:
      suNames = sg.find("serviceUnits").text
      suNames = suNames.split(",")
      print suNames
      for suName in suNames:
        if suName[0] != "?":
          su = ET.fromstring(cli.run("ls %s" % suName))
          if len(su) != 0:
            su=su[0]
            sus.append(su)
            compNames = su.find("components").text
            compNames = compNames.split(",")
            for compName in compNames:
              if compName[0] != "?":
                comp = ET.fromstring(cli.run("ls %s" % compName))
                if len(comp) != 0:
                  comps.append(comp)
              else:
                comps.append("<error>Unresolved component: %s</error>" % compName[1:])
        else:
          sus.append("<error>Unresolved service unit: %s</error>" % suName[1:])
    else:
      cli.display("""<text size="24" fore="#108010">Invalid entity name "%s"</text>""" % element)
      return None
          
    cli.display(ET.tostring(sg))
    for su in sus:
      if type(su) in types.StringTypes:
        cli.display(su)
      else:
        cli.display(ET.tostring(su))
    for comp in comps:
      if type(comp) in types.StringTypes:
        cli.display(comp)
      else:
        cli.display(ET.tostring(comp))

    cli.display("""<text size="24" fore="#108010"></text>""")
    return "" # If you return None, the CLI will look for another matching function


# Now create an instance of your class and pass it to the CLI
cli.add(ExampleCommands())

# OK we are finished here, output a message to to CLI console saying so.
cli.display("<text>AMF extensions loaded</text>")

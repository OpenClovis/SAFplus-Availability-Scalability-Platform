import pdb
import xml.etree.ElementTree as ET
print "Script arguments are: %s" % ", ".join(argv)

# Define a function
def ShowBroken(entityType):
  # Access data from CLI commands.  This data is returned as XML, so put it into the elementtree XML parser
  availList = ET.fromstring(cli.run("ls /safplusAmf/%s/*/adminState" % entityType))

  # Iterate through the children in the XML
  for n in availList:
    if n.text == "on":
      path = n.attrib["path"].split("/") # All first children contain the full path to this node
      eName = path[-2]  # Path is somehthing like: /safplusAmf/Node/<nodeName>/adminState, so this grabs the entity name 
      ePath = path[0:-1]  # Grabs everything but "adminState"
      presenceState = ET.fromstring(cli.run("ls %s/presenceState" % ("/".join(ePath))))[0].text  # So I can now get the presence state of the same entity
      if presenceState != "instantiated":   # If admin state is on but the entity is OFF then warn the user
        cli.display("<text>Entity %s should be on but is %s</text>" % (eName, presenceState))

# Argument parsing: if no args then look at nodes and SUs, otherwise display what the user asks for
if len(argv) == 0:
  ShowBroken("Node")
  ShowBroken("ServiceUnit")
else:
  ShowBroken(argv[0])


# This section of the example defines how commands can be added to the CLI

class ExampleCommands:
  # Boilerplate code:  Initalization 
  def __init__(self):
    self.context = None
  # Boilerplate code:  The CLI calls your command class back with the CLI context.  But you won't need to use this context variable unless you are doing complex stuff...
  def setContext(self,context):
    self.context = context

  # All commands are defined as do_<command>.  So this function defines the "basicStatus" command.
  def do_basicStatus(self,*flags):
    """Display the status of the Basic application    
    """
    # This demonstrates an API called "get" to access a single element.  This API is a wrapper around
    # run("ls ...") which subsequently parses the XML and grabs the text of the first element in the response.
    # It will throw an exception of the element cannot be accessed
    try:
      c0 = cli.get("/safplusAmf/Component/c0/presenceState")
    except cli.Error, e:
      c0 = str(e)

    # Or if you give it a second parameter, then that parameter will be returned instead of throwing an exception
    c1 = cli.get("/safplusAmf/Component/c1/presenceState","not defined in the model")

    cli.display("""<text size="24" fore="#108010">The two Basic components' status are "%s" and "%s"</text>""" % (c0,c1))
    return "" # If you return None, the CLI will look for another matching function

  # Add a quick command to start the Basic service group
  def do_basicStart(self):
    cli.display("<text>Starting the basic application</text>")
    ret = cli.run("set /safplusAmf/ServiceGroup/sg0/adminState on")
    return ret # If you return None, the CLI will look for another matching function
  # And one to stop it
  def do_basicStop(self):
    cli.display("<text>Stopping the basic application</text>")
    ret = cli.run("set /safplusAmf/ServiceGroup/sg0/adminState off")
    return ret


# Now create an instance of your class and pass it to the CLI
cli.add(ExampleCommands())

# OK we are finished here, output a message to to CLI console saying so.
cli.display("<text>cliexample.py script completed</text>")

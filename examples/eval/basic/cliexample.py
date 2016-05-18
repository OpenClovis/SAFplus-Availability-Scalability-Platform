import pdb
import xml.etree.ElementTree as ET
print "Script arguments are: %s" % ", ".join(argv)

# Define a function
def ShowBroken(entityType):

  # Access data from CLI commands.  This data is returned as XML, so put it into the elementtree XML parser
  availList = ET.fromstring(cli("ls /safplusAmf/%s/*/adminState" % entityType))

  # Iterate through the children in the XML
  for n in availList:
    if n.text == "on":
      path = n.attrib["path"].split("/") # All first children contain the full path to this node
      eName = path[-2]  # Path is somehthing like: /safplusAmf/Node/<nodeName>/adminState, so this grabs the entity name 
      ePath = path[0:-1]  # Grabs everything but "adminState"
      presenceState = ET.fromstring(cli("ls %s/presenceState" % ("/".join(ePath))))[0].text  # So I can now get the presence state of the same entity
      if presenceState != "instantiated":   # If admin state is on but the entity is OFF then warn the user
        display("<text>Entity %s should be on but is %s</text>" % (eName, presenceState))

# Argument parsing: if no args then look at nodes and SUs, otherwise display what the user asks for
if len(argv) == 0:
  ShowBroken("Node")
  ShowBroken("ServiceUnit")
else:
  ShowBroken(argv[0])



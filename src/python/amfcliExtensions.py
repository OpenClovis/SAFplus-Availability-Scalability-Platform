import pdb
import types
import xml.etree.ElementTree as ET


# This section of the example defines how commands can be added to the CLI

class ExampleCommands:
  # Boilerplate code:  Initalization 
  def __init__(self):
    self.context = None
  # Boilerplate code:  The CLI calls your command class back with the CLI context.  But you won't need to use this context variable unless you are doing complex stuff...
  def setContext(self,context):
    self.context = context

  # All commands are defined as do_<command>.  So this function defines the "basicStatus" command.
  def do_show(self,element):
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
      if len(sg) == 0: sg = None  # top elem has no children
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

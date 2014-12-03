import common
import svg

class EntityType:
  """This defines the concept, for example 'Service Group'"""
  def __init__(self,name, data):
    self.name = name
    self.data = data
    self.iconFile = data.get("icon", "generic.svg")

    f = open(common.fileResolver(self.iconFile),"r")
    self.iconSvg =  svg.Svg(f.read(),(256,128))
    f.close()

    self.buttonFile = data.get("button", "generic_button.svg")
    f = open(common.fileResolver(self.iconFile),"r")
    self.buttonSvg =  svg.Svg(f.read(),(32,32))
    f.close()

  
class Entity:
  """This is a UML object of a particular entity type, for example the 'Apache' service group"""
  def __init__(self, entityType):
    self.et = entityType

class Instance:
  """This is an actual instance of an Entity, for example the 'Apache' service group running on node 'ctrl0'"""
  def __init__(self, entity):
    self.entity = entity


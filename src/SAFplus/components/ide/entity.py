import pdb
from types import *
import common
import svg

nameIdx = {
}

def NameCreator(typ):
  """Create a unique name by using the type and an instance number"""
  idx = nameIdx.get(typ, 1)
  nameIdx[typ] = idx+1
  return typ + str(idx)

class EntityType:
  """This defines the concept, for example 'Service Group'"""
  def __init__(self,name, data, context=None):
    self.name = name
    self.data = data
    self.context = context  # I may need to know the larger context (the model) to resolve types
    self.iconFile = data.get("icon", "generic.svg")

    f = open(common.fileResolver(self.iconFile),"r")
    self.iconSvg =  svg.Svg(f.read(),(256,128))
    f.close()

    self.buttonFile = data.get("button", "generic_button.svg")
    f = open(common.fileResolver(self.buttonFile),"r")
    self.buttonSvg =  svg.Svg(f.read(),(32,32))
    f.close()

  def CreateEntity(self,pos, size=None):
    """Create an entity of this type"""
    if not size:
      size = self.iconSvg.size
    ret = Entity(self, pos,size)
    return ret


class Entity:
  """This is a UML object of a particular entity type, for example the 'Apache' service group"""
  def __init__(self, entityType, pos, size):
    self.et = entityType
    assert(type(pos) == TupleType);
    self.pos  = pos
    self.size = size
    self.scale = (1.0,1.0)
    self.rotate = 0.0
    self.data = {}
    self.updateDataFields()
    self.data["entityType"] = self.et.name
    self.data["name"] = NameCreator(entityType.name)
    self.bmp  = self.et.iconSvg.instantiate(self.size,self.data)

  def updateDataFields(self):
    """Creates/modifies the data fields in this entity based on the entityType"""
    new = {}
    for (name,metadata) in self.et.data.items():  # Dig thru everything in the entity type, moving it all into the entity
      if type(metadata) == DictType: # If its not a dictionary type it cannot be changed; don't copy over
        if self.data.has_key(name):  # If its assigned pull it over
          new[name] = self.data[name]
        else:  # Create it new
          if metadata.has_key("default"):
            new[name] = metadata["default"]
          elif metadata.has_key("type"):
            new[name] = self.et.context.defaultForType(metadata["type"])
          else:
            new[name] = ""
    self.data = new  

class Instance:
  """This is an actual instance of an Entity, for example the 'Apache' service group running on node 'ctrl0'"""
  def __init__(self, entity, data):
    self.entity = entity
    self.data = data #micro dom data
    #binding entity and data, this make straight render in gui
    #{entity.a : value1, entity.b: value 2}

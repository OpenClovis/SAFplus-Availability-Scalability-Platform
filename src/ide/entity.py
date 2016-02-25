import pdb
import microdom
from types import *
import common
import svg
import copy

nameIdx = {
}

def NameCreator(typ):
  """Create a unique name by using the type and an instance number"""
  idx = nameIdx.get(typ, 1)
  nameIdx[typ] = idx+1
  return typ + str(idx)

def EntityTypeSortOrder(a,b):
  if not type(a[1]) is DictType:
    return 1
  if not type(b[1]) is DictType:
    return -1
  return cmp(a[1].get("order",10000),b[1].get("order",10001))
  

class EntityType:
  """This defines the concept, for example 'Service Group'"""
  def __init__(self,name, data, context=None):
    self.name = name
    self.data = data
    self.context = context  # I may need to know the larger context (the model) to resolve types
    self.iconFile = data.get("icon", "generic.svg")
    self.customInstantiator = None
    f = open(common.fileResolver(self.iconFile),"r")
    self.iconSvg =  svg.Svg(f.read(),(256,128))
    f.close()

    self.buttonFile = data.get("button", "generic_button.svg")
    f = open(common.fileResolver(self.buttonFile),"r")
    self.buttonSvg =  svg.Svg(f.read(),(32,32))
    f.close()

  def createEntity(self,pos, size=None,children=False):
    """Create an entity of this type, located a pos of the specified size.  If children is true, create the entire logical group """
    if not size:
      size = self.iconSvg.size
    ret = Entity(self, pos,size)
    return ret
 

class ContainmentArrow:
  def __init__(self, container, offset, contained, end_offset, midpoints=None):
    """Creates a containment arrow.  Pass the "container" Entity object and the pixel tuple "offset" from the container's position.  And pass the "contained" Entity object and the pixel tuple "offset" from the contained's position.  The midpoints parameter allows the arrow to be curved using sequential first order bezier curves
    """
    self.container = container
    self.beginOffset = offset
    self.contained = contained
    self.endOffset = end_offset
    self.midpoints = midpoints

class Entity:
  """This is a UML object of a particular entity type, for example the 'Apache' service group"""
  def __init__(self, entityType, pos, size,name=None,data=None):
    self.et = entityType
    assert(type(pos) == TupleType);
    self.pos  = pos
    self.size = size
    self.scale = (1.0,1.0)
    self.rotate = 0.0
    self.data = {}
    self.updateDataFields(dataDict=data)
    self.customInstantiator = entityType.customInstantiator
    self.instanceLocked = {}  # Whether this data data fields can be changed by an instance
    self.data["entityType"] = self.et.name
    self.data["name"] = NameCreator(entityType.name) if name is None else name
    self.bmp  = self.et.iconSvg.instantiate(self.size,self.data)
    self.containmentArrows = []

    # Put this entity name into namely dict
    if name:
      NameCreator(entityType.name)

  def duplicate(self,name=None, dupContainmentArrows=False):
    newEnt = copy.copy(self)
    newEnt.data = copy.deepcopy(self.data)
    newEnt.data["name"] = NameCreator(self.et.name) if name is None else name
    newEnt.bmp  = self.et.iconSvg.instantiate(self.size,self.data)
    newEnt.pos = (newEnt.pos[0] + 20, newEnt.pos[1] + 20)
    newEnt.containmentArrows = []
    if dupContainmentArrows:
      for ca in self.containmentArrows:
        ca.contained.childOf.add(newEnt)
        cai = copy.copy(ca)
        cai.container = newEnt
        newEnt.containmentArrows.append(cai)
    return newEnt

  def recreateBitmap(self):
    self.bmp  = self.et.iconSvg.instantiate(self.size,self.data)

  def canContain(self, entity):
    """Return true if this entity can contain the passed entity"""
    # TODO: check the entitytype relationship
    # TODO: make sure that the ordinality is correct (i.e. if self can only contain one entity, make sure that currently self is containing no of them)
    return True

  def canBeContained(self, entity):
    """Return true if this entity can be contained by the passed entity"""
    # TODO: check the entitytype relationship
    # TODO: make sure that the ordinality is correct (i.e. if self can only be contained by one entity, make sure that currently self is contained by no entity)
    return True

  def createContainmentArrowTo(self,entity):
    """Creates a containment arrow to the supplied entity.  Essentially, this creates a containment relationship because the arrow defines the relationship"""
    self.containmentArrows.append(ContainmentArrow(self,(0,0),entity,(0,0)))

  def deleteContainmentArrowTo(self,entity):
    """Removes the containment arrow to the supplied entity.  Essentially, removes the containment relationship because the arrow defines the relationship"""
    for i in self.containmentArrows:
      if i.contained == entity:
        self.containmentArrows.remove(i)
        break

  def isContainer(self, typeDicts):
    for (name,metadata) in typeDicts.items():
      if type(metadata) == DictType:
        return True
    return False
  
  def updateDataFields(self, dataDict = None, typeDict = None, data = None):
    """Creates/modifies the data fields in this entity based on the entityType"""
    if data == None:
      data = self.data
    if typeDict == None:
      typeDict = self.et.data

    #new = self.data  # Keep what is already there
    for (name,metadata) in typeDict.items():  # Dig thru everything in the entity type, moving it all into the entity
      if type(metadata) == DictType: # If its not a dictionary type it cannot be changed; don't copy over
        if metadata.get('config', True) == False: continue  # Do not copy any runtime fields over; they are not interesting to us.        
        if dataDict and dataDict.has_key(name):
          val = dataDict[name]
          if microdom.microdomFilter(val):
            if metadata.has_key('type'):
              try:
                data[name] = val.data_.strip()
              except AttributeError, e:  # Could be a number...
                data[name] = val.data_
            else:
              # Build child container data
              typeChildsDict = typeDict[name]
              if self.isContainer(typeChildsDict):
                data[name] = {}
                self.updateDataFields(val, typeChildsDict, data[name])
          else:
            data[name] = val
        elif data.has_key(name):  # If its assigned pull it over
          pass
        else:  # Create it new
          if metadata.has_key("default"):
            data[name] = metadata["default"]
          elif metadata.has_key("type"):
            data[name] = self.et.context.defaultForType(metadata["type"])
          else:
            # Build child container default data
            typeChildsDict = typeDict[name]
            if self.isContainer(typeChildsDict):
              data[name] = {}
              self.updateDataFields({}, typeChildsDict, data[name])
            else:
              data[name] = ""
    #self.data = new
  
  def createInstance(self,pos, size=None,children=False, name=None, parent=None):
    """Create an entity of this type"""
    if not size:
      size = self.size    
    if parent and self.et.name == "ServiceGroup":      
      ret = parent.sgInstantiator(self,pos,size,children,name)
    else: 
      newdata = copy.deepcopy(self.data)
      ret = Instance(self,newdata,pos,size,name=name)
    return ret
 

class Instance(Entity):
  """This is an actual instance of an Entity, for example the 'Apache' service group running on node 'ctrl0'
  It has every field of the Entity so is derived from the Entity class.  However the instance's parent is self.et, not the parent class
  """
  def __init__(self, entity, data,pos, size,name=None):
    if data is None: data = copy.deepcopy(entity.data)
    if pos is None: pos = entity.pos
    if size is None: size = entity.size
    Entity.__init__(self,entity.et, pos, size,name=NameCreator(entity.data["name"]) if name is None else name,data=data)
    self.entity = entity  # This could be a bit confusing WRT Entity, because the type of the instance is the entity, the type of the entity is the entityType
    # Now, self.entity.et => yang define type
    # self.entity => entity type of instance
    # tag entity type => <NodeType> <ServiceGroupType> ... entityType + 'Type'
    # data = merge entity Type's data and this entity data

    #binding entity and data, this make straight render in gui
    #{entity.a : value1, entity.b: value 2}
    self.bmp  = self.entity.et.iconSvg.instantiate(self.size,self.data)
    self.relativePos = (5,5)  # For layout this is set if the position should be relative to the parent's position
    self.childOf = set()

    # Put this entity name into namely dict
    if name:
      NameCreator(entity.data["name"])

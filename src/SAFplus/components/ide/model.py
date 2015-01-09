import pdb
import random
import xml.dom.minidom
import microdom
import types
import common
from module import Module
import svg
import entity

VERSION = "7.0"

defaultForBuiltinType = {
  "boolean": False,
  "integer": 0,
}



class Model:
  """Rather then type/instance, there are really 2 levels instantiations, more along the lines of C++ template,type,instance.  What I mean by this is that the object defined in SAFplusAmf.yang is really a "meta-type".  Take the example of a Service Group.  You first "instantiate" this in the UML editor to create the "Apache" (for example) Service Group.  Next you "instantiate" the "Apache Service Group" to create a particular instance of Apache running on 2 nodes.

The user can modify the configuration after every instantiation, but also has the option to "lock" particular configuration so downstream instantiation cannot modify it.

For example, the user instantiates the Apache Service Group (for example), selects 1+1 redundancy and then "locks" it.  The user also selects 3 restarts before failover but does NOT lock that.  Now, when the Apache Service Group is instantiated on say node1, the user CANNOT change the redundancy model, but he can change the # of restarts (for this particular instance). 

SAFplus6      SAFPlus7        SAFplus7 model.py code            What I'm talking about
hardcoded     .yang           entityTypes                       Meta-types  (e.g. Service Group)
config        <entities>      entities                          entities        (e.g. Apache web browser)
instantiated  <instances>     instances                         instances     (e.g. Apache running on 2 particular nodes)
  """
  def __init__(self, modelfile=None):
    self.init()
    if modelfile:
      self.load(modelfile)

  def init(self):
    """Clear this model, forgetting everything"""
    self.data = {} # empty model
    self.filename = None
    self.modules = {}
    self.dataTypes= {}
    self.entityTypes = {}
    self.entities = {}
    self.instances = {}

  def delete(self, items):
    """Accept a list of items in a variety of formats to be deleted"""
    if type(items) is types.ListType or isinstance(items,set):
      for item in items:
        self.delete(item)
    if type(items) is types.DictType:
      for item in items.items():
        self.delete(item)

    if type(items) in types.StringTypes:
      #if self.entityTypes.get(item):
      #  self.deleteEntity(self.entities[item])
      if self.entities.get(items):
        self.deleteEntity(self.entities[item])
      if self.instances.get(items):
        self.deleteInstance(self.entities[item])

    if isinstance(items,entity.Entity): self.deleteEntity(items)
    elif isinstance(items,entity.Instance): self.deleteInstance(items)

  
  def deleteEntity(self,entity):
    """Delete this instance of Entity from the model"""
    entname = entity.data["name"]
    for (name,e) in self.entities.items():
      e.containmentArrows[:] = [ x for x in e.containmentArrows if x.contained != entity]
    del self.entities[entname]

    # Also delete the entity from the microdom
    entities = self.data.getElementsByTagName("entities")
    if entities:
      entities[0].delChild(entities[0].findOneByChild("name",entname))


  def deleteInstance(self,inst):
    assert(0)  # Not implemented

  def load(self, fileOrString):
    """Load an XML representation of the model"""
    if fileOrString[0] != "<":  # XML must begin with opener
      self.filename = common.fileResolver(fileOrString)
      f = open(self.filename,"r")
      fileOrString = f.read()
      f.close()
    dom = xml.dom.minidom.parseString(fileOrString)
    self.data = microdom.LoadMiniDom(dom.childNodes[0])

    self.loadModules()

    # Populate the helper variables from the microdom
    entities = self.data.getElementsByTagName("entities")
    if entities:
      assert(len(entities)==1)
      entities = entities[0]
      fileEntLst = []
      for ed in entities.children(microdom.microdomFilter):
        name = ed["name"].data_
        entType = self.entityTypes[ed.tag_]
        pos = None
        size = None
        # TODO load the pos and size from the model (if it exists)
        if pos is None:
          pos = self.makeUpAScreenPosition()
          size = entType.iconSvg.size
        eo = entity.Entity(entType,pos,size,name)
        eo.updateDataFields(ed)
        self.entities[name] = eo
        fileEntLst.append((ed,eo))

      # Look for relationships.  I can't do this until all the entities are created
      for (ed,eo) in fileEntLst:        
        for et in self.entityTypes.items():   # Look through all the children for a key that corresponds to the name of an entityType (+ s), eg: "ServiceGroups"
          if ed.child_.has_key(et[0] + 's'):
            linkstr = ed.child_[et[0] + 's'].data_
            linklst = linkstr.split(",")
            for link in linklst:
              contained = self.entities.get(link,None)
              if contained:
                # TODO: look the positions up in the GUI section of the xml file
                ca = entity.ContainmentArrow(eo,(0,0),contained,(0,0),[])
                eo.containmentArrows.append(ca)
              else:  # target of the link is missing, so drop the link as well.  This could happen if the user manually edits the XML
                # TODO: create some kind of warning/audit log in share.py that we can post messages to.
                pass
      # Recreate all the images in case loading data would have changed them.
      for (ed,eo) in fileEntLst: 
        eo.recreateBitmap()       

      # Get instance lock fields
      ide = self.data.getElementsByTagName("ide")
      if ide:
        for (name,e) in self.entities.items():
          etType = ide[0].getElementsByTagName(e.et.name)
          if etType:
            et = etType[0].getElementsByTagName(name)
            if et:
              for ed in et[0].children(microdom.microdomFilter):
                e.instanceLocked[str(ed.tag_)] = ed.data_

    # Load instances
    for (path, obj) in self.data.find("instances"):
      fileEntLst = []
      for entityType in self.entityTypes.keys():
        for instance in obj.children(lambda(x): x if (type(x) is types.InstanceType and x.__class__ is microdom.MicroDom and x.tag_ == entityType) else None):
          if instance.child_.has_key("entityType"):
            data = instance.children_
            ent = self.entities.get(instance.entityType.data_)
            entityInstance = entity.Instance(ent, data, ent.pos, ent.size, instance.name.data_)

            entityInstance.updateDataFields(instance)

            # Copy instance locked, then bind to readonly wxwidget
            entityInstance.instanceLocked = ent.instanceLocked.copy()
            self.instances[instance.name.data_] = entityInstance
            
            fileEntLst.append((instance,entityInstance))

      for (ed,eo) in fileEntLst:
        for et in self.entityTypes.items():   # Look through all the children for a key that corresponds to the name of an entityType (+ s), eg: "ServiceGroups"
          child = et[0][0].lower() + et[0][1:] + 's'
          for ch in ed.children(lambda(x): x if (type(x) is types.InstanceType and x.__class__ is microdom.MicroDom and x.tag_ == child) else None):
            # Strip out instance-identifier if any
            childName = str(ch.data_).replace("/%s" %et[0],"")
            ent2 = self.instances[childName[1:]]
            ca = entity.ContainmentArrow(eo,(0,0),ent2,(0,0),[])
            eo.containmentArrows.append(ca)


  def makeUpAScreenPosition(self):
    return (random.randint(0,800),random.randint(0,800))

  def save(self, filename=None):
    """Save XML representation of the model"""
    if filename is None: filename = "test.xml" # self.filename
    f = open(filename,"w")
    f.write(self.xmlify())
    f.close()
    

  def loadModules(self):
    """Load the modules specified in the model"""
    for (path, obj) in self.data.find("modules"):
      for module in obj.children(microdom.microdomFilter):   
        filename = module.data_.strip()
        print module.tag_, ": ", filename
        if not self.modules.has_key(filename):  # really load it since it does not exist
          tmp = self.modules[filename] = Module(filename)
          self.entityTypes.update(tmp.entityTypes)  # make the entity types easily accdef xmlify(self):
          for (typName,data) in tmp.ytypes.items():
            self.dataTypes[typName] = data

    # Set the entityType's context to this model so it can resolve referenced types, etc.
    for (name,e) in self.entityTypes.items():
      e.context = self

  def defaultForType(self,typ):
    """Figure out a reasonable default for the passed type"""
    ret = defaultForBuiltinType.get(typ,None)  # Is the type a builtin?
    if ret: return ret
    # TODO: look in the model's type list for this type and figure out a default
    return ""

  def updateMicrodom(self):
    """Write the dynamically changing information back to the loaded microdom tree.
       The reason I don't create an entirely new tree is to preserve any application extensions that might have been put into the file.
    """
    # Locate or create the needed sections in the XML file

    #   find or create the entity area in the microdom
    entities = self.data.getElementsByTagName("entities")
    if not entities:
      entities = microdom.MicroDom({"tag_":"entities"},[],[])
      self.data.addChild(entities)
    else: 
      assert(len(entities)==1)
      entities = entities[0]
    
    #   find or create the GUI area in the microdom
    ide = self.data.getElementsByTagName("ide")
    if not ide:
      ide = microdom.MicroDom({"tag_":"ide"},[],[])
      self.data.addChild(ide)
    else: 
      assert(len(ide)==1)
      ide = ide[0]
    

    # Write out the entities


    #   iterate through all entities writing them to the microdom, or changing the existing microdom
    for (name,e) in self.entities.items():
      entity = entities.findOneByChild("name",name)
      if not entity:
        entity = microdom.MicroDom({"tag_":e.et.name},[],[])
        entities.addChild(entity)
      # Write all the data fields into the model's microdom
      entity.update(e.data)  
      # Now write all the arrows
      contains = {} # Create a dictionary to hold all linkages by type
      for arrow in e.containmentArrows:
        # Add the contained object to the dictionary keyed off of the object's entitytype
        tmp = contains.get(arrow.contained.et.name,[])
        tmp.append(arrow.contained.data["name"])
        contains[arrow.contained.et.name] = tmp
      # Now erase the missing linkages from the microdom
      for (key, val) in self.entityTypes.items():   # Look through all the children for a key that corresponds to the name of an entityType (+ s), eg: "ServiceGroups"
          if not contains.has_key(key): # Element is an entity type but no linkages
            if entity.child_.has_key(key + 's'): entity.delChild(key + 's')
      # Ok now write the linkages to the microdom
      for (key, val) in contains.items():
        k = key + "s"
        if entity.child_.has_key(k): entity.delChild(k)
        entity.addChild(microdom.MicroDom({"tag_":k},[",".join(val)],""))  # TODO: do we really need to pluralize?  Also validate comma separation is ok
      # TODO: write the IDE specific information to the IDE area of the model xml

      # Building instance lock fields
      etType = ide.getElementsByTagName(e.et.name)
      if not etType:
        etType = microdom.MicroDom({"tag_":e.et.name},[],[])
        ide.addChild(etType)
      else: 
        assert(len(etType)==1)
        etType = etType[0]
      et = etType.getElementsByTagName(name)
      if not et:
        et = microdom.MicroDom({"tag_":name},[],[])
        etType.addChild(et)
      else: 
        assert(len(et)==1)
        et = et[0]

      et.update(e.instanceLocked)

  def xmlify(self):
    """Returns an XML string that defines the IDE Model, for saving to disk"""
    self.updateMicrodom()
    return self.data.pretty()

def UnitTest(m=None):
  """This unit test relies on a particular model configuration, located in testModel.xml"""
  from entity import *
  if not m:
    m = Model()
    m.load("testModel.xml")
  
  appt = m.entityTypes["Application"]
  app = m.entities["app"] = Entity(appt,(0,0),(100,20))
  sgt = m.entityTypes["ServiceGroup"]
  sg = m.entities["sg"] = Entity(sgt,(0,0),(100,20))
  
  if not app.canContain(sg):
    raise "Test failed"
  if sg.canContain(app):
    raise "Test failed"

  if not sg.canBeContained(app):
    raise "Test failed"
  if app.canBeContained(sg):
    raise "Test failed"

  # Now hook the sg up and then test it again
  app.containmentArrows.append(ContainmentArrow(app,(0,0),sg,(0,0)))

  app2 = m.entities["app2"] = Entity(appt,(0,0),(100,20))

  if not sg.canBeContained(app):
    raise "Test failed: should return true because sg is contained in app"
  if sg.canBeContained(app2):
    raise "Test failed: should return false because sg can only be contained in one app"
  
  
def Test():
  import pdb
  m = Model()
  m.load("testModel.xml")
  for (path, obj) in m.data.find("modules"):
    for module in obj.children(lambda(x): x if (type(x) is types.InstanceType and x.__class__ is microdom.MicroDom) else None):   
      print module.tag_, ": ", module.data_
  print m.entityTypes.keys()

  #1. Build flatten entity instance
  #2. Build relation ship between instances
  for (path, obj) in m.data.find("instances"):
    fileEntLst = {}
    for entityType in m.entityTypes.keys():
      m.instances[entityType] = []
      for instance in obj.children(lambda(x): x if (type(x) is types.InstanceType and x.__class__ is microdom.MicroDom and x.tag_ == entityType) else None):
        if instance.child_.has_key("entityType"):
          data = instance.children_
          ent = m.entities.get(instance.entityType.data_)
          entityInstance = entity.Instance(ent, {}, ent.pos, ent.size, instance.name.data_)
          entityInstance.updateDataFields(instance)
          # Copy instance locked, then bind to readonly wxwidget
          entityInstance.instanceLocked = ent.instanceLocked.copy()

          m.instances[entityType].append(entityInstance)

          fileEntLst["/%s/%s" %(entityType, instance.name.data_)] = entityInstance


    
    # UnitTest(m)
  # m.save()
  return m

theModel = None

def TestRender(ctx):
  posx = 10
  posy = 10
  for et in theModel.entityTypes.items():
    bmp = et[1].iconSvg.instantiate((256,128))
    svg.blit(ctx,bmp,(posx,posy),(1,1))
    posx += 300
    if posx > 900:
      posx = 10
      posy += 150

def GuiTest():
  global theModel
  theModel = Test()
  import pyGuiWrapper as gui
  gui.go(lambda x,y=TestRender: gui.Panel(x,y))

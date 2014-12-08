import xml.dom.minidom
import microdom
import types
import common
from module import Module
import svg
import entity

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
    self.entityTypes = {}
    self.entities = {}
    self.instances = {}

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

  def loadModules(self):
    """Load the modules specified in the model"""
    for (path, obj) in self.data.find("modules"):
      for module in obj.children(lambda(x): x if (type(x) is types.InstanceType and x.__class__ is microdom.MicroDom) else None):   
        filename = module.data_.strip()
        print module.tag_, ": ", filename
        if not self.modules.has_key(filename):  # really load it since it does not exist
          tmp = self.modules[filename] = Module(filename)
          self.entityTypes.update(tmp.entityTypes)  # make the entity types easily accdef xmlify(self):

    # Set the entityType's context to this model so it can resolve referenced types, etc.
    for (name,e) in self.entityTypes.items():
      e.context = self

  def defaultForType(self,typ):
    """Figure out a reasonable default for the passed type"""
    ret = defaultForBuiltinType.get(typ,None)  # Is the type a builtin?
    if ret: return ret
    # TODO: look in the model's type list for this type and figure out a default
    return ""


  def xmlify(self):
    """Returns an XML string that defines the IDE Model, for saving to disk"""
    pass

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
    for entityType in m.entityTypes.keys():
      m.instances[entityType] = []
      for instance in obj.children(lambda(x): x if (type(x) is types.InstanceType and x.__class__ is microdom.MicroDom and x.tag_ == entityType) else None):
        data = instance.children_
        entityInstance = entity.Instance(m.entityTypes[entityType], data)
        m.instances[entityType].append(entityInstance)
    print m.instances
  # UnitTest(m)
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

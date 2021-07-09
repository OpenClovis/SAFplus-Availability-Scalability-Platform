import os
import pdb
import copy
import random
import xml.dom.minidom
import microdom
import types
import common
from module import Module
import svg
import entity
from entity import Entity
import generate
import share
import wx  




VERSION = "7.0"
MAX_RECURSIVE_INSTANTIATION_DEPTH = 5

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

  def directory(self):
    """Returns the location of this model on disk """
    return os.path.dirname(self.filename)

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
        self.deleteEntity(self.entities[items])
      if self.instances.get(items):
        self.deleteInstance(self.instances[items])

    if isinstance(items,entity.Instance):
      if share.instancePanel:
        share.instancePanel.deleteEntities([items], False)
      self.deleteInstance(items)
    elif isinstance(items, entity.Entity): 
      #self.deleteEntity(items)
      entname = items.data["name"]
      insToDelete = []
      for name,e in self.instances.items():
        if e.entity.data["name"] == entname:
          insToDelete.append(e)
      if share.instancePanel:
        share.instancePanel.deleteEntities(insToDelete, False)
      for i in insToDelete:
        # delete instances and its related instances (instances have relationship with it)
        self.deleteInstance(i)
      self.deleteEntity(items)
  def deleteEntity(self,entity):
    """Delete this instance of Entity from the model"""
    entname = entity.data["name"]
    for (name,e) in self.entities.items():
      e.containmentArrows[:] = [ x for x in e.containmentArrows if x.contained != entity]
      for k,v in e.data.items():         
         if (v == entity.data['name']):
            e.data[k] = ''
            break
    del self.entities[entname]   
       
    # Also delete the entity from the microdom
    #entities = self.data.getElementsByTagName("entities")
    #if entities:
    #  entities[0].delChild(entities[0].findOneByChild("name",entname))
    self.deleteEntityFromMicrodom(entname, entity.et.name)

    """Delete entity.Instance of Entity type
    nameInstances = [name for (name, e) in self.instances.items() if e.entity.data["name"] == entname]
    self.delete(nameInstances)
    """

  def deleteEntityFromMicrodom(self, entname, enttype):
    entities = self.data.getElementsByTagName("entities")    
    if entities:
      entities[0].delChild(entities[0].findOneByChild("name",entname))

    ide = self.data.getElementsByTagName("ide")
    if ide:
      entTypes = ide[0].getElementsByTagName(enttype)
      if entTypes:
        e = entTypes[0].getElementsByTagName(entname)
        if e:
          entTypes[0].delChild(e[0])

    ideEntities = self.data.getElementsByTagName("ide_entity_info")
    if ideEntities:
      e = ideEntities[0].getElementsByTagName(entname)
      if e:
        ideEntities[0].delChild(e[0])
      # delete entity in containment arrows if any
      name = "containmentArrows"
      caTags = ideEntities[0].getElementsByTagName(name)
      if caTags:
        name = "_"+entname
        for caTag in caTags:
          t = caTag.getElementsByTagName(name)
          if t:            
            caTag.delChild(t[0])
      
  def deleteWireFromMicrodom(self, containerName, containedName):
    ideEntities = self.data.getElementsByTagName("ide_entity_info")
    if ideEntities:
        e = ideEntities[0].getElementsByTagName(containerName)
        if e:
          arrows = e[0].getElementsByTagName("containmentArrows")
          if arrows:
            name = "_" + containedName
            t = arrows[0].getElementsByTagName(name)
            if t:
              arrows[0].delChild(t[0])

  def getEntitiesAndInfos(self):
    entities = self.data.getElementsByTagName("entities")
    entitiesInfo = self.data.getElementsByTagName("ide_entity_info")
    if entities and entitiesInfo:
      return (entities[0].pretty(), entitiesInfo[0].pretty())
    else: return ("", "")

  def getInstanceInfomation(self):
    self.updateMicrodom()
    instances = self.data.getElementsByTagName("instances")
    if instances: 
      return instances[0].pretty()

  def setEntitiesAndInfos(self, data):
    c = self.data.getElementsByTagName("entities")
    if c:
      self.data.delChild(c[0])
    dom  = xml.dom.minidom.parseString(data[0])
    entities = microdom.LoadMiniDom(dom.childNodes[0])
    self.data.addChild(entities)
      
    c = self.data.getElementsByTagName("ide")
    if c:
      c1 = c[0].getElementsByTagName("ide_entity_info")
      if c1:
        c[0].delChild(c1[0])
        dom1 = xml.dom.minidom.parseString(data[1])
        entitiesInfo = microdom.LoadMiniDom(dom1.childNodes[0])
        c[0].addChild(entitiesInfo)
    self.entities = {}
    self.loadDataInfomation()

  def loadDataInfomation(self):
    entities = self.data.getElementsByTagName("entities")
    ideEntities = self.data.getElementsByTagName("ide_entity_info")
    if ideEntities: ideEntities = ideEntities[0]  # Get first item in the list
    if entities:
      assert(len(entities)==1)
      entities = entities[0]
      fileEntLst = []
      for ed in entities.children(microdom.microdomFilter):
        name = ed["name"].data_
        entType = self.entityTypes[ed.tag_]

        pos = None
        size = None
        if ideEntities: # Load the pos and size from the model (if it exists)
          ideInfo = ideEntities.getElementsByTagName(name)
          if ideInfo:
            ideInfo = ideInfo[0]
            pos = common.str2Tuple(ideInfo["position"].data_)
            size = common.str2Tuple(ideInfo["size"].data_)

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
                (beginOffset, endOffset, midpoints) = self.getContainmemtArrowPos(ideEntities, eo, contained)
                ca = entity.ContainmentArrow(eo,beginOffset,contained,endOffset,midpoints)
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

    instances = self.data.find("instances")
    if instances:
      for (path, obj) in instances:
        fileEntLst = []
        for entityType in self.entityTypes.keys():
          for instance in obj.children(lambda(x): x if (type(x) is types.InstanceType and x.__class__ is microdom.MicroDom and x.tag_ == entityType) else None):
            if instance.child_.has_key("%sType"%entityType):
              entityTypeName = instance.child_.get("%sType"%entityType).data_

              # Entity of this instance
              entityParent = self.entities.get(entityTypeName)
              if not entityParent:
                continue
              entityInstance = entity.Instance(entityParent, instance, (0,0), (10,10), instance.name.data_)
              entityInstance.updateDataFields(instance)
    
              # Copy instance locked, then bind to readonly wxwidget
              entityInstance.instanceLocked = entityParent.instanceLocked.copy()
              self.instances[instance.name.data_] = entityInstance
              fileEntLst.append((instance,entityInstance))
  
      for (ed,eo) in fileEntLst:
        for et in self.entityTypes.items():   # Look through all the children for a key that corresponds to the name of an entityType (+ s), eg: "ServiceGroups"
          child = et[0][0].lower() + et[0][1:] + 's'
          for ch in ed.children(lambda(x): x if (type(x) is types.InstanceType and x.__class__ is microdom.MicroDom and x.tag_ == child) else None):
            # Strip out instance-identifier if any
            childName = str(ch.data_)[str(ch.data_).rfind("/")+1:]
            contained = self.instances.get(childName,None)
            if contained:
              # TODO: look the positions up in the GUI section of the xml file
              ca = entity.ContainmentArrow(eo,(0,0),contained,(0,0),[])
              contained.childOf.add(eo)
              eo.containmentArrows.append(ca)
            else:  # target of the link is missing, so drop the link as well.  This could happen if the user manually edits the XML
              # TODO: create some kind of warning/audit log in share.py that we can post messages to.
              pass
    
    entity.updateNamelyDict(self)

  def deleteInstance(self,inst):
    self.recursiveDeleteInstance(inst)
    
  def deleteInstanceFromMicrodom(self, entname):
    instances = self.data.getElementsByTagName("instances")
    if instances:    
      instances[0].delChild(instances[0].findOneByChild("name",entname))

  def recursiveDeleteInstance(self,inst):
    entname = inst.data["name"]    
    #if len(inst.containmentArrows)==0:
    #  self.deleteInstanceFromMicrodom(entname)
    #  for (name, e) in self.instances.items():
    #    if name==entname:
    #      del self.instances[name]
    #  return
    for ca in inst.containmentArrows:
      self.recursiveDeleteInstance(ca.contained)
    if len(inst.containmentArrows)>0:
      del inst.containmentArrows[:]
    self.deleteInstanceFromMicrodom(entname)
    del self.instances[entname]
    for (name,e) in self.instances.items():
      e.containmentArrows = [ x for x in e.containmentArrows if x.contained != inst]

  def connect(self,container, contained):
    """Connects 2 instances together.  Returns the containment arrow instance"""
    assert(isinstance(container,entity.Instance))  # TODO, allow this function to connect 2 entities (but not 1 instance and 1 entity)
    assert(isinstance(contained,entity.Instance))
    ca = entity.ContainmentArrow(container,(0,0),contained,(0,0))
    container.containmentArrows.append(ca)
    contained.childOf.add(container)
    return ca

  def isProxyOf(self,proxy, proxied):
    if proxy.data['csiType']==proxied.data['proxyCSI']:
      #print 'same csi for proxied [%s]'%proxied.data['name']
      for ca in proxy.containmentArrows:
        #print 'ca of [%s]: container [%s]. contained [%s]' %(proxy.data['name'],ca.container.data['name'],ca.contained.data['name'])
        if ca.contained.data['name']==proxied.data['name']:
          #print 'proxied found'
          return True
    #print 'no proxied found'
    return False
     

  def generateSource(self,srcDir):
    #print 'eneter generateSource'
    output = common.FilesystemOutput()
    #comps = filter(lambda entity: entity.et.name == 'Component' and entity.data['NonSafComponents']!='', self.entities.values()) # SA_Aware comp no proxied
    #proxyComps = filter(lambda entity: entity.et.name == 'Component' and len(entity.data['NonSafComponents'])>0, self.entities.values())
    comps = []
    proxyComps = []
    for c in filter(lambda entity: entity.et.name == 'Component',self.entities.values()):
      noProxied = True
      #print c.data['name']
      #print 'in outer loop'
      for nsc in filter(lambda entity: entity.et.name == 'NonSafComponent',self.entities.values()):
        #print nsc.data['name']
        if self.isProxyOf(c, nsc):            
          proxyComps.append(c)
          noProxied = False
          print 'found proxied. break'
          break
        #print 'continue inner loop'
      if noProxied:
        comps.append(c)
      #print 'continue outer loop'

      #if e.et.name == 'Component' and e.data.has_key('NonSafComponents') and len(e.data['NonSafComponents'])==0:
      #  comps.append(e)
      #elif e.et.name == 'Component' and e.data.has_key('NonSafComponents') and len(e.data['NonSafComponents'])>0:
      #  proxyComps.append(e)
        
    #print 'generateSource: %s' %str(proxyComps)
    srcDir = os.sep.join([srcDir, "src"])


    files = []
    
    # Create Makefile
    files += generate.topMakefile(output, srcDir,[c.data["name"] for c in comps+proxyComps])    



    for c in comps:
      if os.path.exists(srcDir+os.sep+c.data['name']+os.sep+'proxyMain.cxx'):
        #print 'model[%d]: We will delete = %s'%(sys._getframe().f_lineno, srcDir+os.sep+c.data['name']+os.sep+'proxymain.cxx') 
        os.popen('rm -rf '+srcDir+os.sep+c.data['name']+os.sep+'proxyMain.cxx')
      files += generate.cpp(output, srcDir, c, c.data)


    proxyFiles = []
    for proxy in proxyComps:
      if os.path.exists(srcDir+os.sep+proxy.data['name']+os.sep+'main.cxx'):
        #print 'model[%d]: We will delete = %s'%(sys._getframe().f_lineno, srcDir+os.sep+proxy.data['name']+os.sep+'main.cxx')
        os.popen('rm -rf '+srcDir+os.sep+proxy.data['name']+os.sep+'main.cxx')
      proxyFiles += generate.cpp(output, srcDir, proxy, proxy.data, True)

    # Delete unnecessary .cxx file
    for folder in os.popen('ls '+srcDir).read().split():
      if folder not in [c.data['name'] for c in comps+proxyComps] and folder != 'Makefile':
        #print 'model[%d]: We will delete %s folder'%(sys._getframe().f_lineno, folder)
        cmd = 'rm -rf ' + srcDir + os.sep + folder
        os.popen('%s'%cmd) 

    return files,proxyFiles


  def load(self, fileOrString):
    """Load an XML representation of the model"""
    if fileOrString[0] != "<":  # XML must begin with opener
      self.filename = common.fileResolver(fileOrString)
      with open(self.filename,"r") as f:
        fileOrString = f.read()
    dom = xml.dom.minidom.parseString(fileOrString)
    self.data = microdom.LoadMiniDom(dom.childNodes[0])

    self.loadModules()
    self.loadDataInfomation()
    return True

  def getContainmemtArrowPos(self, ideEntities, container, contained):
    name = container.data["name"]    
    containerTags = ideEntities.getElementsByTagName(name)
    if not containerTags:
      return ((0,0), (0,0), [])
    containerTag = containerTags[0]
    name = "containmentArrows"
    caTags = containerTag.getElementsByTagName(name)
    if caTags:
      containedEntName = "_"+contained.data["name"]
      for caTag in caTags:
        t = caTag.getElementsByTagName(containedEntName)
        if t:
          t = t[0]
          beginOffset = common.str2Tuple(t["beginOffset"].data_)
          endOffset = common.str2Tuple(t["endOffset"].data_)
          if t["midpoints"] and t["midpoints"].data_:
            midPoints = [common.str2Tuple(t["midpoints"].data_)]
          else:
            midPoints = None
          return (beginOffset, endOffset, midPoints)      
    return ((0,0), (0,0), [])

  def makeUpAScreenPosition(self):
    return (random.randint(0,800),random.randint(0,800))

  def save(self, filename=None):
    """Save XML representation of the model"""
    if filename is None: filename = self.filename
    with open(filename,"w") as f:
      f.write(self.xmlify())

  def loadModules(self):
    """Load the modules specified in the model"""
    for (path, obj) in self.data.find("modules"):
      for module in obj.children(microdom.microdomFilter):   
        filename = module.data_.strip()
        #print module.tag_, ": ", filename

        if not os.path.dirname(filename) or len(os.path.dirname(filename).strip()) == 0:
            filename = os.sep.join([os.path.dirname(self.filename), filename])

        if not self.modules.has_key(filename):  # really load it since it does not exist
          tmp = self.modules[filename] = Module(filename)
          self.entityTypes.update(tmp.entityTypes)  # make the entity types easily accdef xmlify(self):
          for (typName,data) in tmp.ytypes.items():
            self.dataTypes[typName] = data

    # Set the entityType's context to this model so it can resolve referenced types, etc.
    for (name,e) in self.entityTypes.items():
      e.context = self

  def loadModuleFromFile(self, moduleFile):
    """Load the modules specified in the model"""
    if not self.modules.has_key(moduleFile):  # really load it since it does not exist
      tmp = self.modules[moduleFile] = Module(moduleFile)
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
    # First, update the model to make sure that it is internally consistent
    for (name,i) in self.instances.items():
      for parent in filter(lambda ent: isinstance(ent, Entity), i.childOf):  # If the object has parent pointers, update them.  This is pretty specific to SAFplus data types...
        fieldName = parent.et.name[0].lower() + parent.et.name[1:]  # uncapitalize the first letter to make it use SAFplus bumpycase
        if i.data.has_key(fieldName):
          i.data[fieldName] = parent.data["name"]

    # Locate or create the needed sections in the XML file

    #   find or create the entity area in the microdom
    entities = self.data.getElementsByTagName("entities")
    if not entities:
      entities = microdom.MicroDom({"tag_":"entities"},[],[])
      self.data.addChild(entities)
    else: 
      assert(len(entities)==1)
      entities = entities[0]
    
    # Find or create the GUI area in the microdom.  The GUI area is structured like:
    # ide
    #   ide_entity_info
    #   ide_instance_info
    ide = self.data.getElementsByTagName("ide")
    if not ide:
      ide = microdom.MicroDom({"tag_":"ide"},[],[])
      self.data.addChild(ide)
    else: 
      assert(len(ide)==1)
      ide = ide[0]
          
    ideEntities = ide.getElementsByTagName("ide_entity_info")
    if not ideEntities:
      ideEntities = microdom.MicroDom({"tag_":"ide_entity_info"},[],[])
      ide.addChild(ideEntities)
    else: 
      assert(len(ideEntities)==1)
      ideEntities = ideEntities[0]

    ideInsts = ide.getElementsByTagName("ide_instance_info")
    if not ideInsts:
      ideInsts = microdom.MicroDom({"tag_":"ide_instance_info"},[],[])
      ide.addChild(ideInsts)
    else: 
      assert(len(ideInsts)==1)
      ideInsts = ideInsts[0]
    

    # Write out the entities


    #   iterate through all entities writing them to the microdom, or changing the existing microdom
    for (name,e) in self.entities.items():
      # Find the existing DOM nodes for the entity information, creating the node if it is missing
      entity = entities.findOneByChild("name",name)
      if not entity:
        entity = microdom.MicroDom({"tag_":e.et.name},[],[])
        entities.addChild(entity)
      ideEntity = ideEntities.getElementsByTagName(name)
      if ideEntity: ideEntity = ideEntity[0]
      else:
        ideEntity = microdom.MicroDom({"tag_":name},[],[])
        ideEntities.addChild(ideEntity)

      # Remove all "None", replacing with the default or ""
      temp = {}
      for (key,val) in e.data.items():
        if val is None:
          val = e.et.data[key].get("default",None)
          if val is None:
            val = ""
        if val == "None": val = ""
        temp[key] = val

      # Write all the data fields into the model's microdom
      entity.update(temp)
      # write the IDE specific information to the IDE area of the model xml
      ideEntity["position"] = str(e.pos)
      ideEntity["size"] = str(e.size) 
      # Now write all the arrows
      contains = {} # Create a dictionary to hold all linkages by type
      for arrow in e.containmentArrows:
        # Add the contained object to the dictionary keyed off of the object's entitytype
        tmp = contains.get(arrow.contained.et.name,[])
        tmp.append(arrow.contained.data["name"])
        contains[arrow.contained.et.name] = tmp
        # TODO: write the containment arrow IDE specific information to the IDE area of the model xml
        self.writeContainmentArrow(ideEntity, arrow)
      # Now erase the missing linkages from the microdom
      for (key, val) in self.entityTypes.items():   # Look through all the children for a key that corresponds to the name of an entityType (+ s), eg: "ServiceGroups"
          if not contains.has_key(key): # Element is an entity type but no linkages
            if entity.child_.has_key(key + 's'): entity.delChild(key + 's')
      # Ok now write the linkages to the microdom
      for (key, val) in contains.items():
        k = key + "s"
        if entity.child_.has_key(k): entity.delChild(k)
        entity.addChild(microdom.MicroDom({"tag_":k},[",".join(val)],""))  # TODO: do we really need to pluralize?  Also validate comma separation is ok

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


    # Find or create the instance area in the microdom
    instances = self.data.getElementsByTagName("instances")
    if not instances:
      instances = microdom.MicroDom({"tag_":"instances"},[],[])
      self.data.addChild(instances)
    else: 
      assert(len(instances)==1)
      instances = instances[0]

    # iterate through all instances writing them to the microdom, or changing the existing microdom
    for (name,e) in self.instances.items():
      instance = instances.findOneByChild("name",name)
      if not instance:
        instance = microdom.MicroDom({"tag_":e.et.name},[],[])
        instances.addChild(instance)

      # Remove all "None", replacing with the default or ""
      temp = {}      
      for (key,val) in e.data.items():
        if val is None:
          val = e.et.data[key].get("default",None)
          if val is None:
            val = ""
        if val == "None": val = ""
        temp[key] = val

      # Add module and xpath attributes
      # workaround for NonSafComponent: xpath for it is same as Component
      expath = e.entity.et.data["xpath"]
      idx = expath.rfind('NonSafComponent')
      if idx != -1:
         expath = expath[:idx]+'Component'
      instance.addAttribute("xpath",expath + ("[@name=\"%s\"]" % e.data["name"]))
      instance.addAttribute("module",e.entity.et.data["module"])

      # Write all the data fields into the model's microdom
      instance.update(temp)  
      # Now write all the arrows
      contains = {} # Create a dictionary to hold all linkages by type
      for arrow in e.containmentArrows:
        # Add the contained object to the dictionary keyed off of the object's entitytype
        # leaf-list entity type with camelCase(s)
        key = arrow.contained.et.name[0].lower() + arrow.contained.et.name[1:] + 's'
        tmp = contains.get(key,[])
        tmp.append(arrow.contained.data["name"])
        contains[key] = tmp

      # Now erase the missing linkages from the microdom
      for (key, val) in self.entityTypes.items():   # Look through all the children for a key that corresponds to the name of an entityType (+ s), eg: "serviceUnits"
          key = key[0].lower() + key[1:] + 's'
          if not contains.has_key(key): # Element is an entity type but no linkages
            if instance.child_.has_key(key): instance.delChild(key)

      # Ok now write the linkages to the microdom
      for (key, vals) in contains.items():
        if instance.child_.has_key(key): instance.delChild(key)
        for val in vals:
          instance.addChild(microdom.MicroDom({"tag_":key},[val],""))  # TODO: do we really need to pluralize?  Also validate comma separation is ok

      # Extra parent entity name
      entityParentVal = e.entity.data["name"]
      entityParentKey = "%sType"%e.et.name
      if instance.child_.has_key(entityParentKey): instance.delChild(entityParentKey)
      instance.addChild(microdom.MicroDom({"tag_":entityParentKey},[entityParentVal],""))
  
  def createChild(self, parent, childName):
    name = childName
    childTag = parent.getElementsByTagName(name)
    if childTag:
      childTag = childTag[0]
    else:
      childTag = microdom.MicroDom({"tag_":name},[],[])
      parent.addChild(childTag)
    return childTag

  def writeContainmentArrow(self, ideEntity, arrow):
    # create <containmentArrows> inside <entity>
    name = "containmentArrows"
    caTag = self.createChild(ideEntity, name)
    # create <containedEntity> inside <containmentArrows> 
    name = "_"+arrow.contained.data["name"]
    containedEntTag = self.createChild(caTag, name)
    t = containedEntTag
    # create <beginOffset> inside <containedEntity> 
    name = "beginOffset"
    t[name] = str(arrow.beginOffset)
    # create <endOffset> inside <containedEntity> 
    name = "endOffset"
    t[name] = str(arrow.endOffset)
    # create <midpoints> inside <containedEntity>
    name = "midpoints"
    if arrow.midpoints:
      t[name] = str(tuple(arrow.midpoints[0]))
    else:
      t[name] = ""

  def duplicate(self,entities,recursive=False, flag=False):
    """Duplicate a set of entities or instances and potentially all of their children. The last argument 'flag' indicates that
       this is the copy of ServiceUnit or ServiceInstance, otherwise, Component or ComponentServiceInstance. It is the one of 
       the criteria that childOf attribute of the entity/instance can be duplicated or not. Suppose that you want to copy
       ServiceUnit or ServiceInstance, its parents have to be duplicated, of course, however, when its children is duplicated
       (Component or ComponentServiceInstance), their parents cannot be duplicated. For example, supposing there is SU1->Comp1, 
       SG1->SU1, Node1->SU1 (Comp1 is a child of SU1; SU1 is a child of SG1 and Node1). SU1 is copied, suppose SU11 is a copy of SU1. 
       As a result, SU11 should be a child of SG1 and Node1 but Comp11 (is a copy of Comp1) should only be a child of SU11, NOT SU1. 
       But another example, if only Comp1 is copied and Comp11 is a copy of Comp1, so, its parent should be duplicated, too.
       In this case, Comp11 should be a child of SU1, too (because Comp1 is copied inside SU1)
       This fix (with a fix at duplicate() function in entity.py gains this purpose"""

    ret = []
    addtl = []
    for e in entities:
      name=entity.NameCreator(e.data["name"])  # Let's see if the instance is already here before we recreate it.
      while True:
        ei = self.instances.get(name,None)  
        if ei: # if this instance exists, try to get another name
          name=entity.NameCreator(e.data['name'], name)
        else:
          break
      if e.et.name in ('ServiceUnit', 'ServiceInstance'):
        dupChildOf = True
      else:
        if flag:
          dupChildOf = False
        else:
          dupChildOf = True
      newEnt = e.duplicate(name, not recursive, dupChildOf=dupChildOf)  # if we don't want recursive duplication, then dup the containment arrows.      
      if recursive:  # otherwise dup the containment arrows and the objects they point to
        for ca in e.containmentArrows:
          (contained,xtra) = self.duplicate([ca.contained],recursive,flag=True)
          assert(len(contained)==1)  # It must be 1 because we only asked to duplicate one entity
          contained = contained[0]
          contained.childOf.add(newEnt)
          cai = copy.copy(ca)
          cai.container = newEnt
          cai.contained = contained
          newEnt.containmentArrows.append(cai)
          addtl.append(contained)
          addtl.append(xtra)
      ret.append(newEnt)
      if isinstance(newEnt,entity.Instance):
        self.instances[name] = newEnt
      elif isinstance(newEnt,entity.Entity):
        self.entities[name] = newEnt
      else:
        assert(0)
        
    return (ret,addtl)

  def getInstantiatedNodes(self):
    nodes = []
    for (name, i) in self.instances.items():
      if i.data['entityType'] == 'Node':
         print 'append [%s] to the returned list'%name
         nodes.append(i)
    return nodes

  def needInstantiate(self, ent):
    if ent.data['entityType'] == 'ServiceUnit':
      nodeList = self.getInstantiatedNodes()
      for node in nodeList:
        for ca in node.entity.containmentArrows:
          if ent == ca.contained:
             print '[%s] is the child of [%s]'%(ent.data['name'],node.entity.data['name'])
             return True
          else:
             print '[%s] is NOT the child of [%s]'%(ent.data['name'], node.entity.data['name'])
      return False
    return True

  def recursiveInstantiation(self,ent,instances=None, depth=1):
    if not instances: instances = self.instances
    children = []
    if 1:
      name=entity.NameCreator(ent.data["name"])  # Let's see if the instance is already here before we recreate it.
      ei = instances.get(name,None)  
      if not ei:
        print 'instantiating [%s]'%name
        ei = entity.Instance(ent, None,pos=None,size=None,name=name)
        instances[name] = ei
      depth = depth + 1
      # 2 ways recursive:
      #   1. SG -> SI -> CSI
      #   2. Node -> SU -> Component
      if ent.et.name != "ComponentServiceInstance":
        if depth<=MAX_RECURSIVE_INSTANTIATION_DEPTH:
          for ca in ent.containmentArrows:
            if ca.container.et.name == 'Component':
              print 'skip creating instance which is a child (such as NonSafComponent) of Component'
              continue
            if not self.needInstantiate(ca.contained):
              print 'skip instantiating [%s] because its SG or Node are not instantiated yet'%ca.contained.data['name']
              continue
            (ch, xtra) = self.recursiveInstantiation(ca.contained,instances, depth)
            ch.childOf.add(ei)
            cai = copy.copy(ca)
            cai.container = ei
            cai.contained = ch
            ei.containmentArrows.append(cai)
            print 'created arrow [%s-->%s] for [%s]'% (ei.data['name'],ch.data['name'], ei.data['name'])
            children.append(ch)
      else:
        print 'model::recursiveInstantiation: do not create recursive instance for [%s], type [%s]' % (name, ent.et.name)
      return (ei, instances)

  def recursiveDuplicateInst(self,inst,instances=None, depth=1):
    if not instances: instances = self.instances    
    if 1:
      name=entity.NameCreator(inst.data["name"])  # Let's see if the instance is already here before we recreate it.
      print 'model::recursiveAndDuplicateInst: new dup inst name = [%s]' % name
      ei = instances.get(name,None)  
      if not ei:
        ei = inst.duplicate(name)
        instances[name] = ei
        # add the SG and SU relationship
        self.addContainmenArrow(inst, ei)
          
      depth = depth + 1      
      if depth<=MAX_RECURSIVE_INSTANTIATION_DEPTH:
        for ca in inst.containmentArrows:
          print 'model::recursiveAndDuplicateInst: ca = [%s]' % ca.contained.data["name"]
          ch = self.recursiveDuplicateInst(ca.contained,instances, depth)
          print 'model::recursiveAndDuplicateInst: ch name = [%s]' % ch.data["name"]
          for parent in ch.childOf:
            if parent.data['entityType'] == "ServiceUnit" and ei.et.name == "ServiceUnit":
              ch.childOf = set()
          ch.childOf.add(ei)
          cai = copy.copy(ca)
          cai.container = ei
          cai.contained = ch
          ei.containmentArrows.append(cai)
    
      return ei

  def addContainmenArrow(self, inst, newinst):
    if inst.et.name=="ServiceUnit":
      newinst.childOf = set()
      for e in filter(lambda entInt: entInt.et.name=="ServiceGroup", self.instances.values()):        
        for ca in e.containmentArrows:
          if ca.contained==inst:
            newinst.childOf.add(e)
            e.createContainmentArrowTo(newinst)            

  def xmlify(self):
    """Returns an XML string that defines the IDE Model, for saving to disk"""
    self.updateMicrodom()
    return self.data.pretty()

def UnitTest(m=None):
  """This unit test relies on a particular model configuration, located in testModel.xml"""
  import entity
  if not m:
    m = Model()
    m.load("testModel.xml")
  
  appt = m.entityTypes["Application"]
  app = m.entities["app"] = entity.Entity(appt,(0,0),(100,20))
  sgt = m.entityTypes["ServiceGroup"]
  sg = m.entities["sg"] = entity.Entity(sgt,(0,0),(100,20))
  
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

  app2 = m.entities["app2"] = entity.Entity(appt,(0,0),(100,20))

  if not sg.canBeContained(app):
    raise "Test failed: should return true because sg is contained in app"
  if sg.canBeContained(app2):
    raise "Test failed: should return false because sg can only be contained in one app"

  m.entities["appSG"].createInstance((0,0),(100,40),"sg0")
  m.instances[sg0.data["name"]] = sg0
 
  
def Test():
  import pdb
  m = Model()
  m.load("testModel.xml")
  for (path, obj) in m.data.find("modules"):
    for module in obj.children(lambda(x): x if (type(x) is types.InstanceType and x.__class__ is microdom.MicroDom) else None):   
      print module.tag_, ": ", module.data_
  print m.entityTypes.keys()
  # pdb.set_trace()

  #sg0 = m.entities["appSG"].createInstance((0,0),(100,40),"sg0")
  (sg,instances) = m.recursiveInstantiation(m.entities["appSG"])

  instances["app1"].data["instantiate"]["command"] = "./app1 param"
  node = m.entities["SC"].createInstance((0,0),(100,40),False,"sc0")
  su = instances["ServiceUnit11"]

  m.instances.update(instances)
  m.instances[node.data["name"]] = node
  m.connect(node,su)
  # m.instances[sg0.data["name"]] = sg0

  #1. Build flatten entity instance
  #2. Build relation ship between instances
  # Load instances
  #print instances
  # UnitTest(m)
  m.save("test.xml")
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

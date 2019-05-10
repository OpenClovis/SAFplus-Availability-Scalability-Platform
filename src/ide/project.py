import os
import os.path
import pdb
import math
import time
from types import *

import wx
import  wx.lib.newevent

import microdom
import xml.dom.minidom
from common import log
import re
import ntpath
import common
import sys
import share
import pexpect
import wx.aui as aui
import wx.lib.scrolledpanel as scrolled
from distutils.dir_util import copy_tree
import subprocess
import webbrowser

PROJECT_LOAD = wx.NewId()
PROJECT_SAVE = wx.NewId()
PROJECT_SAVE_AS = wx.NewId()

PROJECT_WILDCARD = "SAFplus Project (*.spp)|*.spp|All files (*.*)|*.*"

PROJECT_VALIDATE = wx.NewId()
PROJECT_BUILD = wx.NewId()
MAKE_IMAGES      = wx.NewId() 
IMAGES_DEPLOY    = wx.NewId()
HELP_CONTENTS    = wx.NewId()
HELP_ABOUT       = wx.NewId()

ProjectLoadedEvent, EVT_PROJECT_LOADED = wx.lib.newevent.NewCommandEvent()  # Must be a command event so it propagates
ProjectNewEvent, EVT_PROJECT_NEW = wx.lib.newevent.NewCommandEvent()
ProjectSaveAsEvent, EVT_PROJECT_SAVE_AS = wx.lib.newevent.NewCommandEvent()

class Project(microdom.MicroDom):
  def __init__(self, filename=None):
    self.dirty = False
    self._safplusModel = None
    self.datamodel = None
    self.dataModelPlugin = None
    self.name = ""    
    self.prjXmlData = None
    self.fileProject = filename
    if filename: self.load(filename)
    pass

  def directory(self):
    """Returns the location of this model on disk """
    return os.path.dirname(self.projectFilename)   

  def modelFilename(self):
    return self.name+"Model.xml"

  #def sourceFilename(self):
  #  return self.name+".cpp"

  #def headerFilename(self):
  #  return self.name+".h"

  def load(self,filename):    
    self.projectFilename = filename
    self.name = os.path.splitext(os.path.basename(filename))[0]
    md = microdom.LoadFile(filename)
    microdom.MicroDom.__init__(self,md.attributes_, md.children_, md.data_)    
    # TODO validate the structure of the microdom tree
    self.loadPlugin()

  def setSAFplusModel(self,mdl):
    self._safplusModel = mdl

  def save(self, filename=None):
    if not filename: filename = self.projectFilename
    # Perhaps, the prj xml contents will not be changed, so it should be saved once on new
    # Now save the model to XML
    self._safplusModel.save()

  def new(self, datamodel):    
    self.name = os.path.splitext(os.path.basename(self.projectFilename))[0]
    self.datamodel = datamodel
    # project file (.ssp), model (.xml) file should be created. Model file is created with main tags in which data is empty
    self.createPrjXml()
    self.createModelXml()
    md = microdom.LoadFile(self.projectFilename)
    microdom.MicroDom.__init__(self,md.attributes_, md.children_, md.data_)
    self.loadPlugin()

  def saveAs(self, fromProject):    
    self.name = os.path.splitext(os.path.basename(self.projectFilename))[0]
    self.doSaveAs(fromProject)

  def doSaveAs(self, fromProject):
    def copy(srcpath, destpath):
      f = open(srcpath, "r")
      data = f.read()
      f.close()
      f = open(destpath, "w")
      f.write(data)
      f.close()
    srcRootdir = fromProject.directory()
    destRootdir = self.directory()
    for subdir, dirs, files in os.walk(srcRootdir):
      for f in files:
        srcpath = os.path.join(subdir, f)
        dstpath = os.path.join(destRootdir, srcpath.replace(os.path.join(srcRootdir, ""), ""))
        if dstpath.find(fromProject.modelFilename()) != -1:
          dstpath = dstpath.replace(fromProject.modelFilename(), self.modelFilename())
        else:
          if dstpath.find(fromProject.name+'.spp') != -1:
            dstpath = dstpath.replace(fromProject.name+'.spp', self.name+'.spp') 
        if os.path.exists(dstpath):
          if os.stat(srcpath).st_mtime > os.stat(dstpath).st_mtime:
            copy(srcpath, dstpath)
        else:
          copy(srcpath, dstpath)
      for d in dirs:
        srcpath = os.path.join(subdir, d)
        dstpath = os.path.join(destRootdir, srcpath.replace(os.path.join(srcRootdir, ""), ""))
        if not os.path.exists(dstpath):
          os.mkdir(dstpath)
    self.updatePrjXml(fromProject)

  def loadPlugin(self):
    """Loads the data model plugin if one exists"""
    modulename = self.datamodel.data_.split(".")[0]
    try:
      self.dataModelPlugin = __import__(modulename)
    except ImportError, e:
      print "Warning: No custom code for this data model.  Tried to import %s" % modulename

  def createPrjXml(self):
    dom = xml.dom.minidom.Document()
    safplusPrjElem = dom.createElement("SAFplusProject")
    dom.appendChild(safplusPrjElem)
    safplusPrjElem.setAttribute("version", "7.0")

    datamodelElem = dom.createElement("datamodel")
    safplusPrjElem.appendChild(datamodelElem)
    datamodel = ntpath.basename(self.datamodel)
    datamodelText = dom.createTextNode(datamodel)
    datamodelElem.appendChild(datamodelText)

    modelElem = dom.createElement("model")
    safplusPrjElem.appendChild(modelElem)
    modelText = dom.createTextNode(self.modelFilename())
    modelElem.appendChild(modelText)

    sourceElem = dom.createElement("src")
    safplusPrjElem.appendChild(sourceElem)
    #sourceText = dom.createTextNode(self.sourceFilename()+" "+self.headerFilename())
    #sourceElem.appendChild(sourceText)

    self.prjXmlData = microdom.LoadMiniDom(dom.childNodes[0])
    f = open(self.projectFilename,"w")
    f.write(self.prjXmlData.pretty())
    f.close()    
        
  def createModelXml(self):
    """ Create the project model.xml but there is no detail data in it """
    dom = xml.dom.minidom.Document()
    safplusModelElem = dom.createElement("SAFplusModel")
    dom.appendChild(safplusModelElem)
    safplusModelElem.setAttribute("version", "7.0")

    moduleElem = dom.createElement("modules")
    safplusModelElem.appendChild(moduleElem)
    yangElem = dom.createElement("yang")
    moduleElem.appendChild(yangElem)    
    datamodel = ntpath.basename(self.datamodel)
    datamodelText = dom.createTextNode(datamodel)
    yangElem.appendChild(datamodelText)

    entitiesElem = dom.createElement("entities")
    safplusModelElem.appendChild(entitiesElem)    

    ideElem = dom.createElement("ide")
    safplusModelElem.appendChild(ideElem)

    instancesElem = dom.createElement("instances")
    safplusModelElem.appendChild(instancesElem)        

    data = microdom.LoadMiniDom(dom.childNodes[0])
    f = open(self.directory()+os.sep+self.modelFilename(),"w")
    f.write(data.pretty())
    f.close()  

  def loadModel(self):
    if not self.prjXmlData:
      f = open(self.projectFilename,"r")
      data = f.read()
      f.close()
      dom = xml.dom.minidom.parseString(data)
      self.prjXmlData = microdom.LoadMiniDom(dom.childNodes[0])

  def updatePrjXmlSource(self, srcFiles):
    self.loadModel()
    sources = self.prjXmlData.getElementsByTagName("src")    
    #data="" 
    #for f in srcFiles:
    #  data+=f
    #  data+=" "
    #sources[0].data_ = data
    sources[0].children_ = srcFiles    
    f = open(self.projectFilename,"w")
    f.write(self.prjXmlData.pretty())
    f.close()

  def updatePrjXml(self, fromProject):
    self.loadModel()
    # update model
    model = self.prjXmlData.getElementsByTagName("model")    
    newModel = []
    for m in model[0].children_:
      newModel.append(m.replace(fromProject.modelFilename(), self.modelFilename()))
    model[0].children_ = newModel    
    # update sources
    sources = self.prjXmlData.getElementsByTagName("src")    
    newSrc = []
    for src in sources[0].children_:
      newSrc.append(src.replace(fromProject.directory(), self.directory()))
    sources[0].children_ = newSrc
    f = open(self.projectFilename,"w")
    f.write(self.prjXmlData.pretty())
    f.close()

class ProjectTreeCtrl(wx.TreeCtrl):
  def __init__(self, parent, id, pos, size, style):
    self.parent = parent
    wx.TreeCtrl.__init__(self, parent, id, pos, size, style)

    def OnCompareItems(self, item1, item2):
      t1 = self.GetItemText(item1)
      t2 = self.GetItemText(item2)
      self.log.write('compare: ' + t1 + ' <> ' + t2 + '\n')
      if t1 < t2: return -1
      if t1 == t2: return 0
      return 1


class ProjectTreePanel(wx.Panel):
  def __init__(self, parent,guiPlaces):
        self.parent = parent
        self.currentActiveProject = None
        # Use the WANTS_CHARS style so the panel doesn't eat the Return key.
        wx.Panel.__init__(self, parent, -1, style=wx.WANTS_CHARS)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        tID = wx.NewId()
        self.guiPlaces = guiPlaces
        self.projects = []

        self.problems = []

        self.entitiesRelation = {
          "ServiceGroup"            :["ServiceInstance", "ServiceUnit"],
          "ServiceInstance"         :["ComponentServiceInstance"],
          "ComponentServiceInstance":["Component"],
          "Component"               :None,
          "Cluster"                 :["Node"],
          "Node"                    :["ServiceUnit"],
          "ServiceUnit"             :["Component"]
        }

        self.entitiesParent = {
          "ServiceGroup"            : None,
          "ServiceInstance"         :["ServiceGroup"],
          "ComponentServiceInstance":["ServiceInstance"],
          "Component"               :["ComponentServiceInstance", "ServiceUnit"],
          "Cluster"                 :None,
          "Node"                    :["Cluster"],
          "ServiceUnit"             :["Node", "ServiceGroup"]
        }

        self.warn = "WARNNING"
        self.error = "ERROR"
        self.info = "INFOMATION"
        self.instantiation = "Instantiation"
        self.modelling   = "Modelling"
        self.pathProject = None

        self.color = {
          self.error: wx.Colour(255, 0, 0),
          self.warn: wx.Colour(0, 0, 255),
          self.info: wx.Colour(0, 0, 255)
        }

        self.tree = ProjectTreeCtrl(self, tID, wx.DefaultPosition, wx.DefaultSize,
                               wx.TR_HAS_BUTTONS
                               | wx.TR_EDIT_LABELS
                               | wx.TR_MULTIPLE
                               | wx.TR_HIDE_ROOT
                               )

        isz = (16,16)
        il = wx.ImageList(isz[0], isz[1])
        fldridx     = il.Add(wx.ArtProvider_GetBitmap(wx.ART_FOLDER,      wx.ART_OTHER, isz))
        fldropenidx = il.Add(wx.ArtProvider_GetBitmap(wx.ART_FOLDER_OPEN, wx.ART_OTHER, isz))
        fileidx     = il.Add(wx.ArtProvider_GetBitmap(wx.ART_NORMAL_FILE, wx.ART_OTHER, isz))

        self.root = self.tree.AddRoot("Projects")  # Not going to be shown anyway
        self.tree.SetPyData(self.root, None)

        # Insert my tools etc into the GUI
        self.fileMenu = guiPlaces.menu["File"]
        self.fileMenu.Append(wx.ID_NEW, "&New\tAlt-n", "New Project")
        self.fileMenu.Append(PROJECT_LOAD, "L&oad\tAlt-l", "Load Project")
        self.fileMenu.Append(PROJECT_SAVE, "S&ave\tAlt-s", "Save Project")
        self.fileMenu.Enable(PROJECT_SAVE, False)
        self.fileMenu.Append(PROJECT_SAVE_AS, "Save As...\tAlt-a", "Save As")
        self.fileMenu.Enable(PROJECT_SAVE_AS, False)
        

        # bind the menu event to an event handlerfff
        self.fileMenu.Bind(wx.EVT_MENU, self.OnNew, id=wx.ID_NEW)
        self.fileMenu.Bind(wx.EVT_MENU, self.OnLoad, id=PROJECT_LOAD)
        self.fileMenu.Bind(wx.EVT_MENU, self.OnSave, id=PROJECT_SAVE)
        self.fileMenu.Bind(wx.EVT_MENU, self.OnSaveAs, id=PROJECT_SAVE_AS)

        #
        self.menuProject = guiPlaces.menu["Project"]
        self.menuProject.Append(PROJECT_VALIDATE, "Validate Project", "Validate Project")
        self.menuProject.Append(MAKE_IMAGES, "Make Image(s)...", "Make Image(s)...")
        self.menuProject.Append(IMAGES_DEPLOY, "Deploy Image(s)...", "Deploy Image(s)...")
        self.menuProject.Append(PROJECT_BUILD, "Build Project", "Build Project")

        self.menuProject.Enable(PROJECT_VALIDATE, False)
        self.menuProject.Enable(MAKE_IMAGES, False)
        self.menuProject.Enable(IMAGES_DEPLOY, False)
        self.menuProject.Enable(PROJECT_BUILD, False)
        self.menuProject.Bind(wx.EVT_MENU, self.OnValidate, id=PROJECT_VALIDATE)
        self.menuProject.Bind(wx.EVT_MENU, self.OnMakeImages, id=MAKE_IMAGES)
        self.menuProject.Bind(wx.EVT_MENU, self.OnDeploy, id=IMAGES_DEPLOY)
        self.menuProject.Bind(wx.EVT_MENU, self.OnBuild, id=PROJECT_BUILD)

        self.menuHelp = guiPlaces.menu["Help"]
        self.menuHelp.Append(HELP_CONTENTS, "Help Contents", "Help Contents")
        self.menuHelp.Append(HELP_ABOUT, "About", "About")
        self.menuHelp.Enable(HELP_CONTENTS, True)
        self.menuHelp.Enable(HELP_ABOUT, True)
        self.menuHelp.Bind(wx.EVT_MENU, self.OnHelpConents, id=HELP_CONTENTS)
        self.menuHelp.Bind(wx.EVT_MENU, self.OnHelpAbout, id=HELP_ABOUT)

        # wx 2.8 compatibility
        wx.EVT_MENU(guiPlaces.frame, wx.ID_NEW, self.OnNew)
        wx.EVT_MENU(guiPlaces.frame, PROJECT_LOAD, self.OnLoad)
        wx.EVT_MENU(guiPlaces.frame, PROJECT_SAVE, self.OnSave)
        wx.EVT_MENU(guiPlaces.frame, PROJECT_SAVE_AS, self.OnSaveAs)

  def active(self):
    i = self.tree.GetSelections()
    if i:
      i = i[0]
    else:
      i = self.tree.GetFirstVisibleItem()

    project = self.tree.GetItemPyData(i)
    if type(project) is TupleType: project = project[0]
      
    return project

  def latest(self):
    i = self.tree.GetLastChild(self.root)
    if i.IsOk():
       project = self.tree.GetItemPyData(i)
       if type(project) is TupleType: project = project[0]
       self.tree.ClearFocusedItem()
       self.tree.SetFocusedItem(i)
       return project
    return None

  def isPrjLoaded(self, prjPath):
     name = os.path.splitext(os.path.basename(prjPath))[0]
     (child, cookie) = self.tree.GetFirstChild(self.root)
     while child.IsOk():
       p = self.tree.GetItemPyData(child)
       if p.name == name:
         return True
       (child, cookie) = self.tree.GetNextChild(self.root, cookie)
     return False

  def fillDataModel(self, srcPath, destPath):
     #if not os.path.exists(srcPath):
     #   print 'model patch [%s] does not exist' % srcPath
     #   return False
     if os.path.exists(destPath) and srcPath==destPath:
        return True
     output = common.FilesystemOutput()
     with open(srcPath) as f:
        content = f.read()        
        output.write(destPath, content)
        f.close()
     return True          

  def populateGui(self,project,tree):

     il = wx.ImageList(15,15)
     self.dirIcon = il.Add(wx.ArtProvider.GetBitmap(wx.ART_FOLDER, wx.ART_OTHER, (15,15)))
     self.fldropenidx = il.Add(wx.ArtProvider.GetBitmap(wx.ART_FILE_OPEN,   wx.ART_OTHER, (15,15)))
     self.fileIcon = il.Add(wx.ArtProvider.GetBitmap(wx.ART_NORMAL_FILE, wx.ART_OTHER, (15,15)))
    
     self.tree.AssignImageList(il)

     self.pathProject = project.projectFilename
     projName = os.path.basename(self.pathProject)
     projName = os.path.splitext(projName)[0]
     prjT = self.tree.AppendItem(self.root, projName)
     self.setIconForItem(prjT)
     self.projects.append(prjT)
     self.tree.SetPyData(prjT, project)

    #  dmT = self.tree.AppendItem(prjT, "datamodel")
     for ch in project.datamodel.children():
       for c in ch.split():
         c = c.strip()
         fileT = self.tree.AppendItem(prjT, c)
         self.tree.SetPyData(fileT, (project,c))  # TODO more descriptive PY data 
         self.setIconForItem(fileT)
    #  mdlT = self.tree.AppendItem(prjT, "model")
     
     for ch in project.model.children():
       for c in ch.split():
         c = c.strip()
         fileT = self.tree.AppendItem(prjT,c)
         self.tree.SetPyData(fileT, (project,c))  # TODO more descriptive PY data   
         self.setIconForItem(fileT)
     srcT = self.tree.AppendItem(prjT, "src")
     self.setIconForItem(srcT, typeDir=True)
     
     for ch in project.src.children():
       for c in ch.split():
         c = c.strip()
         l = c.split('src/')[1]
         branchs = l.split('/')
         root = srcT

         pToBranch = "%s/src" % projName
         for branch in branchs:
           pToBranch += '/%s' % branch
          #  pToBranch = re.search( '%s/.*%s' % (projName,branch), c).group()
           item = self.getItemByLabel(self.tree, pToBranch, self.tree.GetRootItem())
           if item.IsOk():
             root = item
           else:
             current = self.tree.AppendItem(root, branch)
             self.setIconForItem(current)
             root = current
    #      fileT = self.tree.AppendItem(srcT,c)
         self.tree.SetPyData(fileT, (project,c))  # TODO more descriptive PY data   

  def setIconForItem(self, item, typeFile=False, typeDir=False):
    path = self.getFullPath(item)

    if os.path.isfile(path) or typeFile:
      self.tree.SetItemImage(item, self.fileIcon, wx.TreeItemIcon_Normal)
    if os.path.isdir(path) or typeDir:
      self.tree.SetItemImage(item, self.dirIcon, wx.TreeItemIcon_Normal)
      self.tree.SetItemImage(item, self.fldropenidx,wx.TreeItemIcon_Expanded)
 
  def getItemByLabel(self, tree, label, root):
      item, cookie = tree.GetFirstChild(root)

      while item.IsOk():
          text = self.getPathTree(item)
          if text is not None:
            if text.lower() == label.lower():
                return item

          if tree.ItemHasChildren(item):
              match = self.getItemByLabel(tree, label, item)
              if match.IsOk():
                  return match
          item, cookie = tree.GetNextChild(root, cookie)

      return wx.TreeItemId()

  def getPathTree(self, item):
    root = self.tree.GetRootItem()
    path = self.tree.GetItemText(item)

    while True:
        item = self.tree.GetItemParent(item)
        if item==root:
            break
        path = self.tree.GetItemText(item) + '/' + path
    return path
    
  def getFullPath(self, item):
    path = re.sub(self.getPrjName(), "", self.getPrjPath())
    path = path + self.getPathTree(item)

    return path

  def updateTreeItem(self, project, itemText, srcFiles):
     projName = os.path.basename(project.projectFilename)
     projName = os.path.splitext(projName)[0]
     (child, cookie) = self.tree.GetFirstChild(self.root)
     while child.IsOk():
       p = self.tree.GetItemPyData(child)
       if p.name == project.name:
         break
       (child, cookie) = self.tree.GetNextChild(self.root, cookie)
     if child.IsOk():
        (c, cookie) = self.tree.GetFirstChild(child)
        while (c.IsOk()):           
           print 'updateTreeItem: child explored [%s]' % self.tree.GetItemText(c)
           if self.tree.GetItemText(c) == itemText:
              break
           (c, cookie) = self.tree.GetNextChild(child, cookie)
     if c.IsOk():
        self.tree.DeleteChildren(c)
        project.updatePrjXmlSource(srcFiles)
        for f in srcFiles:
          f = f.strip()
          l = f.split('src/')[1]
          branchs = l.split('/')
          root = c

          pToBranch = "%s/src" % projName
          for branch in branchs:
            pToBranch += '/%s' % branch
            # pToBranch = re.search( '%s/.*%s' % (projName,branch), f).group()

            item = self.getItemByLabel(self.tree, pToBranch, self.tree.GetRootItem())
            if item.IsOk():
              root = item
            else:
              current = self.tree.AppendItem(root, branch)
              self.setIconForItem(current)
              root = current
          #  fileT = self.tree.AppendItem(c,f)
          #  self.tree.SetPyData(fileT, (project.name,f))         


  def OnLoad(self,event):
    dlg = wx.FileDialog(
            self, message="Choose a file",
            defaultDir=common.getMostRecentPrjDir(),
            defaultFile="",
            wildcard=PROJECT_WILDCARD,
            style=wx.OPEN | wx.CHANGE_DIR  # | wx.MULTIPLE
            )
    if dlg.ShowModal() == wx.ID_OK:
      paths = dlg.GetPaths()
      log.write('You selected %d files: %s' % (len(paths),str(paths))) 
      #self.tree.SetItemImage(self.root, fldridx, wx.TreeItemIcon_Normal)
      #self.tree.SetItemImage(self.root, fldropenidx, wx.TreeItemIcon_Expanded)
      for p in paths:
        if self.isPrjLoaded(p): return 
        project = Project(p)                 
        self.populateGui(project, self.root)
      evt = ProjectLoadedEvent(EVT_PROJECT_LOADED.evtType[0])
      wx.PostEvent(self.parent,evt)
      self.fileMenu.Enable(PROJECT_SAVE, True)
      self.fileMenu.Enable(PROJECT_SAVE_AS, True) 
      self.menuProject.Enable(PROJECT_VALIDATE, True)

  def OnNew(self,event):
    dlg = NewPrjDialog()
    dlg.ShowModal()
    dlg.Destroy()
    if dlg.what == "OK":
      print 'handling ok clicked'                   
      log.write('You selected %d files: %s; datamodel: %s' % (len(dlg.path),str(dlg.path), dlg.datamodel.GetTextCtrlValue()))       
      if not self.fillDataModel(dlg.datamodel.GetTextCtrlValue(), dlg.prjDir+os.sep+"SAFplusAmf.yang"):
        return
      project = Project()
      project.projectFilename = dlg.path
      project.new(dlg.datamodel.GetTextCtrlValue())
      self.populateGui(project, self.root)
      evt = ProjectNewEvent(EVT_PROJECT_NEW.evtType[0])
      wx.PostEvent(self.parent,evt)
      self.fileMenu.Enable(PROJECT_SAVE, True)
      self.fileMenu.Enable(PROJECT_SAVE_AS, True)
      self.menuProject.Enable(PROJECT_VALIDATE, True)

  def OnSave(self,event):
    saved = []
    self.currentActiveProject.save() 
    saved.append(self.currentActiveProject.name)
    #for p in self.projects:
    #  prj = self.tree.GetPyData(p)
    #  prj.save()
    #  saved.append(prj.name)
    self.guiPlaces.statusbar.SetStatusText("Projects %s saved." % ", ".join(saved),0);

    index = self.guiPlaces.frame.tab.GetSelection()
    curPage = self.guiPlaces.frame.tab.GetPage(index)
    if curPage in self.guiPlaces.frame.openFile.values():
      curPage.onSave()

  def OnSaveAs(self, event):
    dlg = SaveAsDialog()
    dlg.ShowModal()
    if dlg.what == "OK":
      print 'SaveAs: handling ok clicked'                   
      log.write('SaveAs: You selected %d files: %s' % (len(dlg.path),str(dlg.path)))

      if self.isPrjLoaded(dlg.path): return 
      project = Project()
      project.projectFilename = dlg.path
      currentActivePrj = self.latest()
      project.saveAs(currentActivePrj)
      self.guiPlaces.statusbar.SetStatusText("Saving as completed",0);

  def OnSize(self, event):
        w,h = self.GetClientSizeTuple()
        self.tree.SetDimensions(0, 0, w, h) 

  def OnValidate(self, event):
    # Clear old problems
    del self.problems[:]

    if share.detailsPanel is None:
      return
    share.detailsPanel.model.updateMicrodom()

    self.validateComponentModel()
    # Validate components relation ship
    self.parseComponentRelationships()

    self.validateNodeProfiles()
    # Validate intances 
    self.validateAmfConfig()

    # Validate configuration data
    self.validateDataConfig()

    # Update all problems on modal problems tag
    self.updateModalProblems()

  def getPrjPath(self):
    prjPath, name = os.path.split(self.pathProject)
    return prjPath

  def getPrjName(self):
    prjPath, name = os.path.split(self.pathProject)
    return os.path.splitext(name)[0]

  def validateAmfConfig(self):
    self.validateNodeIntances()
    self.validateServiceGroups()

  def updateProblems(self, level="", msg="", source=""):
    msgInfo = self.getMessageInfo(level, msg, source)
    self.problems.append(msgInfo)

  def validateDataConfig(self):
    '''
    @summary    : Validation data config on modelling and intances
    @param      : NA
    @return     : NA
    '''
    #TODO
    pass

  def validateNodeProfiles(self):
    '''
    @summary    : This function validation node/service child intance
    @param      : NA
    @return     : NA
    '''
    instances = share.detailsPanel.model.data.getElementsByTagName("instances")[0]
    
    # Validate Node child intance
    nNodeIntance = 0
    for (name,e) in share.detailsPanel.model.instances.items():
      if e.data['entityType'] == "Node":
        nNodeIntance += 1
        nSUIntance = 0
        for arrow in e.containmentArrows:
          if arrow.contained.data['entityType'] == "ServiceUnit":
            nSUIntance += 1
        if nSUIntance == 0:
          msg = "Node instance (%s) does not have any ServiceUnit instances defined" % name
          self.updateProblems(self.warn, msg, self.instantiation)

      if e.data['entityType'] == "ServiceUnit":
        nComponent = 0
        for arrow in e.containmentArrows:
          if arrow.contained.data['entityType'] == "Component":
            nComponent += 1
        if nComponent == 0:
          msg = "ServiceUnit instance (%s) does not have any Component instances defined" % name
          self.updateProblems(self.warn, msg, self.instantiation)

    if nNodeIntance == 0:
      msg = "There are no Node instances defined in AMF Configuration"
      self.updateProblems(self.warn, msg, self.instantiation)
      
    # Validate ServiceGroup child intance
    nServiceGroup = 0
    for (name,e) in share.detailsPanel.model.instances.items():
      if e.data['entityType'] == "ServiceGroup":
        nServiceGroup += 1
        nSI = 0
        for arrow in e.containmentArrows:
          if arrow.contained.data['entityType'] == "ServiceInstance":
            nSI += 1
        if nSI == 0:
          msg = "ServiceGroup instance (%s) does not have any ServiceInstance defined" % name
          self.updateProblems(self.warn, msg, self.instantiation)

      if e.data['entityType'] == "ServiceInstance":
        nCSI = 0
        for arrow in e.containmentArrows:
          if arrow.contained.data['entityType'] == "ComponentServiceInstance":
            nCSI += 1
        if nCSI == 0:
          msg = "SI (%s) does not have any ComponentServiceInstance defined" % name
          self.updateProblems(self.warn, msg, self.instantiation)



    if nServiceGroup == 0:
      msg = "There are no ServiceGroup instances defined in AMF Configuration"
      self.updateProblems(self.warn, msg, self.instantiation)

  def validateComponentModel(self):
    cntCluster = 0
    for (name, e) in share.detailsPanel.model.entities.items():
      if e.data['entityType'] == "Cluster":
        cntCluster += 1

    if cntCluster == 0:
      msg = "Model should contain at least one Cluster"
      self.updateProblems(self.error, msg, self.modelling)
    elif cntCluster > 1:
      msg = "Model should not contain more than one Cluster"
      self.updateProblems(self.error, msg, self.modelling)
    else:
      pass
    
    print "self.problems: %s" % self.problems

  def validateNodeIntances(self):
    '''
    @summary    : This function validation node intance
    @param      : NA
    @return     : NA
    '''
    nodeNameList = []
    suNameList = []
    cNameList = []

    for (name, e) in share.detailsPanel.model.entities.items():
      if e.data['entityType'] == "Node":
        nodeNameList.append(name)
      if e.data['entityType'] == "ServiceUnit":
        suNameList.append(name)
      if e.data['entityType'] == "Component":
        cNameList.append(name)

    instances = share.detailsPanel.model.data.getElementsByTagName("instances")[0]

    for (name,e) in share.detailsPanel.model.instances.items():
      # validate AMF configuration has valid Node
      if e.data['entityType'] == "Node":
        instance = instances.findOneByChild("name",name)
        nodeTypeName = instance.child_.get("NodeType").children_[0]
        print "nodeTypeName: %s" % nodeTypeName
        if nodeTypeName not in nodeNameList:
          msg = "AMF configuration has invalid Node " + str(nodeTypeName)
          self.updateProblems(self.error, msg, self.instantiation)

        # validate AMF configuration has valid SI
        for arrow in e.containmentArrows:
          print "arrow.contained.data: %s" % arrow.contained.data
          if arrow.contained.data['entityType'] == "ServiceUnit":
            suName = arrow.contained.data['name']
            instance = instances.findOneByChild("name",suName)
            suType = instance.child_.get("ServiceUnitType").children_[0]
            print "suType: %s" % suType
            if suType not in suNameList:
              msg = "AMF configuration has invalid SU " + str(suName)
              self.updateProblems(self.error, msg, self.instantiation)

            # validate AMF configuration has valid CSI
            for (nameEntity,entity) in share.detailsPanel.model.instances.items():
              if entity.data['entityType'] == "ServiceUnit" and nameEntity == suName:
                for arrowSu in entity.containmentArrows:
                   if arrowSu.contained.data['entityType'] == "Component":
                     cName = arrowSu.contained.data['name']
                     instance = instances.findOneByChild("name",cName)
                     cType = instance.child_.get("ComponentType").children_[0]
                     print "cType: %s" % cType
                     if cType not in cNameList:
                        msg = "AMF configuration has invalid Component " + str(cName)
                        self.updateProblems(self.error, msg, self.instantiation)

  def validateServiceGroups(self):
    '''
    @summary    : This function validation service group
    @param      : NA
    @return     : NA
    '''
    sgNameList = []
    siNameList = []
    csiNameList = []

    for (name, e) in share.detailsPanel.model.entities.items():
      if e.data['entityType'] == "ServiceInstance":
        siNameList.append(name)
      elif e.data['entityType'] == "ComponentServiceInstance":
        csiNameList.append(name)
      elif e.data['entityType'] == "ServiceGroup":
        sgNameList.append(name)

    instances = share.detailsPanel.model.data.getElementsByTagName("instances")[0]

    for (name,e) in share.detailsPanel.model.instances.items():

      # validate AMF configuration has valid SG
      if e.data['entityType'] == "ServiceGroup":
        instance = instances.findOneByChild("name",name)
        sgName = instance.child_.get("ServiceGroupType").children_[0]
        if sgName not in sgNameList:
          msg = "AMF configuration has invalid SG " + str(sgName)
          self.updateProblems(self.error, msg, self.instantiation)

        # validate AMF configuration has valid SI
        for arrow in e.containmentArrows:
          if arrow.contained.data['entityType'] == "ServiceInstance":
            siName = arrow.contained.data['name']
            instance = instances.findOneByChild("name",siName)
            siType = instance.child_.get("ServiceInstanceType").children_[0]
            if siType not in siNameList:
              msg = "AMF configuration has invalid SI " + str(siName)
              self.updateProblems(self.error, msg, self.instantiation)

            # validate AMF configuration has valid CSI
            for (nameEntity,entity) in share.detailsPanel.model.instances.items():
              if entity.data['entityType'] == "ServiceInstance" and nameEntity == siName:
                for arrowSi in entity.containmentArrows:
                   if arrowSi.contained.data['entityType'] == "ComponentServiceInstance":
                     csiName = arrowSi.contained.data['name']
                     instance = instances.findOneByChild("name",csiName)
                     csiType = instance.child_.get("ComponentServiceInstanceType").children_[0]
                     if csiType not in csiNameList:
                        msg = "AMF configuration has invalid CSI " + str(siName)
                        self.updateProblems(self.error, msg, self.instantiation)
 
  def parseComponentRelationships(self):
    '''
    @summary    : Validation entity has valid child and parent in modelling screen
    @param      : NA
    @return     : NA
    '''
    for (name, e) in share.detailsPanel.model.entities.items():
      eType = e.data["entityType"]
      self.validateParentOfComponent(name, eType)
      if self.entitiesRelation[eType] is not None:

        for link in self.entitiesRelation[eType]:
          result = False
          for arrow in e.containmentArrows:
            if arrow.contained.data["entityType"] == link:
              result = True
          if result is False:
            msg = "%s is not associated to any of the %ss" % (e.data["name"], link)
            self.updateProblems(self.error, msg, self.modelling)

  def validateParentOfComponent(self, cName, eType):
    '''
    @summary    : Validation entity has valid parent
    @param      : cName 
    @param      : eType 
    @return     : NA
    '''
    if self.entitiesParent[eType] is not None:
      for pType in self.entitiesParent[eType]:

        found = False
        for (name, e) in share.detailsPanel.model.entities.items():

          if e.data["entityType"] == pType:
            for arrow in e.containmentArrows:
              if arrow.contained.data["entityType"] == eType and arrow.contained.data["name"] == cName:
                found = True
                break
          if found:
            break

        if found is False:
          msg = "%s is not associated to any of the %s" % (cName, pType)
          self.updateProblems(self.error, msg, self.modelling)

  def getMessageInfo(self, level="", msg="", source=""):
    '''
    @summary    : Validation entity has valid parent
    @param      : level - failed level ERROR/WARN/INFOMATION
    @param      : msg - problem message
    @return     : problem - dict
    '''
    problem = {}
    problem["level"] = level
    # problem["number"] = number
    problem["msg"] = msg
    problem["source"] = source
    return problem

  def updateModalProblems(self):
    '''
    @summary    : Update problems view
    @param      : NA
    @param      : NA
    @return     : NA
    '''
    # sort problem by level
    problems = sorted(self.problems, key = lambda i: i['level']) 

    self.guiPlaces.frame.modelProblems.DeleteAllItems()
    for problem in problems:
      index = self.guiPlaces.frame.modelProblems.InsertStringItem(sys.maxsize, problem['level'])
      self.guiPlaces.frame.modelProblems.SetItemTextColour(index, self.color[problem['level']])
      # self.guiPlaces.frame.modelProblems.SetStringItem(index, 1, problem['number'])
      self.guiPlaces.frame.modelProblems.SetStringItem(index, 1, problem['msg'])
      self.guiPlaces.frame.modelProblems.SetStringItem(index, 2, problem['source'])

  def OnDeploy(self, event):
    dlg = DeployDialog(self)
    dlg.ShowModal()
    dlg.Destroy()

  def OnBuild(self, event):
    prjPath, name = os.path.split(self.currentActiveProject.projectFilename)
    srcPath = prjPath + '/src'
    os.system('cd %s; make' % srcPath)

  def OnMakeImages(self, event):
    dlg = MakeImages(self)
    dlg.ShowModal()
    dlg.Destroy

  def OnHelpConents(self, event):
    '''
    @summary    : Open help contents
    '''
    webbrowser.open('http://safplus7.openclovis.com/')

  def OnHelpAbout(self, event):
    '''
    @summary    : Show safplus information
    '''
    self.dlg = wx.Dialog(None, title="Deployment Details", size=(350,220))
    vBox = wx.BoxSizer(wx.VERTICAL)
    hBox = wx.BoxSizer(wx.HORIZONTAL)
    text = wx.StaticText(self.dlg, label="OpenClovis IDE")
    text2 = wx.StaticText(self.dlg, label="Version: 7.0")
    text3 = wx.StaticText(self.dlg, label="Build: 1.0")
    self.dlg.SetBackgroundColour('#FFFFFF')

    okBtn = wx.Button(self.dlg, label="OK")
    okBtn.Bind(wx.EVT_BUTTON, self.onClickOkCloseBtn)

    img = wx.EmptyImage(35, 35)
    imageCtrl = wx.StaticBitmap(self.dlg, wx.ID_ANY, wx.BitmapFromImage(img))
    imgPath = "resources/images/ClovisLogo_32_32.gif"
    img = wx.Image(imgPath, wx.BITMAP_TYPE_ANY)

    img = img.Scale(35,35)
    imageCtrl.SetBitmap(wx.BitmapFromImage(img))

    hBox.Add(imageCtrl, 0, wx.ALL|wx.CENTER, 5)
    hBox.Add(text, 0, wx.ALL|wx.CENTER, 5)
    vBox.Add(hBox, 0, wx.ALL|wx.ALIGN_LEFT, 5)
    vBox.Add(text2, 0, wx.LEFT, 50)
    vBox.Add(text3, 0, wx.LEFT, 50)
    vBox.Add(okBtn, 0, wx.TOP|wx.ALIGN_RIGHT, 65)

    self.dlg.SetSizer(vBox)
    self.dlg.ShowModal()
    self.dlg.Destroy()

  def onClickOkCloseBtn(self, event):
    self.dlg.Close()

class DeployDialog(wx.Dialog):
    """
    Class to define deploy images dialog
    """
    def __init__(self, parent):
      """Constructor"""
      wx.Dialog.__init__(self, None, title="Deployment Details", size=(600,450))

      self.parent = parent
      self.SetBackgroundColour('#F2F1EF')
      LabelSize = (100,25) 
      EntrySize = (300,25)

      horizontalBox0 = wx.BoxSizer(wx.HORIZONTAL)
      label = wx.StaticText(self, label="Please provide valid infomation to deploy image", size=(400, 25))
      label.SetForegroundColour((0,0,0)) 
      horizontalBox0.Add(label, 0, wx.ALL|wx.CENTER, border=5)

      horizontalBox1 = wx.BoxSizer(wx.HORIZONTAL)
      hostName = wx.StaticText(self, label="Target address", size=LabelSize)
      horizontalBox1.Add(hostName, 0, wx.ALL|wx.CENTER, 5)
      self.host = wx.TextCtrl(self,size=EntrySize)
      self.host.Bind(wx.EVT_TEXT, self.onTargetAdressChange)
      horizontalBox1.Add(self.host, 10, wx.ALL | wx.EXPAND, 5)

      horizontalBox2 = wx.BoxSizer(wx.HORIZONTAL)
      hostName = wx.StaticText(self, label="User name", size=LabelSize)
      horizontalBox2.Add(hostName, 0, wx.ALL|wx.CENTER, 5)
      self.user = wx.TextCtrl(self,size=EntrySize)
      self.user.Bind(wx.EVT_TEXT, self.onUserNameChange)
      horizontalBox2.Add(self.user, 10, wx.ALL | wx.EXPAND, 5)

      horizontalBox3 = wx.BoxSizer(wx.HORIZONTAL)
      hostName = wx.StaticText(self, label="Password", size=LabelSize)
      horizontalBox3.Add(hostName, 0, wx.ALL|wx.CENTER, 5)
      self.password = wx.TextCtrl(self,size=EntrySize, style=wx.TE_PASSWORD)
      self.password.Bind(wx.EVT_TEXT, self.onPasswordChange)
      horizontalBox3.Add(self.password, 10, wx.ALL | wx.EXPAND, 5)

      horizontalBox4 = wx.BoxSizer(wx.HORIZONTAL)
      hostName = wx.StaticText(self, label="Target location", size=LabelSize)
      horizontalBox4.Add(hostName, 0, wx.ALL|wx.CENTER, 5)
      self.targetLocation = wx.TextCtrl(self,size=EntrySize)
      self.targetLocation.Bind(wx.EVT_TEXT, self.onTargetLocationChange)
      horizontalBox4.Add(self.targetLocation, 10, wx.ALL | wx.EXPAND, 5)

      vBox = wx.BoxSizer(wx.VERTICAL)
      vBox.Add(horizontalBox0, 0, wx.ALL, 5)
      vBox.Add(horizontalBox1, 0, wx.ALL, 5)
      vBox.Add(horizontalBox2, 0, wx.ALL, 5)
      vBox.Add(horizontalBox3, 0, wx.ALL, 5)
      vBox.Add(horizontalBox4, 0, wx.ALL, 5)

      restore_btn = wx.Button(self, label="Restore Defaults")
      restore_btn.Bind(wx.EVT_BUTTON, self.onClickRestoreDefaultBtn)
      deploy_btn = wx.Button(self, label="Deploy")
      deploy_btn.Bind(wx.EVT_BUTTON, self.onDeployImageHandler)

      btn_sizer = wx.BoxSizer(wx.HORIZONTAL)
      btn_sizer.Add(restore_btn, 0, wx.ALL|wx.CENTER, 5)
      btn_sizer.Add(deploy_btn, 0, wx.ALL|wx.CENTER, 5)  
      vBox.Add(btn_sizer, 0, wx.TOP|wx.ALIGN_RIGHT, 5)

      cancel_btn = wx.Button(self, label="Cancel")
      cancel_btn.Bind(wx.EVT_BUTTON, self.onClickCancelBtn)
      deployAll_btn = wx.Button(self, label="Deploy All")
      deployAll_btn.Bind(wx.EVT_BUTTON, self.onClickDeployAllImageBtn)

      btn_sizer2 = wx.BoxSizer(wx.HORIZONTAL)
      btn_sizer2.Add(cancel_btn, 0, wx.ALL|wx.CENTER, 5)
      btn_sizer2.Add(deployAll_btn, 0, wx.ALL|wx.CENTER, 5)  

      self.nodeList = wx.ListView(self, wx.ID_ANY, (0,0), (185,350), style=wx.LC_EDIT_LABELS | wx.LC_REPORT |wx.SUNKEN_BORDER)#, style=wx.LC_REPORT|wx.SUNKEN_BORDER|wx.LC_SINGLE_SEL|wx.LC_VRULES|wx.LC_HRULES)
      self.nodeList.InsertColumn(0, "Nodes")
      self.nodeList.SetColumnWidth(0, 185)
      self.nodeList.Bind(wx.EVT_LIST_ITEM_FOCUSED, self.onSelectNodeChange)

      hBox = wx.BoxSizer(wx.HORIZONTAL)
      hBox.Add(self.nodeList, 0, wx.ALL, 5)
      hBox.Add(vBox, 0, wx.ALL, 5)

      vBox1 = wx.BoxSizer(wx.VERTICAL)
      vBox1.Add(hBox, 0, wx.ALL|wx.CENTER, 5)
      vBox1.Add(btn_sizer2, 0, wx.ALL|wx.ALIGN_RIGHT, 5)

      self.SetSizer(vBox1)
      vBox1.Layout()

      self.deployInfos = {}

      self.initDeploymentInfo()

      self.Bind(wx.EVT_CLOSE, self.onDialogClose)

      self.infoChange = False

    def initDeploymentInfo(self):
      nodeIntances = []
      try:
        for (name, e) in share.detailsPanel.model.instances.items():
          if e.data['entityType'] == "Node":
            # self.nodeList.InsertStringItem(sys.maxsize, name)
            nodeIntances.append(name)
        targetInfo = microdom.LoadFile("%s/target.xml" % self.parent.getPrjPath()).pretty()

        dom = xml.dom.minidom.parseString(targetInfo)
        md = microdom.LoadMiniDom(dom.childNodes[0])
      except:
        pass

      nodeIntances = sorted(nodeIntances)
      self.curNode = nodeIntances[0]

      for img in nodeIntances:
        image = {}
        try:
          image['targetAdress'] = str(md[img].targetAdress.data_).strip()
          image['userName'] = str(md[img].userName.data_).strip()
          image['password'] = str(md[img].password.data_).strip()
          image['targetLocation'] = str(md[img].targetLocation.data_).strip()
        except:
          image['targetAdress'] = ""
          image['userName'] = ""
          image['password'] = ""
          image['targetLocation'] = ""
          self.infoChange = True
        self.nodeList.InsertStringItem(sys.maxsize, img)
        self.deployInfos[img] = image

      self.updateDeployInfoCurNode()

    def updateDeployInfoCurNode(self):
      # Set old config for current Node
      self.host.SetValue(self.deployInfos[self.curNode]['targetAdress'])
      self.user.SetValue(self.deployInfos[self.curNode]['userName'])
      self.password.SetValue(self.deployInfos[self.curNode]['password'])
      self.targetLocation.SetValue(self.deployInfos[self.curNode]['targetLocation'])

    def onTargetAdressChange(self, event):
      self.deployInfos[self.curNode]['targetAdress'] = self.host.GetValue()
      self.infoChange = True

    def onUserNameChange(self, event):
      self.deployInfos[self.curNode]['userName'] = self.user.GetValue()
      self.infoChange = True

    def onPasswordChange(self, event):
      self.deployInfos[self.curNode]['password'] = self.password.GetValue()
      self.infoChange = True

    def onTargetLocationChange(self, event):
      self.deployInfos[self.curNode]['targetLocation'] = self.targetLocation.GetValue()
      self.infoChange = True

    def onSaveAllDeploymentInfo(self):
      md = microdom.MicroDom({"tag_":"Nodes"},[],[])
      md.update(self.deployInfos)
      f = open("%s/target.xml" % self.parent.getPrjPath(),"w")
      f.write(md.pretty())
      f.close()

    def onClickCancelBtn(self, event):
      self.Close()

    def onClickDeployAllImageBtn(self, event):
      prjPath, name = os.path.split(self.parent.currentActiveProject.projectFilename)
      for key in self.deployInfos:
        #TODO update srcImage
        srcImage = prjPath + '/images/' + key
        if os.path.isdir(srcImage):
          self.deploymentSingleImage(self.deployInfos[key], srcImage)
        else:
          print "Image don't exist - fail"   

    def deploymentSingleImage(self, info, srcImage):
      if (info['targetAdress'] == "") or (info['userName'] == "") or (info['password'] == "") or (info['targetLocation'] == ""):
        print "Invalid infomation to deploy image"
        return False
      cmd = 'scp -r %s %s@%s:~/%s' % (srcImage, info['userName'], info['targetAdress'], info['targetLocation'])
      print cmd
      try:
        child = pexpect.spawn("%s" % cmd)
        child.expect('assword:*')
        child.sendline('%s' % info['password'])
        cnt = 0
        timeout = 60
        while child.isalive() and cnt < timeout :
          cnt += 1
          time.sleep(1)
        if cnt < timeout:
          print "Deploy image successfuly"
        else:
          print "Deploy image longer than %s(s) - fail" % timeout
      except:
        print "Error while deploy image"

    
    def onClickRestoreDefaultBtn(self, event):
      self.host.SetValue("")
      self.user.SetValue("")
      self.password.SetValue("")
      self.targetLocation.SetValue("")

    def onDeployImageHandler(self, event):
      #TODO update 
      prjPath, name = os.path.split(self.parent.currentActiveProject.projectFilename)
      srcImage = prjPath + '/images/' + self.curNode

      # srcImage = "/home/tuyen.nguyen/Desktop/tmp"
      if os.path.isdir(srcImage):
        img = self.deployInfos[self.curNode]
        self.deploymentSingleImage(img, srcImage)
      else:
        print "Image don't exist - fail"

    def onSelectNodeChange(self, event):
      item = self.nodeList.GetFocusedItem()
      if item == -1:
        return None
      self.curNode = self.nodeList.GetItemText(item)
      self.updateDeployInfoCurNode()

    def onDialogClose(self, event):
      if self.infoChange:
        self.onSaveAllDeploymentInfo()
      self.Destroy()

class MakeImages(wx.Dialog):
    """
    Class define makeimages dialog
    """
    def __init__(self, parent):
      wx.Dialog.__init__(self, None, title="Make Images Configuration", size=(455, 480))
      self.parent = parent
      mainBox = wx.BoxSizer(wx.VERTICAL)
      labelTitle = wx.StaticText(self, label="Config make image setting",size=(455,25))
      bookStyle = aui.AUI_NB_DEFAULT_STYLE
      bookStyle &= ~(aui.AUI_NB_CLOSE_ON_ACTIVE_TAB)
      self.configPanel = wx.aui.AuiNotebook(self, style=bookStyle, size=(436, 341))
      self.general = GeneralPage(self)
      self.configPanel.AddPage(self.general, "General")

      cancel_btn = wx.Button(self, label="Cancel")
      cancel_btn.Bind(wx.EVT_BUTTON, self.onClickCancelBtn)
      ok_btn = wx.Button(self, label="Ok")
      ok_btn.Bind(wx.EVT_BUTTON, self.onClickOkBtn)

      btn_sizer = wx.BoxSizer(wx.HORIZONTAL)
      btn_sizer.Add(cancel_btn, 0, wx.ALL|wx.CENTER, 5)
      btn_sizer.Add(ok_btn, 0, wx.ALL|wx.CENTER, 5)  

        
      mainBox.Add(labelTitle, 0, wx.TOP|wx.LEFT, 20)
      mainBox.Add(self.configPanel, 0, wx.ALL|wx.CENTER, 5)
      mainBox.Add(btn_sizer, 0, wx.TOP|wx.ALIGN_RIGHT,5)

      self.SetSizer(mainBox)
      mainBox.Layout()
      
    def onClickOkBtn(self, event):
      self.general.getAllRawConf()
      self.general.saveAllImgConfig()
      prjPath, name = os.path.split(self.parent.currentActiveProject.projectFilename)
      self.general.makeImages(prjPath)
      self.Close()

    def onClickCancelBtn(self, event):
      self.Close()

class RawInfo(wx.Panel):
  """
  Class to define general page
  """
  def __init__(self, parent, node):
    """Constructor"""
    wx.Panel.__init__(self, parent, size=(415, 25))
    hBox = wx.BoxSizer(wx.HORIZONTAL)

    self.txtName = wx.StaticText(self, label=node['name'], size=(130,25))
    s = [str(i) for i in range (1,17)]
    self.num = wx.ComboBox(self, choices = s, size=(110,25))
    self.num.SetValue(node['slot'])
    self.net = wx.TextCtrl(self, size=(150,25))
    self.net.SetValue(node['netInterface'])

    hBox.Add(self.txtName, 0, wx.ALL|wx.CENTER, border=5, )
    hBox.Add(self.num, 0, wx.ALL|wx.CENTER, border=5)
    hBox.Add(self.net, 0, wx.ALL|wx.CENTER, border=5)

    self.SetSizer(hBox)
    hBox.Layout()

  def getNodeName(self):
    return self.txtName.GetLabelText()

  def getSlotValue(self):
    return self.num.GetValue()

  def getNetInterfaceValue(self):
    return self.net.GetValue()

class GeneralPage(scrolled.ScrolledPanel):
  """
  Class to define general page
  """
  #----------------------------------------------------------------------
  def __init__(self, parent):
    """Constructor"""
    scrolled.ScrolledPanel.__init__(self, parent, style = wx.TAB_TRAVERSAL|wx.SUNKEN_BORDER, size=(425,310))
    self.parent = parent
    self.SetupScrolling(True, True)
    self.SetScrollRate(10, 10)
    self.SetBackgroundColour('#F2F1EF')

    hBox = wx.BoxSizer(wx.HORIZONTAL)
    intanceName = wx.StaticText(self, label="Node Instance", size=(130,25))
    slotNum = wx.StaticText(self, label="Slot Number", size=(110,25))
    netInterface = wx.StaticText(self, label="Network Interface", size=(150,25))
    hBox.Add(intanceName, 0, wx.ALL|wx.ALIGN_TOP, 5)
    hBox.Add(slotNum, 0, wx.ALL|wx.ALIGN_TOP, 5)
    hBox.Add(netInterface, 0, wx.ALL|wx.ALIGN_TOP, 5)

    mainBox = wx.BoxSizer(wx.VERTICAL)
    mainBox.Add(hBox, 0, wx.ALL|wx.ALIGN_TOP, 5)

    nodeIntances = []
    self.imagesConfig = {}
    try:
      for (name, e) in share.detailsPanel.model.instances.items():
        if e.data['entityType'] == "Node":
          nodeIntances.append(name)

      targetInfo = microdom.LoadFile("%s/imagesConfig.xml" % self.parent.parent.getPrjPath()).pretty()

      dom = xml.dom.minidom.parseString(targetInfo)
      md = microdom.LoadMiniDom(dom.childNodes[0])
    except:
      pass

    for img in nodeIntances:
      image = {}
      image['name'] = img
      try:
        image['slot'] = str(md[img].slot.data_).strip()
        image['netInterface'] = str(md[img].netInterface.data_).strip()
      except:
        image['slot'] = ""
        image['netInterface'] = ""
        self.infoChange = True
      self.imagesConfig[img] = image

    self.listRawConf = []
    for name in sorted(self.imagesConfig.keys()):
      raw = RawInfo(self, self.imagesConfig[name])
      self.listRawConf.append(raw)
      mainBox.Add(raw, 0, wx.ALL|wx.TOP, 5)

    self.SetSizer(mainBox)
    mainBox.Layout()

  def getAllRawConf(self):
    for raw in self.listRawConf:
      self.imagesConfig[raw.getNodeName()]['slot'] = raw.getSlotValue()
      self.imagesConfig[raw.getNodeName()]['netInterface'] = raw.getNetInterfaceValue()
      # print "Slot value: %s" % raw.getSlotValue()
      # print "Net interface value: %s" % raw.getNetInterfaceValue()
    print "Var config:%s" % self.imagesConfig

  def saveAllImgConfig(self):
    md = microdom.MicroDom({"tag_":"Nodes"},[],[])
    md.update(self.imagesConfig)
    f = open("%s/imagesConfig.xml" % self.parent.parent.getPrjPath(),"w")
    f.write(md.pretty())
    f.close()

  def makeImages(self, prjPath):
    tarGet = str(subprocess.check_output(['g++','-dumpmachine'])).strip()
    baseImage = prjPath + '/images/%s' % tarGet
    owd = os.getcwd()

    os.chdir('../mk')
    tool = os.path.abspath("safplus_packager.py")

    os.chdir('%s' % prjPath)
    os.system('python %s -a %s %s.tgz' % (tool, tarGet, tarGet))
    os.system('cp *.xml %s/bin' % baseImage)
    print 'python %s -a %s' % (tool, tarGet)

    os.chdir(owd)
    os.system('cp resources/setup %s/bin' % baseImage)
    os.system('rm -rf images')
    if os.path.isdir(baseImage):
      for img in self.imagesConfig:
        tarImg = prjPath + '/images/' + img
        # copy_tree(baseImage, tarImg)
        os.system('cp -r %s %s' % (baseImage, tarImg))
        os.system('cd %s/images/; tar -zcvf %s.tar.gz %s' % (prjPath, img, img))
        self.updateImageConfig(tarImg, self.imagesConfig[img])

  def updateImageConfig(self, tarGet, imgConf):
    fConf = tarGet + '/bin/setup'
    s = open(fConf).read()
    s = s.replace('ASP_NODENAME=node0', 'ASP_NODENAME=%s' % imgConf['name'] )
    s = s.replace('SAFPLUS_BACKPLANE_INTERFACE=eth0', 'SAFPLUS_BACKPLANE_INTERFACE=%s' % imgConf['netInterface'] )
    f = open(fConf, 'w')
    f.write(s)
    f.close()
      
class NewPrjDialog(wx.Dialog):
    """
    Class to define new prj dialog
    """
 
    #----------------------------------------------------------------------
    def __init__(self):
        """Constructor"""
        wx.Dialog.__init__(self, None, title="New project", size=(430,240))
        LabelSize = (100,25) 
        EntrySize = (300,25)
        prjname_sizer = wx.BoxSizer(wx.HORIZONTAL)
 
        prj_lbl = wx.StaticText(self, label="Project name", size=LabelSize)
        prjname_sizer.Add(prj_lbl, 0, wx.ALL|wx.CENTER, 5)
        self.prjName = wx.TextCtrl(self,size=EntrySize)
        prjname_sizer.Add(self.prjName, 10, wx.ALL | wx.EXPAND, 5)

        prjlocation_sizer = wx.BoxSizer(wx.HORIZONTAL)
        prj_location_lbl = wx.StaticText(self, label="Location", size=LabelSize)
        prjlocation_sizer.Add(prj_location_lbl, 0, wx.ALL|wx.CENTER, 5)
        
        self.prjLocation = wx.DirPickerCtrl(self, message="Choose a project location", style=wx.DIRP_DEFAULT_STYLE|wx.DIRP_USE_TEXTCTRL,size=EntrySize) 
        self.prjLocation.SetPath(common.getMostRecentPrjDir())
        prjlocation_sizer.Add(self.prjLocation, 0, wx.ALL, 5)
 
        datamodel_sizer = wx.BoxSizer(wx.HORIZONTAL)
        datamodel_lbl = wx.StaticText(self, label="Data model", size=LabelSize)
        datamodel_sizer.Add(datamodel_lbl, 0, wx.ALL|wx.CENTER, 5)
        
        defaultModel = common.fileResolver("SAFplusAmf.yang")
        self.datamodel = wx.FilePickerCtrl(self, message="Choose a file", path="SAFplusAmf.yang", style=wx.FLP_DEFAULT_STYLE|wx.FLP_USE_TEXTCTRL,  wildcard="Data model (*.yang)|*.yang", size=EntrySize)
        datamodel_sizer.Add(self.datamodel, 0, wx.ALL, 5)
 
        main_sizer = wx.BoxSizer(wx.VERTICAL)
        main_sizer.Add(prjname_sizer, 0, wx.ALL, 5)
        main_sizer.Add(prjlocation_sizer, 0, wx.ALL, 5)
        main_sizer.Add(datamodel_sizer, 0, wx.ALL, 5)
 
        OK_btn = wx.Button(self, label="OK")
        OK_btn.Bind(wx.EVT_BUTTON, self.onBtnHandler)
        cancel_btn = wx.Button(self, label="Cancel")
        cancel_btn.Bind(wx.EVT_BUTTON, self.onBtnHandler)
  
        btn_sizer = wx.BoxSizer(wx.HORIZONTAL)
        btn_sizer.Add(OK_btn, 0, wx.ALL|wx.CENTER, 5)
        btn_sizer.Add(cancel_btn, 0, wx.ALL|wx.CENTER, 5)  
              
        main_sizer.Add(btn_sizer, 0, wx.ALL|wx.CENTER, 5)

        self.SetSizer(main_sizer)
        main_sizer.Layout()
        prjname_sizer.Layout()
 
    #----------------------------------------------------------------------
    def onBtnHandler(self, event):
        what = event.GetEventObject().GetLabel()
        print 'about to %s' % what  
        if (what == "OK"):
           prjName = self.prjName.GetValue()
           if len(prjName)==0:
              msgBox = wx.MessageDialog(self, "Project name is missing. Please specify a new project", style=wx.OK|wx.CENTRE)
              msgBox.ShowModal()
              msgBox.Destroy()
              return     

           self.prjDir = self.prjLocation.GetTextCtrlValue()+os.sep
           if not self.prjLocation.GetTextCtrlValue() or not os.path.exists(self.prjDir):
              msgBox = wx.MessageDialog(self, "Project location is missing or does not exist. Please choose a location for the new project", style=wx.OK|wx.CENTRE)
              msgBox.ShowModal()
              msgBox.Destroy()
              return

           t = self.datamodel.GetTextCtrlValue()
           if not t or not os.path.exists(t) or not re.search('.yang$', t):
              msgBox = wx.MessageDialog(self, "Data model is missing or does not exist. Please choose a valid data model for the new project", style=wx.OK|wx.CENTRE)
              msgBox.ShowModal()
              msgBox.Destroy()
              return           
              
           pos = prjName.rfind('.spp')
           if pos<0:
              self.prjDir+=prjName
              self.path = self.prjDir+os.sep+prjName+'.spp'              
           else:              
              self.prjDir+= prjName[0:pos]
              self.path=self.prjDir+os.sep+prjName
           if not os.path.exists(self.prjDir):
              os.makedirs(self.prjDir)
           if os.path.exists(self.path):
              msgBox = wx.MessageDialog(self, "Project exists. Please specify another name", style=wx.OK|wx.CENTRE)
              msgBox.ShowModal()
              msgBox.Destroy()
              return
        self.what = what
        self.Close()
    
    

class SaveAsDialog(wx.Dialog):
    """
    Class to define new prj dialog
    """
 
    #----------------------------------------------------------------------
    def __init__(self):
        """Constructor"""
        wx.Dialog.__init__(self, None, title="Save project as...", size=(430,200))
         
        prjname_sizer = wx.BoxSizer(wx.HORIZONTAL)
 
        prj_lbl = wx.StaticText(self, label="Project name", size=(100,25))
        prjname_sizer.Add(prj_lbl, 0, wx.ALL|wx.CENTER, 5)
        self.prjName = wx.TextCtrl(self)
        prjname_sizer.Add(self.prjName, 0, wx.ALL, 5)

        prjlocation_sizer = wx.BoxSizer(wx.HORIZONTAL)
        prj_location_lbl = wx.StaticText(self, label="Location", size=(100,25))
        prjlocation_sizer.Add(prj_location_lbl, 0, wx.ALL|wx.CENTER, 5)
        
        self.prjLocation = wx.DirPickerCtrl(self, message="Choose a project location", style=wx.DIRP_DEFAULT_STYLE|wx.DIRP_USE_TEXTCTRL,size=(300,25)) 
        self.prjLocation.SetPath(common.getMostRecentPrjDir())
        prjlocation_sizer.Add(self.prjLocation, 0, wx.ALL, 5)

 
        main_sizer = wx.BoxSizer(wx.VERTICAL)
        main_sizer.Add(prjname_sizer, 0, wx.ALL, 5)
        main_sizer.Add(prjlocation_sizer, 0, wx.ALL, 5)
         
        OK_btn = wx.Button(self, label="OK")
        OK_btn.Bind(wx.EVT_BUTTON, self.onBtnHandler)
        cancel_btn = wx.Button(self, label="Cancel")
        cancel_btn.Bind(wx.EVT_BUTTON, self.onBtnHandler)
  
        btn_sizer = wx.BoxSizer(wx.HORIZONTAL)
        btn_sizer.Add(OK_btn, 0, wx.ALL|wx.CENTER, 5)
        btn_sizer.Add(cancel_btn, 0, wx.ALL|wx.CENTER, 5)  
              
        main_sizer.Add(btn_sizer, 0, wx.ALL|wx.CENTER, 5)

        self.SetSizer(main_sizer)
 
    #----------------------------------------------------------------------
    def onBtnHandler(self, event):
        what = event.GetEventObject().GetLabel()
        print 'about to %s' % what  
        if (what == "OK"):
           prjName = self.prjName.GetValue()
           if len(prjName)==0:
              msgBox = wx.MessageDialog(self, "Project name is missing. Please specify a new project", style=wx.OK|wx.CENTRE)
              msgBox.ShowModal()
              msgBox.Destroy()
              return     

           self.prjDir = self.prjLocation.GetTextCtrlValue()+os.sep
           if not self.prjLocation.GetTextCtrlValue() or not os.path.exists(self.prjDir):
              msgBox = wx.MessageDialog(self, "Project location is missing or does not exist. Please choose a location for the new project", style=wx.OK|wx.CENTRE)
              msgBox.ShowModal()
              msgBox.Destroy()
              return                              
           pos = prjName.rfind('.spp')
           if pos<0:
              self.prjDir+=prjName
              self.path = self.prjDir+os.sep+prjName+'.spp'              
           else:              
              self.prjDir+= prjName[0:pos]
              self.path=self.prjDir+os.sep+prjName
           if not os.path.exists(self.prjDir):
              os.makedirs(self.prjDir)
           if os.path.exists(self.path):
              msgBox = wx.MessageDialog(self, "Project exists. Please specify another name", style=wx.OK|wx.CENTRE)
              msgBox.ShowModal()
              msgBox.Destroy()
              return
        self.what = what
        self.Close()

    def onBrowse(self, event):
        print 'about to browse project location'        
        dlg = wx.DirDialog(
            self, message="Choose a project location",
            defaultPath=common.getMostRecentPrjDir(),             
            style=wx.DD_DEFAULT_STYLE | wx.DD_CHANGE_DIR
            )
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            print 'You selected %d path: %s' % (len(path),str(path))
            self.prjLocationName.SetValue(path)

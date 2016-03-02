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

PROJECT_LOAD = wx.NewId()
PROJECT_SAVE = wx.NewId()
PROJECT_SAVE_AS = wx.NewId()

PROJECT_WILDCARD = "SAFplus Project (*.spp)|*.spp|All files (*.*)|*.*"

ProjectLoadedEvent, EVT_PROJECT_LOADED = wx.lib.newevent.NewCommandEvent()  # Must be a command event so it propagates
ProjectNewEvent, EVT_PROJECT_NEW = wx.lib.newevent.NewCommandEvent()
ProjectSaveAsEvent, EVT_PROJECT_SAVE_AS = wx.lib.newevent.NewCommandEvent()

class Project(microdom.MicroDom):
  def __init__(self, filename=None):
    self.dirty = False
    self._safplusModel = None
    self.datamodel = None
    self.name = ""    
    self.prjXmlData = None
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

    sourceElem = dom.createElement("source")
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
    sources = self.prjXmlData.getElementsByTagName("source")    
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
    sources = self.prjXmlData.getElementsByTagName("source")    
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
        # Use the WANTS_CHARS style so the panel doesn't eat the Return key.
        wx.Panel.__init__(self, parent, -1, style=wx.WANTS_CHARS)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        tID = wx.NewId()
        self.guiPlaces = guiPlaces
        self.projects = []

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
        

        # bind the menu event to an event handler
        self.fileMenu.Bind(wx.EVT_MENU, self.OnNew, id=wx.ID_NEW)
        self.fileMenu.Bind(wx.EVT_MENU, self.OnLoad, id=PROJECT_LOAD)
        self.fileMenu.Bind(wx.EVT_MENU, self.OnSave, id=PROJECT_SAVE)
        self.fileMenu.Bind(wx.EVT_MENU, self.OnSaveAs, id=PROJECT_SAVE_AS)
        
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
     p = project.projectFilename
     projName = os.path.basename(p)
     prjT = self.tree.AppendItem(self.root, projName)
     self.projects.append(prjT)
     self.tree.SetPyData(prjT, project)
     dmT = self.tree.AppendItem(prjT, "datamodel")
     for ch in project.datamodel.children():
       for c in ch.split():
         c = c.strip()
         fileT = self.tree.AppendItem(dmT, c)
         self.tree.SetPyData(fileT, (project,c))  # TODO more descriptive PY data   
     mdlT = self.tree.AppendItem(prjT, "model")
     
     for ch in project.model.children():
       for c in ch.split():
         c = c.strip()
         fileT = self.tree.AppendItem(mdlT,c)
         self.tree.SetPyData(fileT, (project,c))  # TODO more descriptive PY data   
     srcT = self.tree.AppendItem(prjT, "source")
     
     for ch in project.source.children():
       for c in ch.split():
         c = c.strip()
         fileT = self.tree.AppendItem(srcT,c)
         self.tree.SetPyData(fileT, (project,c))  # TODO more descriptive PY data   
 
     # TODO: add children under subdirectories    
  def updateTreeItem(self, project, itemText, srcFiles):
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
           fileT = self.tree.AppendItem(c,f)
           self.tree.SetPyData(fileT, (project.name,f))         


  def OnLoad(self,event):
    dlg = wx.FileDialog(
            self, message="Choose a file",
            defaultDir=os.getcwd(), 
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

  def OnNew(self,event):
    dlg = NewPrjDialog()
    dlg.ShowModal()
    if dlg.what == "OK":
      print 'handling ok clicked'                   
      log.write('You selected %d files: %s; datamodel: %s' % (len(dlg.path),str(dlg.path), dlg.datamodel.GetValue()))       
      if not self.fillDataModel(dlg.datamodel.GetValue(), dlg.prjDir+os.sep+"SAFplusAmf.yang"):
        return
      project = Project()
      project.projectFilename = dlg.path
      project.new(dlg.datamodel.GetValue())
      self.populateGui(project, self.root)
      evt = ProjectNewEvent(EVT_PROJECT_NEW.evtType[0])
      wx.PostEvent(self.parent,evt)
      self.fileMenu.Enable(PROJECT_SAVE, True)
      self.fileMenu.Enable(PROJECT_SAVE_AS, True)

  def OnSave(self,event):
    saved = []
    for p in self.projects:
      prj = self.tree.GetPyData(p)
      prj.save()
      saved.append(prj.name)
    self.guiPlaces.statusbar.SetStatusText("Projects %s saved." % ", ".join(saved),0);

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

class NewPrjDialog(wx.Dialog):
    """
    Class to define new prj dialog
    """
 
    #----------------------------------------------------------------------
    def __init__(self):
        """Constructor"""
        wx.Dialog.__init__(self, None, title="New project", size=(430,240))
         
        prjname_sizer = wx.BoxSizer(wx.HORIZONTAL)
 
        prj_lbl = wx.StaticText(self, label="Project name", size=(100,25))
        prjname_sizer.Add(prj_lbl, 0, wx.ALL|wx.CENTER, 5)
        self.prjName = wx.TextCtrl(self)
        prjname_sizer.Add(self.prjName, 0, wx.ALL, 5)

        prjlocation_sizer = wx.BoxSizer(wx.HORIZONTAL)
        prj_location_lbl = wx.StaticText(self, label="Location", size=(100,25))
        prjlocation_sizer.Add(prj_location_lbl, 0, wx.ALL|wx.CENTER, 5)
        self.prjLocationName = wx.TextCtrl(self, size=(200, 25))
        self.prjLocationName.SetValue(common.getMostRecentPrjDir())
        prjlocation_sizer.Add(self.prjLocationName, 0, wx.ALL, 5)
        prj_location_btn = wx.Button(self, label="Browse")
        prj_location_btn.Bind(wx.EVT_BUTTON, self.onBrowse)
        prjlocation_sizer.Add(prj_location_btn, 0, wx.ALL, 5)
 
        datamodel_sizer = wx.BoxSizer(wx.HORIZONTAL)
 
        datamodel_lbl = wx.StaticText(self, label="Data model", size=(100,25))
        datamodel_sizer.Add(datamodel_lbl, 0, wx.ALL|wx.CENTER, 5)
        self.datamodel = wx.TextCtrl(self, size=(200, 25))
        datamodel_sizer.Add(self.datamodel, 0, wx.ALL, 5)
        select_btn = wx.Button(self, label="Browse")
        select_btn.Bind(wx.EVT_BUTTON, self.onSelect)
        datamodel_sizer.Add(select_btn, 0, wx.ALL, 5)
 
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
           self.prjDir = self.prjLocationName.GetValue()+os.sep
           if not self.prjLocationName.GetValue() or not os.path.exists(self.prjDir):
              msgBox = wx.MessageDialog(self, "Project location is missing or does not exist. Please choose a location for the new project", style=wx.OK|wx.CENTRE)
              msgBox.ShowModal()
              msgBox.Destroy()
              return
           t = self.datamodel.GetValue()
           if not t or not os.path.exists(t) or not re.search('.yang$', t):
              msgBox = wx.MessageDialog(self, "Data model is missing or does not exist. Please choose a valid data model for the new project", style=wx.OK|wx.CENTRE)
              msgBox.ShowModal()
              msgBox.Destroy()
              return           
           #self.path = os.getcwd()+os.sep+prjName                      
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
    
    def onSelect(self, event):
        print 'about to select'
        dlg = wx.FileDialog(
            self, message="Choose a file",
            defaultDir=os.getcwd(), 
            defaultFile="",
            wildcard="Data model (*.yang)|*.yang",
            style=wx.OPEN | wx.CHANGE_DIR
            )
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            print 'You selected %d files: %s' % (len(path),str(path))
            self.datamodel.SetValue(path)

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
        self.prjLocationName = wx.TextCtrl(self, size=(200, 25))
        self.prjLocationName.SetValue(common.getMostRecentPrjDir())
        prjlocation_sizer.Add(self.prjLocationName, 0, wx.ALL, 5)
        prj_location_btn = wx.Button(self, label="Browse")
        prj_location_btn.Bind(wx.EVT_BUTTON, self.onBrowse)
        prjlocation_sizer.Add(prj_location_btn, 0, wx.ALL, 5)
 
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
           self.prjDir = self.prjLocationName.GetValue()+os.sep
           if not self.prjLocationName.GetValue() or not os.path.exists(self.prjDir):
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

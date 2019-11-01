import os
import os.path
import pdb
import math
import time
from types import *
import threading
import paramiko
import pickle
from cryptography.fernet import Fernet
import shutil
import string

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
import style_dialog
import styles
from progressdialog import ProgressDialog
import texts
import umlEditor

PROJECT_LOAD = wx.NewId()
PROJECT_SAVE = wx.NewId()
PROJECT_SAVE_AS = wx.NewId()
PROJECT_SAVE_ALL = wx.NewId()

PROJECT_WILDCARD = "SAFplus Project (*.spp)|*.spp|All files (*.*)|*.*"

EDIT_UNDO         = wx.NewId()
EDIT_REDO         = wx.NewId()
EDIT_RELOAD       = wx.NewId()
EDIT_CUT          = wx.NewId()
EDIT_COPY         = wx.NewId()
EDIT_PATSE        = wx.NewId()
EDIT_FIND         = wx.NewId()
EDIT_REPLACE      = wx.NewId()
EDIT_TOGGLE_LINE_COMMENT  = wx.NewId()
EDIT_TOGGLE_BLOCK_COMMENT = wx.NewId()
EDIT_SORT                 = wx.NewId()
EDIT_LOWERCASE            = wx.NewId()
EDIT_UPPERCASE            = wx.NewId()

GO_TO_LINE       = wx.NewId()

PROJECT_VALIDATE = wx.NewId()
PROJECT_OPEN     = wx.NewId()
PROJECT_CLOSE    = wx.NewId()
PROJECT_BUILD = wx.NewId()
PROJECT_CLEAN = wx.NewId()
MAKE_IMAGES      = wx.NewId() 
IMAGES_DEPLOY    = wx.NewId()
PROJECT_PROPERTIES = wx.NewId()

TOOL_FONT_COLOR  = wx.NewId()
TOOL_CLEAR_PROJECT_DATA = wx.NewId()

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
    # self.log.write('compare: ' + t1 + ' <> ' + t2 + '\n')
    item1IsDir = os.path.isdir(self.parent.getFullPath(item1))
    item2IsDir = os.path.isdir(self.parent.getFullPath(item2))
    if item1IsDir and not item2IsDir:
        return -1
    elif not item1IsDir and item2IsDir:
        return 1
    if t1 < t2: return -1
    if t1 == t2: return 0
    return 1

class RenameDialog(wx.Dialog):
    """
    Class to define RenameDialog
    """
    def __init__(self, parent, fileName):
        """Constructor"""
        wx.Dialog.__init__(self, None, wx.ID_ANY, fileName[2],size= (336,165), style=wx.DEFAULT_DIALOG_STYLE)
        self.parent = parent
        self.fileName = fileName
        self.panel = wx.Panel(self, wx.ID_ANY)
        sizeLabel = (296,27)
        self.currentInfo = wx.StaticText(self.panel, label=fileName[1], size=sizeLabel, pos =(20,10))
        self.lineColumn = wx.TextCtrl(self.panel, size=sizeLabel, pos=(20,42))
        self.lineColumn.SetValue(fileName[0])
        self.line = wx.StaticLine(self.panel, size=(296,1), pos=(20, 84))
        self.okBtn = wx.Button(self.panel, label="OK", size=(82,27), pos =(234,100))
        self.cancelBtn = wx.Button(self.panel, label="Cancel", size=(82,27), pos =(134,100))
        self.okBtn.Bind(wx.EVT_BUTTON, self.onOkClicked)
        self.cancelBtn.Bind(wx.EVT_BUTTON, self.onCancelClicked)
        self.lineColumn.Bind(wx.EVT_TEXT, self.onTextChange)
        self.okBtn.Enable(False)
    def onOkClicked(self, event):
        self.fileName[0] = self.lineColumn.GetValue().strip()
        self.EndModal(True)
    def onCancelClicked(self, event):
        self.EndModal(False)
    def onTextChange(self, event):
        newName = self.lineColumn.GetValue()
        for c in newName:
          if c in string.whitespace:
            self.okBtn.Enable(False)
            return
        path = os.path.join(self.fileName[3], newName)
        if os.path.isfile(path) or os.path.isdir(path):
          self.okBtn.Enable(False)
          return
        self.okBtn.Enable(True)

class ProjectTreePanel(wx.Panel):
  def __init__(self, parent,guiPlaces,baseClass):
        self.parent = baseClass
        self.currentActiveProject = None
        # Use the WANTS_CHARS style so the panel doesn't eat the Return key.
        wx.Panel.__init__(self, parent, -1, style=wx.WANTS_CHARS)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        tID = wx.NewId()
        self.guiPlaces = guiPlaces
        self.projects = []

        self.problems = []
        self.prjProperties = {}
        # self.getPrjProperties()

        self.entitiesRelation = {
          "Application"             : None, #Skip validation for Application entity
          "ServiceGroup"            :["ServiceInstance", "ServiceUnit"],
          "ServiceInstance"         :["ComponentServiceInstance"],
          "ComponentServiceInstance":["Component"],
          "Component"               :None,
          "Cluster"                 :["Node"],
          "Node"                    :["ServiceUnit"],
          "ServiceUnit"             :["Component"]
        }

        self.entitiesParent = {
          "Application"             : None, #Skip validation for Application entity
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
        self.instantiation = ""
        self.modelling   = ""
        self.currentProjectPath = None
        self.currentImagesConfig = {}
        self.cancelProgress = False
        self.currentProcessCommand = None
        self.currentProcess = None

        self.color = {
          self.error: wx.Colour(255, 0, 0),
          self.warn: wx.Colour(0, 0, 255),
          self.info: wx.Colour(0, 0, 255)
        }

        self.tree = ProjectTreeCtrl(self, tID, wx.DefaultPosition, wx.DefaultSize,
                               wx.TR_HAS_BUTTONS
                               #| wx.TR_EDIT_LABELS
                               | wx.TR_MULTIPLE
                               | wx.TR_HIDE_ROOT
                               )
        self.tree.Bind(wx.EVT_TREE_ITEM_RIGHT_CLICK, self.OnShowPopup)
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
        self.fileMenu.AppendSeparator()
        self.fileMenu.Append(PROJECT_SAVE, "S&ave\tAlt-s", "Save Project")
        self.fileMenu.Append(PROJECT_SAVE_AS, "Save As...\tAlt-a", "Save As")
        self.fileMenu.Append(PROJECT_SAVE_ALL, "Save All", "")
        self.fileMenu.AppendSeparator()
        self.fileMenu.Enable(PROJECT_SAVE, False)
        self.fileMenu.Enable(PROJECT_SAVE_AS, False)
        # self.fileMenu.Enable(PROJECT_SAVE_ALL, False)

        # bind the menu event to an event handlerfff
        self.fileMenu.Bind(wx.EVT_MENU, self.OnNew, id=wx.ID_NEW)
        self.fileMenu.Bind(wx.EVT_MENU, self.OnLoad, id=PROJECT_LOAD)
        self.fileMenu.Bind(wx.EVT_MENU, self.OnSave, id=PROJECT_SAVE)
        self.fileMenu.Bind(wx.EVT_MENU, self.OnSaveAs, id=PROJECT_SAVE_AS)
        self.fileMenu.Bind(wx.EVT_MENU, self.OnSaveAll, id=PROJECT_SAVE_ALL)

        # Insert edit tool into the GUI
        self.editMenu = guiPlaces.menu["Edit"]
        self.editMenu.Append(EDIT_UNDO, "&Undo\tCtrl-z", "Undo")
        self.editMenu.Append(EDIT_REDO, "&Redo\tCtrl-y", "Redo")
        self.editMenu.Append(EDIT_RELOAD, "&Reload\tCtrl-r", "Reload")
        self.editMenu.AppendSeparator()
        self.editMenu.Append(EDIT_CUT,  "&Cut\tCtrl-x",  "Cut")
        self.editMenu.Append(EDIT_COPY,"&Copy\tCtrl-c","Copy")
        self.editMenu.Append(EDIT_PATSE, "&Patse\tCtrl-v", "Paste")
        self.editMenu.AppendSeparator()
        self.editMenu.Append(EDIT_FIND, "&Find\tCtrl-f", "Find")
        self.editMenu.Append(EDIT_REPLACE, "&Replace\tCtrl-h", "Replace")
        self.editMenu.AppendSeparator()
        self.editMenu.Append(EDIT_TOGGLE_LINE_COMMENT, "&Toggle Line Comment\tCtrl-.", "")
        # self.editMenu.Append(EDIT_TOGGLE_BLOCK_COMMENT, "&Toggle Block Comment\tCtrl-y", "")
        self.editMenu.AppendSeparator()
        self.editMenu.Append(EDIT_SORT, "&Sort", "Sort")
        self.editMenu.Append(EDIT_LOWERCASE, "&Lowercase", "Lowercase")
        self.editMenu.Append(EDIT_UPPERCASE, "&Uppercase", "Uppercase")

        self.editMenu.Bind(wx.EVT_MENU, self.onUndo, id=EDIT_UNDO)
        self.editMenu.Bind(wx.EVT_MENU, self.onRedo, id=EDIT_REDO)
        self.editMenu.Bind(wx.EVT_MENU, self.onReload, id=EDIT_RELOAD)
        self.editMenu.Bind(wx.EVT_MENU, self.onCut, id=EDIT_CUT)
        self.editMenu.Bind(wx.EVT_MENU, self.onCopy, id=EDIT_COPY)
        self.editMenu.Bind(wx.EVT_MENU, self.onPaste, id=EDIT_PATSE)
        self.editMenu.Bind(wx.EVT_MENU, self.onFind, id=EDIT_FIND)
        self.editMenu.Bind(wx.EVT_MENU, self.onReplace, id=EDIT_REPLACE)
        self.editMenu.Bind(wx.EVT_MENU, self.onToggleLineComment, id=EDIT_TOGGLE_LINE_COMMENT)
        self.editMenu.Bind(wx.EVT_MENU, self.onSort, id=EDIT_SORT)
        self.editMenu.Bind(wx.EVT_MENU, self.onLowercase, id=EDIT_LOWERCASE)
        self.editMenu.Bind(wx.EVT_MENU, self.onUppercase, id=EDIT_UPPERCASE)

        self.editMenu = guiPlaces.menu["Go"]
        self.editMenu.Append(GO_TO_LINE, "&Goto Line...\tCtrl-g", "")
        self.editMenu.Bind(wx.EVT_MENU, self.onGoToLine, id=GO_TO_LINE)
        #
        self.menuProject = guiPlaces.menu["Project"]
        self.menuProject.Append(PROJECT_OPEN, "Open Project", "Open Project")
        self.menuProject.Append(PROJECT_CLOSE, "Close Project", "Close Project")
        self.menuProject.AppendSeparator()
        self.menuProject.Append(PROJECT_BUILD, "Build Project", "Build Project")
        self.menuProject.Append(PROJECT_CLEAN, "Clean...", "Clean...")
        self.menuProject.AppendSeparator()
        self.menuProject.Append(PROJECT_VALIDATE, "Validate Project", "Validate Project")
        self.menuProject.Append(MAKE_IMAGES, "Make Image(s)...", "Make Image(s)...")
        self.menuProject.Append(IMAGES_DEPLOY, "Deploy Image(s)...", "Deploy Image(s)...")
        self.menuProject.AppendSeparator()
        self.menuProject.Append(PROJECT_PROPERTIES, "Properties", "Properties")
        self.menuProject.Enable(PROJECT_VALIDATE, False)
        self.menuProject.Enable(MAKE_IMAGES, False)
        self.menuProject.Enable(IMAGES_DEPLOY, False)
        self.menuProject.Enable(PROJECT_BUILD, False)
        self.menuProject.Enable(PROJECT_CLEAN, False)
        self.menuProject.Enable(PROJECT_PROPERTIES, False)
        self.menuProject.Bind(wx.EVT_MENU, self.OnLoad, id=PROJECT_OPEN)
        self.menuProject.Bind(wx.EVT_MENU, self.OnCloseProject, id=PROJECT_CLOSE)
        self.menuProject.Bind(wx.EVT_MENU, self.OnValidate, id=PROJECT_VALIDATE)
        self.menuProject.Bind(wx.EVT_MENU, self.OnMakeImages, id=MAKE_IMAGES)
        self.menuProject.Bind(wx.EVT_MENU, self.OnDeploy, id=IMAGES_DEPLOY)
        self.menuProject.Bind(wx.EVT_MENU, self.OnBuild, id=PROJECT_BUILD)
        self.menuProject.Bind(wx.EVT_MENU, self.OnClean, id=PROJECT_CLEAN)
        self.menuProject.Bind(wx.EVT_MENU, self.OnProperties, id=PROJECT_PROPERTIES)

        self.menuWindows = guiPlaces.menu["Windows"]
        self.menuWindows.Append(wx.NewId(), texts.modelling, "")
        self.menuWindows.Append(wx.NewId(), texts.model_details, "")
        self.menuWindows.Append(wx.NewId(), texts.instantiation, "")
        self.menuWindows.Append(wx.NewId(), texts.instance_details, "")

        self.menuTools = guiPlaces.menu["Tools"]
        self.menuTools.Append(TOOL_FONT_COLOR, "Fonts & Colors", "")
        self.menuTools.Append(TOOL_CLEAR_PROJECT_DATA, "Clear project data", "")
        self.menuTools.Bind(wx.EVT_MENU, self.OnSettingFontColor, id=TOOL_FONT_COLOR)
        self.menuTools.Bind(wx.EVT_MENU, self.OnClearProjectData, id=TOOL_CLEAR_PROJECT_DATA)
        self.menuTools.Enable(TOOL_CLEAR_PROJECT_DATA, False)

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
  def OnShowPopup(self, event):
    self.popupmenu = wx.Menu()
    selectItem = self.tree.GetFocusedItem()
    itemPath = self.getFullPath(selectItem)
    if os.path.isdir(itemPath):
      prjPath = self.tree.GetPyData(selectItem).directory()
      if itemPath == prjPath:
        menus = ["New File", "New Folder", "Open Containing Folder", "Rename", "Close Project"]
      else:
        menus = ["New File", "New Folder", "Open Containing Folder", "Rename", "Delete"]
    elif os.path.isfile(itemPath):
      menus = ["Open Containing Folder", "Rename", "Delete"]
    else: 
      menus = ["Rename", "Delete"]
    for text in menus:
        item = self.popupmenu.Append(-1, text)
        self.Bind(wx.EVT_MENU, self.onSelectContext)
    self.PopupMenu(self.popupmenu, event.GetPoint())
    self.popupmenu.Destroy()

  def reverseReplace(self, st, old, new, occur):
    li = st.rsplit(old, occur)
    return new.join(li)

  def showDialog(self, dlgInfo):
    dlg = RenameDialog(self, dlgInfo)
    result = dlg.ShowModal()
    dlg.Destroy()
    return result

  def deleteSingleItem(self, file):
    frame = self.guiPlaces.frame
    if file in frame.openFile.keys():
      page = frame.openFile[file]
      index = frame.tab.GetPageIndex(page)
      if index != -1:
        frame.tab.DeletePage(index)
      del frame.openFile[file]

  def deleteTreeRecursion(self, path):
    for file in self.getDirFiles(path): 
      if os.path.isfile(file):
        self.deleteSingleItem(file)
      elif os.path.isdir(file):
          self.deleteTreeRecursion(file)

  def renameTreeRecursion(self, path, oldPath, newPath):
    for file in self.getDirFiles(path):
      if os.path.isfile(file):
        frame = self.guiPlaces.frame
        if file in frame.openFile.keys():
          page = frame.openFile[file]
          p = page.control.file_path.replace(oldPath, newPath, 1)
          frame.openFile[p] = frame.openFile.pop(file)
          page.control.file_path = p
      elif os.path.isdir(file):
        self.renameTreeRecursion(file, oldPath, newPath)

  def onSelectContext(self, event):
    item = self.popupmenu.FindItemById(event.GetId())
    text = item.GetText()
    selectedItem = self.tree.GetFocusedItem()
    itemText = self.tree.GetItemText(selectedItem)
    itemPath = self.getFullPath(selectedItem)
    if text == "Delete":
      style = wx.OK | wx.CANCEL | wx.ICON_QUESTION
      dialog = wx.MessageDialog(self, "Are you sure you want to delete '%s' from the system ?" % itemText, 'Delete Resources', style)
      result = dialog.ShowModal()
      if result != wx.ID_OK:
        return
      if os.path.isfile(itemPath):
        self.deleteSingleItem(itemPath)
        os.remove(itemPath)
      elif os.path.isdir(itemPath):
        self.deleteTreeRecursion(itemPath)
        shutil.rmtree(itemPath)
      self.tree.Delete(selectedItem)
    elif text == "Rename":
      parentPath = self.reverseReplace(itemPath, '/'+ itemText,"", 1)
      dlgInfo = [itemText, "New name", "Rename Resource", parentPath]
      if not self.showDialog(dlgInfo):
        return
      self.tree.SetItemText(selectedItem, dlgInfo[0])
      newPath = self.reverseReplace(itemPath, itemText, dlgInfo[0], 1)
      if os.path.isfile(itemPath):
        frame = self.guiPlaces.frame
        if itemPath in frame.openFile.keys():
          page = frame.openFile[itemPath]
          index = frame.tab.GetPageIndex(page)
          if index != -1:
            label = frame.tab.GetPageText(index)
            label = label.replace(itemText, dlgInfo[0], 1)
            frame.tab.SetPageText(index, label)
          frame.openFile[newPath] = frame.openFile.pop(itemPath)
          frame.openFile[newPath].control.file_path = newPath
          frame.openFile[newPath].control.detect_language()
      elif os.path.isdir(itemPath):
        self.renameTreeRecursion(itemPath, itemPath, newPath)
      os.rename(itemPath, newPath)
      parentItem = self.tree.GetItemParent(selectedItem)
      self.tree.SortChildren(parentItem)
    elif text == "New File":
      dlgInfo = ["", "File name", "New File", itemPath]
      if not self.showDialog(dlgInfo):
        return
      path = os.path.join(itemPath, dlgInfo[0])
      open(path,'a').close()
      itemId = self.tree.PrependItem(selectedItem, dlgInfo[0])
      self.tree.SortChildren(selectedItem)
      pyData = self.tree.GetPyData(selectedItem)
      self.tree.SetPyData(itemId, pyData)
      self.setIconForItem(itemId, typeFile=True)
    elif text == "New Folder":
      dlgInfo = ["", "Folder name", "New Folder", itemPath]
      if not self.showDialog(dlgInfo):
        return
      path = os.path.join(itemPath, dlgInfo[0])
      os.mkdir(path)
      itemId = self.tree.PrependItem(selectedItem, dlgInfo[0])
      self.tree.SortChildren(selectedItem)
      pyData = self.tree.GetPyData(selectedItem)
      self.tree.SetPyData(itemId, pyData)
      self.setIconForItem(itemId, typeDir=True)
    elif text == "Open Containing Folder":
      parentPath = self.reverseReplace(itemPath, '/'+ itemText,"", 1)
      webbrowser.open(parentPath)
    elif text == "Close Project":
      self.OnCloseProject(None)

  def active(self):
    i = self.tree.GetSelections()
    if i:
      i = i[0]
    else:
      i = self.tree.GetFirstVisibleItem()
    if not i.IsOk():
      return None
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

  def getDirFiles(self, path):
    dirs = [os.path.join(path, f) for f in os.listdir(path) if os.path.isdir(os.path.join(path, f))]
    files = [os.path.join(path, f) for f in os.listdir(path) if os.path.isfile(os.path.join(path, f))]
    if len(dirs) > 1:
      dirs.sort()
    if len(files) > 1:
      files.sort()
    dirFiles = dirs + files
    return dirFiles

  def getFileName(self, path):
    return path.split('/')[-1]

  def buildTreeRecursion(self, path, parentId, project):
    for file in self.getDirFiles(path): 
        id = self.tree.AppendItem(parentId, self.getFileName(file))
        self.tree.SetPyData(id, project)
        # self.tree.SetPyData(id, (project, self.getFileName(file)))
        if os.path.isfile(file):
          self.setIconForItem(id, typeFile=True)
        elif os.path.isdir(file):
            self.setIconForItem(id, typeDir=True)
            self.buildTreeRecursion(file, id, project)

  def populateGui(self, project, tree):
    il = wx.ImageList(15,15)
    self.dirIcon = il.Add(wx.ArtProvider.GetBitmap(wx.ART_FOLDER, wx.ART_OTHER, (15,15)))
    self.fldropenidx = il.Add(wx.ArtProvider.GetBitmap(wx.ART_FILE_OPEN,   wx.ART_OTHER, (15,15)))
    self.fileIcon = il.Add(wx.ArtProvider.GetBitmap(wx.ART_NORMAL_FILE, wx.ART_OTHER, (15,15)))
    self.tree.AssignImageList(il)
    self.currentProjectPath = project.projectFilename
    prjPath = self.getPrjPath()
    rootName = prjPath.split('/')[-1]
    # projName = os.path.basename(self.currentProjectPath)

    # projName = os.path.splitext(projName)[0]
    # print "projName", projName
    prjT = self.tree.AppendItem(self.root, rootName)
    self.projects.append(prjT)
    self.tree.SetPyData(prjT, project)
    # self.tree.SetPyData(prjT, (project, projName))
    self.setIconForItem(prjT, typeDir=True)
    srcDir = os.path.join(self.getPrjPath(), "src")
    if not os.path.isdir(srcDir):
      os.mkdir(srcDir)
    self.buildTreeRecursion(self.getPrjPath(), prjT, project)

  def setIconForItem(self, item, typeFile=False, typeDir=False):
    path = ""
    if not typeFile or not typeDir:
      path = self.getFullPath(item)

    if os.path.isfile(path) or typeFile:
      self.tree.SetItemImage(item, self.fileIcon, wx.TreeItemIcon_Normal)
    elif os.path.isdir(path) or typeDir:
      self.tree.SetItemImage(item, self.dirIcon, wx.TreeItemIcon_Normal)
      self.tree.SetItemImage(item, self.fldropenidx,wx.TreeItemIcon_Expanded)
 
  def getItemByLabel(self, tree, label, root):
      item, cookie = tree.GetFirstChild(root)
      while item.IsOk():
          text = self.getFullPath(item)
          if text:
            if text == label:
                return item
          # if tree.ItemHasChildren(item):
          #     match = self.getItemByLabel(tree, label, item)
          #     if match.IsOk():
          #         return match
          item, cookie = tree.GetNextChild(root, cookie)

      return None

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
    prjPath = self.getPrjPath()
    rootName = prjPath.split('/')[-1]
    path = prjPath[0:(len(prjPath)-len(rootName))]
    path = path + self.getPathTree(item)

    return path

  def getItemAbsolutePath(self, item):
    result = self.tree.GetPyData(item)
    prjPath = result.directory()
    rootName = prjPath.split('/')[-1]
    path = prjPath[0:(len(prjPath)-len(rootName))]
    path = path + self.getPathTree(item)

    return path

  def updateTreeItem(self, project, itemText, srcFiles=None):
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
      path = os.path.join(self.getPrjPath(), itemText)
      if not c.IsOk():
        if not os.path.isdir(path):
          os.mkdir(path)
        c = self.tree.PrependItem(child, itemText)
        self.setIconForItem(c, typeDir=True)
        self.tree.SortChildren(child)
      self.tree.DeleteChildren(c)
      if itemText == texts.src:
        project.updatePrjXmlSource(srcFiles)
      self.buildTreeRecursion(path, c, project)   

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
      self.menuProject.Enable(PROJECT_BUILD, True)
      self.menuProject.Enable(PROJECT_CLEAN, True)
      self.menuProject.Enable(MAKE_IMAGES, True)
      self.menuProject.Enable(IMAGES_DEPLOY, True)
      self.menuProject.Enable(PROJECT_PROPERTIES, True)
      self.menuTools.Enable(TOOL_CLEAR_PROJECT_DATA, True)

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
      self.menuProject.Enable(PROJECT_BUILD, True)
      self.menuProject.Enable(PROJECT_CLEAN, True)
      self.menuProject.Enable(MAKE_IMAGES, True)
      self.menuProject.Enable(IMAGES_DEPLOY, True)
      self.menuProject.Enable(PROJECT_PROPERTIES, True)
      self.menuTools.Enable(TOOL_CLEAR_PROJECT_DATA, True)

      os.system('mkdir -p %s/configs' % self.getPrjPath())

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
    if curPage.__class__.__name__ == "Page":
      curPage.onSave(None)

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
  
  def OnSaveAll(self, event):
    if self.currentActiveProject:
      saved = []
      self.currentActiveProject.save() 
      saved.append(self.currentActiveProject.name)
      self.guiPlaces.statusbar.SetStatusText("Projects %s saved." % ", ".join(saved),0)

    n = self.guiPlaces.frame.tab.GetPageCount()
    for index in range(n):
      Page = self.guiPlaces.frame.tab.GetPage(index)
      if Page.__class__.__name__ == "Page":
        Page.onSave(None, index)

  def OnSize(self, event):
    w,h = self.GetClientSizeTuple()
    self.tree.SetDimensions(0, 0, w, h)
     
  def removePageByObj(self, page):
    try:
      index = self.guiPlaces.frame.tab.GetPageIndex(page)
      if index != -1:
        self.guiPlaces.frame.tab.DeletePage(index)
    except:
      pass

  def OnCloseProject(self, event):
    prj = self.active()
    if not prj:
      return
    selectItem = self.tree.GetFocusedItem()
    itemPath = self.getFullPath(selectItem)
    if os.path.isdir(itemPath):
      prjPath = self.tree.GetPyData(selectItem).directory()
    else: return
    if itemPath != prjPath: return
    del self.guiPlaces.frame.undo[self.currentProjectPath]
    self.deleteTreeRecursion(itemPath)
    if self.currentActiveProject != self.tree.GetPyData(selectItem):
      self.tree.Delete(selectItem)
      return
    self.tree.Delete(selectItem)
    frame = self.guiPlaces.frame
    prj = self.active()
    if prj:
      frame.loadProject(prj)
      self.tree.SetFocusedItem(self.tree.GetFirstVisibleItem())
      pageText = frame.getCurrentPageText(0)
      self.guiPlaces.frame.enableTools(pageText)
    else:
      model = self.guiPlaces.frame.model
      self.removePageByObj(model.instanceDetails)
      self.removePageByObj(model.instance)
      self.removePageByObj(model.modelDetails)
      self.removePageByObj(model.uml)
      frame.cleanupTools()
      # frame.cleanupMenus()

  def OnValidate(self, event):
    if not self.tree.GetFirstVisibleItem().IsOk(): return
    self.guiPlaces.frame.setCurrentTabInfoByText(texts.model_problems)
    # Clear old problems
    del self.problems[:]
    if share.detailsPanel is None:
      return
    self.modelling = "%s Modelling" % self.getPrjName()
    self.instantiation = "%s Instantiation" % self.getPrjName()
    share.detailsPanel.model.updateMicrodom()
    try:
      self.validateComponentModel()
      self.parseComponentRelationships()
      self.validateNodeProfiles()
      self.validateAmfConfig()
      self.validateDataConfig()
    except Exception, e:
      self.updateProblems(self.error, "%s exception occur while validate model\n (%s)" % (str(Exception),str(e)), "")
    # Update all problems on modal problems tag
    self.updateModalProblems()

  def getPrjPath(self):
    prjPath, name = os.path.split(self.currentProjectPath)
    return prjPath

  def getPrjName(self):
    prjPath, name = os.path.split(self.currentProjectPath)
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
    '''
    #TODO
    pass

  def validateNodeProfiles(self):
    '''
    @summary    : This function validation node/service child intance
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

  def validateNodeIntances(self):
    '''
    @summary    : This function validation node intance
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
        if nodeTypeName not in nodeNameList:
          msg = "AMF configuration has invalid Node " + str(nodeTypeName)
          self.updateProblems(self.error, msg, self.instantiation)

        # validate AMF configuration has valid SI
        for arrow in e.containmentArrows:
          if arrow.contained.data['entityType'] == "ServiceUnit":
            suName = arrow.contained.data['name']
            instance = instances.findOneByChild("name",suName)
            suType = instance.child_.get("ServiceUnitType").children_[0]
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
                     if cType not in cNameList:
                        msg = "AMF configuration has invalid Component " + str(cName)
                        self.updateProblems(self.error, msg, self.instantiation)

  def validateServiceGroups(self):
    '''
    @summary    : This function validation service group
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
    if not self.tree.GetFirstVisibleItem().IsOk(): return
    if not share.detailsPanel.model.instances:
      msg = "Please create images for this project using 'Make image(s)' obtion and try again."
      cap = "Deploy images errors for %s" % self.getPrjName()
      self.showDialogError(cap, msg)
    else:
      dlg = DeployDialog(self)
      dlg.ShowModal()
      dlg.Destroy()
      self.updateTreeItem(self.currentActiveProject, texts.configs)

  def log_console_info(self, text):
    console = self.guiPlaces.frame.console
    asctime = time.strftime('%H:%M:%S', time.localtime())
    textformat = "%s INFO: %s" % (asctime, text)
    console.AppendText(textformat)

  def log_console_error(self, text):
    console = self.guiPlaces.frame.console
    console.SetDefaultStyle(wx.TextAttr(colText=wx.Colour(0x96,0x0d,0x0d)))
    asctime = time.strftime('%H:%M:%S', time.localtime())
    textformat = "%s ERROR: %s" % (asctime, text)
    console.AppendText(textformat)
    console.SetDefaultStyle(wx.TextAttr(wx.BLACK))

  def log_info(self, text):
    text = str(text).strip() + '\n'
    wx.CallAfter(self.log_console_info, text)

  def log_error(self, text):
    text = str(text).strip() + '\n'
    wx.CallAfter(self.log_console_error, text)

  def execute(self, command, enable_log=True):
    '''
    @summary    : Implement command in subprocess and get output while process running
    '''
    if self.cancelProgress:
      return False
    self.currentProcessCommand = command
    self.currentProcess = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    while True:
      out = self.currentProcess.stdout.readline()
      if not out and self.currentProcess.poll() != None:
          break
      if enable_log:
        self.log_info(out)
    while True:
      out = self.currentProcess.stderr.readline()
      if not out and self.currentProcess.poll() != None:
          break
      if enable_log:
        self.log_error(out)
    # user press cancel button while process is running
    if self.cancelProgress:
      return False
    return True

  def stopCurrentProcess(self):
    '''
    @summary    : found and cancel childs process
    '''
    self.log_info("Build cancelled.")
    if not self.currentProcess:
      return
    elif isinstance(self.currentProcess, paramiko.SSHClient):
      self.currentProcess.close()
    else:
      cmd = "ps --forest -o pid -g $(ps -o sid= -p %s)" % self.currentProcess.pid
      processChild = os.popen(cmd).read()
      start = False
      for p in processChild.split('\n'):
        pid = p.strip()
        if pid == str(self.currentProcess.pid):
          start = True
        if start and pid:
          cmd = "kill -9 %s" % pid
          subprocess.Popen(cmd, shell=True, stderr=subprocess.PIPE)

  def setCancelProgress(self, status):
    self.cancelProgress = status

  def OnBuild(self, event):
    if not self.tree.GetFirstVisibleItem().IsOk(): return
    prjPath, name = os.path.split(self.currentActiveProject.projectFilename)
    srcPath = prjPath + '/src'
    cmd = 'cd %s; make' % srcPath
    self.runningLongProcess(self.execute, (cmd, ))

  def OnClean(self, event):
    if not self.tree.GetFirstVisibleItem().IsOk(): return
    prjPath, name = os.path.split(self.currentActiveProject.projectFilename)
    srcPath = prjPath + '/src'
    cmd = 'cd %s; make clean' % srcPath
    self.runningLongProcess(self.execute, (cmd, ))

  def OnProperties(self, event):
    if not self.tree.GetFirstVisibleItem().IsOk(): return
    properties = PropertiesDialog(self)
    properties.ShowModal()
    properties.Destroy()
    self.updateTreeItem(self.currentActiveProject, texts.configs)

  def OnClearProjectData(self, event):
    dlg = ClearProjectData(self)
    dlg.ShowModal()
    dlg.Destroy()

  def OnMakeImages(self, event):
    if not self.tree.GetFirstVisibleItem().IsOk(): return
    if not share.detailsPanel.model.instances:
      msg = "Project has not been built. Please build the project and try again."
      cap = "Build images errors for %s" % self.getPrjName()
      self.showDialogError(cap, msg)
    else:
      dlg = MakeImages(self)
      result = dlg.ShowModal()
      dlg.Destroy()
      if result:
        self.runningLongProcess(self.makeImages, (self.getPrjPath(), ))
      self.updateTreeItem(self.currentActiveProject, texts.images)
      self.updateTreeItem(self.currentActiveProject, texts.configs)

  def runningLongProcess(self, func, parram):
    self.guiPlaces.frame.console.SetValue('')
    self.guiPlaces.frame.setCurrentTabInfoByText(texts.console)
    runningThread = threading.Thread(target = func, args = parram)
    runningThread.start()
    progressDialog = ProgressDialog(self, runningThread)
    progressDialog.ShowModal()
    progressDialog.Destroy()

  def makeImages(self, prjPath):
    os.system('rm -rf images/*')
    tarGet = str(subprocess.check_output(['g++','-dumpmachine'])).strip()
    baseImage = prjPath + '/images/%s' % tarGet
    owd = os.getcwd()
    os.chdir('../mk')
    tool = os.path.abspath("safplus_packager_ide.py")
    os.chdir('%s' % prjPath)
    cmd = 'python %s -a %s %s.tgz' % (tool, tarGet, tarGet)
    self.log_info("Generating image...")
    if not self.execute('%s' % cmd):
      os.chdir(owd)
      return False
    os.system('cp *.xml %s/bin' % baseImage)
    os.chdir(owd)
    if os.path.isdir(baseImage):
      for img in self.currentImagesConfig:
        tarImg = prjPath + '/images/' + img
        self.log_info("Building %s...\n" % tarImg)
        cmd = 'cp -r %s %s' % (baseImage, tarImg)
        if not self.execute(cmd, False):
          return False
        os.system('cp resources/setup %s/bin' % tarImg)
        self.updateImageConfig(tarImg, self.currentImagesConfig[img])
        self.log_info("Creating tarball: %s.tar.gz\n" % tarImg)
        cmd = 'cd %s/images/; tar -zcvf %s.tar.gz %s' % (prjPath, img, img)
        if not self.execute(cmd, False):
          return False
        self.log_info("Blade specific tarballs created.\n")

  def updateImageConfig(self, tarGet, imgConf):
    fConf = tarGet + '/bin/setup'
    s = open(fConf).read()
    s = s.replace('ASP_NODENAME=node0', 'ASP_NODENAME=%s' % imgConf['name'] )
    s = s.replace('SAFPLUS_BACKPLANE_INTERFACE=eth0', 'SAFPLUS_BACKPLANE_INTERFACE=%s' % imgConf['netInterface'] )
    f = open(fConf, 'w')
    f.write(s)
    f.close()

  def showDialogError(self, cap, msg):
    dialog = wx.MessageDialog(None, message=msg, caption=cap, style=wx.ICON_ERROR | wx.OK)
    dialog.ShowModal()
    dialog.Destroy()

  def OnSettingFontColor(self, event):
    dlg = style_dialog.StyleDialog(self, self.parent.style_manager)
    dlg.Bind(style_dialog.EVT_STYLE_CHANGED, self.onStyleChanged)
    dlg.Centre()
    dlg.ShowModal()

  def onStyleChanged(self, event):
    n = self.guiPlaces.frame.tab.GetPageCount()
    for index in range(n):
      Page = self.guiPlaces.frame.tab.GetPage(index)
      if Page.__class__.__name__ == "Page":
        Page.control.detect_language()

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
    '''
    @summary    : Close safplus information
    '''
    self.dlg.Close()

  def getPrjProperties(self):
    '''
    @summary    : Get project properties to prjProperties varent
    '''
    try:
      targetInfo = microdom.LoadFile("%s/configs/prjProperties.xml" % self.getPrjPath()).pretty()

      dom = xml.dom.minidom.parseString(targetInfo)
      md = microdom.LoadMiniDom(dom.childNodes[0])
    except:
      pass

    item = {}
    try:
      item['backupMode'] = str(md.backupMode.data_).strip()
      item['backupNumber'] = str(md.backupNumber.data_).strip()
    except:
      item['backupMode'] = "prompt"
      item['backupNumber'] = "5"
      self.propsChange = True

    self.prjProperties = item

  def savePrjProperties(self):
    '''
    @summary    : Save properties to xml file
    '''
    md = microdom.MicroDom({"tag_":"prjProperties"},[],[])
    md.update(self.prjProperties)
    f = open("%s/configs/prjProperties.xml" % self.getPrjPath(),"w")
    f.write(md.pretty())
    f.close()

  def getPageControl(self):
    currentPage = self.guiPlaces.frame.tab.GetCurrentPage()
    if currentPage.__class__.__name__ == "Page":
      return currentPage
    return False

  def onUndo(self, event):
    currentPage = self.guiPlaces.frame.tab.GetCurrentPage()
    if currentPage.__class__.__name__ == "Page":
      currentPage.control.Undo()
    elif isinstance(currentPage, umlEditor.Panel):
      currentPage.Undo() 

  def onRedo(self, event):
    currentPage = self.guiPlaces.frame.tab.GetCurrentPage()
    if currentPage.__class__.__name__ == "Page":
      currentPage.control.Redo()
    elif isinstance(currentPage, umlEditor.Panel):
      currentPage.Redo() 

  def onReload(self, event):
    pageControl = self.getPageControl()
    if pageControl:
      pageControl.reloadFile()

  def onCut(self, event):
    pageControl = self.getPageControl()
    if pageControl:
      pageControl.control.Cut()

  def onCopy(self, event):
    pageControl = self.getPageControl()
    if pageControl:
      pageControl.control.Copy()

  def onPaste(self, event):
    pageControl = self.getPageControl()
    if pageControl:
      pageControl.control.Paste()

  def onFind(self, event):
    '''
    @summary    : open find dialog
    '''
    pageControl = self.getPageControl()
    if pageControl:
      pageControl.Find()

  def onReplace(self, event):
    '''
    @summary    : open replace dialog for find/replace
    '''
    pageControl = self.getPageControl()
    if pageControl:
      pageControl.Replace()

  def onToggleLineComment(self, event):
    '''
    @summary    : Toggle Line Comment
    '''
    pageControl = self.getPageControl()
    if pageControl:
      pageControl.control.toggle_comment()

  def onSort(self, event):
    '''
    @summary    : sort selected line text in text view
    '''
    pageControl = self.getPageControl()
    if pageControl:
      pageControl.control.sort()

  def onLowercase(self, event):
    '''
    @summary    : convert selected text to lowercase
    '''
    pageControl = self.getPageControl()
    if pageControl:
      pageControl.control.lower()

  def onUppercase(self, event):
    '''
    @summary    : convert selected text to uppercase
    '''
    pageControl = self.getPageControl()
    if pageControl:
      pageControl.control.upper()

  def onGoToLine(self, event):
    pageControl = self.getPageControl()
    if pageControl:
      pageControl.onGoToLine()
    
class ClearProjectData(wx.Dialog):
    """
    Class clear project data & config
    """
    def __init__(self, parent):
      """Constructor"""
      wx.Dialog.__init__(self, None, title="Clear project data", size=(513,479))
      self.parent = parent
      self.SetBackgroundColour('#F2F1EF')

      vBox = wx.BoxSizer(wx.VERTICAL)

      self.deploy = wx.CheckBox(self, id=wx.ID_ANY, size=(400,30), label="Clear targets infomation")
      self.deploy.SetValue(True)
      self.properties = wx.CheckBox(self, id=wx.ID_ANY, size=(400,30), label="Clear project properties")
      self.properties.SetValue(True)
      self.imageCfg = wx.CheckBox(self, id=wx.ID_ANY, size=(400,30), label="Clear image configuration")
      self.imageCfg.SetValue(True)
      self.imageFile = wx.CheckBox(self, id=wx.ID_ANY, size=(400,30), label="Remove images file")
      self.imageFile.SetValue(True)
      self.srcBak = wx.CheckBox(self, id=wx.ID_ANY, size=(400,30), label="Remove backup resource")
      self.srcBak.SetValue(False)

      vBox.Add(self.deploy, 0, wx.ALL|wx.ALIGN_CENTER, 5)
      vBox.Add(self.properties, 0, wx.ALL|wx.ALIGN_CENTER, 5)
      vBox.Add(self.imageCfg, 0, wx.ALL|wx.ALIGN_CENTER, 5)
      vBox.Add(self.imageFile, 0, wx.ALL|wx.ALIGN_CENTER, 5)
      vBox.Add(self.srcBak, 0, wx.ALL|wx.ALIGN_CENTER, 5)

      line = wx.StaticLine(self, size=(513,1))
      vBox.Add(line, 1, wx.TOP, 180)

      CancelBtn = wx.Button(self, label="Cancel")
      CancelBtn.Bind(wx.EVT_BUTTON, self.onClickCancelBtn)
      ClearBtn = wx.Button(self, label="Clear data")
      ClearBtn.Bind(wx.EVT_BUTTON, self.onClickClearBtn)

      hBox = wx.BoxSizer(wx.HORIZONTAL)
      hBox.Add(CancelBtn, 0, wx.LEFT, 300)
      hBox.Add(ClearBtn, 0, wx.LEFT, 10)
      vBox.Add(hBox, 0, wx.ALL, 15)
      self.SetSizer(vBox)

    def onClickCancelBtn(self, event):
      self.Close()

    def onClickClearBtn(self, event):
      path = self.parent.getPrjPath()
      if self.deploy.GetValue():
        f = "%s/configs/target.xml" % path
        if os.path.isfile(f):
          os.system('echo "" > %s' % f)

      if self.properties.GetValue():
        f = "%s/configs/prjProperties.xml" % path
        if os.path.isfile(f):
          os.system('echo "" > %s' % f)

      if self.imageCfg.GetValue():
        f = "%s/configs/imagesConfig.xml" % path
        if os.path.isfile(f):
          os.system('echo "" > %s' % f)

      if self.imageFile.GetValue():
        dir_images = os.path.join(path, texts.images)
        if os.path.isdir(dir_images):
          if len(os.listdir(dir_images)):
            os.system("rm -rf %s/*" % dir_images)
            self.parent.updateTreeItem(self.parent.currentActiveProject, texts.images)

      if self.srcBak.GetValue():
        dir_bak = os.path.join(path, texts.src_bak)
        if os.path.isdir(dir_bak):
          if len(os.listdir(dir_bak)):
            os.system("rm -f %s/*.tar.gz" % dir_bak)
            self.parent.updateTreeItem(self.parent.currentActiveProject, texts.src_bak)

      self.Close()

class PropertiesDialog(wx.Dialog):
    """
    Class to define Properties dialog
    """
    def __init__(self, parent):
      """Constructor"""
      text = "Properties for %s" % parent.getPrjName()
      wx.Dialog.__init__(self, None, title=text, size=(668,535))

      self.parent = parent
      self.SetBackgroundColour('#F2F1EF')
     
      hBox = wx.BoxSizer(wx.HORIZONTAL)
      vBox = wx.BoxSizer(wx.VERTICAL)
      listProperties = ['Clovis Project System']
      self.properties = wx.ListBox(self, wx.ID_ANY, (0,0), (224,454), choices = listProperties, style=wx.LC_EDIT_LABELS | wx.LC_REPORT |wx.SUNKEN_BORDER)
      self.properties.Bind(wx.EVT_LISTBOX, self.onSelectChange)
      vBox1 = wx.BoxSizer(wx.VERTICAL)

      label1 = wx.StaticText(self, label="Clovis System Project", size=(200,25))
      font = wx.Font(11, wx.DEFAULT, wx.NORMAL, wx.BOLD)
      label1.SetFont(font)

      self.okBtn = wx.Button(self, label="OK")
      self.okBtn.Bind(wx.EVT_BUTTON, self.onClickOkBtn)
      cancelBtn = wx.Button(self, label="Cancel")
      cancelBtn.Bind(wx.EVT_BUTTON, self.onClickCancelBtn)

      self.systemProject = SystemProject(self)
      self.parent.getPrjProperties()
      self.systemProject.mBackup.SetValue(str(self.parent.prjProperties['backupMode']).strip())
      self.systemProject.nBackup.SetValue(str(self.parent.prjProperties['backupNumber']).strip())

      vBox1.Add(label1, 0, wx.ALL, 10)
      vBox1.Add(self.systemProject, 0, wx.ALL, 0)
      hBox.Add(self.properties, 0, wx.ALL, 0)
      hBox.Add(vBox1, 0, wx.ALL, 0)

      hBox2 = wx.BoxSizer(wx.HORIZONTAL)
      hBox2.Add(self.okBtn, 0, wx.ALL|wx.CENTER, 0)
      hBox2.Add(cancelBtn, 0, wx.ALL, 10)

      vBox.Add(hBox, 0, wx.ALL, 0)
      vBox.Add(hBox2, 0, wx.ALL|wx.ALIGN_RIGHT, 0)
      self.SetSizer(vBox)
   
    def onSelectChange(self, event):
      pass

    def onClickOkBtn(self, event):
      '''
      @summary    : get properties and save to xml file
      '''
      self.parent.prjProperties['backupMode'] = self.systemProject.mBackup.GetValue()
      self.parent.prjProperties['backupNumber'] = self.systemProject.nBackup.GetValue()

      self.parent.savePrjProperties()
      self.Close()

    def onClickCancelBtn(self, event):
      self.Close()

class SystemProject(wx.Panel):
    def __init__(self, parent):
      self.parent = parent
      wx.Panel.__init__(self, parent, -1, style=wx.WANTS_CHARS)
      vBox = wx.BoxSizer(wx.VERTICAL)
      hBox1 = wx.BoxSizer(wx.HORIZONTAL)
      label1 = wx.StaticText(self, label="Source Backup Mode")
      s = ["prompt", "always", "never"]
      self.mBackup = wx.ComboBox(self, choices = s, size=(290,25), style=wx.CB_READONLY)
      hBox1.Add(label1, 0, wx.ALL|wx.ALIGN_LEFT, 5)
      hBox1.Add(self.mBackup, 0, wx.ALL|wx.ALIGN_RIGHT, 5)

      hBox2 = wx.BoxSizer(wx.HORIZONTAL)
      label2 = wx.StaticText(self, label="Number of Backups")
      self.nBackup = wx.TextCtrl(self, size=(290,25))

      hBox2.Add(label2, 0, wx.ALL|wx.ALIGN_LEFT, 5)
      hBox2.Add(self.nBackup, 0, wx.ALL|wx.ALIGN_RIGHT, 5)

      vBox.Add(hBox1, 0, wx.ALL, 0)
      vBox.Add(hBox2, 0, wx.ALL, 0)
      self.SetSizer(vBox)
      self.nBackup.Bind(wx.EVT_TEXT, self.onTextChange)

    def onTextChange(self, event):
      '''
      @summary    : Validate backup number
      '''
      number = str(self.nBackup.GetValue()).strip()
      try: 
        int(number)
      except ValueError:
        self.parent.okBtn.Disable()
        return
      if int(number) >= 1 and int(number) < 100:
        self.parent.okBtn.Enable()
      else:
        self.parent.okBtn.Disable()
        

class DeployDialog(wx.Dialog):
    """
    Class to define deploy images dialog
    """
    def __init__(self, parent):
      """Constructor"""
      wx.Dialog.__init__(self, parent, title="Deployment Details", size=(600,450))

      self.parent = parent
      self.SetBackgroundColour('#F2F1EF')
      LabelSize = (120,25) 
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
      btn_sizer.Add(restore_btn, 0, wx.CENTER|wx.LEFT, 142)
      btn_sizer.Add(deploy_btn, 0, wx.CENTER|wx.LEFT, 10)
      vBox.Add(btn_sizer, 1, wx.TOP, 15)

      cancel_btn = wx.Button(self, label="Cancel")
      cancel_btn.Bind(wx.EVT_BUTTON, self.onClickCancelBtn)
      deployAll_btn = wx.Button(self, label="Deploy All")
      deployAll_btn.Bind(wx.EVT_BUTTON, self.onClickDeployAllImageBtn)

      btn_sizer2 = wx.BoxSizer(wx.HORIZONTAL)
      btn_sizer2.Add(deployAll_btn, 0, wx.ALL|wx.CENTER, 5)
      btn_sizer2.Add(cancel_btn, 0, wx.ALL|wx.CENTER, 5)  

      self.nodeList = wx.ListView(self, wx.ID_ANY, (0,0), (185,350), style=wx.LC_EDIT_LABELS | wx.LC_REPORT |wx.SUNKEN_BORDER)#, style=wx.LC_REPORT|wx.SUNKEN_BORDER|wx.LC_SINGLE_SEL|wx.LC_VRULES|wx.LC_HRULES)
      self.nodeList.InsertColumn(0, "Nodes")
      self.nodeList.SetColumnWidth(0, 185)
      self.nodeList.Bind(wx.EVT_LIST_ITEM_FOCUSED, self.onSelectNodeChange)

      hBox = wx.BoxSizer(wx.HORIZONTAL)
      hBox.Add(self.nodeList, 0, wx.ALL, 5)
      hBox.Add(vBox, 0, wx.ALL, 5)

      line = wx.StaticLine(self, size=(600,1), style=wx.LI_HORIZONTAL)
      vBox1 = wx.BoxSizer(wx.VERTICAL)
      vBox1.Add(hBox, 0, wx.ALL|wx.CENTER, 5)
      vBox1.Add(line, 0, wx.ALIGN_TOP, 0)
      vBox1.Add(btn_sizer2, 0, wx.ALL|wx.ALIGN_RIGHT, 5)
      self.SetSizer(vBox1)
      vBox1.Layout()
      self.deployInfos = {}
      self.key = self.getKey()
      self.initDeploymentInfo()

    def getKey(self):
      home = os.path.expanduser("~")
      path = home + "/.clovis/ide"
      pathFile = path + "/key"
      if os.path.isfile(pathFile):
        try:
          file = open(pathFile, 'rb')
          p = pickle.Unpickler(file)
          key = str(p.load())
          file.close()
          if len(key) == 44 and key[-1] == '=':
            return key
        except:
          pass
      key = Fernet.generate_key()
      if not os.path.isdir(path):
        os.makedirs(path)
      try:
        file = open(pathFile, 'wb')
        p = pickle.Pickler(file, -1)
        p.dump(key)
        file.close()
      except:
        pass
      return key

    def unciphered(self, text):
      cipherSuite = Fernet(self.key)
      uncipheredText = cipherSuite.decrypt(text)
      return uncipheredText

    def ciphered(self, text):
      cipherSuite = Fernet(self.key)
      cipheredText = cipherSuite.encrypt(str(text).encode())
      return cipheredText

    def initDeploymentInfo(self):
      nodeIntances = []
      file = None
      try:
        for (name, e) in share.detailsPanel.model.instances.items():
          if e.data['entityType'] == "Node":
            nodeIntances.append(name)
        file = open("%s/configs/target.xml" % self.parent.getPrjPath(), 'rb')
        p = pickle.Unpickler(file)
        md = p.load()
      except:
        pass
      finally:
        if file:
          file.close()
      nodeIntances = sorted(nodeIntances)
      self.curNode = nodeIntances[0]

      for img in nodeIntances:
        image = {}
        try:
          image['targetAdress'] = self.unciphered(md[img]['targetAdress'])
          image['userName'] = self.unciphered(md[img]['userName'])
          image['password'] = self.unciphered(md[img]['password'])
          image['targetLocation'] = self.unciphered(md[img]['targetLocation'])
        except:
          image['targetAdress'] = ""
          image['userName'] = ""
          image['password'] = ""
          image['targetLocation'] = ""
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
      self.onSaveAllDeploymentInfo()

    def onUserNameChange(self, event):
      self.deployInfos[self.curNode]['userName'] = self.user.GetValue()
      self.onSaveAllDeploymentInfo()

    def onPasswordChange(self, event):
      self.deployInfos[self.curNode]['password'] = self.password.GetValue()
      self.onSaveAllDeploymentInfo()

    def onTargetLocationChange(self, event):
      self.deployInfos[self.curNode]['targetLocation'] = self.targetLocation.GetValue()
      self.onSaveAllDeploymentInfo()

    def getCipheredConfigData(self):
      cipheredConfig = {}
      for img in self.deployInfos:
        image = {}
        image['targetAdress'] = self.ciphered(self.deployInfos[img]['targetAdress'])
        image['userName'] = self.ciphered(self.deployInfos[img]['userName'])
        image['password'] = self.ciphered(self.deployInfos[img]['password'])
        image['targetLocation'] = self.ciphered(self.deployInfos[img]['targetLocation'])
        cipheredConfig[img] = image
      return cipheredConfig

    def onSaveAllDeploymentInfo(self):
      file = None
      try:
        file = open("%s/configs/target.xml" % self.parent.getPrjPath(),"wb")
        p = pickle.Pickler(file, -1)
        cipheredConfig = self.getCipheredConfigData()
        p.dump(cipheredConfig)
      except:
        pass
      finally:
        if file:
          file.close()

    def onClickCancelBtn(self, event):
      self.Close()

    def onClickDeployAllImageBtn(self, event):
      self.Close()
      self.parent.runningLongProcess(self.deployAllImage, ())

    def deployAllImage(self):
      prjPath = self.parent.getPrjPath()
      for key in self.deployInfos:
        srcImage = prjPath + '/images/' + key
        if self.parent.cancelProgress:
          break
        if os.path.isfile(srcImage + '.tar.gz'):
          self.deploymentSingleImage(self.deployInfos[key], srcImage)
        else:
          self.parent.log_error("Image %s.tar.gz don't exist." % srcImage)
          self.parent.log_error("Deploy failure")

    def waitForCompletedAndVerifyResult(self, stdout, stderr):
      result = True
      while True:
        out = stdout.readline()
        if not out:
          break
      while True:
        out = stderr.readline()
        if not out:
          break
        result = False
        self.parent.log_error(out)
      return result

    def deploymentSingleImage(self, info, srcImage):
      self.parent.log_info("Start deploy image: %s" % srcImage)
      if (info['targetAdress'] == "") or (info['userName'] == "") or (info['password'] == "") or (info['targetLocation'] == ""):
        self.parent.log_error("Invalid deployment infomation")
        return False
      remoteClient = None
      try:
        remoteClient = paramiko.SSHClient()
        self.parent.currentProcess = remoteClient
        remoteClient.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        self.parent.log_info("Connecting to host...")
        remoteClient.connect(hostname=info['targetAdress'], username=info['userName'], password=info['password'])
        sftp = remoteClient.open_sftp()
        fileName = str(srcImage).split('/')[-1] + ".tar.gz"
        localpath = srcImage + ".tar.gz"
        remotepath = info['targetLocation'] + "/" + fileName
        self.parent.log_info("Deploying image to host...")
        sftp.put(localpath, remotepath)
        stdin, stdout, stderr = remoteClient.exec_command('cd %s;tar -xzvf %s' % (info['targetLocation'],fileName))
        if self.waitForCompletedAndVerifyResult(stdout, stderr):
          self.parent.log_info("Deploy image successfuly")
        else:
          self.parent.log_error("Deploy image failure")
      except Exception as e:
        self.parent.log_error("Exception occured while deploying image")
        if e.message:
          self.parent.log_error(e.message)
      finally:
        if remoteClient:
          remoteClient.close()

    def onClickRestoreDefaultBtn(self, event):
      self.host.SetValue("")
      self.user.SetValue("")
      self.password.SetValue("")
      self.targetLocation.SetValue("")

    def onDeployImageHandler(self, event):
      srcImage = self.parent.getPrjPath() + '/images/' + self.curNode
      if os.path.isfile(srcImage + '.tar.gz'):
        img = self.deployInfos[self.curNode]
        self.parent.runningLongProcess(self.deploymentSingleImage, (img, srcImage, ))
      else:
        self.parent.guiPlaces.frame.setCurrentTabInfoByText(texts.console)
        self.parent.log_error("Image %s.tar.gz don't exist.\nDeploy failure" % srcImage)
        self.parent.log_error("Deploy failure")

    def onSelectNodeChange(self, event):
      item = self.nodeList.GetFocusedItem()
      if item == -1:
        return None
      self.curNode = self.nodeList.GetItemText(item)
      self.updateDeployInfoCurNode()

class MakeImages(wx.Dialog):
    """
    Class define makeimages dialog
    """
    def __init__(self, parent):
      wx.Dialog.__init__(self, None, title="Make Images Configuration", size=(455, 480))
      self.parent = parent
      mainBox = wx.BoxSizer(wx.VERTICAL)
      labelTitle = wx.StaticText(self, label="Config make images settings",size=(455,25))
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
      btn_sizer.Add(ok_btn, 0, wx.ALL|wx.CENTER, 5)
      btn_sizer.Add(cancel_btn, 0, wx.ALL|wx.CENTER, 5)  

      self.Bind(wx.EVT_CLOSE, self.onClickCancelBtn)
        
      mainBox.Add(labelTitle, 0, wx.TOP|wx.LEFT, 20)
      mainBox.Add(self.configPanel, 0, wx.ALL|wx.CENTER, 5)
      mainBox.Add(btn_sizer, 0, wx.TOP|wx.ALIGN_RIGHT,5)

      self.SetSizer(mainBox)
      mainBox.Layout()
      
    def onClickOkBtn(self, event):
      self.general.getAllRawConf()
      self.general.saveAllImgConfig()
      self.parent.currentImagesConfig = self.general.imagesConfig
      self.EndModal(True)

    def onClickCancelBtn(self, event):
      self.EndModal(False)

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
    self.num = wx.ComboBox(self, choices = s, size=(110,25), style=wx.CB_READONLY)
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

      targetInfo = microdom.LoadFile("%s/configs/imagesConfig.xml" % self.parent.parent.getPrjPath()).pretty()

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

  def saveAllImgConfig(self):
    md = microdom.MicroDom({"tag_":"Nodes"},[],[])
    md.update(self.imagesConfig)
    f = open("%s/configs/imagesConfig.xml" % self.parent.parent.getPrjPath(),"w")
    f.write(md.pretty())
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
        self.what = None
 
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

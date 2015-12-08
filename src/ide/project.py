import os
import os.path
import pdb
import math
import time
from types import *

import wx
import  wx.lib.newevent

import microdom
from common import log

PROJECT_LOAD = wx.NewId()
PROJECT_SAVE = wx.NewId()

PROJECT_WILDCARD = "SAFplus Project (*.spp)|*.spp|All files (*.*)|*.*"

ProjectLoadedEvent, EVT_PROJECT_LOADED = wx.lib.newevent.NewCommandEvent()  # Must be a command event so it propagates

class Project(microdom.MicroDom):
  def __init__(self, filename=None):
    self.dirty = False
    if filename: self.load(filename)
    pass

  def directory(self):
    """Returns the location of this model on disk """
    return os.path.dirname(self.projectFilename)   

  def load(self,filename):
    self.projectFilename = filename
    self.name = os.path.splitext(os.path.basename(filename))[0]
    md = microdom.LoadFile(filename)
    microdom.MicroDom.__init__(self,md.attributes_, md.children_, md.data_)
    # TODO validate the structure of the microdom tree

  def save(self, filename=None):
    if not filename: filename = self.projectFilename
    assert(0) # TODO

  def new(self):
    assert(0) # TODO


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
        menu = guiPlaces.menu["File"]
        menu.Append(PROJECT_LOAD, "L&oad\tAlt-l", "Load Project")
        menu.Append(PROJECT_SAVE, "S&ave\tAlt-s", "Save Project")

        # bind the menu event to an event handler
        menu.Bind(wx.EVT_MENU, self.OnLoad, id=PROJECT_LOAD)
        menu.Bind(wx.EVT_MENU, self.OnSave, id=PROJECT_SAVE)

  def active(self):
    i = self.tree.GetSelections()
    if i:
      i = i[0]
    else:
      i = self.tree.GetFirstVisibleItem()

    project = self.tree.GetItemPyData(i)
    if type(project) is TupleType: project = project[0]
      
    return project

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
        project = Project(p)
        self.populateGui(project, self.root)
      evt = ProjectLoadedEvent(EVT_PROJECT_LOADED.evtType[0])
      wx.PostEvent(self.parent,evt)


  def OnSave(self,event):
    pass

  def OnSaveAs(self,event):
    dlg = wx.FileDialog(
            self, message="Save file as...",
            defaultDir=os.getcwd(), 
            defaultFile="",
            wildcard=wildcard,
            style=wx.SAVE | wx.CHANGE_DIR
            )
    if dlg.ShowModal() == wx.ID_OK:
      paths = dlg.GetPaths()
      log.write('You selected %d files: %s' % (len(paths),str(paths)))
    pass


  def OnSize(self, event):
        w,h = self.GetClientSizeTuple()
        self.tree.SetDimensions(0, 0, w, h) 

import os
import inspect
import pdb
import math
import time
from types import *
from collections import namedtuple

import wx
import wx.aui

import instanceEditor
import entityDetailsDialog
import umlEditor
from project import Project, ProjectTreePanel, EVT_PROJECT_LOADED, EVT_PROJECT_NEW
import common
import model

class SAFplusFrame(wx.Frame):
    """
    Frame for the SAFplus UML editor
    """
    def __init__(self, parent, title):
        wx.Frame.__init__(self, parent, -1, title, pos=(150, 150), size=(800, 600))
        self.model = {}
       # Create the menubar
        self.menuBar = wx.MenuBar()
        # and a menu 
        self.menu = wx.Menu()
        # and a toolbar
        self.tb = self.CreateToolBar()
        self.tb.SetToolBitmapSize((24,24))
        # add an item to the menu, using \tKeyName automatically
        # creates an accelerator, the third param is some help text
        # that will show up in the statusbar
        self.menu.Append(wx.ID_EXIT, "E&xit\tAlt-X", "Exit")

        # bind the menu event to an event handler
        self.Bind(wx.EVT_MENU, self.OnTimeToClose, id=wx.ID_EXIT)
        self.Bind(EVT_PROJECT_LOADED, self.OnProjectLoaded)
        self.Bind(EVT_PROJECT_NEW, self.OnProjectNew)

        # and put the menu on the menubar
        self.menuBar.Append(self.menu, "&File")
        self.SetMenuBar(self.menuBar)

        self.sb = self.CreateStatusBar()
        
        self.guiPlaces = common.GuiPlaces(self.menuBar, self.tb, self.sb, { "File": self.menu }, None)

        # Now create the Panel to put the other controls on.
        panel = self.panel = None # panelFactory(self,menuBar,tb,sb) # wx.Panel(self)
        self.prjSplitter = wx.SplitterWindow(self, style=wx.SP_3D)
        self.project = ProjectTreePanel(self.prjSplitter,self.guiPlaces)
        self.tab = wx.aui.AuiNotebook(self.prjSplitter)
        self.help = wx.TextCtrl(self.tab, -1, "test", style=wx.TE_MULTILINE)
        self.tab.AddPage(self.help, "Welcome")
        self.prjSplitter.SplitVertically(self.project,self.tab,200)
        # And also use a sizer to manage the size of the panel such
        # that it fills the frame
        self.sizer = wx.BoxSizer()
        self.sizer.Add(self.prjSplitter, 1, wx.EXPAND)
        self.SetSizer(self.sizer)

    def cleanupTabs(self):
      """remove all editor window tabs"""
      # while 1:
        #w = self.tab.GetPage(0)
        #if not w: break
      while self.tab.DeletePage(0): pass

    def OnProjectLoaded(self,evt):
      """Called when a project is loaded in the project tree"""
      print "New project loaded"

      # clean up -- if we support multi-projects this won't happen
      self.cleanupTabs()
      self.model = {}

      # Now load the new one:
      # prj = self.project.active()
      prj = self.project.latest()
      if not prj: return
      self.model[prj.name] = t = namedtuple('model','model uml instance details')
      # only 1 model file allowed for now
      t.model = model.Model()
      prj.setSAFplusModel(t.model)
      modelFile = os.path.join(prj.directory(), prj.model.children()[0].strip())
      t.model.load(modelFile)
      t.uml = umlEditor.Panel(self.tab,self.guiPlaces.menubar, self.guiPlaces.toolbar, self.guiPlaces.statusbar, t.model)
      self.tab.AddPage(t.uml, prj.name + " Modelling")
      t.details = entityDetailsDialog.Panel(self.tab,self.guiPlaces.menubar, self.guiPlaces.toolbar, self.guiPlaces.statusbar, t.model,False)
      self.tab.AddPage(t.details, prj.name + " Model Details")
      t.instance = instanceEditor.Panel(self.tab,self.guiPlaces.menubar, self.guiPlaces.toolbar, self.guiPlaces.statusbar, t.model)
      self.tab.AddPage(t.instance, prj.name + " Instantiation")
      t.details = entityDetailsDialog.Panel(self.tab,self.guiPlaces.menubar, self.guiPlaces.toolbar, self.guiPlaces.statusbar, t.model,True)
      self.tab.AddPage(t.details, prj.name + " Instance Details")

    def OnProjectNew(self,evt):
      """Called when a new project is created"""
      print "New project created"

      # clean up -- if we support multi-projects this won't happen
      self.cleanupTabs()
      self.model = {}

      # Now create the new one:
      #prj = self.project.active()
      prj = self.project.latest()
      if not prj: return
      self.model[prj.name] = t = namedtuple('model','model uml instance details')
      # only 1 model file allowed for now
      t.model = model.Model()
      prj.setSAFplusModel(t.model)
      modelFile = os.path.join(prj.directory(), prj.model.children()[0].strip())
      #t.model.loadModuleFromFile(prj.datamodel)
      t.model.load(modelFile)
      t.uml = umlEditor.Panel(self.tab,self.guiPlaces.menubar, self.guiPlaces.toolbar, self.guiPlaces.statusbar, t.model)
      self.tab.AddPage(t.uml, prj.name + " Modelling")
      t.details = entityDetailsDialog.Panel(self.tab,self.guiPlaces.menubar, self.guiPlaces.toolbar, self.guiPlaces.statusbar, t.model,False)
      self.tab.AddPage(t.details, prj.name + " Model Details")
      t.instance = instanceEditor.Panel(self.tab,self.guiPlaces.menubar, self.guiPlaces.toolbar, self.guiPlaces.statusbar, t.model)
      self.tab.AddPage(t.instance, prj.name + " Instantiation")
      t.details = entityDetailsDialog.Panel(self.tab,self.guiPlaces.menubar, self.guiPlaces.toolbar, self.guiPlaces.statusbar, t.model,True)
      self.tab.AddPage(t.details, prj.name + " Instance Details")

    def OnTimeToClose(self, evt):
      """Event handler for the button click"""
      print "Quitting..."
      self.Close()



class SAFplusApp(wx.App):
    """ WX Application wrapper for SAFplus IDE"""
    def __init__(self, redirect):
      wx.App.__init__(self, redirect=redirect)

    def OnInit(self):
      self.frame = SAFplusFrame(None, "SAFplus IDE")
      self.SetTopWindow(self.frame)

      print "Print statements go to this stdout window by default."
      self.frame.Show(True)
      return True


def go():
  global app
  common.programDirectory = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))) 
  app = SAFplusApp(redirect=False)
  app.MainLoop()

import threading

guithread = None

def start(panelFactory):
  global guithread
  args = None 
  guithread = threading.Thread(group=None, target=go, name="GUI",args = args)
  guithread.start()


if __name__ == "__main__":
  go()


def Test():
  go()

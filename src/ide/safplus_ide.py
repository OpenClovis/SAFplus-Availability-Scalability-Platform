import os
import inspect
import pdb
import math
import time
from types import *
from collections import namedtuple
import re
import control
import styles
import texts

# import wxversion
# wxversion.select("2.8")
import wx
import wx.aui
import wx.stc as stc
#import wx.lib.fancytext as fancytext
from wx.html import HtmlWindow
import sys
import wx.lib.agw.ultimatelistctrl as ULC

import instanceEditor
import entityDetailsDialog
import umlEditor
from project import Project, ProjectTreePanel, EVT_PROJECT_LOADED, EVT_PROJECT_NEW, PROJECT_SAVE, PROJECT_SAVE_AS, PROJECT_VALIDATE, \
                    PROJECT_BUILD, PROJECT_CLEAN, MAKE_IMAGES, IMAGES_DEPLOY, PROJECT_PROPERTIES, TOOL_CLEAR_PROJECT_DATA
import common
import model

class SAFplusFrame(wx.Frame):
    """
    Frame for the SAFplus UML editor
    """
    def __init__(self, parent, title):
        wx.Frame.__init__(self, parent, -1, title, pos=(150, 150), size=(1160, 850))
        self.model = None
        self.problems = None
        self.currentActivePrj = None # indicating that this the current project which is active
        self.undo = {}
       # Create the menubar
        self.style_manager = styles.StyleManager()
        self.menuBar = wx.MenuBar()
        # and a menu 
        self.menu = wx.Menu()
        self.menuEdit = wx.Menu()
        self.menuGo = wx.Menu()
        self.menuProject = wx.Menu()
        self.menuModelling = wx.Menu()
        self.menuInstantiation = wx.Menu()
        self.menuWindows = wx.Menu()
        self.menuTools = wx.Menu()
        self.menuHelp = wx.Menu()
        # and a toolbar
        self.tb = self.CreateToolBar()
        self.tb.SetToolBitmapSize((24,24))
        # add an item to the menu, using \tKeyName automatically
        # creates an accelerator, the third param is some help text
        # that will show up in the statusbar
        self.menu.Append(wx.ID_EXIT, "E&xit\tAlt-X", "Exit")
        self.menu.AppendSeparator()        
        # bind the menu event to an event handler
        self.Bind(wx.EVT_MENU, self.OnTimeToClose, id=wx.ID_EXIT)
        self.Bind(EVT_PROJECT_LOADED, self.OnProjectLoaded)
        self.Bind(EVT_PROJECT_NEW, self.OnProjectLoaded) # Basically, new project is like load project. With new project, at the beginning, data for it was created although it's empty. After that, the newly created project is loaded with its data model

        # and put the menu on the menubar
        self.menuBar.Append(self.menu, "&File")
        self.menuBar.Append(self.menuEdit, "&Edit")
        self.menuBar.Append(self.menuGo, "&Go")
        self.menuBar.Append(self.menuProject, "&Project")
        self.menuBar.Append(self.menuModelling, "&Modelling")
        self.menuBar.Append(self.menuInstantiation, "&Instantiation")
        self.menuBar.Append(self.menuWindows, "&Windows")
        self.menuBar.Append(self.menuTools, "&Tools")
        self.menuBar.Append(self.menuHelp, "&Help")

        self.menuHelp.Bind(wx.EVT_MENU_OPEN, self.onHelpMenu)
        self.menuWindows.Bind(wx.EVT_MENU, self.onWindowsMenu)

        self.SetMenuBar(self.menuBar)

        self.sb = self.CreateStatusBar()
        
        self.guiPlaces = common.GuiPlaces(self,self.menuBar, self.tb, self.sb, { "File": self.menu, "Edit": self.menuEdit,
            "Project": self.menuProject, "Modelling":self.menuModelling, "Instantiation":self.menuInstantiation, "Windows": self.menuWindows, 
            "Tools": self.menuTools, "Help": self.menuHelp, "Go" : self.menuGo }, None)

        # Now create the Panel to put the other controls on.
        panel = self.panel = None # panelFactory(self,menuBar,tb,sb) # wx.Panel(self)
        self.prjSplitter2 = wx.SplitterWindow(self, style=wx.SP_3D)
        self.prjSplitter = wx.SplitterWindow(self.prjSplitter2, style=wx.SP_3D)
        self.project = ProjectTreePanel(self.prjSplitter,self.guiPlaces, self)
        self.Bind(wx.EVT_TREE_ITEM_ACTIVATED, self.onPrjTreeActivated, self.project.tree) # handle an event when user double-clicks on a project at the tree on the left to switch views to it or to set it active
        self.tab = wx.aui.AuiNotebook(self.prjSplitter)
        
        # Create panel that contain model problems
        bookStyle = wx.aui.AUI_NB_DEFAULT_STYLE
        bookStyle &= ~(wx.aui.AUI_NB_CLOSE_ON_ACTIVE_TAB)
        self.infoPanel = wx.aui.AuiNotebook(self.prjSplitter2, style=bookStyle)

        self.help = HtmlWindow(self.tab, -1)
        self.help.LoadFile("resources/intro.html")
        self.tab.AddPage(self.help, "Welcome")        
        self.Bind(wx.aui.EVT_AUINOTEBOOK_PAGE_CLOSE, self.onPageClosing, self.tab) # handle tab page close
        self.prjSplitter2.SplitHorizontally(self.prjSplitter, self.infoPanel, 640)
        self.prjSplitter.SplitVertically(self.project, self.tab, 200)
        # And also use a sizer to manage the size of the panel such
        # that it fills the frame
        self.sizer = wx.BoxSizer()
        self.sizer.Add(self.prjSplitter2, 1, wx.EXPAND)
        self.SetSizer(self.sizer)        
        # add recent projects menu items
        self.menu.AppendSeparator()
        self.recentPrjMenu = wx.Menu()
        self.menu.AppendMenu(wx.NewId(), "Recent", self.recentPrjMenu, "Recent projects")
        self.loadRecentProjects()
        self.loadInfoPanel()
        self.Bind(wx.EVT_CLOSE, self.OnCloseFrame)

        self.openFile = {}

        if 0:  # For development, periodically load the html so we can easily see changes
          self.timer = wx.Timer(self)
          self.Bind(wx.EVT_TIMER, self.update, self.timer)
          self.timer.Start(1000)

        self.Bind(wx.EVT_SIZE, self.OnSizeChange)

    def loadInfoPanel(self):
        self.modelProblems = ULC.UltimateListCtrl(self, wx.ID_ANY, agwStyle=ULC.ULC_AUTOARRANGE | ULC.ULC_REPORT | ULC.ULC_VRULES | ULC.ULC_HRULES | ULC.ULC_SINGLE_SEL | ULC.ULC_HAS_VARIABLE_ROW_HEIGHT)
        self.modelProblems.InsertColumn(0, "Severity Level")
        self.modelProblems.InsertColumn(1, "Message")
        self.modelProblems.InsertColumn(2, "Location")
        self.modelProblems.SetColumnWidth(0, 250)
        self.modelProblems.SetColumnWidth(1, 560)
        self.modelProblems.SetColumnWidth(2, 350)
        self.infoPanel.AddPage(self.modelProblems, texts.model_problems)
        self.console = wx.TextCtrl(self, style=wx.TE_MULTILINE|wx.TE_READONLY)
        self.infoPanel.AddPage(self.console, texts.console)

    def OnSizeChange(self, event):
        w, h = self.GetSize()
        clw_2 = 350
        if w > 1160:
          clw_2 = w - 810
        self.modelProblems.SetColumnWidth(2, clw_2)
        event.Skip()

    def setCurrentTabInfoByText(self, text):
        n = self.infoPanel.GetPageCount()
        for index in range(n):
          if self.infoPanel.GetPageText(index) == text:
            self.infoPanel.SetSelection(index)
            return True
        return False

    def update(self, event):
        self.help.LoadFile("resources/intro.html")
        

    def OnCloseFrame(self,event):
        # print "Closing application"
        fileName = ""
        n = self.tab.GetPageCount()
        cnt = 0
        for index in range(0,n):
            text = self.tab.GetPageText(index)
            if '*' in text:
                cnt += 1
                fileName += '\n' + text[1:]
        style = wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION
        style |= wx.CANCEL
        if fileName:
            dialog = wx.MessageDialog(self, 'There are %s documents with unsaved changes. Save changes before closing? %s' % (cnt,fileName), 'Save Changes?', style)
            result = dialog.ShowModal()
            if result == wx.ID_YES:
                self.project.OnSaveAll(None)
            elif result == wx.NO:
                pass
            elif result == wx.ID_CANCEL:
                return False
        self.Destroy()

    def cleanupTabs(self):
      """remove all editor window tabs"""
      # while 1:
        #w = self.tab.GetPage(0)
        #if not w: break
      while self.tab.DeletePage(1): pass  # 1 because I don't want to clean up the help tab

    def cleanupTools(self):
      self.tb.DeletePendingEvents()      
      self.tb.ClearTools()

    def cleanupMenus(self):
      menuItems = self.menuModelling.GetMenuItems()
      for item in menuItems:
        self.menuModelling.Delete(item.Id)
      menuItems = self.menuInstantiation.GetMenuItems()
      for item in menuItems:
        self.menuInstantiation.Delete(item.Id)      
      # menuItems = self.menuWindows.GetMenuItems()
      # for item in menuItems:
      #   self.menuWindows.Delete(item.Id)

    def OnProjectLoaded(self,evt):
      """Called when a project is loaded in the project tree"""
      print "New project loaded"

      # clean up -- if we support multi-projects this won't happen
      #self.cleanupTabs()
      #self.model = {}

      # Now load the new one:
      # prj = self.project.active()
      prj = self.project.latest()      
      self.loadProject(prj)

    def loadProject(self, prj):
      if not prj: return
      index = 0
      self.project.currentProjectPath = prj.projectFilename
      self.currentActivePrj = prj
      self.project.currentActiveProject = prj
      self.tab.Unbind(wx.aui.EVT_AUINOTEBOOK_PAGE_CHANGED) # need to unbind to not catch page delete event b/c we only want to catch page selection event
      if not self.model:
        self.cleanupTabs()
        self.model = t = namedtuple('model','model uml instance modelDetails instanceDetails')
        t.model = model.Model()
        prj.setSAFplusModel(t.model)
        modelFile = os.path.join(prj.directory(), prj.model.children()[0].strip())
        t.model.load(modelFile)
        t.uml = umlEditor.Panel(self.tab,self.guiPlaces, t.model)
        self.tab.InsertPage(0, t.uml, self.getCurrentPageText(0))
        t.modelDetails = entityDetailsDialog.Panel(self.tab,self.guiPlaces, t.model,isDetailInstance=False)
        self.tab.InsertPage(1, t.modelDetails, self.getCurrentPageText(1))
        t.instance = instanceEditor.Panel(self.tab,self.guiPlaces, t.model)
        self.tab.InsertPage(2, t.instance, self.getCurrentPageText(2))
        t.instanceDetails = entityDetailsDialog.Panel(self.tab,self.guiPlaces, t.model,isDetailInstance=True)
        self.tab.InsertPage(3, t.instanceDetails, self.getCurrentPageText(3))
      else:
        print 'OnProjectLoaded: model is not None'
        self.cleanupTools()
        self.cleanupMenus()
        t = self.model
        prj.setSAFplusModel(t.model)
        modelFile = os.path.join(prj.directory(), prj.model.children()[0].strip())
        print "model file: %s" % modelFile
        t.model.init()
        t.model.load(modelFile)
        if t.uml:
          t.uml.setModelData(t.model)
          t.uml.deleteTools()
          t.uml.addTools()
          t.uml.refresh()
        else:
          t.uml = umlEditor.Panel(self.tab,self.guiPlaces, t.model)
          self.tab.InsertPage(0, t.uml, self.getCurrentPageText(0), select=True)
        if t.modelDetails:
          t.modelDetails.refresh()
        else:
          t.modelDetails = entityDetailsDialog.Panel(self.tab,self.guiPlaces, t.model,isDetailInstance=False)
          self.tab.InsertPage(1, t.modelDetails, self.getCurrentPageText(1))
        if t.instance:
          t.instance.setModelData(t.model)
          t.instance.refresh()
          t.instance.addTools()
        else:
          t.instance = instanceEditor.Panel(self.tab,self.guiPlaces, t.model)
          self.tab.InsertPage(2, t.instance, self.getCurrentPageText(2))
        if t.instanceDetails:
          t.instanceDetails.setModelData(t.model)
          t.instanceDetails.refresh()
        else:
          t.instanceDetails = entityDetailsDialog.Panel(self.tab,self.guiPlaces, t.model,isDetailInstance=True)
          self.tab.InsertPage(3, t.instanceDetails, self.getCurrentPageText(3))
        index = self.setPagesText(t.uml, self.getCurrentPageText(0))
        self.setPagesText(t.modelDetails, self.getCurrentPageText(1))
        self.setPagesText(t.instance, self.getCurrentPageText(2))
        self.setPagesText(t.instanceDetails, self.getCurrentPageText(3))
      if self.project.currentProjectPath not in self.undo.keys():
        self.undo[self.project.currentProjectPath] = ([],[])
      t.uml.undoData = self.undo[self.project.currentProjectPath][0]
      t.uml.redoData = self.undo[self.project.currentProjectPath][1]
      self.tab.Bind(wx.aui.EVT_AUINOTEBOOK_PAGE_CHANGED, self.onPageChanged) # bind to catch page selection event
      if self.tab.GetCurrentPage() == t.uml:
        self.enableTools(self.getCurrentPageText(0))
      else:
        self.tab.SetSelection(index) # open uml model view by default
      # append to recent projects repository and update the menu
      self.updateRecentProject(prj)
      if self.currentActivePrj.dataModelPlugin:
        self.currentActivePrj.dataModelPlugin.init(t, self.guiPlaces)
      
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
      t.uml = umlEditor.Panel(self.tab,self.guiPlaces, t.model)
      self.tab.AddPage(t.uml, prj.name + " Modelling")
      t.details = entityDetailsDialog.Panel(self.tab,self.guiPlaces, t.model,isDetailInstance=False)
      self.tab.AddPage(t.details, prj.name + " Model Details")
      t.instance = instanceEditor.Panel(self.tab,self.guiPlaces, t.model)
      self.tab.AddPage(t.instance, prj.name + " Instantiation")
      t.details = entityDetailsDialog.Panel(self.tab,self.guiPlaces, t.model,isDetailInstance=True)
      self.tab.AddPage(t.details, prj.name + " Instance Details")
      if self.prj.dataModelPlugin:
        self.prj.dataModelPlugin.init(t, self.guiPlaces)

    def OnTimeToClose(self, evt):
      """Event handler for the button click"""
      print "Quitting..."
      self.Close()
   
    def setPagesText(self, page, text):
      index = self.tab.GetPageIndex(page)
      self.tab.SetPageText(index, text)
      return index

    def loadRecentProjects(self):
      recentPrjs = common.getRecentPrjs()
      for i in recentPrjs[::-1]:
        itemId = wx.NewId()
        self.recentPrjMenu.Append(itemId, i.replace('\n',''))
        self.recentPrjMenu.Bind(wx.EVT_MENU, self.onRecentPrjMenu, id=itemId)

    def updateRecentProject(self, prj):
      """Updates the recent project list with the passed project"""
      common.addRecentPrj(prj.projectFilename)
      menuItems = self.recentPrjMenu.GetMenuItems()
      #if len(menuItems)>0 and menuItems[0].GetItemLabelText()==prj.projectFilename:
      #  return
      for item in menuItems:
        self.recentPrjMenu.Delete(item.Id)
      self.loadRecentProjects()
      

    def onRecentPrjMenu(self, evt):
      itemId = evt.GetId()
      p = self.recentPrjMenu.FindItemById(itemId).GetItemLabelText()      
      print 'onRecentPrjMenu: name [%s]; id [%d]' % (p, itemId) 
      if self.project.isPrjLoaded(p): return 
      project = Project(p)                 
      self.project.populateGui(project, self.project.root)
      prj = self.project.latest()      
      self.loadProject(prj)
      self.menu.Enable(PROJECT_SAVE, True)
      self.menu.Enable(PROJECT_SAVE_AS, True)
      self.menuProject.Enable(PROJECT_VALIDATE, True)
      self.menuProject.Enable(PROJECT_BUILD, True)
      self.menuProject.Enable(PROJECT_CLEAN, True)
      self.menuProject.Enable(MAKE_IMAGES, True)
      self.menuProject.Enable(IMAGES_DEPLOY, True)
      self.menuProject.Enable(PROJECT_PROPERTIES, True)
      self.menuTools.Enable(TOOL_CLEAR_PROJECT_DATA, True)

    def onPrjTreeActivated(self, evt):
      """ handle an event when user double-clicks on an item at the tree on the left to switch views to it or to set it active """
      pt = evt.GetPoint()
      item, _ = self.project.tree.HitTest(pt)
      if item:       
        print "onPrjTreeActivated [%s]" % self.project.tree.GetItemText(item)
        if self.project.tree.GetItemParent(item) == self.project.root: # check to see if this is the project name
          prjname = os.path.splitext(self.project.tree.GetItemText(item))[0]
          print 'project [%s] is activated' % prjname
          prj = self.project.active()
          print 'project [%s] is selected' % prj.name
          if prj == self.currentActivePrj:
            return
          else:
            self.loadProject(prj)
        else: 
          # for create, update status of edit page
          path = self.project.getItemAbsolutePath(item)
          if os.path.isfile(path):
            
            if path in self.openFile.keys():
              if "DELETED Page object" in str(self.openFile[path]):
                del self.openFile[path]

            if not (path in self.openFile.keys()):
              fileName = str(self.project.tree.GetItemText(item))
              p = Page(self, path)
              self.tab.AddPage(p, fileName)
              self.openFile[path] = p

            pageIndex = self.tab.GetPageIndex(self.openFile[path])
            self.tab.SetSelection(pageIndex)
          
          pass #TODO : handling other tree item clicked e.g. c++ source or others

    def onPageChanged(self, evt):
      page = self.tab.GetPageText(evt.GetSelection())
      print 'onPageChangedEvent: page [%s] is selected' % page
      self.enableTools(page)
      currentPage = self.tab.GetCurrentPage()
      if isinstance(currentPage, entityDetailsDialog.Panel):
        if self.model.uml:
          if self.model.uml.undoAction:
            currentPage.refresh()
            self.model.uml.undoAction = False

    def enableTools(self, page):
      t = self.model
      if not t: return      
      if re.sub("\*", "", page) == self.getCurrentPageText(0):
        # uml modelling is selected, now enable tools belonging to it and disable the others not belonging to it
        if t.uml: 
          t.uml.enableTools(True)
          t.uml.enableMenuItems(True) # set menu items state also
        if t.instance:
          t.instance.enableTools(False)
          t.instance.enableMenuItems(False) # set menu items state also
      elif re.sub("\*", "", page) == self.getCurrentPageText(2):
        # instantiation is selected, now enable tools belonging to it and disable the others not belonging to it
        if t.uml:
          t.uml.enableTools(False)
          t.uml.enableMenuItems(False)
        if t.instance:
          t.instance.enableTools(True)
          t.instance.enableMenuItems(True)
      else:
        # disable all tools because in these pages, we do not model
        if t.uml:
          if self.tab.GetPageIndex(t.uml) != wx.NOT_FOUND:
            t.uml.enableTools(False)
            t.uml.enableMenuItems(False)
        if t.instance:
          if self.tab.GetPageIndex(t.instance) != wx.NOT_FOUND:
            t.instance.enableTools(False)
            t.instance.enableMenuItems(False)

    def getCurrentPageText(self, pageIdx):
      if pageIdx==0:
        return self.currentActivePrj.name + " Modelling"
      elif pageIdx==1:
        return self.currentActivePrj.name + " Model Details"
      elif pageIdx==2:
        return self.currentActivePrj.name + " Instantiation"
      elif pageIdx==3:
        return self.currentActivePrj.name + " Instance Details"
      return None

    def onPageClosing(self, evt):
      print 'page closing event launched'
      idx = evt.GetSelection()
      if idx >= 0:
        page = self.tab.GetPage(idx)
        if page.__class__.__name__ == "Page":
          page.control.confirm_close(False)
          del self.openFile[page.control.file_path]
          return
      pageText = self.tab.GetPageText(idx)
      print 'page [%s] idx [%d] closing' % (pageText, idx)
      t = self.model
      if not t:
        return
      if pageText == self.getCurrentPageText(0) or pageText[1:] == self.getCurrentPageText(0):
        t.uml.deleteMyTools()        
      #   pageIdx = 0
      # elif pageText == self.getCurrentPageText(1) or pageText[1:] == self.getCurrentPageText(0):
      #   pageIdx = 1
      elif pageText == self.getCurrentPageText(2) or pageText[1:] == self.getCurrentPageText(2):
        t.instance.deleteMyTools()
        # pageIdx = 2
      # elif pageText == self.getCurrentPageText(3):        
      #   pageIdx = 3
      # else:
      #   pageIdx = -1
      # if pageIdx==-1:
      #   return
      # print 'insert menu item id [%d] text [%s]' % (pageIdx, pageText)
      # self.menuWindows.Append(pageIdx, pageText)
      # self.menuWindows.Bind(wx.EVT_MENU, self.onWindowsMenu, id=pageIdx)
      #evt.Veto()
      # self.tab.RemovePage(idx)

    def onWindowsMenu(self, evt):
      if not self.project.active():
        return
      print 'onWindowsMenu launched'
      idx = evt.GetId()
      pageText = self.menuWindows.GetLabel(idx)
      print 'menu item [%s] idx [%d] clicked' % (pageText, idx)
      t = self.model
      index = -1
      if pageText == texts.modelling and t.uml:
        index = self.tab.GetPageIndex(t.uml)
      elif pageText == texts.model_details and t.modelDetails:
        index = self.tab.GetPageIndex(t.modelDetails)
      elif pageText == texts.instantiation and t.instance:
        index = self.tab.GetPageIndex(t.instance)
      elif pageText == texts.instance_details and t.instanceDetails:
        index = self.tab.GetPageIndex(t.instanceDetails)
      if index != -1:
        self.tab.SetSelection(index)
        return True

      if pageText == texts.modelling:
        if t.instance:
          t.instance.deleteTools()
        else:
          self.tb.ClearTools()
        page = t.uml = umlEditor.Panel(self.tab,self.guiPlaces, t.model)
        if t.instance:
          t.instance.addTools()
        textId = pageIdx = 0  
      elif pageText == texts.model_details:
        page = t.modelDetails = entityDetailsDialog.Panel(self.tab,self.guiPlaces, t.model,isDetailInstance=False)
        textId = pageIdx = 1 #if t.uml else 0               
      elif pageText == texts.instantiation:
        page = t.instance = instanceEditor.Panel(self.tab,self.guiPlaces, t.model)
        if t.uml and t.modelDetails:
          pageIdx = 2
        elif (t.uml and not t.modelDetails) or (not t.uml and t.modelDetails):
          pageIdx = 1
        else:
          pageIdx = 2
        textId = 2
      else:
        page = t.instanceDetails = entityDetailsDialog.Panel(self.tab,self.guiPlaces, t.model,isDetailInstance=True)
        textId = pageIdx = 3
      text = self.getCurrentPageText(textId)
      print 'insert page id [%d] text [%s]' % (pageIdx, text)
      self.tab.InsertPage(pageIdx, page, text)
      self.tab.SetSelection(pageIdx)

    def onHelpMenu(self, evt):
      if self.help:
        n = self.tab.GetPageCount()
        self.tab.SetSelection(n-1)
      else:
        self.help = HtmlWindow(self.tab, -1)
        self.help.LoadFile("resources/intro.html")
        n = self.tab.GetPageCount()
        self.tab.InsertPage(n, self.help, "Welcome")       
        self.tab.SetSelection(n)

    def deleteWindowsMenuItem(self, text):
      menuItems = self.menuWindows.GetMenuItems()
      for item in menuItems:
        if item.GetItemLabelText() == text:
          self.menuWindows.Delete(item.Id)
          break

    def insertPage(self, idx):
      t = self.model      
      if not t: return
      page = None
      if idx==1: 
        page = t.modelDetails = entityDetailsDialog.Panel(self.tab,self.guiPlaces, t.model,isDetailInstance=False)        
      elif idx==3:
        page = t.instanceDetails = entityDetailsDialog.Panel(self.tab,self.guiPlaces, t.model,isDetailInstance=True)
      if page:
        pageText = self.getCurrentPageText(idx)
        self.tab.InsertPage(idx, page, pageText, select=True)
        self.deleteWindowsMenuItem(pageText)

    def modelChange(self):
      '''
      @summary    : add character '*' if data on model is change
      '''
      index = self.tab.GetSelection()
      label = self.tab.GetPageText(index)
      if not ('*' in label):
        self.tab.SetPageText(index, "*%s" % label)

class SAFplusApp(wx.App):
    """ WX Application wrapper for SAFplus IDE"""
    def __init__(self, redirect):
      wx.App.__init__(self, redirect=redirect)

    def OnInit(self):
      self.frame = SAFplusFrame(None, "SAFplus IDE")
      self.SetTopWindow(self.frame)
      self.SetExitOnFrameDelete(True)
      print "Print statements go to this stdout window by default."
      self.frame.Show(True)
      return True

class Page(wx.Panel):
  """
  Class to define general page
  """
  def __init__(self, parent, pathFile):
    """Constructor"""
    wx.Panel.__init__(self, parent)
    self.parent = parent
    self.control = control.EditorControl(self, wx.BORDER_NONE, pathFile)

    # SAVE_ID = wx.NewId()
    # self.Bind(wx.EVT_MENU, self.onSave, id=SAVE_ID)
    # accelTbl = wx.AcceleratorTable([(wx.ACCEL_CTRL, ord('S'), SAVE_ID)])
    # self.SetAcceleratorTable(accelTbl)

    self.sizer = wx.BoxSizer(wx.VERTICAL)
    self.sizer.Add(self.control,1,wx.EXPAND)
    self.SetSizer(self.sizer)
    self.sizer.Fit(self)
    self.isReloadFile = False
    self.pathFile = pathFile
    self.control.Bind(stc.EVT_STC_CHANGE, self.onEditChange)
    self.timer = wx.Timer(self)
    self.Bind(wx.EVT_TIMER, self.onTimer, self.timer)
    self.timer.Start(1000)

  def Find(self):
    self.onKeyCombo(0)

  def Replace(self):
    self.onKeyCombo(wx.FR_REPLACEDIALOG)

  def onKeyCombo(self, fStyle):
    '''
    @summary    : search/replace text
    '''
    data = wx.FindReplaceData(flags=0)
    data.SetFlags(1)
    data.SetFindString(self.control.GetSelectedText())
    dlg = FindReplaceDialog(self, data, "", fStyle)
    dlg.ShowModal()
    dlg.Destroy()

  def onSave(self, event, index = -1):
    '''
    @summary    : save file content and remove * character
    '''
    label = ''
    if index == -1:
      index = self.parent.tab.GetSelection()
    label = self.parent.tab.GetPageText(index)

    if '*' in label:
      label = re.sub("\*", "", label)
      self.parent.tab.SetPageText(index, label)
      self.control.save_file()

  def onTimer(self, event):
    if not os.path.isfile(self.control.file_path):
      return
    lastModified = os.path.getmtime(self.control.file_path)
    currentTime = time.time()
    diff = currentTime - lastModified
    if diff > 1.5:
      return
    index = self.parent.tab.GetPageIndex(self)
    label = self.parent.tab.GetPageText(index)
    if not ('*' in label):
      try:
        file = open(self.control.file_path, 'r')
        if self.control.GetValue() != file.read():
          self.isReloadFile = True
          self.control.reload_file(False)
          self.isReloadFile = False
      finally:
        if file:
          file.close()

  def reloadFile(self):
    self.control.reload_file(False)
    index = self.parent.tab.GetSelection()
    label = self.parent.tab.GetPageText(index)
    if '*' in label:
      label = re.sub("\*", "", label)
      self.parent.tab.SetPageText(index, label)

  def onEditChange(self, event):
    '''
    @summary    : add character '*' if file content is change
    '''
    index = self.parent.tab.GetPageIndex(self)
    label = self.parent.tab.GetPageText(index)
    if not ('*' in label) and (not self.isReloadFile):
      self.control.edited = True
      try:
        file = open(self.control.file_path,'r')
        if self.control.GetValue() != file.read():
          self.parent.tab.SetPageText(index, "*%s" % label)
      finally:
        if file:
          file.close()

  def onGoToLine(self):
    lineMax = self.control.GetLineCount()
    dlg = GoToLine(self, lineMax)
    dlg.ShowModal()
    dlg.Destroy()

class FindReplaceDialog(wx.FindReplaceDialog):
    """
    Class to define FindReplaceDialog dialog
    """
    def __init__(self, parent, d, t, s):
      """Constructor"""
      wx.FindReplaceDialog.__init__(self, parent, data=d, title=t, style=s)
      self.parent = parent
      self.control = self.parent.control
      self.Bind(wx.EVT_FIND, self.find)
      self.Bind(wx.EVT_FIND_NEXT, self.findNext)
      self.Bind(wx.EVT_FIND_REPLACE, self.findReplace)
      self.Bind(wx.EVT_FIND_REPLACE_ALL, self.findReplaceAll)
      self.Bind(wx.EVT_FIND_CLOSE, self.findClose)

    def find(self, event):
      '''
      @summary    : call for first click on find button
      '''
      self.findNext(event)

    def findNext(self, event):
      '''
      @summary    : click on find button from second time
      '''
      text = self.GetData().GetFindString()
      flags = self.GetData().GetFlags()
      previous = not (flags & 0x1)
      wrap = True
      if text and self.control:
          self.control.find(text, previous, wrap, flags, False)

    def findReplace(self, event):
      '''
      @summary    : This method implement replace text
      '''
      flags = self.GetData().GetFlags()
      selection = self.control.GetSelectedText()
      a, b = self.control.GetSelection()
      replacement = self.GetData().GetReplaceString()
      if selection:
        self.control.ReplaceSelection(replacement)
      if not (flags & 0x1):
        self.control.SetSelection(a, a)
      self.find(event)

    def findReplaceAll(self, event):
      '''
      @summary    : This method implement replace all text
      '''
      text = self.GetData().GetFindString()
      flags = self.GetData().GetFlags() & 0x6
      replacement = self.GetData().GetReplaceString()
      self.control.replace_all(text, replacement, flags, False)

    def findClose(self, event):
      '''
      @summary    : Close search/replace dialog
      '''
      self.Destroy()

class GoToLine(wx.Dialog):
    """
    Class to define GoToLine dialog
    """
    def __init__(self, parent, lineMax):
      """Constructor"""
      wx.Dialog.__init__(self, None, wx.ID_ANY, "Goto Line",size= (336,165), style=wx.DEFAULT_DIALOG_STYLE)
      self.parent = parent
      self.panel = wx.Panel(self, wx.ID_ANY)
      sizeLabel = (296,27)
      text = "Line (1 - %s)" % lineMax
      self.currentInfo = wx.StaticText(self.panel, label=text, size=sizeLabel, pos =(20,10))
      self.lineColumn = wx.TextCtrl(self.panel, size=sizeLabel, pos=(20,42))
      self.line = wx.StaticLine(self.panel, size=(296,1), pos=(20, 84))
      self.btn = wx.Button(self.panel, label="OK", size=(82,27), pos =(234,100))
      self.btn.Bind(wx.EVT_BUTTON, self.onClicked)
      self.lineColumn.Bind(wx.EVT_TEXT, self.onTextChange)

    def onClicked(self, event):
      self.Close()

    def onTextChange(self, event):
      value = self.lineColumn.GetValue().strip()
      self.lineColumn.SetSelection(-1, -1)
      control = self.parent.control
      try:
        value = int(value)
        max = control.GetLineCount()
        if value > 0 and value <= max:
            pos = control.PositionFromLine(value-1)
            control.SetSelection(pos, pos)
            control.EnsureCaretVisible()
      except:
        pass

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

import pdb
import math
import time
from types import *

import wx
import wx.lib.wxcairo

import dot
from common import *
from module import Module
from entity import *
from model import Model
import entityDetailsDialog
import share

import instanceEditor
import instanceDetailsDialog

class Panel(wx.Panel):
    def __init__(self, parent,menubar,toolbar,statusbar,model):
      wx.Panel.__init__(self, parent, style = wx.TAB_TRAVERSAL|wx.SUNKEN_BORDER)
      
      self.vsplitter = wx.SplitterWindow(self, wx.ID_ANY, style=wx.SP_3D | wx.SP_BORDER | wx.SP_LIVE_UPDATE)

      self.instanceEditor = instanceEditor.Panel(self.vsplitter,menubar,toolbar,statusbar,model)
      
      self.hsplitter = wx.SplitterWindow(self.vsplitter, wx.ID_ANY, style=wx.SP_3D | wx.SP_BORDER | wx.SP_LIVE_UPDATE)
      self.details = instanceDetailsDialog.Panel(self.hsplitter,menubar,toolbar,statusbar,model)
      self.detailsItems = instanceDetailsDialog.Panel(self.hsplitter,menubar,toolbar,statusbar,model)

      self.vsplitter.SplitVertically(self.hsplitter, self.instanceEditor)
      self.vsplitter.SetSashPosition(0)
      self.vsplitter.SetSashGravity(0.0)
      

      self.hsplitter.SplitHorizontally(self.details, self.detailsItems)
      self.hsplitter.SetSashPosition(0)
      self.hsplitter.SetSashGravity(0.0)

      sizer = wx.BoxSizer(wx.VERTICAL)
      sizer.Add(self.vsplitter, 1, wx.EXPAND)
      self.SetSizer(sizer)

def Test():
  import time
  import pyGuiWrapper as gui
  global model
  model = Model()
  model.load("testModel.xml")

  gui.go(lambda parent,menu,tool,status,m=model: Panel(parent,menu,tool,status, m))
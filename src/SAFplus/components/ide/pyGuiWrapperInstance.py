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
      
      self.splitter = wx.SplitterWindow(self, wx.ID_ANY, style=wx.SP_3D | wx.SP_BORDER | wx.SP_LIVE_UPDATE)

      self.instanceEditor = instanceEditor.Panel(self.splitter,menubar,toolbar,statusbar,model)
      self.details = instanceDetailsDialog.Panel(self.splitter,menubar,toolbar,statusbar,model)
      
      self.splitter.SplitVertically(self.details, self.instanceEditor)
      self.splitter.SetSashPosition(1)
      self.splitter.SetSashGravity(0.0)

      sizer = wx.BoxSizer(wx.VERTICAL)
      sizer.Add(self.splitter, 1, wx.EXPAND)
      self.SetSizer(sizer)

def Test():
  import time
  import pyGuiWrapper as gui
  global model
  model = Model()
  model.load("testModel.xml")

  gui.go(lambda parent,menu,tool,status,m=model: Panel(parent,menu,tool,status, m))
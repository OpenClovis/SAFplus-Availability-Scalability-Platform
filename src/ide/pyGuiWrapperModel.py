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
import share

import entityDetailsDialog
import umlEditor

class Panel(wx.Panel):
    def __init__(self, parent,menubar,toolbar,statusbar,model):
      wx.Panel.__init__(self, parent, style = wx.TAB_TRAVERSAL|wx.SUNKEN_BORDER)
      
      self.vsplitter = wx.SplitterWindow(self, wx.ID_ANY, style=wx.SP_3D | wx.SP_BORDER | wx.SP_LIVE_UPDATE)

      self.umlEditor = umlEditor.Panel(self.vsplitter,menubar,toolbar,statusbar,model)
      self.details = entityDetailsDialog.Panel(self.vsplitter,menubar,toolbar,statusbar,model)

      self.vsplitter.SplitVertically(self.details, self.umlEditor)
      self.vsplitter.SetSashPosition(0)
      self.vsplitter.SetSashGravity(0.0)

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
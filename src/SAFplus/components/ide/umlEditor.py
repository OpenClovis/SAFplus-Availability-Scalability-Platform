import wx
import wx.lib.wxcairo
import cairo
import rsvg
import yang
#from wx.py import shell,
#from wx.py import  version

from module import Module
from entity import Entity
from model import Model

class MyPanel(wx.Panel):
    def __init__(self, parent):
      wx.Panel.__init__(self, parent, -1, style=wx.SUNKEN_BORDER)
      self.Bind(wx.EVT_PAINT, self.OnPaint)
      self.model=None

    def OnPaint(self, evt):
        #dc = wx.PaintDC(self)
        dc = wx.BufferedPaintDC(self)
        dc.SetBackground(wx.Brush('white'))
        dc.Clear()        
        self.render(dc)
 
    def setModel(self,model):
      self.model = model

    def render(self, dc):
        """Put the entities on the screen"""
        ctx = wx.lib.wxcairo.ContextFromDC(dc)
        if 1:
         # now draw something with cairo
         ctx.set_line_width(15)
         ctx.move_to(125, 25)
         ctx.line_to(225, 225)
         ctx.rel_line_to(-200, 0)
         ctx.close_path()
         ctx.set_source_rgba(0, 0, 0.5, 1)
         ctx.stroke()

def Test():
  import time
  import pyGuiWrapper as gui
  gui.start(lambda x: MyPanel(x))
  time.sleep(2)
  
  model = Model()
  gui.app.frame.panel.setModel(model)
  pass

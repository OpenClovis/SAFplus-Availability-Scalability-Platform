import pdb

import wx
#from wx.py import shell,
#from wx.py import  version
import math
import svg
try:
    import wx.lib.wxcairo
    import cairo
    import rsvg
    haveCairo = True
except ImportError:
    haveCairo = False

from model import *
from entity import *
thePanel = None


class Panel(wx.Panel):
    def __init__(self, parent,menubar,toolbar,statusbar,model):
        global thePanel
        wx.Panel.__init__(self, parent, -1, style=wx.SUNKEN_BORDER)
        self.lockedSvg = svg.SvgFile("locked.svg") 
        self.unlockedSvg = svg.SvgFile("unlocked.svg") 
        self.lockedBmp = self.lockedSvg.bmp((24,24))
        self.unlockedBmp = self.unlockedSvg.bmp((24,24))
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        

        thePanel = self
        e = model.entities["MyServiceGroup"]
        self.showEntity(e)


    def showEntity(self,ent):
      items = ent.et.data.items()
      items = sorted(items,EntityTypeSortOrder)
      sizer = wx.GridBagSizer(len(items)+1,3)
      row=0
      prompt = wx.StaticText(self,-1,"Name:",style =wx.ALIGN_RIGHT)
      query  = wx.TextCtrl(self, -1, "")
      sizer.Add(prompt,(row,0))
      sizer.Add(query,(row,1),flag=wx.EXPAND)
      sizer.Add(wx.StaticBitmap(self, -1, self.unlockedBmp),(row,2))
      row += 1
      for item in items:
        if type(item[1]) is DictType:
          s = item[1].get("prompt",item[0])
          print s
          if not s: s = item[0]
          prompt = wx.StaticText(self,-1,s,style =wx.ALIGN_RIGHT)
          query  = wx.TextCtrl(self, -1, "")
          #b = wx.BitmapButton(self, -1, self.unlockedBmp if row&1 else self.lockedBmp)
          b = wx.BitmapButton(self, -1, self.unlockedBmp,style = wx.NO_BORDER )
          b.SetBitmap(self.unlockedBmp);
          b.SetBitmapSelected(self.lockedBmp)
          b.SetToolTipString("Allow/disallow changes during instantiation")
          sizer.Add(prompt,(row,0))
          sizer.Add(query,(row,1),flag=wx.EXPAND)
          sizer.Add(b,(row,2))
          row +=1
      sizer.AddGrowableCol(1)
      self.SetSizer(sizer)
      self.Refresh()

    def OnPaint(self, evt):
        if self.IsDoubleBuffered():
            dc = wx.PaintDC(self)
        else:
            dc = wx.BufferedPaintDC(self)
        dc.SetBackground(wx.Brush('white'))
        dc.Clear()
        
        #self.Render(dc)

    def Render(self, dc):
        # now draw something with cairo
        ctx = wx.lib.wxcairo.ContextFromDC(dc)
        ctx.set_line_width(15)
        ctx.move_to(125, 25)
        ctx.line_to(225, 225)
        ctx.rel_line_to(-200, 0)
        ctx.close_path()
        ctx.set_source_rgba(0, 0, 0.5, 1)
        ctx.stroke()

        # and something else...
        ctx.arc(200, 200, 80, 0, math.pi*2)
        ctx.set_source_rgba(0, 1, 1, 0.5)
        ctx.fill_preserve()
        ctx.set_source_rgb(1, 0.5, 0)
        ctx.stroke()

        # Draw some text
        face = wx.lib.wxcairo.FontFaceFromFont(
            wx.FFont(10, wx.SWISS, wx.FONTFLAG_BOLD))
        ctx.set_font_face(face)
        ctx.set_font_size(60)
        ctx.move_to(360, 180)
        ctx.set_source_rgb(0, 0, 0)
        ctx.show_text("Hello")

        # Text as a path, with fill and stroke
        ctx.move_to(400, 220)
        ctx.text_path("World")
        ctx.set_source_rgb(0.39, 0.07, 0.78)
        ctx.fill_preserve()
        ctx.set_source_rgb(0,0,0)
        ctx.set_line_width(2)
        ctx.stroke()

        #Render svg file
        cairo_surface = self.SvgToCairo('comp')
        ctx.translate(100,200)
        ctx.scale(0.7, 0.7)
        ctx.rotate(0)
        ctx.set_source_surface(cairo_surface, 100, 200)
        ctx.identity_matrix()
        ctx.paint()

        cairo_surface = self.SvgToCairo('csi')
        ctx.translate(100,200)
        ctx.scale(0.7, 0.7)
        ctx.rotate(0)
        ctx.set_source_surface(cairo_surface, 300, 200)
        ctx.identity_matrix()
        ctx.paint()

        cairo_surface = self.SvgToCairo('node')
        ctx.translate(100,200)
        ctx.scale(0.7, 0.7)
        ctx.rotate(0)
        ctx.set_source_surface(cairo_surface, 500, 200)
        ctx.identity_matrix()
        ctx.paint()

    def SvgToCairo(self, iconType, config = {}):
        #dataFileSvg = svg.loadSvgIcon(iconType, config);

        #svgFile = rsvg.Handle(data = dataFileSvg)
        svgwidth = 100 # svgFile.get_property('width')
        svgheight = 100 # svgFile.get_property('height')

        img = cairo.ImageSurface(cairo.FORMAT_ARGB32, svgwidth,svgheight)
        ictx = cairo.Context(img)
        ictx.set_operator(cairo.OPERATOR_SOURCE)
        ictx.set_operator(cairo.OPERATOR_OVER)
        ictx.set_source_rgba(0,0,0,1) #Transparent
        ictx.paint()
        #svgFile.render_cairo(ictx)
        return img


def Test():
  import time
  import pyGuiWrapper as gui

  mdl = Model()
  mdl.load("testModel.xml")

  sgt = mdl.entityTypes["ServiceGroup"]
  sg = mdl.entities["MyServiceGroup"] = Entity(sgt,(0,0),(100,20))

  gui.go(lambda parent,menu,tool,status,m=mdl: Panel(parent,menu,tool,status, m))
  #time.sleep(2)
  #thePanel.showEntity(sg)

import sys
import os
import re
import string
import pdb
from xml.dom import minidom
from string import Template

import wx
import wx.lib.wxcairo
import cairo
import rsvg

import common

def SvgFile(filename):
  filename = common.fileResolver(filename)
  f = open(filename,"r")
  data = f.read()
  f.close()
  return Svg(data)


class Svg:
  def __init__(self, data,svgSize=None):
    self.rawsvg = data
    self.tmpl8  = string.Template(self.rawsvg)
    self.size = svgSize  # This is a workaround because rsvg does not have get_dimensions()

  def instantiate(self, size, subst=None):
    if subst:
      data = self.tmpl8.safe_substitute(subst)
    else:
      data = self.rawsvg
    hdl = rsvg.Handle()
    hdl.write(data)
    hdl.close()
    # svgSize = hdl.get_dimensions()
    # TODO calc scaling to fill size

    image = cairo.ImageSurface(cairo.FORMAT_ARGB32,*size)
    cr = cairo.Context(image)
    cr.set_source_rgba(0,0,0,0)
    cr.paint()
    hdl.render_cairo(cr)
    del hdl
    return image

def blit(ctx, bmp, location, scale=(1,1), rotate=0):
  ctx.save()
  ctx.translate(*location)
  ctx.rotate(rotate)
  ctx.scale(*scale)
  ctx.set_source_surface(bmp,0,0)
  ctx.paint()
  ctx.restore()
  

class TestPanel(wx.Panel):
    def __init__(self, parent):
      wx.Panel.__init__(self, parent, -1, style=wx.SUNKEN_BORDER)
      self.Bind(wx.EVT_PAINT, self.OnPaint)
      self.model=None

    def OnPaint(self, evt):
        dc = wx.BufferedPaintDC(self)
        dc.SetBackground(wx.Brush('white'))
        dc.Clear()        
        self.render(dc)

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

        tmp = SvgFile("sg.svg")
        bmp = tmp.instantiate((256,128),{"entityType":"Service Group", "name":"Test","preferredNumActiveServiceUnits":1,"preferredNumStandbyServiceUnits":2,"preferredNumIdleServiceUnits":3})
        blit(ctx,bmp,(100,100))

        tmp = SvgFile("sg_button.svg")
        bmp = tmp.instantiate((32,32))
        blit(ctx,bmp,(10,10),(4,4))


def TestRender(ctx):
        if 1:
         # now draw something with cairo
         ctx.set_line_width(15)
         ctx.move_to(125, 25)
         ctx.line_to(225, 225)
         ctx.rel_line_to(-200, 0)
         ctx.close_path()
         ctx.set_source_rgba(0, 0, 0.5, 1)
         ctx.stroke()

        tmp = SvgFile("sg.svg")
        bmp = tmp.instantiate((256,128),{"entityType":"Service Group", "name":"Test","preferredNumActiveServiceUnits":1,"preferredNumStandbyServiceUnits":2,"preferredNumIdleServiceUnits":3})
        blit(ctx,bmp,(100,100))

        tmp = SvgFile("sg_button.svg")
        bmp = tmp.instantiate((32,32))
        blit(ctx,bmp,(10,10),(4,4))
  

def Test():
  import time
  import pyGuiWrapper as gui
  gui.go(lambda x,y=TestRender: gui.Panel(x,y))


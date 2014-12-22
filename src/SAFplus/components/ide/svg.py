import sys
import math
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

import genshi
import genshi.template

import common

def SvgFile(filename):
  filename = common.fileResolver(filename)
  f = open(filename,"r")
  data = f.read()
  f.close()
  return Svg(data)

def opt(fn):
  try:
    ret = fn()
    return ret
  except:
    return ""

class Svg:
  def __init__(self, data,svgSize=None):
    self.rawsvg = data
    #self.tmpl8  = string.Template(self.rawsvg)
    self.tmpl8 = genshi.template.MarkupTemplate(self.rawsvg) # genshi.XML(self.rawsvg)
    self.size = svgSize  # This is a workaround because rsvg does not have get_dimensions()
 
  def prep(self,size=None, subst=None):
    if subst:
      subst["_sizeX"] = size[0]  # Allow the image to be changed based on the size
      subst["_sizeY"] = size[1]
      #data = self.tmpl8.safe_substitute(subst)
      data = self.tmpl8.generate(opt=opt,**subst)
      data = data.render('xml')
      # TODO: We need a much more sophisticated substitution, like used in web development
      # so that we can capture complex resizing behavior.  At a minimum substitution needs to be able to evaluate expressions.
    else:
      data = self.rawsvg
    hdl = rsvg.Handle()
    hdl.write(data)
    hdl.close()
    svgSize = (hdl.get_property('width'), hdl.get_property('height'))
    if size is None or size == svgSize:
      size = svgSize
      scale = (1.0,1.0)
    else:
      frac = min( float(size[0])/svgSize[0], float(size[1])/svgSize[1] )
      scale = (frac, frac)  # We don't want it to be distorted
      size = (int(math.ceil(svgSize[0]*frac)), int(math.ceil(svgSize[1]*frac))) # allocate the smallest buffer that fits the image dimensions < the provided size
    return (hdl,size,scale)    

  def bmp(self, size=None, subst=None,bkcol=(230,230,230,wx.ALPHA_OPAQUE)):
    """Return a bitmap.  Using cairo is preferred, but this is useful for things like buttons"""
    (hdl,size,scale) = self.prep(size,subst)
    dc = wx.MemoryDC()
    bitmap = wx.EmptyBitmap(size[0],size[1])
    dc.SelectObject(bitmap)
    dc.SetBackground(wx.Brush(wx.Colour(*bkcol)))  # Note, I don't think bitmap supports alpha
    dc.SetBackgroundMode(wx.TRANSPARENT)
    dc.Clear()
    #dc.SetPen(wx.Pen(wx.Colour(255,1,1,wx.ALPHA_OPAQUE)))
    #dc.SetBrush(wx.Brush(wx.Colour(255,1,1,128)))
    #dc.DrawLine(0,0,size[0],size[1])
    ctx = wx.lib.wxcairo.ContextFromDC(dc)
    ctx.scale(*scale)
    hdl.render_cairo(ctx)
    del ctx
    del dc
    return bitmap 

  def instantiate(self, size=None, subst=None):
    (hdl,size,scale) = self.prep(size,subst)
    image = cairo.ImageSurface(cairo.FORMAT_ARGB32,size[0],size[1])
    cr = cairo.Context(image)
    cr.set_source_rgba(0,0,0,0)
    cr.paint()
    cr.scale(*scale)
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
  


def TestRender(ctx,dc):
        if 1:
         # now draw something with cairo
         ctx.set_line_width(15)
         ctx.move_to(125, 25)
         ctx.line_to(225, 225)
         ctx.rel_line_to(-200, 0)
         ctx.close_path()
         ctx.set_source_rgba(0, 0, 0.5, 1)
         ctx.stroke()

        tmp = SvgFile("sg2.svg")
        bmp = tmp.instantiate((256,128),{"entityType":"Service Group", "name":"Test","preferredNumActiveServiceUnits":1,"preferredNumStandbyServiceUnits":2,"preferredNumIdleServiceUnits":3})
        blit(ctx,bmp,(100,100))
 
        wxbmp = tmp.bmp((256,128),{"entityType":"Service Group", "name":"Test","preferredNumActiveServiceUnits":1,"preferredNumStandbyServiceUnits":2,"preferredNumIdleServiceUnits":3})
        bmpdc = wx.MemoryDC()
        bmpdc.SelectObject(wxbmp)
        dc.Blit(100,500,256,128,bmpdc,0,0)
        del bmpdc

        bmp = tmp.instantiate((64,128),{"entityType":"Service Group", "name":"Test","preferredNumActiveServiceUnits":1,"preferredNumStandbyServiceUnits":2,"preferredNumIdleServiceUnits":3})
        blit(ctx,bmp,(400,100))

        bmp = tmp.instantiate((200,400),{"entityType":"Service Group", "name":"Test","preferredNumActiveServiceUnits":1,"preferredNumStandbyServiceUnits":2,"preferredNumIdleServiceUnits":3})
        blit(ctx,bmp,(100,300))


        tmp = SvgFile("sg_button.svg")
        bmp = tmp.instantiate((64,64))
        blit(ctx,bmp,(10,10),(1,1))
        bmp = tmp.instantiate((16,64))
        blit(ctx,bmp,(100,10),(1,1))
        bmp = tmp.instantiate((64,16))
        blit(ctx,bmp,(200,10),(1,1))
 

def Test():
  import time
  import pyGuiWrapper as gui
  gui.go(lambda parent,menu,tool,status,y=TestRender: gui.Panel(parent,menu,tool,status,y))


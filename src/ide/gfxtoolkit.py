from gfxmath import *

import wx
import wx.lib.wxcairo
import cairo
import rsvg
import yang
import svg
import wx.lib.scrolledpanel as scrolled

import dot
import time
from types import *

class Gesture:
  """A gesture is a combination of activities by the user that combine to form a single action.  For example: drag and drop, drag box.
     Gestures are defined by a particular sequence of mouse actions, but the meaning (the intended result) of those actions is not defined in the gesture.  The effect of actions is defined in the tool.
  """
  def __init__(self):
    self.active = False
    pass
  def start(self,panel,pos):
    """This gesture is activated"""
    pass
  def change(self,panel,event):
    """The mouse state has changed"""
    pass
  def finish(self,panel,pos):
    """Gesture is completed"""
    pass


class Tool:
  """A tool is a programmatic mode that when activated modifies the document or view in a particular manner based on what Gesture the user inputs.
     To understand the difference between Gestures and tools let us look at the example of zoom and selection tools.  In both of these tools, there are at least 2 Gestures: left click (zoom centered there, or select that object), and drag box (zoom so the drawn box fills the entire window, or select all objects inside the box).  
  """
  def __init__(self, panel):
    self.panel = panel

  def OnSelect(self, panel,event):
    """This tool is being used"""
    return False

  def OnUnselect(self,panel,event):
    """This tool is no longer being used"""
    return False
 
  def OnEditEvent(self,panel, event):
    """Some kind of event happened in the editor space that this tool may want to handle"""
    return False

  def render(self,ctx):
    pass

class BoxGesture(Gesture):
  """Creates a greyed box that changes as the mouse is dragged that can be used to size or select something"""
  def __init__(self):
    Gesture.__init__(self)
    self.rect = None
    pass

  def start(self,panel,pos):
    self.active  = True
    self.downPos = pos
    self.panel   = panel
    self.rect    = (pos[0],pos[1],pos[0],pos[1])

  def change(self,panel, event):
    #pos = event.GetPositionTuple()
    pos = panel.CalcUnscrolledPosition(event.GetPositionTuple())
    assert(self.active)
    self.rect=(min(self.downPos[0],pos[0]),min(self.downPos[1],pos[1]),max(self.downPos[0],pos[0]),max(self.downPos[1],pos[1]))
    print "selecting", self.rect
    panel.drawers.add(self)
    panel.Refresh()

  def finish(self, panel, pos):
    print "finish"
    self.active = False
    panel.drawers.discard(self)
    panel.Refresh()
    return self.rect    

  def render(self, ctx):
    if self.rect:
      t = self.rect
      ctx.set_line_width(2)
      ctx.move_to(t[0],t[1])
      ctx.line_to(t[0],t[3])
      ctx.line_to(t[2],t[3])
      ctx.line_to(t[2],t[1])
      ctx.line_to(t[0],t[1])
      ctx.close_path()
      ctx.set_source_rgba(.3, .2, 0.3, .4)
      ctx.stroke_preserve()
      ctx.set_source_rgba(.5, .3, 0.5, .05)
      ctx.fill()

class LineGesture(Gesture):
  """Creates a grey line that changes as the mouse is dragged that can be used connect things"""
  def __init__(self,circleRadius=5,color=(0,0,0,.9)):
    Gesture.__init__(self)
    self.color = color
    self.circleRadius = circleRadius
    self.downPos = self.curPos = None

  def start(self,panel,pos):
    self.active  = True
    self.downPos = self.curPos = pos
    self.panel   = panel
    panel.drawers.add(self)

  def change(self,panel, event):
    #self.curPos = event.GetPositionTuple()
    self.curPos = panel.CalcUnscrolledPosition(event.GetPositionTuple())
    assert(self.active)
    panel.Refresh()

  def finish(self, panel, pos):
    self.active = False
    panel.drawers.discard(self)
    panel.Refresh()
    return (self.downPos,self.curPos)

  def render(self, ctx):
    if self.downPos:      
      ctx.set_source_rgba(*self.color)
      ctx.set_line_width(6)
      ctx.move_to(*self.downPos)
      ctx.line_to(*self.curPos)
      ctx.close_path()
      ctx.stroke()
      if 1:
        ctx.arc(self.downPos[0],self.downPos[1], self.circleRadius, 0, 2*3.141592637);
        ctx.close_path()
        ctx.stroke_preserve()
        ctx.set_source_rgba(self.color[0],self.color[1],self.color[2],self.color[3])
        ctx.fill()

class LazyLineGesture(Gesture,wx.Timer):
  """Creates a grey line that changes as the mouse is dragged that can be used connect things.  This class adds additional eye-candy -- when you drag the line quickly it resists the motion as if it was in water and curves!  The curve slowly straightens back into a line.
  """
  def __init__(self,color=(0,0,0,.9),circleRadius=5,linethickness=4,arrowlen=15, arrowangle=PI/8):
    Gesture.__init__(self)
    wx.Timer.__init__(self)
    self.cust = dot.Dot()
    
    self.cust.color = color
    self.cust.buttonRadius = circleRadius
    self.cust.lineThickness = linethickness
    self.cust.arrowLength = arrowlen
    self.cust.arrowAngle = arrowangle

    self.downPos = self.curPos = None
    self.center = None

    self.frameInterval = 10   # Targeted milliseconds between frame redraws
    self.frameAverage = self.frameInterval  # Actual (measured) milliseconds between redraws
    self.lastFrame = None  # When the last Notify occurred

    self.settleTime = 250.0  # How long in milliseconds should the line take to straighten

  def Notify(self):
    realCenter=((self.curPos[0]+self.downPos[0])/2.0,(self.curPos[1]+self.downPos[1])/2.0)
    centerVector = ((realCenter[0]-self.center[0])/(self.settleTime/self.frameInterval),(realCenter[1]-self.center[1])/(self.settleTime/self.frameInterval))
    self.center = (self.center[0]+centerVector[0],self.center[1]+centerVector[1])  # move the center that way.
    self.panel.Refresh()

    # This code adjusts the animation speed by examining the frame rate
    tmp = time.time()*1000
    elapsed = (tmp - self.lastFrame)  # In milliseconds
    self.frameAverage = (self.frameAverage*19.0 + elapsed)/20.0  # averaging the frames

    if self.frameAverage > self.frameInterval*1.2 or self.frameAverage < self.frameInterval*.8:
      # Actual frame interval is different than what the code was hoping for, so adjust the code accordingly
      self.frameInterval = int(self.frameAverage)
      self.Stop()
      self.Start(self.frameInterval)
    self.lastFrame = tmp
    #print tmp, self.frameInterval, self.frameAverage

  def start(self,panel,pos):
    self.active  = True
    self.center = self.downPos = self.curPos = pos
    self.panel   = panel
    self.timer = wx.Timer(self)
    self.lastFrame = time.time()*1000
    wx.Timer.Start(self,self.frameInterval)
    panel.drawers.add(self)

  def change(self,panel, event):
    #self.curPos = event.GetPositionTuple()
    self.curPos = panel.CalcUnscrolledPosition(event.GetPositionTuple())
    assert(self.active)
    panel.Refresh()

  def finish(self, panel, pos):
    self.active = False
    panel.drawers.discard(self)
    panel.Refresh()
    self.Stop()
    self.timer = None
    return (self.downPos,self.curPos,self.center)

  def render(self, ctx):
    if self.downPos:   
      drawCurvyArrow(ctx, self.downPos,self.curPos,[self.center],self.cust)


class FadingX(wx.Timer):
  """Creates a grey line that changes as the mouse is dragged that can be used connect things"""
  def __init__(self,panel,pos,time=3):
    wx.Timer.__init__(self)
    self.panel = panel
    self.sz=(15,15)
    self.pos = pos
    self.count = time*10
    self.color = (1,0,0,1)
    self.inc   = 1.0/self.count
    self.lineThickness = 6
    panel.drawers.add(self)
    wx.Timer.Start(self,100)

  def Notify(self):
    self.count -= 1
    self.color = (self.color[0], self.color[1], self.color[2], self.color[3]-self.inc)
    if self.count == 0:  # All done
      self.panel.drawers.discard(self)
      self.Stop()
    self.panel.Refresh()

  def render(self, ctx):
    ctx.save()
    ctx.set_source_rgba(*self.color)
    ctx.set_line_width(self.lineThickness)
    ctx.set_line_cap(cairo.LINE_CAP_ROUND)
    ctx.move_to(self.pos[0]-self.sz[0],self.pos[1]-self.sz[1])
    ctx.line_to(self.pos[0]+self.sz[0],self.pos[1]+self.sz[1])
    ctx.move_to(self.pos[0]+self.sz[0],self.pos[1]-self.sz[1])
    ctx.line_to(self.pos[0]-self.sz[0],self.pos[1]+self.sz[1])
    ctx.stroke()
    ctx.restore()

def drawCurvyArrow(ctx, startPos,endPos,middlePos,cust):
      if type(cust) is DictType: cust = dot.Dot(cust)
      if not middlePos:
        middlePos = [((startPos[0]+endPos[0])/2,(startPos[1]+endPos[1])/2)]
      ctx.save()
      ctx.set_source_rgba(*cust.color)
      ctx.set_line_width(cust.lineThickness)

      # We don't want the curvy arrow's line to lay over the beginning button
      buttonRadius = cust.get("buttonRadius", 4)
      if buttonRadius: # so go a few pixels in the correct direction
        lv = lineVector(startPos,middlePos[0])
        sp = (startPos[0] + lv[0]*(buttonRadius-.05),startPos[1] + lv[1]*(buttonRadius-.05))
      else:
        sp = startPos


      # Calculate the arrow at the end of the line
      (a,b) = calcArrow(middlePos[0][0],middlePos[0][1],endPos[0],endPos[1],cust.arrowLength,cust.arrowAngle)

      # We don't want the line to overlap the arrow at the end of it, so 
      # calculate a new endpoint for the line by finding the midpoint of the back of the arrow
      ep = ((a[0]+b[0])/2,(a[1]+b[1])/2)

      ctx.move_to(*sp)
      try:
        ctx.curve_to (middlePos[0][0],middlePos[0][1],middlePos[0][0],middlePos[0][1], ep[0],ep[1]);
      except:
        pdb.set_trace()

      ctx.stroke()

      ctx.set_line_width(.1)  # These will be filled, so set the containing line width to be invisibly thin

      if buttonRadius:
        ctx.arc(startPos[0],startPos[1], cust.buttonRadius, 0, 2*3.141592637);
        ctx.close_path()
        ctx.stroke_preserve()
        ctx.set_source_rgba(cust.color[0],cust.color[1],cust.color[2],cust.color[3])
        ctx.fill()

      # Draw the arrow on the end
      ctx.set_source_rgba(*cust.color)
      ctx.move_to(endPos[0],endPos[1])
      ctx.line_to(*a)
      ctx.line_to(*b)
      ctx.close_path()
      ctx.stroke_preserve()
      ctx.fill()
      ctx.restore()

def drawIntersectRect(ctx, rect):
  if 0: # account for scaling
    ctx.save()
    ctx.set_font_size(12)
    ctx.select_font_face("Georgia", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_BOLD)
    text = "AMF Configuration"
    xbearing, ybearing, width, height, xadvance, yadvance = (ctx.text_extents(text))
    if width < rect.width-8:
      ctx.move_to(rect.x + 4 + (rect.width-8)/2  - width/2, rect.y + (rect.height-8)/2)
      ctx.set_source_rgba(.3, .2, 0.3, .4)
      ctx.show_text(text)
    ctx.fill()
    ctx.restore()

class BoxGesture(Gesture):
  """Creates a greyed box that changes as the mouse is dragged that can be used to size or select something"""
  def __init__(self):
    Gesture.__init__(self)
    self.rect = None
    pass

  def start(self,panel,pos):
    self.active  = True
    self.downPos = pos
    self.panel   = panel
    self.rect    = (pos[0],pos[1],pos[0],pos[1])

  def change(self,panel, event):
    #pos = event.GetPositionTuple()
    pos = panel.CalcUnscrolledPosition(event.GetPositionTuple())
    assert(self.active)
    self.rect=(min(self.downPos[0],pos[0]),min(self.downPos[1],pos[1]),max(self.downPos[0],pos[0]),max(self.downPos[1],pos[1]))
    print "selecting", self.rect
    panel.drawers.add(self)
    panel.Refresh()

  def finish(self, panel, pos):
    self.active = False
    panel.drawers.discard(self)
    panel.Refresh()
    return self.rect    

  def render(self, ctx):
    if self.rect:
      t = self.rect
      ctx.set_line_width(2)
      ctx.move_to(t[0],t[1])
      ctx.line_to(t[0],t[3])
      ctx.line_to(t[2],t[3])
      ctx.line_to(t[2],t[1])
      ctx.line_to(t[0],t[1])
      ctx.close_path()
      ctx.set_source_rgba(.3, .2, 0.3, .4)
      ctx.stroke_preserve()
      ctx.set_source_rgba(.5, .3, 0.5, .05)
      ctx.fill()

class LineGesture(Gesture):
  """Creates a grey line that changes as the mouse is dragged that can be used connect things"""
  def __init__(self,circleRadius=5,color=(0,0,0,.9)):
    Gesture.__init__(self)
    self.color = color
    self.circleRadius = circleRadius
    self.downPos = self.curPos = None

  def start(self,panel,pos):
    self.active  = True
    self.downPos = self.curPos = pos
    self.panel   = panel
    panel.drawers.add(self)

  def change(self,panel, event):
    #self.curPos = event.GetPositionTuple()
    self.curPos = panel.CalcUnscrolledPosition(event.GetPositionTuple())
    assert(self.active)
    panel.Refresh()

  def finish(self, panel, pos):
    self.active = False
    panel.drawers.discard(self)
    panel.Refresh()
    return (self.downPos,self.curPos)

  def render(self, ctx):
    if self.downPos:      
      ctx.set_source_rgba(*self.color)
      ctx.set_line_width(6)
      ctx.move_to(*self.downPos)
      ctx.line_to(*self.curPos)
      ctx.close_path()
      ctx.stroke()
      if 1:
        ctx.arc(self.downPos[0],self.downPos[1], self.circleRadius, 0, 2*3.141592637);
        ctx.close_path()
        ctx.stroke_preserve()
        ctx.set_source_rgba(self.color[0],self.color[1],self.color[2],self.color[3])
        ctx.fill()

class LazyLineGesture(Gesture,wx.Timer):
  """Creates a grey line that changes as the mouse is dragged that can be used connect things.  This class adds additional eye-candy -- when you drag the line quickly it resists the motion as if it was in water and curves!  The curve slowly straightens back into a line.
  """
  def __init__(self,color=(0,0,0,.9),circleRadius=5,linethickness=4,arrowlen=15, arrowangle=PI/8):
    Gesture.__init__(self)
    wx.Timer.__init__(self)
    self.cust = dot.Dot()
    
    self.cust.color = color
    self.cust.buttonRadius = circleRadius
    self.cust.lineThickness = linethickness
    self.cust.arrowLength = arrowlen
    self.cust.arrowAngle = arrowangle

    self.downPos = self.curPos = None
    self.center = None

    self.frameInterval = 10   # Targeted milliseconds between frame redraws
    self.frameAverage = self.frameInterval  # Actual (measured) milliseconds between redraws
    self.lastFrame = None  # When the last Notify occurred

    self.settleTime = 250.0  # How long in milliseconds should the line take to straighten

  def Notify(self):
    realCenter=((self.curPos[0]+self.downPos[0])/2.0,(self.curPos[1]+self.downPos[1])/2.0)
    centerVector = ((realCenter[0]-self.center[0])/(self.settleTime/self.frameInterval),(realCenter[1]-self.center[1])/(self.settleTime/self.frameInterval))
    self.center = (self.center[0]+centerVector[0],self.center[1]+centerVector[1])  # move the center that way.
    self.panel.Refresh()

    # This code adjusts the animation speed by examining the frame rate
    tmp = time.time()*1000
    elapsed = (tmp - self.lastFrame)  # In milliseconds
    self.frameAverage = (self.frameAverage*19.0 + elapsed)/20.0  # averaging the frames

    if self.frameAverage > self.frameInterval*1.2 or self.frameAverage < self.frameInterval*.8:
      # Actual frame interval is different than what the code was hoping for, so adjust the code accordingly
      self.frameInterval = int(self.frameAverage)
      self.Stop()
      self.Start(self.frameInterval)
    self.lastFrame = tmp
    #print tmp, self.frameInterval, self.frameAverage

  def start(self,panel,pos):
    self.active  = True
    self.center = self.downPos = self.curPos = pos
    self.panel   = panel
    self.timer = wx.Timer(self)
    self.lastFrame = time.time()*1000
    wx.Timer.Start(self,self.frameInterval)
    panel.drawers.add(self)

  def change(self,panel, event):
    #self.curPos = event.GetPositionTuple()
    self.curPos = panel.CalcUnscrolledPosition(event.GetPositionTuple())
    assert(self.active)
    panel.Refresh()

  def finish(self, panel, pos):
    self.active = False
    panel.drawers.discard(self)
    panel.Refresh()
    self.Stop()
    self.timer = None
    return (self.downPos,self.curPos,self.center)

  def render(self, ctx):
    if self.downPos:   
      drawCurvyArrow(ctx, self.downPos,self.curPos,[self.center],self.cust)
 
def getRectInstance(ent, scale):
  return wx.Rect(ent.pos[0]*scale, ent.pos[1]*scale, ent.size[0]*ent.scale[0]*scale, ent.size[1]*ent.scale[1]*scale)

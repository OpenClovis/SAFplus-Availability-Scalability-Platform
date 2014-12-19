import pdb
import math
import time
from types import *

import wx
import wx.lib.wxcairo
import cairo
import rsvg
import yang
import svg
import wx.lib.scrolledpanel as scrolled
#from wx.py import shell,
#from wx.py import  version
import dot
from common import *
from module import Module
from entity import *
from model import Model

import share
 
ENTITY_TYPE_BUTTON_START = 100
ZOOM_BUTTON = 99
CONNECT_BUTTON = 98
SELECT_BUTTON = 97

COL_MARGIN = 100
COL_SPACING = 2
COL_WIDTH = 128

ROW_MARGIN = 64
ROW_SPACING = 2
ROW_WIDTH   = 50

PI = 3.141592636

linkNormalLook = dot.Dot({ "color":(0,0,.8,.75), "lineThickness": 4, "buttonRadius": 6, "arrowLength":15, "arrowAngle": PI/8 })

def lineRectIntersect(lineIn,lineOut, rectUL, rectLR):
  """Given a line segment defined by 2 points: lineIn and lineOut where lineIn is INSIDE the rectangle
     Given a rectangle defined by 2 points rectUL and rectLR where rectUL < rectLR (ul is upper left) and where the rectangle is aligned with the axes.

     Return the point where the line intersects the rectangle.

     This function can be used to make the pointer arrows start and end at the edges of the entities
  """
  # TODO
  return lineIn 

def calcArrow(start_x, start_y, end_x, end_y,length=5.0,degrees=PI/10.0):
  angle = math.atan2 (end_y - start_y, end_x - start_x) + PI;
  x1 = end_x + length * math.cos(angle - degrees);
  y1 = end_y + length * math.sin(angle - degrees);
  x2 = end_x + length * math.cos(angle + degrees);
  y2 = end_y + length * math.sin(angle + degrees);
  return((x1,y1),(x2,y2))

def lineVector(a,b):
  ret = (b[0]-a[0], b[1]-a[1])
  length = math.sqrt(ret[0]*ret[0] + ret[1]*ret[1])
  if length == 0:
    return (0,0)
  ret = (ret[0]/length, ret[1]/length)
  return ret

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


def rectOverlaps(rect_a, rect_b):
    """Assumes tuples of format x0,y0, x1,y1 where 0 is upper left (lowest), 1 is lower right
       This code finds if the rectangles are separate by checking if the right side of one rectangle is further left then the leftmost side of the other, etc...
    """
    print rect_a, rect_b
    separate = rect_a[2] < rect_b[0] or rect_a[0] > rect_b[2] or rect_a[1] > rect_b[3] or rect_a[3] < rect_b[1]
    return not separate

class Gesture:
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



class Tool:
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

class EntityTool(Tool):
  def __init__(self, panel,entity):
    self.entity = entity
    Tool.__init__(self,panel)
    self.box = BoxGesture()

  def OnSelect(self, panel,event):
    panel.statusBar.SetStatusText("Click to create a new %s" % self.entity.data["name"],0);
    return True

  def OnUnselect(self,panel,event):
    print "Unselected %s" % self.entity.data["name"]
    return True

  def OnEditEvent(self,panel, event):
    pos = panel.CalcUnscrolledPosition(event.GetPositionTuple())
    ret = False
    if isinstance(event,wx.MouseEvent):
      if event.ButtonDown(wx.MOUSE_BTN_LEFT):  # Select
        self.box.start(panel,pos)
        ret = True
      elif event.Dragging():
        self.box.change(panel,event)
        ret = True
      elif event.ButtonUp(wx.MOUSE_BTN_LEFT):
        rect = self.box.finish(panel,pos)
        size = (rect[2]-rect[0],rect[3]-rect[1])
        if size[0] < 15 or size[1] < 15:  # its so small it was probably an accidental drag rather then a deliberate sizing
          size = None
        ret = self.panel.CreateNewInstance(self.entity,(rect[0],rect[1]),size)
    return ret
    

class LinkTool(Tool):
  def __init__(self, panel):
    Tool.__init__(self,panel)
    self.line =   LazyLineGesture(color=(0,0,0,.9),circleRadius=5,linethickness=4,arrowlen=15, arrowangle=PI/8)
    self.startEntity = None
    self.err = None

  def OnSelect(self, panel,event):
    panel.statusBar.SetStatusText("Click on an entity and drag to another entity to create a relationship.",0);
    return True

  def OnUnselect(self,panel,event):
    return True

  def OnEditEvent(self,panel, event):
    #pos = event.GetPositionTuple()
    pos = panel.CalcUnscrolledPosition(event.GetPositionTuple())
    ret = False
    if isinstance(event,wx.MouseEvent):
      if event.ButtonDown(wx.MOUSE_BTN_LEFT):  # Select
        entities = panel.findEntitiesAt(pos)
        if len(entities) != 1:
          panel.statusBar.SetStatusText("You must choose a single starting entity!",0);
          # TODO show a red X under the cursor
          self.err = FadingX(panel,pos,2)
        else:
          self.startEntity = entities.pop()
          self.line.start(panel,pos)          
        ret = True

      elif event.Dragging():
        if self.startEntity: self.line.change(panel,event)
        ret = True

      elif event.ButtonUp(wx.MOUSE_BTN_LEFT):
        if self.startEntity:
          line = self.line.finish(panel,pos)
          entities = panel.findEntitiesAt(pos)
          if len(entities) != 1:
            panel.statusBar.SetStatusText("You must choose a single ending entity!",0);
            self.err = FadingX(panel,pos,1)
          else:
            self.endEntity = entities.pop()
            if not self.startEntity.canContain(self.endEntity):
              panel.statusBar.SetStatusText("Relationship is not allowed.  Most likely %s entities cannot contain %s entities.  Or maybe you've exceeded the number of containments allowed" % (self.startEntity.et.name, self.endEntity.et.name),0);
              self.err = FadingX(panel,pos,1)
            elif not self.endEntity.canBeContained(self.startEntity):
              panel.statusBar.SetStatusText("Relationship is not allowed.  Most likely %s entities can only be contained by one %s entity."  % (self.endEntity.et.name, self.startEntity.et.name),0);
              self.err = FadingX(panel,pos,1)
            else:
              # Add the arrow into the model.  the arrow's location is relative to the objects it connects
              ca = ContainmentArrow(self.startEntity, (line[0][0]-self.startEntity.pos[0],line[0][1] - self.startEntity.pos[1]), self.endEntity, (line[1][0]-self.endEntity.pos[0],line[1][1]-self.endEntity.pos[1]), [line[2]] if len(line)>1 else None)
              self.startEntity.containmentArrows.append(ca)

    return ret
    
  def CreateNewInstance(self,panel,position,size=None):
    """Create a new instance of this entity type at this position"""
    panel.statusBar.SetStatusText("Created %s" % self.entityType.name,0);
    panel.entities.append(self.entityType.createEntity(position, size))
    panel.Refresh()
    return True
 

class SelectTool(Tool):
  def __init__(self, panel):
    self.panel = panel
    self.defaultStatusText = "Click to edit configuration.  Double click to expand/contract.  Drag to move."
    self.selected = set()  # This is everything that is currently selected... using shift or ctrl click may mean the more is selected then currently touching
    self.touching = set()  # This is everything that the cursor is currently touching
    self.rect = None       # Will be something if a rectangle selection is being used
    #self.downPos = None    # Where the left mouse button was pressed
    self.boxSel = BoxGesture()

  def OnSelect(self, panel,event):
    panel.statusBar.SetStatusText(self.defaultStatusText,0);
    return True

  def OnUnselect(self,panel,event):
    pass

  def render(self,ctx):
    pass

  def OnEditEvent(self,panel, event):
    pos = panel.CalcUnscrolledPosition(event.GetPositionTuple())
    if isinstance(event,wx.MouseEvent):
      if event.ButtonDown(wx.MOUSE_BTN_LEFT):  # Select
        entities = panel.findEntitiesAt(pos)
        self.dragPos = pos
        if not entities:
          panel.statusBar.SetStatusText(self.defaultStatusText,0);
          self.touching = set()
          self.boxSel.start(panel,pos)
          return False
        print "Touching %s" % ", ".join([ e.data["name"] for e in entities])
        panel.statusBar.SetStatusText("Touching %s" % ", ".join([ e.data["name"] for e in entities]),0);

        # If you touch something else, your touching set changes.  But if you touch something in your current touch group then nothing changes
        # This enables behavior like selecting a group of entities and then dragging them (without using the ctrl key)
        if not entities.issubset(self.touching):
          self.touching = set(entities)
        # If the control key is down, then add to the currently selected group, otherwise replace it.
        if event.ControlDown():
          self.selected = self.selected.union(self.touching)
        else: self.selected = self.touching.copy()
        self.updateSelected()
        return True
      if event.Dragging():
        # if you are touching anything, then drag everything
        if self.touching and self.dragPos:
          delta = (pos[0]-self.dragPos[0], pos[1] - self.dragPos[1])
          # TODO deal with scaling and rotation in delta
          if delta[0] != 0 and delta[1] != 0:
            for e in self.selected:
              e.pos = (e.pos[0] + delta[0], e.pos[1] + delta[1])  # move all the touching objects by the amount the mouse moved
          self.dragPos = pos
          panel.Refresh()
        else:  # touching nothing, this is a selection rectangle
          self.boxSel.change(panel,event)

      if event.ButtonUp(wx.MOUSE_BTN_LEFT):
        # TODO: Move the data in this entity to the configuration editing sidebar, and expand it if its minimized.
        
        # Or find everything inside a selection box
        if self.boxSel.active:
          #panel.drawers.discard(self)
          self.touching = panel.findEntitiesTouching(self.boxSel.finish(panel,pos))

          if event.ControlDown():
            self.selected = self.selected.union(self.touching)
          else: 
            self.selected = self.touching
          panel.Refresh()
        
      elif event.ButtonDClick(wx.MOUSE_BTN_LEFT):
        entity = panel.findEntitiesAt(pos)
        if not entity: return False
    self.updateSelected()
  
  def updateSelected(self):
    if len(self.selected) == 1:
      if share.detailsPanel:
        share.detailsPanel.showEntity(next(iter(self.selected)))

class ZoomTool(Tool):
  def __init__(self, panel):
    self.panel = panel
    self.defaultStatusText = "Click (+) to zoom in.  Right click (-) to zoom out."
    self.rect = None       # Will be something if a rectangle selection is being used
    #self.downPos = None    # Where the left mouse button was pressed
    self.boxSel = BoxGesture()
    self.scale = 1
    self.scaleRange = 0.2
    self.minScale = 0.2
    self.maxScale = 10

  def OnSelect(self, panel,event):
    panel.statusBar.SetStatusText(self.defaultStatusText,0);
    return True

  def OnUnselect(self,panel,event):
    pass

  def render(self,ctx):
    pass

  def OnEditEvent(self,panel, event):
    pos = panel.CalcUnscrolledPosition(event.GetPositionTuple())
    if isinstance(event, wx.MouseEvent):
      scale = self.scale
      if event.ButtonDown(wx.MOUSE_BTN_LEFT) or event.ButtonDown(wx.MOUSE_BTN_RIGHT):  # Select
        self.boxSel.start(panel,pos)
      elif event.Dragging():
        # if you are touching anything, then drag everything
        self.boxSel.change(panel,event)
      elif event.ButtonUp(wx.MOUSE_BTN_LEFT):
        rect = self.boxSel.finish(panel,pos)
        delta = (rect[2]-rect[0],rect[3]-rect[1])
        # Center rect selected
        pos[0] = rect[0] + delta[0]/2
        pos[1] = rect[1] + delta[1]/2

        if (self.scale + self.scaleRange) < self.maxScale:
          scale = self.scale + self.scaleRange

      elif event.ButtonUp(wx.MOUSE_BTN_RIGHT):
        rect = self.boxSel.finish(panel,pos)
        delta = (rect[2]-rect[0],rect[3]-rect[1])
        # Center rect selected
        pos[0] = rect[0] + delta[0]/2
        pos[1] = rect[1] + delta[1]/2

        if (self.scale - self.scaleRange) > self.minScale:
          scale = self.scale - self.scaleRange

      self.SetScale(scale, pos)

      # TODO: 
      # '-' => Zoom out center
      # '+' => Zoom in center
      # 'Ctrl + 0' => Reset


  def SetScale(self, scale, pos):
    if scale != self.scale:
      self.scale = scale

      # Update scale for entities
      for (name,e) in self.panel.entities.items():
        e.scale = (self.scale, self.scale)

      self.panel.Refresh()

      # TODO: scroll wrong??? 
      scrollx, scrolly = self.panel.GetScrollPixelsPerUnit();
      size = self.panel.GetClientSize()
      self.panel.Scroll((pos[0] - size.x/2)/scrollx, (pos[1] - size.y/2)/scrolly)

# Global of this panel for debug purposes only.  DO NOT USE IN CODE
dbgIep = None

class Panel(scrolled.ScrolledPanel):
    def __init__(self, parent,menubar,toolbar,statusbar,model):
      scrolled.ScrolledPanel.__init__(self, parent, style = wx.TAB_TRAVERSAL|wx.SUNKEN_BORDER)
      # These variables define what types can be instantiated in the outer row/columns of the edit tool
      self.rowTypes = ["ServiceGroup","Application"]
      self.columnTypes = ["Node"]

      self.SetupScrolling(True, True)
      self.SetScrollRate(10, 10)
      self.Bind(wx.EVT_SIZE, self.OnReSize)

      self.Bind(wx.EVT_PAINT, self.OnPaint)
      dbgIep = self 
      self.menuBar = menubar
      self.toolBar = toolbar
      self.statusBar = statusbar
      self.model=model
      self.tool = None  # The current tool
      self.drawers = set()

      # The position of the panel's viewport within the larger drawing
      self.location = (0,0)
      self.rotate = 0.0
      self.scale = (1.0,1.0)

      # Buttons and other IDs that are registered may need to be looked up to turn the ID back into a python object
      self.idLookup={}  

      # Ordering of instances in the GUI display, from the upper left
      self.columns = []
      self.rows = []

      # Add toolbar buttons

      # first get the toolbar    
      if not self.toolBar:
        self.toolBar = wx.ToolBar(self,-1)
        self.toolBar.SetToolBitmapSize((24,24))

      tsize = self.toolBar.GetToolBitmapSize()

      # example of adding a standard button
      #new_bmp =  wx.ArtProvider.GetBitmap(wx.ART_NEW, wx.ART_TOOLBAR, tsize)
      #self.toolBar.AddLabelTool(10, "New", new_bmp, shortHelp="New", longHelp="Long help for 'New'")

      # Add the umlEditor's standard tools
      self.toolBar.AddSeparator()
      bitmap = svg.SvgFile("connect.svg").bmp(tsize, { }, (222,222,222,wx.ALPHA_OPAQUE))
      self.toolBar.AddRadioTool(CONNECT_BUTTON, bitmap, wx.NullBitmap, shortHelp="connect", longHelp="Draw relationships between entities")
      self.idLookup[CONNECT_BUTTON] = LinkTool(self)

      bitmap = svg.SvgFile("pointer.svg").bmp(tsize, { }, (222,222,222,wx.ALPHA_OPAQUE))
      self.toolBar.AddRadioTool(SELECT_BUTTON, bitmap, wx.NullBitmap, shortHelp="select", longHelp="Select one or many entities.  Click entity to edit details.  Double click to expand/contract.")
      self.idLookup[SELECT_BUTTON] = SelectTool(self)

      bitmap = svg.SvgFile("zoom.svg").bmp(tsize, { }, (222,222,222,wx.ALPHA_OPAQUE))
      self.toolBar.AddRadioTool(ZOOM_BUTTON, bitmap, wx.NullBitmap, shortHelp="zoom", longHelp="Left click (+) to zoom in. Right click (-) to zoom out.")
      self.idLookup[ZOOM_BUTTON] = ZoomTool(self)

      # Add the custom entity creation tools as specified by the model's YANG
      self.addEntityTools()

      # Set up to handle tool clicks
      self.toolBar.Bind(wx.EVT_TOOL, self.OnToolClick)  # id=start, id2=end to bind a range
      self.toolBar.Bind(wx.EVT_TOOL_RCLICKED, self.OnToolRClick)

      # Set up events that tools may be interested in
      toolEvents = [ wx.EVT_LEAVE_WINDOW, wx.EVT_LEFT_DCLICK, wx.EVT_LEFT_DOWN , wx.EVT_LEFT_UP, wx.EVT_MOUSEWHEEL , wx.EVT_MOVE,wx.EVT_MOTION , wx.EVT_RIGHT_DCLICK, wx.EVT_RIGHT_DOWN , wx.EVT_RIGHT_UP, wx.EVT_KEY_DOWN, wx.EVT_KEY_UP]
      for t in toolEvents:
        self.Bind(t, self.OnToolEvent)

    def OnReSize(self, event):
      self.Refresh(False)
      event.Skip()
    
    def addEntityTools(self):
      """Iterate through all the entity types, adding them as tools"""
      tsize = self.toolBar.GetToolBitmapSize()

      sortedent = self.model.entities.items()  # Do this so the buttons always appear in the same order
      sortedent.sort()
      buttonIdx = ENTITY_TYPE_BUTTON_START
      for e in sortedent:
        et = e[1].et
        placement = None
        if e[0] in self.columnTypes or e[1].et.name in self.columnTypes:  # Take any entity or entity of a type that we designate a columnar
          placement = "column"
        if e[0] in self.rowTypes or e[1].et.name in self.rowTypes:  # Take any entity or entity of a type that we designate a columnar
          placement = "row"

        if placement:
          buttonSvg = e[1].buttonSvg if hasattr(e[1],"buttonSvg") else et.buttonSvg
          bitmap = buttonSvg.bmp(tsize, { "name":e[0] }, (222,222,222,wx.ALPHA_OPAQUE))  # Use the first 3 letters of the name as the button text if nothing
          shortHelp = e[1].data.get("shortHelp",et.data.get("help",None)) 
          longHelp = e[1].data.get("help",et.data.get("help",None))
          self.toolBar.AddLabelTool(buttonIdx, e[0], bitmap, shortHelp=shortHelp, longHelp=longHelp)
          #self.toolBar.AddRadioTool(buttonIdx, bitmap, wx.NullBitmap, shortHelp=et[0], longHelp=longHelp,clientData=et)
          self.idLookup[buttonIdx] = EntityTool(self,e[1])  # register this button so when its clicked we know about it
          buttonIdx+=1
          e[1].buttonIdx = buttonIdx

      self.toolBar.Realize()

    def OnToolEvent(self,event):
      handled = False
      if self.tool:
        handled = self.tool.OnEditEvent(self, event)
      event.Skip(not handled)  # if you pass false, event will not be processed anymore

    def OnToolClick(self,event):
      co = event.GetClientObject()
      cd = event.GetClientData()
      id = event.GetId()
      print "Tool Clicked %d %s %s" % (id, str(co), str(cd))
      tool = self.idLookup[id]
      if self.tool:
        self.tool.OnUnselect(self,event)
        self.tool = None
      if tool:
        tool.OnSelect(self,event)
        self.tool = tool

    def OnToolRClick(self,event):
      co = event.GetClientObject()
      cd = event.GetClientData()
      id = event.GetId()
      print "Tool Right Clicked %d %s %s" % (id, str(co), str(cd))      
      tool = self.idLookup[id]
      if tool:
        tool.OnRightClick(self,event)

    def OnPaint(self, event):
        #dc = wx.PaintDC(self)
        dc = wx.BufferedPaintDC(self)
        dc.SetBackground(wx.Brush('white'))
        dc.Clear()
        self.PrepareDC(dc)
        self.UpdateVirtualSize(dc)
        self.render(dc)

    def CreateNewInstance(self,entity,position,size=None):
      """Create a new instance of this entity type at this position"""
      placement = None
      if entity.data["name"] in self.columnTypes or entity.et.name in self.columnTypes:
        placement = "column"
      if entity.data["name"] in self.rowTypes or entity.et.name in self.rowTypes:
        placement = "row"
 
      if placement:
        self.statusBar.SetStatusText("Created instance of %s" % entity.data["name"],0);
        # TODO ent = self.entityType.createEntity(position, size)
        if size is None: size = (COL_WIDTH, ROW_WIDTH)  # The layout will automatically update the long size to be the width of the screen
        inst = entity.createInstance(position, size)
        self.model.instances[inst.data["name"]] = inst

        if placement == "row":
          self.rows.append(inst)  # TODO: calculate an insertion position based on the mouse position and the positions of the other entities
        if placement == "column":
          self.columns.append(inst)  # TODO: calculate an insertion position based on the mouse position and the positions of the other entities

        self.Refresh()

      self.layout()
      return True
 

    def GetBoundingBox(self):
      """Calculate the bounding box -- the smallest box that will fit around all the elements in this panel"""
      virtRct = wx.Rect()
      if len(self.model.instances) > 0:
        elst = self.model.instances.items()
        first = elst[0]
        for (name,e) in elst:
          if e == first:
            virtRct = wx.Rect(e.pos[0], e.pos[1], e.size[0]*e.scale[0], e.size[1]*e.scale[1])
          else:
            virtRct.Union(wx.Rect(e.pos[0], e.pos[1], e.size[0]*e.scale[0], e.size[1]*e.scale[1]))
      return virtRct
 
    def UpdateVirtualSize(self, dc):
      virtRct = self.GetBoundingBox()
      # We need to shift the client rectangle to take into account
      # scrolling, converting device to logical coordinates
      self.SetVirtualSize((virtRct.x + virtRct.Width, virtRct.y + virtRct.Height))

    def layout(self):
        colCount = len(self.columns)
        panelSize = self.GetVirtualSize()
        x = COL_MARGIN
        for i in self.columns:
          width = max(COL_WIDTH,i.size[0])  # Pick the minimum width or the user's adjustment, whichever is bigger
          i.pos = (x,0)
          size = (width,panelSize.y)
          if size != i.size:
            i.size = size
            i.recreateBitmap()
          x+=width+COL_SPACING

        y = ROW_MARGIN
        for i in self.rows:
          width = max(ROW_WIDTH,i.size[1])  # Pick the minimum width or the user's adjustment, whichever is bigger
          i.pos = (0,y)
          size = (panelSize.x,width)
          if size != i.size:
            i.size = size
            i.recreateBitmap()
          y+=width+ROW_SPACING        


    def render(self, dc):
        """Put the entities on the screen"""
        ctx = wx.lib.wxcairo.ContextFromDC(dc)
        # Now draw the graph
        ctx.save()
        # First position the screen's view port into the larger document
        ctx.translate(*self.location)
        ctx.rotate(self.rotate)
        ctx.scale(*self.scale)
        # Now draw the links
        # Now draw the entites


        for (name,e) in self.model.instances.items():
          svg.blit(ctx,e.bmp,e.pos,e.scale,e.rotate)
        # Now draw the containment arrows on top
        for (name,e) in self.model.instances.items():
          for a in e.containmentArrows:
            st = a.container.pos
            end = a.contained.pos
            drawCurvyArrow(ctx, (st[0] + a.beginOffset[0],st[1] + a.beginOffset[1]),(end[0] + a.endOffset[0],end[1] + a.endOffset[1]),a.midpoints, linkNormalLook)
        ctx.restore()

        # These are non-model based transient elements that need to be drawn like selection boxes
        for e in self.drawers:
          e.render(ctx)

    def findEntitiesAt(self,pos):
      """Returns the entity located at the passed position """
      # TODO handle the viewscope's translation, rotation, scaling
      ret = set()
      for (name, e) in self.entities.items():
        furthest= (e.pos[0] + e.size[0]*e.scale[0],e.pos[1] + e.size[1]*e.scale[1])
        #print e.data["name"], ": ", pos, " inside: ", e.pos, " to ", furthest
        if pos[0] >= e.pos[0] and pos[1] >= e.pos[1] and pos[0] <= furthest[0] and pos[1] <= furthest[1]:  # mouse is in the box formed by the entity
          ret.add(e)
      return ret

    def findEntitiesTouching(self,rect):
      """Returns all entities touching the passed rectangle """
      # TODO handle the viewscope's translation, rotation, scaling
      ret = set()
      for (name, e) in self.entities.items():
        furthest= (e.pos[0] + e.size[0]*e.scale[0],e.pos[1] + e.size[1]*e.scale[1])
        if rectOverlaps(rect,(e.pos[0],e.pos[1],furthest[0],furthest[1])):  # mouse is in the box formed by the entity
          ret.add(e)
      return ret

model = None

def Test():
  import time
  import pyGuiWrapper as gui
  global model
  model = Model()
  model.load("testModel.xml")

  # gui.go(lambda parent,menu,tool,status,m=model: Panel(parent,menu,tool,status, m))
  gui.start(lambda parent,menu,tool,status,m=model: Panel(parent,menu,tool,status, m))
  return model

# Notes
#  test automation panel.WarpPointer(self,x,y)  "Move pointer"

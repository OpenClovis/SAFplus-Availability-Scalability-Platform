import pdb

import wx
import wx.lib.wxcairo
import cairo
import rsvg
import yang
import svg
#from wx.py import shell,
#from wx.py import  version

from module import Module
from entity import Entity
from model import Model
 
ENTITY_TYPE_BUTTON_START = 100
CONNECT_BUTTON = 99
SELECT_BUTTON = 98

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

class EntityTypeTool(Tool):
  def __init__(self, panel,entityType):
    self.entityType = entityType
    Tool.__init__(self,panel)

  def OnSelect(self, panel,event):
    panel.statusBar.SetStatusText("Click to create a new %s" % self.entityType.name,0);
    return True

  def OnUnselect(self,panel,event):
    print "Unselected %s" % self.entityType.name
    return True

  def OnEditEvent(self,panel, event):
    ret = False
    if isinstance(event,wx.MouseEvent):
      if event.ButtonUp(wx.MOUSE_BTN_LEFT):
        ret = self.CreateNewInstance(panel,event.GetPositionTuple())
    return ret
    
  def CreateNewInstance(self,panel,position,size=None):
    """Create a new instance of this entity type at this position"""
    panel.statusBar.SetStatusText("Created %s" % self.entityType.name,0);
    panel.entities.append(self.entityType.CreateEntity(position, size))
    panel.Refresh()
    return True
 

class SelectTool(Tool):
  def __init__(self, panel):
    self.panel = panel
    self.defaultStatusText = "Click to edit configuration.  Double click to expand/contract.  Drag to move."

  def OnSelect(self, panel,event):
    panel.statusBar.SetStatusText(self.defaultStatusText,0);
    return True

  def OnUnselect(self,panel,event):
    pass

  def OnEditEvent(self,panel, event):
    pos = event.GetPositionTuple()
    # print event

    if isinstance(event,wx.MouseEvent):
      if event.ButtonDown(wx.MOUSE_BTN_LEFT):  # Select
        entities = panel.findEntitiesAt(pos)
        if not entities: 
          panel.statusBar.SetStatusText(self.defaultStatusText,0);
          self.selected = []
          return False
        # print "Selected %s" % ", ".join([ e.data["name"] for e in entities])
        panel.statusBar.SetStatusText("Selected %s" % ", ".join([ e.data["name"] for e in entities]),0);
        self.selected = entities
        self.dragPos = pos
        return True
      if event.Dragging():
        if self.selected and self.dragPos:
          delta = (pos[0]-self.dragPos[0], pos[1] - self.dragPos[1])
          # TODO deal with scaling and rotation in delta
          if delta[0] != 0 and delta[1] != 0:
            for e in self.selected:
              e.pos = (e.pos[0] + delta[0], e.pos[1] + delta[1])  # move all the selected objects by the amount the mouse moved
          self.dragPos = pos
          panel.Refresh()
      if event.ButtonUp(wx.MOUSE_BTN_LEFT):
        # Move the data in this entity to the configuration editing sidebar, and expand it if its minimized.
        self.dragPos = None
        pass
      elif event.ButtonDClick(wx.MOUSE_BTN_LEFT):
        entity = panel.findEntitiesAt(pos)
        if not entity: return False
    pass
  


class Panel(wx.Panel):
    def __init__(self, parent,menubar,toolbar,statusbar,model):
      wx.Panel.__init__(self, parent, -1, style=wx.SUNKEN_BORDER)
      self.Bind(wx.EVT_PAINT, self.OnPaint)
      self.menuBar = menubar
      self.toolBar = toolbar
      self.statusBar = statusbar
      self.model=model
      self.tool = None  # The current tool

      # The position of the panel's viewport within the larger drawing
      self.location = (0,0)
      self.rotate = 0.0
      self.scale = (1.0,1.0)

      # Buttons and other IDs that are registered may need to be looked up to turn the ID back into a python object
      self.idLookup={}  

      # The list of entities that exist in this model, extracted from the model for easy display
      self.entities = []
      # TODO load entities from the model

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
      self.idLookup[CONNECT_BUTTON] = None

      bitmap = svg.SvgFile("pointer.svg").bmp(tsize, { }, (222,222,222,wx.ALPHA_OPAQUE))
      self.toolBar.AddRadioTool(SELECT_BUTTON, bitmap, wx.NullBitmap, shortHelp="select", longHelp="Select one or many entities.  Click entity to edit details.  Double click to expand/contract.")
      self.idLookup[SELECT_BUTTON] = SelectTool(self)

      # Add the custom entity creation tools as specified by the model's YANG
      self.addEntityTools()

      # Set up to handle tool clicks
      self.toolBar.Bind(wx.EVT_TOOL, self.OnToolClick)  # id=start, id2=end to bind a range
      self.toolBar.Bind(wx.EVT_TOOL_RCLICKED, self.OnToolRClick)

      # Set up events that tools may be interested in
      toolEvents = [ wx.EVT_LEAVE_WINDOW, wx.EVT_LEFT_DCLICK, wx.EVT_LEFT_DOWN , wx.EVT_LEFT_UP, wx.EVT_MOUSEWHEEL , wx.EVT_MOVE,wx.EVT_MOTION , wx.EVT_RIGHT_DCLICK, wx.EVT_RIGHT_DOWN , wx.EVT_RIGHT_UP, wx.EVT_KEY_DOWN, wx.EVT_KEY_UP]
      for t in toolEvents:
        self.Bind(t, self.OnToolEvent)

    def addEntityTools(self):
      """Iterate through all the entity types, adding them as tools"""
      tsize = self.toolBar.GetToolBitmapSize()

      sortedEt = self.model.entityTypes.items()  # Do this so the buttons always appear in the same order
      sortedEt.sort()
      buttonIdx = ENTITY_TYPE_BUTTON_START
      for et in sortedEt:
        bitmap = et[1].buttonSvg.bmp(tsize, { "name":et[0][0:3] }, (222,222,222,wx.ALPHA_OPAQUE))  # Use the first 3 letters of the name as the button text if nothing
        shortHelp = et[1].data.get("shortHelp",None)
        longHelp = et[1].data.get("help",None)
        #self.toolBar.AddLabelTool(11, et[0], bitmap, shortHelp=shortHelp, longHelp=longHelp)
        self.toolBar.AddRadioTool(buttonIdx, bitmap, wx.NullBitmap, shortHelp=et[0], longHelp=longHelp,clientData=et)
        self.idLookup[buttonIdx] = EntityTypeTool(self,et[1])  # register this button so when its clicked we know about it
        buttonIdx+=1
        et[1].buttonIdx = buttonIdx
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

    def OnPaint(self, evt):
        #dc = wx.PaintDC(self)
        dc = wx.BufferedPaintDC(self)
        dc.SetBackground(wx.Brush('white'))
        dc.Clear()        
        self.render(dc)


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
        for e in self.entities:
          svg.blit(ctx,e.bmp,e.pos,e.scale,e.rotate)
        ctx.restore()

    def findEntitiesAt(self,pos):
      """Returns the entity located at the passed position """
      # TODO handle the viewscope's translation, rotation, scaling
      ret = []
      for e in self.entities:
        furthest= (e.pos[0] + e.size[0]*e.scale[0],e.pos[1] + e.size[1]*e.scale[1])
        print e.data["name"], ": ", pos, " inside: ", e.pos, " to ", furthest
        if pos[0] >= e.pos[0] and pos[1] >= e.pos[1] and pos[0] <= furthest[0] and pos[1] <= furthest[1]:  # mouse is in the box formed by the entity
          print "matched"
          ret.append(e)
      return ret


def Test():
  import time
  import pyGuiWrapper as gui

  model = Model()
  model.load("testModel.xml")

  gui.go(lambda parent,menu,tool,status,m=model: Panel(parent,menu,tool,status, m))


# Notes
#  test automation panel.WarpPointer(self,x,y)  "Move pointer"

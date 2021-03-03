import os
import pdb
import math
import time
from types import *
import os.path
# import networkx  # generic network description library used during object layout

import wx
import wx.lib.wxcairo
import cairo
import rsvg
import yang
import svg
import wx.lib.scrolledpanel as scrolled
#from wx.py import shell,
#from wx.py import  version
from string import Template
import types
import subprocess

import dot
from common import *
from module import Module
from entity import *
from model import Model
from umlEditor import DeleteTool

from gfxmath import *
from gfxtoolkit import *
from entity import Entity

import share
import texts
import sys
 
ENTITY_TYPE_BUTTON_START = wx.NewId()
SAVE_BUTTON = wx.NewId()
ZOOM_BUTTON = wx.NewId()
COPY_BUTTON = wx.NewId()
#CONNECT_BUTTON = wx.NewId()
SELECT_BUTTON = wx.NewId()
CODEGEN_BUTTON = wx.NewId()
CODEGEN_LANG_C = wx.NewId()
CODEGEN_LANG_CPP = wx.NewId()
CODEGEN_LANG_PYTHON = wx.NewId()
CODEGEN_LANG_JAVA = wx.NewId()

DELETE_BUTTON = wx.NewId()

def reassignCommonToolIds():
  print 'reassignCommonToolIds'
  global ENTITY_TYPE_BUTTON_START
  global SAVE_BUTTON
  global ZOOM_BUTTON
  #global CONNECT_BUTTON
  global SELECT_BUTTON
  global CODEGEN_BUTTON
  global CODEGEN_LANG_C
  global CODEGEN_LANG_CPP
  global CODEGEN_LANG_PYTHON
  global CODEGEN_LANG_JAVA
  global DELETE_BUTTON
  ENTITY_TYPE_BUTTON_START = wx.NewId()
  SAVE_BUTTON = wx.NewId()
  ZOOM_BUTTON = wx.NewId()
  #CONNECT_BUTTON = wx.NewId()
  COPY_BUTTON = wx.NewId()
  SELECT_BUTTON = wx.NewId()
  CODEGEN_BUTTON = wx.NewId()
  CODEGEN_LANG_C = wx.NewId()
  CODEGEN_LANG_CPP = wx.NewId()
  CODEGEN_LANG_PYTHON = wx.NewId()
  CODEGEN_LANG_JAVA = wx.NewId()

  DELETE_BUTTON = wx.NewId()

COL_MARGIN = 250
COL_SPACING = 2
COL_WIDTH = 250

ROW_MARGIN = 100
ROW_SPACING = 2
ROW_WIDTH   = 120

linkNormalLook = dot.Dot({ "color":(0,0,.8,.75), "lineThickness": 4, "buttonRadius": 6, "arrowLength":15, "arrowAngle": PI/8 })

NEW_NODE_INST = "New node instance"
NEW_NODE_INST_INHERIT_FROM_A_NODE = "Inherit from a node instance"

#delta_x = 59 # 
#delta_y = 17
delta_x = 0 
delta_y = 0

def touch(f):
    f = file(f,'a')
    f.close()

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
      if event.ButtonUp(wx.MOUSE_BTN_LEFT):  # Select
        #self.box.start(panel,pos)
        ret = True
      elif event.Dragging():
        #self.box.change(panel,event)
        ret = True
      elif event.ButtonDown(wx.MOUSE_BTN_LEFT):
        #rect = self.box.finish(panel,pos)
        # Real point rectangle
        #rect = convertToRealPos(rect, panel.scale)
        #size = (rect[2]-rect[0],rect[3]-rect[1])
        #if size[0] < 15 or size[1] < 15:  # its so small it was probably an accidental drag rather then a deliberate sizing
        #  size = None
        if self.entity.et.name=="Node" and self.panel.newNodeInstOption!=NEW_NODE_INST:
          ret = self.panel.duplicateNode() 
          if ret:
            self.panel.updateInstDetails()
            self.panel.UpdateVirtualSize()
            self.panel.layout()
            self.panel.loadGrayCells()
            self.panel.Refresh()
          return True
        size = None
        ret = self.panel.CreateNewInstance(self.entity,(pos[0],pos[1]),size)
            
    return ret

class ChildEntities (wx.PopupTransientWindow):
    def __init__ (self, parent, childEntities, style, size):
        wx.PopupTransientWindow.__init__(self, parent, style)

        self.enddelaytime = 1000 #ms
        # The position of the panel's viewport within the larger drawing
        self.location = (0,0)
        self.rotate = 0.0
        self.scale = 1.0

        self.Bind(wx.EVT_ENTER_WINDOW, self.OnWidgetEnter)
        self.Bind(wx.EVT_LEAVE_WINDOW, self.OnWidgetLeave)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_LEFT_DOWN, self.OnCompClick)

        self.childEntities = childEntities
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Fit(self)

        self.SetSizeHints(size[0], size[1])
        self.destroytime = wx.PyTimer(self.DestroyTimer)
        self.destroytime.Start(self.enddelaytime)

        self.Layout()

    def OnPaint(self, event):
        dc = wx.BufferedPaintDC(self)
        #dc.SetBackground(wx.Brush('white'))
        dc.Clear()
        self.render(dc)

    def render(self, dc):
      ctx = wx.lib.wxcairo.ContextFromDC(dc)
      # Now draw the graph
      ctx.save()
      # First position the screen's view port into the larger document
      ctx.translate(*self.location)
      ctx.rotate(self.rotate)
      ctx.scale(self.scale, self.scale)
      # Now draw the links
      # Now draw the entites

      bound = (0, 0) + self.GetClientSizeTuple()
      for ent,place in zip(self.childEntities,partition(len(self.childEntities), bound)):
        ent.pos = (place[0]- 5,place[1] - (bound[3] - bound[1])/5) # (cell.bound[0]+ent.relativePos[0], cell.bound[1]+ent.relativePos[1])
        tmp = ((place[2]-place[0])+10, (place[3]-place[1])+(bound[3] - bound[1])/5+5) # (min(cell.bound[2]-30,64),min(cell.bound[3]-30,64))
        if tmp != ent.size:
          ent.size = tmp
          ent.recreateBitmap()
        svg.blit(ctx,ent.bmp,ent.pos,ent.scale,ent.rotate)

      ctx.restore()

    def OnWidgetEnter(self, event):
      if hasattr(self, "destroytime"):
        if self.destroytime:
          self.destroytime.Stop()
          del self.destroytime

    def OnWidgetLeave(self, event):
      self.destroytime = wx.PyTimer(self.DestroyTimer)
      self.destroytime.Start(self.enddelaytime)
      event.Skip()

    def DestroyTimer(self):
      """ The destruction timer has expired. """

      self.destroytime.Stop()
      del self.destroytime
      try:
        self.Dismiss()
      except:
        pass

    def OnCompClick(self, event):
      pos = event.GetPositionTuple()
      entities = self.findEntitiesAt(pos)
      if entities:
        if share.instanceDetailsPanel:
          share.instanceDetailsPanel.showEntity(next(iter(entities)))
      self.Dismiss()

    def findEntitiesAt(self,pos):
      """Returns the entity located at the passed position """
      ret = set()
      for e in self.childEntities:
        furthest= (e.pos[0] + e.size[0]*e.scale[0],e.pos[1] + e.size[1]*e.scale[1])
        if inBox(pos,(e.pos[0],e.pos[1],furthest[0],furthest[1])): # mouse is in the box formed by the entity
          ret.add(e)
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
          line = convertToRealPos(self.line.finish(panel,pos), panel.scale)
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
    panel.entities.append(self.entityType.createEntity(position, size,children=True))
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
    self.rectBmp = svg.SvgFile("rect.svg").instantiate((24,24), {})
    self.entOrder = ["Component", "ComponentServiceInstance", "ServiceUnit", "ServiceInstance", "Node", "ServiceGroup"] # Select layer entity by order: Component->csi->su->si->node->sg
    self.selectMultiple = False

    self.mouseDownPos = None # to determine the left mouse being clicked

  def OnSelect(self, panel,event):
    panel.statusBar.SetStatusText(self.defaultStatusText,0);
    self.touching.clear()
    self.selected.clear()
    return True

  def OnUnselect(self,panel,event):
    pass

  def render(self,ctx):
    # Draw mini rectangle at left right top bottom corner
    if len(self.selected) > 0:
      for e in self.selected:
        pos = (e.pos[0] * share.instancePanel.scale,e.pos[1] * share.instancePanel.scale) 
        ctx.set_line_width(2)
        ctx.rectangle(
        pos[0] + self.panel.scale*self.panel.translating['horizontal'], 
        pos[1] + self.panel.scale*self.panel.translating['vertical'], 
        20*self.panel.scale, 
        20*self.panel.scale)
        ctx.set_source_rgba(0, 0, 1, 1)
        ctx.fill()

  def OnEditEvent(self,panel, event):
    panel.drawSelectionBox = True
    ret = False
    #pos = panel.CalcUnscrolledPosition(event.GetPositionTuple())
    pos = event.GetPosition()
    scale = self.panel.scale
    if isinstance(event,wx.MouseEvent):
      if event.ButtonDown(wx.MOUSE_BTN_LEFT):  # Select
        self.mouseDownPos = pos 
        self.selectMultiple = False
        self.entities = panel.findEntitiesAt(pos)
        self.dragPos = pos
        if not self.entities:
          panel.statusBar.SetStatusText(self.defaultStatusText,0);
          self.touching = set()
          self.boxSel.start(panel,pos)
          panel.selectedEntities = None
          return False
    
        # Remove the background entities; you can't move them
        ent = set()
        rcent = set()
        for e in self.entities:
          if e.et.name in panel.rowTypes or e.et.name in panel.columnTypes:
            rcent.add(e)
          else:
            ent.add(e)

        # I'm going to let you select row/column entities OR contained entities but not both
        if ent:
          self.entities = ent
        else:
          self.entities = rcent
        print "Touching %s" % ", ".join([ e.data["name"] for e in self.entities])
        if all(e.et.name in ("Component", "ComponentServiceInstance", "ServiceUnit", "ServiceInstance") for e in self.entities):
          panel.selectedEntities = self.entities
        else:
          panel.selectedEntities = None

        panel.statusBar.SetStatusText("Touching %s" % ", ".join([ e.data["name"] for e in self.entities]),0);

        # If you touch something else, your touching set changes.  But if you touch something in your current touch group then nothing changes
        # This enables behavior like selecting a group of entities and then dragging them (without using the ctrl key)
        if not self.entities.issubset(self.touching) or (len(self.touching) != len(self.entities)):
          self.touching = set(self.entities)
          self.selected = self.touching.copy()
        # If the control key is down, then add to the currently selected group, otherwise replace it.
        if event.ControlDown():
          self.selected = self.selected.union(self.touching)
        else: self.selected = self.touching.copy()

      if event.Dragging():
        # if you are touching anything, then drag everything
        if self.touching and self.dragPos:
          delta = ((pos[0]-self.dragPos[0])/scale, (pos[1] - self.dragPos[1])/scale)
          # TODO deal with scaling and rotation in delta
          if delta[0] != 0 or delta[1] != 0:
            for e in self.selected:
              e.pos = (int(e.pos[0] + delta[0]), int(e.pos[1] + delta[1]))  # move all the touching objects by the amount the mouse moved
          self.dragPos = pos
          panel.Refresh()
        else:  # touching nothing, this is a selection rectangle
          self.boxSel.change(panel,event)

      if event.ButtonUp(wx.MOUSE_BTN_LEFT):
        # TODO: Move the data in this entity to the configuration editing sidebar, and expand it if its minimized.
        if self.touching and self.dragPos:
          # Ignore moving component and csi
          for e in filter(lambda e: not e.et.name in (self.panel.ignoreEntities), self.selected):
          #   if e.et.name in panel.rowTypes:
          #     print 'DBG: repositionRow for [%s]'%e.data['name']
          #     panel.repositionRow(e,(pos[0]/scale,pos[1]/scale))
          #   if e.et.name in panel.columnTypes:
          #     print 'DBG: repositionRow for [%s]'%e.data['name']
          #     panel.repositionColumn(e,(pos[0]/scale,pos[1]/scale))
          #   else:
               if e.et.name == 'Component' or e.et.name == 'NonSafComponent':
                 pass # ignore reposition Component or NonSafComponent
               else:
                 print 'DBG: reposition for [%s]'%e.data['name']
                 panel.grid.reposition(e, panel,pos=(pos[0]/scale,pos[1]/scale))
          self.touching = set()
          panel.layout()
          panel.Refresh()

        # Or find everything inside a selection box
        if self.boxSel.active:
          #panel.drawers.discard(self)
          rect = self.boxSel.rect
          delta = (rect[0]-rect[2], rect[1] - rect[3])
          if delta[0] != 0 or delta[1] != 0:
            self.selectMultiple = True

          self.touching = panel.findEntitiesTouching(self.boxSel.finish(panel,pos))

          if event.ControlDown():
            self.selected = self.selected.union(self.touching)
          else: 
            self.selected = self.touching
          panel.Refresh()

        if self.mouseDownPos and self.mouseDownPos == pos: # left mouse button was clicked
          if len(self.entities) == 2:
            flag = True
            for e in self.entities:
              if not e.et.name in panel.rowTypes and not e.et.name in panel.columnTypes:
                flag = False
                break
            if flag:
              panel.createGrayCell(
              (pos[0]-share.instancePanel.translating['horizontal'], 
              pos[1]-share.instancePanel.translating['vertical']),
              self.entities)
              self.panel.Refresh()
        self.mouseDownPos = None 

      elif event.ButtonDClick(wx.MOUSE_BTN_LEFT):
        # Check if double click at "duplicate" button
        e = panel.findAddButtonAt(pos)
        if e:
          self.cloneInstances([e])
          self.panel.Refresh()
          return False

        entities = panel.findEntitiesAt(pos)
        if not entities: return False
        self.showEntity(entities)
      elif event.ButtonDown(wx.MOUSE_BTN_RIGHT):
        entities = panel.findEntitiesAt(pos)
        if not entities: return False

        self.touching = set()
        self.selected = set()
        # Show popup child comps list
        #for e in entities:
        #  if e.et.name in ("ServiceInstance", "ServiceUnit",):
        #    childEnties = []
        #    for a in e.containmentArrows:
        #      childEnties.append(a.contained)
        #    win = ChildEntities(panel, childEnties, wx.SIMPLE_BORDER, size=(COL_WIDTH, ROW_WIDTH))
        #    win.SetPosition(wx.GetMousePosition())
        #    win.Popup()
    if isinstance(event,wx.KeyEvent):
      if event.GetEventType() == wx.EVT_KEY_DOWN.typeId:
        try:
          character = chr(event.GetKeyCode())
        except ValueError:
          character  = None
        print "key code: ", character
        if event.GetKeyCode() ==  wx.WXK_DELETE or event.GetKeyCode() ==  wx.WXK_NUMPAD_DELETE:
          if self.touching:
            self.deleteEntities(self.touching)
            #self.touching.clear()
          elif self.selected:
            self.deleteEntities(self.selected)
            #self.selected.clear()
          self.touching.clear()
          self.selected.clear()
          self.panel.Refresh()
          ret=True # I consumed this event
        elif event.ControlDown() and character ==  'V':
          self.cloneInstances(self.selected)
          self.panel.Refresh()
          ret=True
      else:
        return False # Give this key to someone else

    return ret
  
  def showEntity(self, ents):
    ent = None
    # Show detail entity
    if len(ents) == 1:
      ent = (next(iter(ents)))
    else:
      ents = sorted(ents, key=lambda ent: self.entOrder.index(ent.et.name))
      # Show first entity
      ent = ents[0]
    parentFrame = share.instancePanel.guiPlaces.frame
    if share.instanceDetailsPanel:
      share.instanceDetailsPanel.showEntity(ent)
      index = parentFrame.tab.GetPageIndex(share.instanceDetailsPanel)
      parentFrame.tab.SetSelection(index)
      # parentFrame.tab.SetSelection(3)
    else:
      parentFrame.insertPage(3)
      share.instanceDetailsPanel.showEntity(ent)

  def deleteEntities(self, ents):
    if self.selectMultiple:
      self.panel.deleteEntities(ents)
    else:
      ent = None
      # Delete an entity
      if len(ents) == 1:
        ent = (next(iter(ents)))
      else:
        ents = sorted(ents, key=lambda ent: self.entOrder.index(ent.et.name))
        # Delete first entity
        ent = ents[0]
      self.panel.deleteEntities([ent])

  def filterOut(self, ents, filterRules = []):
    return filter(lambda ent: not ent.et.name in filterRules, ents)

  def cloneInstances(self, ents):
    # Duplicate SG, NODE, SU, SI, COMP and CSI
    ents = sorted(ents, key=lambda ent: self.entOrder.index(ent.et.name))

    newEnts = []
    if self.selectMultiple:
      # TODO: take all entities?
      print "Copy selected instances: %s" % ", ".join([ e.data["name"] for e in ents])
      (newEnts,addtl) = self.panel.model.duplicate(ents,recursive=True, flag=False)
    else:
      # Duplicate first order entity
      (newEnts,addtl) = self.panel.model.duplicate([ents[0]], recursive=True, flag=False)

    # Create ca for new intance component/csi
    for i in filter(lambda ent: isinstance(ent, Entity),  ents[0].childOf):
      for newEnt in newEnts:
        i.createContainmentArrowTo(newEnt)

    # Put all childs instances into hyperlisttree
    #if share.instanceDetailsPanel:
    #  share.instanceDetailsPanel._createTreeEntities()
    if share.instanceDetailsPanel:
      share.instanceDetailsPanel.addTreeItems(newEnts) 
      

    self.panel.layout()


class CopyTool(Tool):
  def __init__(self, panel):
    self.panel = panel
    self.defaultStatusText = "Click anywhere in the editor to copy the selected instance"

  def OnSelect(self, panel,event):
    panel.statusBar.SetStatusText(self.defaultStatusText,0);
    return True

  def OnUnselect(self,panel,event):
    pass

  def OnEditEvent(self,panel, event):
    if isinstance(event,wx.MouseEvent):
      if event.ButtonDown(wx.MOUSE_BTN_LEFT):
        ents = panel.selectedEntities    
        selectTool = panel.idLookup[SELECT_BUTTON]   
        if ents and selectTool:
          if len(ents) == 1:
            ent = (next(iter(ents)))
          else:
            ents = sorted(ents, key=lambda ent: selectTool.entOrder.index(ent.et.name))        
            ent = ents[0]
          print 'copy instance [%s]' % ent.data['name']
          selectTool.cloneInstances([ent])
          panel.Refresh()
          #panel.selectedEntities = None         
        else:
          panel.statusBar.SetStatusText("Please select an instance to copy first",0);

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
    #pos = panel.CalcUnscrolledPosition(event.GetPositionTuple())
    pos = event.GetPosition()
    scale = self.scale
    if isinstance(event, wx.MouseEvent):
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
          scale += self.scaleRange

      elif event.ButtonUp(wx.MOUSE_BTN_RIGHT):
        rect = self.boxSel.finish(panel,pos)
        delta = (rect[2]-rect[0],rect[3]-rect[1])
        # Center rect selected
        pos[0] = rect[0] + delta[0]/2
        pos[1] = rect[1] + delta[1]/2

        if (self.scale - self.scaleRange) > self.minScale:
          scale -= self.scaleRange

      elif event.ControlDown(): 
        if event.GetWheelRotation() > 0:
          if (self.scale + self.scaleRange) < self.maxScale:
            scale += self.scaleRange
        elif event.GetWheelRotation() < 0:
          if (self.scale - self.scaleRange) > self.minScale:
            scale -= self.scaleRange

    if isinstance(event, wx.KeyEvent):
      if event.ControlDown(): 
        if event.GetKeyCode() == wx.WXK_NUMPAD_ADD:
          if (self.scale + self.scaleRange) < self.maxScale:
            scale += self.scaleRange
        elif event.GetKeyCode() == wx.WXK_NUMPAD_SUBTRACT:
          if (self.scale - self.scaleRange) > self.minScale:
            scale -= self.scaleRange
        elif event.GetKeyCode() == wx.WXK_NUMPAD0:
          scale = 1

    self.SetScale(scale, pos)

  def SetScale(self, scale, pos):
    if scale != self.scale:
      self.scale = scale

      # Update scale for panel
      self.panel.scale = self.scale
      self.panel.UpdateVirtualSize()
      self.panel.layout()
      self.panel.Refresh()

      # TODO: scroll wrong??? 
      scrollx, scrolly = self.panel.GetScrollPixelsPerUnit();
      size = self.panel.GetClientSize()
      self.panel.Scroll((pos[0] - size.x/2)/scrollx, (pos[1] - size.y/2)/scrolly)

class SaveTool(Tool):
  def __init__(self, panel):
    self.panel = panel
  
  def OnSelect(self, panel,event):
    dlg = wx.FileDialog(panel, "Save model as...", os.getcwd(), style=wx.SAVE | wx.OVERWRITE_PROMPT, wildcard="*.xml")
    if dlg.ShowModal() == wx.ID_OK:
      filename = dlg.GetPath()
      self.panel.model.save(filename)
      # TODO: Notify (IPC) to GUI instances (C++ wxWidgets) to update
      touch(filename) # If any modified flag trigger on this model.xml, just recreate GUIs
    return False

class GenerateTool(Tool):
  def __init__(self, panel):
    self.panel = panel
    # self.dictGenFunc = {CODEGEN_LANG_C:self.generateC, CODEGEN_LANG_CPP:self.generateCpp, CODEGEN_LANG_PYTHON:self.generatePython, CODEGEN_LANG_JAVA:self.generateJava}


  def OnSelect(self, panel, event):
    if event.GetId() == CODEGEN_BUTTON:
      parentFrame = self.panel.guiPlaces.frame
      parentFrame.project.getPrjProperties()    
      src_bak = False
      if parentFrame.project.prjProperties['backupMode'] == "prompt" and os.listdir(self.panel.model.directory()+'/src'):
        result = self.openBackupDialog()
        if not result: 
          return
        elif result == 2:
          src_bak = True
      elif parentFrame.project.prjProperties['backupMode'] == "always" and os.listdir(self.panel.model.directory()+'/src'):
        self.executeBackupSource()
        src_bak = True
      # code gen must be per-component -- not generation of one type of application
      print 'gen source...'
      files,proxyFiles = panel.model.generateSource(self.panel.model.directory())
      # Copy setup to model
      #os.system('cp resources/setup %s' %self.panel.model.directory())
      print 'files gen: files %s\nproxies %s\n' %(str(files),str(proxyFiles))
      self.panel.statusBar.SetStatusText("Code generation complete")
      # add these files to the "source" part of the project tab and update the project xml file
      #print files
      # parentFrame = self.panel.guiPlaces.frame
      parentFrame.project.updateTreeItem(parentFrame.currentActivePrj, texts.src, files)
      parentFrame.project.updateTreeItem(parentFrame.currentActivePrj, texts.src, proxyFiles)
      if src_bak:
        parentFrame.project.updateTreeItem(parentFrame.currentActivePrj, texts.src_bak)
    return False

  def openBackupDialog(self):
    '''
    @summary    : Show dialog for backup option
    '''
    self.dlg = wx.Dialog(None, title="Backup Configuration ?", size=(620,320))
    vBox = wx.BoxSizer(wx.VERTICAL)
    hBox = wx.BoxSizer(wx.HORIZONTAL)
    parentFrame = self.panel.guiPlaces.frame
    srcDir = "%s/src" % parentFrame.project.getPrjPath()
    backupDir = "%s/src.bak" % parentFrame.project.getPrjPath()
    lablel1 = wx.StaticText(self.dlg, label="Source and configuration files exist in directory:")
    lablel2 = wx.StaticText(self.dlg, label=srcDir)
    lablel3 = wx.StaticText(self.dlg, label="Before generating code would you like to backup these files to the directory:")
    lablel4 = wx.StaticText(self.dlg, label=backupDir)
    self.checkBox = wx.CheckBox(self.dlg, id=wx.ID_ANY, label="Never show this dialog again")
    self.checkBox.Bind(wx.EVT_CHECKBOX, self.onChecked) 
    lablel5 = wx.StaticText(self.dlg, label="(Dialog can be reenable through Project Properties)")

    noBtn = wx.Button(self.dlg, label="No")
    noBtn.Bind(wx.EVT_BUTTON, self.onClickBackupNoBtn)
    cancelBtn = wx.Button(self.dlg, label="Cancel")
    cancelBtn.Bind(wx.EVT_BUTTON, self.onClickBackupCancelBtn)
    yesBtn = wx.Button(self.dlg, label="Yes")
    yesBtn.Bind(wx.EVT_BUTTON, self.onClickBackupYesBtn)

    vBox.Add(lablel1, 0, wx.ALL, 5)
    vBox.Add(lablel2, 0, wx.ALL|wx.LEFT, 15)
    vBox.Add(lablel3, 0, wx.ALL, 5)
    vBox.Add(lablel4, 0, wx.ALL|wx.LEFT, 15)
    vBox.Add(self.checkBox, 0, wx.ALL, 5)
    vBox.Add(lablel5, 0, wx.ALL, 5)

    hBox.Add(noBtn, 0, wx.CENTER, 0)
    hBox.Add(cancelBtn, 0, wx.CENTER, 0)
    hBox.Add(yesBtn, 0, wx.CENTER|wx.RIGHT, 15)
    vBox.Add(hBox, 0, wx.TOP|wx.ALIGN_RIGHT, 30)

    self.dlg.SetSizer(vBox)
    result = self.dlg.ShowModal()
    return result


  def mergeDialog(self):
    '''
    @summary    : Show dialog for merging option
    '''
    # Create popup box
    self.dlg = wx.Dialog(None, title="Merge source code ?", size=(620,320))
    vBox = wx.BoxSizer(wx.VERTICAL)
    hBox = wx.BoxSizer(wx.HORIZONTAL)
    # Not clear 
    parentFrameMerge = self.panel.guiPlaces.frame
    # Project Path + /src
    srcDir = "%s/src" % parentFrameMerge.project.getPrjPath()
    lablel1 = wx.StaticText(self.dlg, label="Source and configuration files exist in directory:")
    lablel2 = wx.StaticText(self.dlg, label=srcDir)
    lablel3 = wx.StaticText(self.dlg, label="Would you like to merge or overwrite the source code:")
    self.checkBox = wx.CheckBox(self.dlg, id=wx.ID_ANY, label="Never show this dialog again")
    self.checkBox.Bind(wx.EVT_CHECKBOX, self.onChecked) 
    lablel5 = wx.StaticText(self.dlg, label="(Dialog can be reenable through Project Properties)")

    noBtn = wx.Button(self.dlg, label="Merge")
    noBtn.Bind(wx.EVT_BUTTON, self.onClickMergeNoBtn)
    cancelBtn = wx.Button(self.dlg, label="Cancel")
    cancelBtn.Bind(wx.EVT_BUTTON, self.onClickMergeCancelBtn)
    yesBtn = wx.Button(self.dlg, label="Overwrite")
    yesBtn.Bind(wx.EVT_BUTTON, self.onClickMergeYesBtn)

    vBox.Add(lablel1, 0, wx.ALL, 5)
    vBox.Add(lablel2, 0, wx.ALL|wx.LEFT, 15)
    vBox.Add(lablel3, 0, wx.ALL, 5)
    vBox.Add(self.checkBox, 0, wx.ALL, 5)
    vBox.Add(lablel5, 0, wx.ALL, 5)

    hBox.Add(noBtn, 0, wx.CENTER, 0)
    hBox.Add(cancelBtn, 0, wx.CENTER, 0)
    hBox.Add(yesBtn, 0, wx.CENTER|wx.RIGHT, 15)
    vBox.Add(hBox, 0, wx.TOP|wx.ALIGN_RIGHT, 30)

    self.dlg.SetSizer(vBox)

    result = self.dlg.ShowModal()
    return result


  def onClickBackupNoBtn(self, event):
    '''
    @summary    : don't backup code resource
    '''
    if self.checkBox.GetValue():
      parentFrame = self.panel.guiPlaces.frame
      parentFrame.project.prjProperties['backupMode'] = "never"
      parentFrame.project.savePrjProperties()
    self.dlg.EndModal(1)
  
  def onClickBackupCancelBtn(self, event):
    '''
    @summary    : cancel backup dialog
    '''
    self.dlg.EndModal(0)

  def onClickBackupYesBtn(self, event):
    '''
    @summary    : backup code resource
    '''
    if self.checkBox.GetValue():
      parentFrame = self.panel.guiPlaces.frame
      parentFrame.project.prjProperties['backupMode'] = "always"
      parentFrame.project.savePrjProperties()
    self.executeBackupSource()
    self.dlg.EndModal(2)




  def onClickMergeNoBtn(self, event):
    '''
    @Summary    : Do not overwrite the existed source code
    '''
    if self.checkBox.GetValue():
      parentFrame = self.panel.guiPlaces.frame
      parentFrame.project.prjProperties['mergeMode'] = "never"
      parentFrame.project.savePrjProperties()
    self.dlg.EndModal(1)
  
  def onClickMergeCancelBtn(self, event):
    '''
    @Summary    : Cancle merging
    '''
    self.dlg.EndModal(0)

  def onClickMergeYesBtn(self, event):
    '''
    @Summary    : Always overwrite the existed source code
    '''
    if self.checkBox.GetValue():
      parentFrame = self.panel.guiPlaces.frame
      parentFrame.project.prjProperties['mergeMode'] = "always"
      parentFrame.project.savePrjProperties()
    self.dlg.EndModal(2)




  def onChecked(self, event):
    pass

  def executeBackupSource(self):
    '''
    @summary    : execute backup code resource and keep backup number  
    '''
    parentFrame = self.panel.guiPlaces.frame
    backupDir = "%s/src.bak" % parentFrame.project.getPrjPath()
    if not os.path.isdir(backupDir):
      os.system("mkdir -p %s" % backupDir)

    tarGet = str(subprocess.check_output(['date','+%d_%m_%Y-%H_%M_%S'])).strip()
    os.system("cd %s; tar -zcvf %s.tar.gz ../src" % (backupDir,  tarGet))

    files = os.popen("cd %s;ls *.tar.gz -lt | awk '{print $9}'" % backupDir).read()
    n = 1
    for f in files.split():
      if n > int(parentFrame.project.prjProperties['backupNumber']):
        os.system("rm -f %s/%s" % (backupDir, f))
      n += 1

# Global of this panel for debug purposes only.  DO NOT USE IN CODE
dbgPanel = None

class GridObject:
  def __init__(self):
    self.bound = None
    self.entities = set()
    self.row = None
    self.col = None
    self.grayBmp = None
    self.grayPos = None
    self.graySize = None

  def addEnt(self, ob):
    self.entities.add(ob)


class GridEntityLayout:
  def __init__(self,rows, columns):
    self.nogrid = GridObject()
    self.grid = []  # Create a 2 dimensional array of sets of objects
    self.rows = rows
    self.columns = columns
    numCols = len(columns)

    #row = [GridObject()]  # zeroth column is row header
    # zeroth row is node header
    #for j in columns:
    #  row.append(GridObject())
    # self.grid.append(row) 

    # Make the 2 dimensional grid
    for i in rows:
      row = []
      for j in columns:
        row.append(GridObject())
      self.grid.append(row)

    self.rowIdx = {}
    count = 0
    for r in rows:
      self.rowIdx[r] = count
      count+=1

    count = 0
    self.colIdx = {}
    for c in columns:
      self.colIdx[c] = count
      count+=1


    # Set up the information in each cell
    y = 0
    x = 0
    for row in self.grid:
      x=0
      rowent = self.rows[y]
      for cell in row:
        col = self.columns[x]
        cell.bound = boxIntersect(rowent.pos,rowent.size,col.pos,col.size)
        cell.row = rowent
        cell.col = col
        # print "(%d,%d) %s" % (x,y,str(cell.bound))
        x += 1
      y += 1

  def reposition(self,instance, panel, sg=None, pos = None):
    """Move an instance somewhere else -- its physical movement may change its parent relationships"""
    if not pos: pos = instance.pos
    for row in self.grid:
      for cell in row:
        # print cell.bound

        # Not allowed to put at cell (0,0)
        if isinstance(cell.row, Margin) and isinstance(cell.col, Margin):
          continue
        # Not allowed put the cell in the Node but outside any SG
        if isinstance(cell.row, Margin):
          continue

        # It IS allowed to put the cell in a SG but not in any node!

        # not allow to put at "disabled" (gray) cell
        if cell.bound in panel.grayCells:
          continue

        if sg and cell.row != sg:
          continue

        b1 = False
        # Does not allow si to be moved to node
        if instance.et.name=="ServiceInstance" and not isinstance(cell.col, Margin) and cell.col.et.name=="Node":
          print 'cannot move SIs/CSIs to Nodes'
          b1 = True
        b2 = False
        rowContained = []
        if not isinstance(cell.row, Margin):
          for ca in cell.row.entity.containmentArrows:
            rowContained.append(ca.contained)        
        #for c in rowContained: print 'row contained:%s'%c.data['name']
        if instance.et.name == 'ServiceInstance' and not isinstance(cell.row, Margin) and instance.entity not in rowContained:
          print 'cannot move: [%s] is not a child of [%s]'%(instance.entity.data['name'], cell.row.entity.data['name'])
          b2 = True
        if b1 or b2: continue
        

          #continue
        print 'next iterator'        
        b1 = False 
        if instance.et.name == 'ServiceUnit' and isinstance(cell.col, Margin) and not isinstance(cell.row, Margin) and cell.row.entity.et.name=='ServiceGroup':
          print 'cannot move [%s] to [%s]' % (instance.data['name'],cell.row.data['name'])
          b1 = True        

        #if instance.et.name == 'ServiceUnit' and not isinstance(cell.col, Margin) and cell.col not in instance.childOf:
        #  print 'cannot move: [%s] is not a child of [%s]'%(instance.data['name'], cell.col.data['name'])
        #  continue
        #print 'instance name [%s], [%s]' %(instance.data['name'],str(instance.entity))

        b2 = False
        colContained = []
        if not isinstance(cell.col, Margin):
          for ca in cell.col.entity.containmentArrows:
            colContained.append(ca.contained)        
        #for c in colContained: print 'col contained:%s'%c.data['name']
        if instance.et.name == 'ServiceUnit' and not isinstance(cell.col, Margin) and instance.entity not in colContained:
          print 'cannot move: [%s] is not a child of [%s]'%(instance.entity.data['name'], cell.col.entity.data['name'])
          b2 = True

        b3 = False
        rowContained = []
        if not isinstance(cell.row, Margin):
          for ca in cell.row.entity.containmentArrows:
            rowContained.append(ca.contained)        
        #for c in rowContained: print 'row contained:%s'%c.data['name']
        if instance.et.name == 'ServiceUnit' and not isinstance(cell.row, Margin) and instance.entity not in rowContained:
          print 'cannot move: [%s] is not a child of [%s]'%(instance.entity.data['name'], cell.row.entity.data['name'])
          b3 = True

        if b1 or b2 or b3: continue        
        if inBox(pos,
        (cell.bound[0]+share.instancePanel.translating['horizontal'],
        cell.bound[1]+share.instancePanel.translating['vertical'], 
        cell.bound[2]+share.instancePanel.translating['horizontal'], 
        cell.bound[3]+share.instancePanel.translating['vertical']) ):
          # Remove the containment arrows (if they exist)
          for i in instance.childOf:  
            i.deleteContainmentArrowTo(instance)
          # Rewrite the back pointers
          instance.childOf = set([cell.row,cell.col])
          # Create the new containment arrows
          for i in instance.childOf:
            if not isinstance(i, Margin) and not isinstance(instance, Margin):
              #to prevent to create containment arrow to the instance itself
              if i!=instance:
                i.createContainmentArrowTo(instance)
          return True
    return False

  def getCell(self, pos):
    for row in self.grid:
      for cell in row:        
        if inBox(pos,cell.bound) and len(cell.entities)==0:
          return cell
    return None

  def getAnyCell(self, pos):
    for row in self.grid:
      for cell in row:
        #if isinstance(cell.row, Margin) and isinstance(cell.col, Margin):
        #  continue
        if inBox(pos,cell.bound):
          return cell
    return None

  def createGrayCell(self, pos, panel):
    scale = panel.scale
    cell = self.getCell((pos[0]/scale,pos[1]/scale))
    if not cell:
      print 'createGrayCell: cell is null at pos: %s' % str(pos)
      return False
    if cell.bound in panel.grayCells:
      print 'createGrayCell: clear gray cell'
      del panel.grayCells[cell.bound]
      return False # return False: clear the gray from the cell
    tmp = svg.SvgFile("gray.svg")    
    print 'createGrayCell: pos (%d,%d); bound(%d,%d,%d,%d)' % (pos[0]/scale,pos[1]/scale,cell.bound[0],cell.bound[1],cell.bound[2],cell.bound[3])    
    cell.graySize = (cell.bound[2]-cell.bound[0],cell.bound[3]-cell.bound[1])
    cell.grayPos = (cell.bound[0]/scale,cell.bound[1]/scale)
    cell.grayBmp = tmp.instantiate((cell.graySize[0]+delta_x,cell.graySize[1]+delta_y))
    panel.grayCells[cell.bound] = cell.grayBmp

    return True # return True: set the gray to the cell

  def idx(self,row,col=-1):
    """Reference a location in the grid either by integer or by entity"""
    if col==-1:
      if type(row) is types.IntType:
        return self.grid[row]
      return self.grid[self.rowIdx[row]]
    if not type(row) is types.IntType:
      row = self.rowIdx[row]
    if not type(col) is types.IntType:
      col = self.colIdx[col]
    return self.grid[row][col]

  def __str__(self):
    for row in self.grid:
      print "\n"
      for cell in row:
        print cell.entities, "  ", 

  def layout(self):
    for row in self.grid:
      for cell in row:
        nitems = len(cell.entities)
        if nitems:
          for ent,place in zip(cell.entities,partition(nitems,cell.bound)):
            ent.pos = (place[0],place[1]) # (cell.bound[0]+ent.relativePos[0], cell.bound[1]+ent.relativePos[1])
            tmp = (place[2]-place[0], place[3]-place[1]) # (min(cell.bound[2]-30,64),min(cell.bound[3]-30,64))
            if tmp != ent.size:
              ent.size = tmp
              ent.recreateBitmap()

    for ent in self.nogrid.entities:
      ent.size=(0,0)

class Margin:
  def __init__(self,pos,size):
    self.pos = pos
    self.size = size
  def createContainmentArrowTo(self,obj):  # No containment arrows in the margin
    pass
  def deleteContainmentArrowTo(self,obj):
    pass


class Panel(scrolled.ScrolledPanel):
    def __init__(self, parent,guiPlaces,model,**cfg):
      global dbgPanel
      dbgPanel = self
      scrolled.ScrolledPanel.__init__(self, parent, style = wx.TAB_TRAVERSAL|wx.SUNKEN_BORDER)
      share.instancePanel = self
      self.addBmp = svg.SvgFile("add.svg").instantiate((24,24), {})

      # self.displayGraph = networkx.Graph()
      # These variables define what types can be instantiated in the outer row/columns of the edit tool
      self.rowTypes = ["ServiceGroup","Application"]
      self.columnTypes = ["Node"]

      #for (name,m) in model.entities.items():
      #  if m.et.name == "ServiceGroup":
      #    m.customInstantiator = lambda entity,pos,size,children,name,pnl=self: pnl.sgInstantiator(entity, pos,size,children,name)

      self.SetupScrolling(True, True)
      self.SetScrollRate(1, 1)
      self.Bind(wx.EVT_SIZE, self.OnReSize)

      self.Bind(wx.EVT_PAINT, self.OnPaint)
      self.Bind(wx.EVT_SCROLLWIN, self.OnScroll)

      self.guiPlaces = guiPlaces
      self.menuBar = self.guiPlaces.menubar
      self.toolBar = self.guiPlaces.toolbar
      self.statusBar = self.guiPlaces.statusbar
      self.model=model
      self.tool = None  # The current tool
      self.drawers = set()
      self.renderArrow = {}  # This dictionary indicates whether an arrow needs to be rendered or whether the parent child relationship is expressed in a different fashion (containment)

      # The position of the panel's viewport within the larger drawing
      self.location = (0,0)
      self.rotate = 0.0
      self.scale = 1.0

      self.data_before = None

      # Buttons and other IDs that are registered may need to be looked up to turn the ID back into a python object
      self.idLookup={}  

      # Ordering of instances in the GUI display, from the upper left
      self.columns = []
      self.rows = []
      self.grid = None

      self.intersects = []

      self.addButtons = {}

      self.grayCells = {}

      self.selectedEntities = None

      self.userSelectionNode = None
      self.menuNodeInstCreate = wx.Menu()
      self.newNodeInstOption = None
      self.menuUserDefineNodeTypes = wx.Menu()
      self.translating = {'vertical':0, 'horizontal':0}
      # Ignore render entities since size(0,0) is invalid
      self.ignoreEntities = ["Component", "ComponentServiceInstance"]

      # Add toolbar buttons

      # first get the toolbar    
      if not self.toolBar:
        self.toolBar = wx.ToolBar(self,-1)
        self.toolBar.SetToolBitmapSize((24,24))

      self.drawSelectionBox = False

      # example of adding a standard button
      #new_bmp =  wx.ArtProvider.GetBitmap(wx.ART_NEW, wx.ART_TOOLBAR, tsize)
      #self.toolBar.AddLabelTool(10, "New", new_bmp, shortHelp="New", longHelp="Long help for 'New'")

      # TODO: add disabled versions to these buttons
      #self.addTools(True)
      #self.addCommonTools()

      # Add the custom entity creation tools as specified by the model's YANG
      #self.addEntityTools()

      # Set up to handle tool clicks
      #self.toolBar.Bind(wx.EVT_TOOL, self.OnToolClick, id=ENTITY_TYPE_BUTTON_START. id2=self.idLookup[len(self.idLookup)-1])  # id=start, id2=end to bind a range
      #self.toolBar.Bind(wx.EVT_TOOL_RCLICKED, self.OnToolRClick)

      # Set up events that tools may be interested in
      toolEvents = [ wx.EVT_LEAVE_WINDOW, wx.EVT_LEFT_DCLICK, wx.EVT_LEFT_DOWN , wx.EVT_LEFT_UP, wx.EVT_MOUSEWHEEL , wx.EVT_MOVE,wx.EVT_MOTION , wx.EVT_RIGHT_DCLICK, wx.EVT_RIGHT_DOWN , wx.EVT_RIGHT_UP, wx.EVT_KEY_DOWN, wx.EVT_KEY_UP]
      for t in toolEvents:
        self.Bind(t, self.OnToolEvent)

      # Building flatten instance from model.xml
      for entInstance in self.model.instances.values():
        """Create a new instance of this entity type at this position"""
        placement = None
        if entInstance.et.name in self.columnTypes:
          placement = "column"
        if entInstance.et.name in self.rowTypes:
          placement = "row"

        if placement:
          if placement == "row":
            self.rows.append(entInstance)  # TODO: calculate an insertion position based on the mouse position and the positions of the other entities
          if placement == "column":
            self.columns.append(entInstance)  # TODO: calculate an insertion position based on the mouse position and the positions of the other entities
      self.addTools(True)
      self.UpdateVirtualSize()
      self.layout()
      self.loadGrayCells()

    def OnScroll(self, event):
      if event.GetOrientation() == wx.HORIZONTAL:
        self.translating['horizontal'] = -10*event.GetPosition()
      elif event.GetOrientation() == wx.VERTICAL:
        self.translating['vertical'] = -10*event.GetPosition()
      self.OnPaint(event)
      self.Refresh()
    

    def addCommonTools(self):
      tsize = self.toolBar.GetToolBitmapSize()
      if 0: # Save is handled at the project level
        bitmap = svg.SvgFile("save_as.svg").bmp(tsize, { }, BAR_GREY)
        self.toolBar.AddTool(SAVE_BUTTON, bitmap, wx.NullBitmap, shortHelpString="save", longHelpString="Save model as...")
        self.idLookup[SAVE_BUTTON] = SaveTool(self)

      s = svg.SvgFile("generate.svg")
      bitmap = s.bmp(tsize, { }, BAR_GREY)
      bitmapDisabled = s.disabledButton(tsize)
      self.toolBar.AddTool(CODEGEN_BUTTON, bitmap, bitmapDisabled, shortHelpString="Generate Source Code", longHelpString="Generate source code ...")
      # add the generate menu item
      menuFile = self.guiPlaces.menu.get("File",None)
      gen = menuFile.Insert(9, CODEGEN_BUTTON, "&Generate code\tAlt-G", "Generate source code ...")
      gen.Enable(False)
     
      idMnuGenerateCode = [CODEGEN_BUTTON, CODEGEN_LANG_C, CODEGEN_LANG_CPP, CODEGEN_LANG_PYTHON, CODEGEN_LANG_JAVA]
      for idx in idMnuGenerateCode:
        self.idLookup[idx] = GenerateTool(self)

      # Add the umlEditor's standard tools
      # self.toolBar.AddSeparator()
      
     
      #bitmap = svg.SvgFile("connect.svg").bmp(tsize, { }, (222,222,222,wx.ALPHA_OPAQUE))
      #self.toolBar.AddRadioTool(CONNECT_BUTTON, bitmap, wx.NullBitmap, shortHelp="connect", longHelp="Draw relationships between entities")
      #self.idLookup[CONNECT_BUTTON] = LinkTool(self)

      s = svg.SvgFile("pointer.svg")
      bitmap = s.bmp(tsize, { }, BAR_GREY)
      bitmapDisabled = s.disabledButton(tsize)
      self.toolBar.AddRadioTool(SELECT_BUTTON, bitmap, bitmapDisabled, shortHelp="select", longHelp="Select one or many entities.  Click entity to edit details.  Double click to expand/contract.")
      self.idLookup[SELECT_BUTTON] = SelectTool(self)

      s = svg.SvgFile("zoom.svg")
      bitmap = s.bmp(tsize, { }, BAR_GREY)
      bitmapDisabled = s.disabledButton(tsize)
      self.toolBar.AddRadioTool(ZOOM_BUTTON, bitmap, bitmapDisabled, shortHelp="zoom", longHelp="Left click (+) to zoom in. Right click (-) to zoom out.")
      self.idLookup[ZOOM_BUTTON] = ZoomTool(self)

      s = svg.SvgFile("copy.svg")
      bitmap = s.bmp(tsize, { }, BAR_GREY)
      bitmapDisabled = s.disabledButton(tsize)
      self.toolBar.AddRadioTool(COPY_BUTTON, bitmap, bitmapDisabled, shortHelp="copy", longHelp="Select an instance, then click this button to copy it")
      self.idLookup[COPY_BUTTON] = CopyTool(self)

      s = svg.SvgFile("remove.svg")
      bitmap = s.bmp(tsize, { }, BAR_GREY)
      bitmapDisabled = s.disabledButton(tsize)
      self.toolBar.AddRadioTool(DELETE_BUTTON, bitmap, bitmapDisabled, shortHelp="Delete entity/entities", longHelp="Select one or many entities. Click entity to delete.")
      self.idLookup[DELETE_BUTTON] = DeleteTool(self)

      # setting the default tool
      self.tool = self.idLookup[SELECT_BUTTON]

      #add event handler for menu generate code
      fileMenu = self.guiPlaces.menu.get("File",None)
      fileMenu.Bind(wx.EVT_MENU, self.OnToolMenu, id=CODEGEN_BUTTON)
    
    def deleteTools(self):   
      self.toolBar.DeletePendingEvents()
      self.toolBar.ClearTools()
      self.deleteMenuItems()      
      self.idLookup.clear()

    def deleteMyTools(self):
      for toolId in self.idLookup:
        self.toolBar.DeleteTool(toolId)
      self.deleteMenuItems()
      self.idLookup.clear()

    def addTools(self, init=False):
      print 'instanceEditor: addTools'      
      if not init:
        reassignCommonToolIds()
        self.idLookup.clear()
      self.addCommonTools()
      self.addEntityTools()
      self.deleteNodeInstCreateMenu()
      self.addNodeInstCreateMenuItems()
      #self.toolBar.Bind(wx.EVT_TOOL, self.OnToolClick, id=ENTITY_TYPE_BUTTON_START, id2=wx.NewId())
      #self.toolBar.Bind(wx.EVT_TOOL_RCLICKED, self.OnToolRClick)      
      for toolId in self.idLookup:
        self.toolBar.Bind(wx.EVT_TOOL, self.OnToolClick, id=toolId)
      self.toolBar.Bind(wx.EVT_TOOL_RCLICKED, self.OnToolRClick)

    def deleteNodeInstCreateMenu(self):
      menuItems = self.menuUserDefineNodeTypes.GetMenuItems()
      for item in menuItems:
        self.menuUserDefineNodeTypes.Delete(item.Id)   
      menuItems = self.menuNodeInstCreate.GetMenuItems()
      for item in menuItems:
        self.menuNodeInstCreate.Delete(item.Id)

    def resetDataMembers(self):
      #for (name,m) in self.model.entities.items():
      #  if m.et.name == "ServiceGroup":
      #    m.customInstantiator = lambda entity,pos,size,children,name,pnl=self: pnl.sgInstantiator(entity, pos,size,children,name)
      self.tool = None  # The current tool
      self.drawers = set()
      self.renderArrow = {}
      # The position of the panel's viewport within the larger drawing
      self.location = (0,0)
      self.rotate = 0.0
      self.scale = 1.0

      # Ordering of instances in the GUI display, from the upper left
      self.columns = []
      self.rows = []

      self.intersects = []

      self.addButtons = {}
     
      self.grayCells = {}

      self.selectedEntities = None

      # Building flatten instance from model.xml
      for entInstance in self.model.instances.values():
        """Create a new instance of this entity type at this position"""
        placement = None
        if entInstance.et.name in self.columnTypes:
          placement = "column"
        if entInstance.et.name in self.rowTypes:
          placement = "row"

        if placement:
          if placement == "row":
            self.rows.append(entInstance)  # TODO: calculate an insertion position based on the mouse position and the positions of the other entities
          if placement == "column":
            self.columns.append(entInstance)  # TODO: calculate an insertion position based on the mouse position and the positions of the other entities
      self.UpdateVirtualSize()
      self.layout()
      self.loadGrayCells()

    def setModelData(self, model):
      self.model = model

    def refresh(self):
      print 'instance refresh called'
      self.resetDataMembers()
      #self.layout()
      self.Refresh()

    def sgInstantiator(self,ent,pos,size,children,name):
      """Custom instantiator for service groups"""
      # TODO: there is one unexpected "app" component being created
      (top, all_created) = self.model.recursiveInstantiation(ent)
      return top
#      newdata = copy.deepcopy(ent.data)
#      ret = Instance(ent, newdata,pos,size,name=name)
#      #Actually merge data from entity
#      ret.updateDataFields(newdata)
#      return ret


    def OnReSize(self, event):
      self.Refresh()
      event.Skip()
    
    def addEntityTools(self):
      """Iterate through all the entity types, adding them as tools"""
      tsize = self.toolBar.GetToolBitmapSize()

      sortedent = self.model.entities.items()  # Do this so the buttons always appear in the same order
      sortedent.sort()
      # buttonIdx = ENTITY_TYPE_BUTTON_START
      menu = self.guiPlaces.menu.get("Instantiation",None)
      for e in sortedent:
        et = e[1].et
        placement = None
        if e[0] in self.columnTypes or e[1].et.name in self.columnTypes:  # Take any entity or entity of a type that we designate a columnar
          placement = "column"
        if e[0] in self.rowTypes or e[1].et.name in self.rowTypes:  # Take any entity or entity of a type that we designate a columnar
          placement = "row"

        if placement:
          buttonIdx = wx.NewId()
          buttonSvg = e[1].buttonSvg if hasattr(e[1],"buttonSvg") else et.buttonSvg
          disabledBitmap = buttonSvg.disabledButton(tsize)
          bitmap = buttonSvg.bmp(tsize, { "name":e[0] }, BAR_GREY)  # Use the first 3 letters of the name as the button text if nothing
          shortHelp = e[1].data.get("shortHelp",et.data.get("help",None)) 
          longHelp = e[1].data.get("help",et.data.get("help",None))
          self.toolBar.AddRadioLabelTool(buttonIdx, e[0], bitmap, disabledBitmap, shortHelp=shortHelp, longHelp=longHelp)
          #self.toolBar.AddRadioTool(buttonIdx, bitmap, wx.NullBitmap, shortHelp=et[0], longHelp=longHelp,clientData=et)
          self.idLookup[buttonIdx] = EntityTool(self,e[1])  # register this button so when its clicked we know about it
          #buttonIdx+=1
          e[1].buttonIdx = buttonIdx
          name = e[0]
          if menu: # If there's a menu for these entity tools, then add the object to the menu as well
            menu.Append(buttonIdx, name, name)
            menu.Bind(wx.EVT_MENU, self.OnToolMenu, id=buttonIdx)

      self.toolBar.Realize()

    def addEntityTool(self, ent):
      name = ent.et.name      
      entExists = False
      for (eid,e) in self.idLookup.items():
        if isinstance(e, EntityTool) and e.entity==ent:
          print 'addEntityTool: Entity [%s] exists' % name
          entExists = True
          break
      if not entExists:
        print 'addEntityTool: Entity [%s] not exists. adding tool' % name
        placement = None
        if name in self.columnTypes:
          placement = "column"
        if name in self.rowTypes:
          placement = "row"

        menu = self.guiPlaces.menu.get("Instantiation",None)

        if placement:
          tsize = self.toolBar.GetToolBitmapSize()
          buttonIdx = wx.NewId()
          buttonSvg = ent.buttonSvg if hasattr(ent,"buttonSvg") else ent.et.buttonSvg
          bitmap = buttonSvg.bmp(tsize, { "name":name }, BAR_GREY)  # Use the first 3 letters of the name as the button text if nothing
          disabledBmp = buttonSvg.disabledButton(tsize)
          shortHelp = ent.data.get("shortHelp",ent.et.data.get("help",None)) 
          longHelp = ent.data.get("help",ent.et.data.get("help",None))
          self.toolBar.AddRadioLabelTool(buttonIdx, name, bitmap, disabledBmp, shortHelp=shortHelp, longHelp=longHelp)
          #print 'instanceEditor::addEntityTools: e[0]:%s;e[1].data[name]:%s' % (e[0], e[1].data["name"])
          #self.toolBar.AddRadioTool(buttonIdx, bitmap, wx.NullBitmap, shortHelp=et[0], longHelp=longHelp,clientData=et)
          self.toolBar.EnableTool(buttonIdx, False)          
          self.idLookup[buttonIdx] = EntityTool(self,ent)  # register this button so when its clicked we know about it          
          ent.buttonIdx = buttonIdx
          # Bind this tool to event handler
          self.toolBar.Bind(wx.EVT_TOOL, self.OnToolClick, id=buttonIdx)
          if menu: # If there's a menu for these entity tools, then add the object to the menu as well
            menu.Append(buttonIdx, name, name)
            menu.Bind(wx.EVT_MENU, self.OnToolMenu, id=buttonIdx)
        self.toolBar.Realize()

    def addNodeInstCreateMenuItems(self):
      i = self.menuNodeInstCreate.Append(wx.NewId(), NEW_NODE_INST)
      self.Bind(wx.EVT_MENU, self.OnMenuNodeInstCreate, i)
      self.createNodeTypeMenu()      
      self.menuNodeInstCreate.AppendMenu(wx.NewId(), NEW_NODE_INST_INHERIT_FROM_A_NODE, self.menuUserDefineNodeTypes, "Create new node instance from another one")

    def modifyEntityTool(self, ent, newValue):      
      if isinstance(newValue, types.BooleanType) == True:
        if newValue == True:
          newValue = 'true'
        elif newValue == False:
          newValue = 'false' 
      name = ent.et.name
      entExists = False
      for (eid,e) in self.idLookup.items():
        if isinstance(e, EntityTool) and e.entity.et.name==name:
          print 'modifyEntityTool: Entity [%s] exists' % name
          entExists = True          
          break
      if entExists:
        print 'modifyEntityTool: Entity [%s] exists --> modify it from toolbar to the new name [%s]' % (name, newValue)
        toolItem = self.toolBar.FindById(eid)
        if toolItem:
          toolItem.SetLabel(newValue)
        else:
          print 'modifyEntityTool: tool item with id [%d] does not exist' % eid
        # modify the corresponding menu item from the instantiation menu too
        menu = self.guiPlaces.menu.get("Instantiation",None)
        if menu:
          menuItem = menu.FindItemById(eid)
          if menuItem:
            menuItem.SetText(newValue)
          else:
            print 'modifyEntityTool: menu item with id [%d] does not exist' % eid
      else:
        print 'modifyEntityTool: tool name [%s] does not exist' % name

    def deleteEntityTool(self, ents):      
      for ent in ents:
        name = ent.et.name
        entExists = False
        for (eid,e) in self.idLookup.items():
          if isinstance(e, EntityTool) and e.entity==ent:
            print 'deleteEntityTool: Entity [%s] exists' % name
            entExists = True          
            break
        if entExists:
          print 'deleteEntityTool: Entity [%s] exists --> delete it from toolbar' % name
          if self.tool == self.idLookup[eid]:
            self.tool = None
          self.toolBar.DeleteTool(eid)
          # delete the corresponding menu item from the instantiation menu too
          menu = self.guiPlaces.menu.get("Instantiation",None)
          if menu:
            menu.Delete(eid)
          del self.idLookup[eid]          

    def deleteMenuItems(self):
      menu = self.guiPlaces.menu.get("Instantiation",None)
      menuItems = menu.GetMenuItems()
      for item in menuItems:
        menu.Delete(item.Id)
      menuFile = self.guiPlaces.menu.get("File",None)
      menuGen = menuFile.FindItemById(CODEGEN_BUTTON)
      if menuGen:
        menuFile.Delete(CODEGEN_BUTTON)

    def enableMenuItems(self, enable):
      menu = self.guiPlaces.menu.get("Instantiation",None)
      menuItems = menu.GetMenuItems()
      for item in menuItems:
        menu.Enable(item.Id, enable)
      menuFile = self.guiPlaces.menu.get("File",None)
      menuFile.Enable(CODEGEN_BUTTON, enable)

    def enableTools(self, enable):
      for toolId in self.idLookup:
        self.toolBar.EnableTool(toolId, enable)

    def OnToolMenu(self,event):
      print "On Tool Menu"
      self.toolBar.ToggleTool(event.GetId(), True)
      self.OnToolClick(event)

    def recordStartChange(self, event):
      isStartChange = False
      if isinstance(event,wx.MouseEvent):
        if event.ButtonDown(wx.MOUSE_BTN_LEFT):
          isStartChange = True
      if isinstance(event,wx.KeyEvent):
        if event.GetEventType() == wx.EVT_KEY_DOWN.typeId and (event.GetKeyCode() ==  wx.WXK_DELETE or event.GetKeyCode() ==  wx.WXK_NUMPAD_DELETE):
          isStartChange = True
      if isStartChange:
        self.data_before = self.model.getInstanceInfomation()

    def recordEndChange(self, event, isEndChange=False):
      if not self.data_before: return
      if isinstance(event,wx.MouseEvent):
        if event.ButtonUp(wx.MOUSE_BTN_LEFT):
          isEndChange = True
      if isinstance(event,wx.KeyEvent):
        if event.GetEventType() == wx.EVT_KEY_DOWN.typeId and (event.GetKeyCode() ==  wx.WXK_DELETE or event.GetKeyCode() ==  wx.WXK_NUMPAD_DELETE):
          isEndChange = True
      if isEndChange:
        data_after = self.model.getInstanceInfomation()
        if self.data_before != data_after:
          self.setIntanceChange()
          self.data_before = None

    def OnToolEvent(self,event):
      # Set tool is SelectTool after press ESC button
      if isinstance(event, wx.KeyEvent):
        if wx.WXK_ESCAPE == event.GetUnicodeKey():
          self.toolBar.ToggleTool(SELECT_BUTTON, True)
          tool = self.idLookup[SELECT_BUTTON]
          if self.tool:
            if isinstance(self.tool, EntityTool) and self.tool.entity.et.name=="Node":
              self.newNodeInstOption = None
          if tool:
            tool.OnSelect(self,event)
            self.tool = tool

      handled = False
      if self.tool:
        self.recordStartChange(event)
        handled = self.tool.OnEditEvent(self, event)
        self.recordEndChange(event)
      event.Skip(not handled)  # if you pass false, event will not be processed anymore

    def OnToolClick(self,event):
      co = event.GetClientObject()
      cd = event.GetClientData()
      id = event.GetId()
      print "Tool Clicked %d %s %s" % (id, str(co), str(cd))
      try:
        tool = self.idLookup[id]
        if (isinstance(self.tool, SelectTool) or isinstance(self.tool, CopyTool)) and not isinstance(tool, CopyTool) and self.drawSelectionBox:
          self.drawSelectionBox = False
          self.Refresh()
          self.selectedEntities = None
        if self.tool:
          self.tool.OnUnselect(self,event)
          self.tool = None
          if isinstance(self.tool, EntityTool) and self.tool.entity.et.name=="Node":
            self.newNodeInstOption = None
        if tool:
          tool.OnSelect(self,event)
          self.tool = tool
        # show the menu node inst create options
        if isinstance(self.tool, EntityTool) and self.tool.entity.et.name=="Node":
          self.PopupMenu(self.menuNodeInstCreate)
      except KeyError, e:
        event.Skip()
        pass # Not one of my tools

    def OnToolRClick(self,event):
      co = event.GetClientObject()
      cd = event.GetClientData()
      id = event.GetId()
      print "Tool Right Clicked %d %s %s" % (id, str(co), str(cd))      
      tool = self.idLookup[id]
      try:
        if tool:
          tool.OnRightClick(self,event)
      except KeyError, e:
        event.Skip()
        pass # Not one of my tools

    def OnMenuNodeInstCreate(self, event):
      item = self.menuNodeInstCreate.FindItemById(event.GetId())
      self.newNodeInstOption = item.GetText()
      print 'OnMenuNodeInstCreate: item [%s] selected' % self.newNodeInstOption
      self.userSelectionNode = self.getNodeInst(self.newNodeInstOption)

    def OnPaint(self, event):
        #dc = wx.PaintDC(self)
        dc = wx.BufferedPaintDC(self)
        dc.SetBackground(wx.Brush('white'))
        dc.Clear()
        self.PrepareDC(dc)
        self.UpdateVirtualSize()
        self.render(dc)

    def renderGrayCells(self, ctx):
      #print 'enter renderGrayCell'
      if self.grid:
       for row in self.grid.grid:
        for cell in row:
          grayBmp = self.grayCells.get(cell.bound, None)
          if grayBmp:
            pos = (cell.bound[0],cell.bound[1])            
            #print 'render gray cell at (%d,%d), size: %s; bound (%d,%d,%d,%d)' %(pos[0], pos[1], str(cell.graySize), cell.bound[0],cell.bound[1], cell.bound[2], cell.bound[3])
            svg.blit(
            ctx,
            grayBmp,
            (pos[0]+self.translating['horizontal'], 
            pos[1]+self.translating['vertical']), 
            (1.0,1.0), 
            0.0)

    def createGrayCell(self, pos, entities=None, save=True):
      added = self.grid.createGrayCell(pos, self)
      if save:
        self.saveGrayCell(added, entities)      

    def loadGrayCells(self):
      print 'enter loadGrayCells'
      self.grayCells = {}
      tagName = "disableAssignmentOn"
      y = ROW_MARGIN+ROW_SPACING
      cell=None
      for e1 in self.rows:
        x = COL_MARGIN+COL_SPACING
        for e2 in self.columns:
          thisNode = e2.data["name"]
          pos = (x,y)
          cell = self.grid.getAnyCell(pos)
          assert(cell)
          values = self.model.instances.get(thisNode, None)
          if values:
            sgs = values.data.get(tagName, None)
            if sgs:
              sgs = sgs.split()
              if e1.data["name"] in sgs:
                self.createGrayCell(pos, entities=None, save=False)
          x=COL_SPACING+cell.bound[2]
        if cell:
          y=ROW_SPACING+cell.bound[3]
        else:
          y=ROW_SPACING
      print 'leave loadGrayCells'
    
    def saveGrayCell(self, added, entities):
      thisSg = ""
      tagName = "disableAssignmentOn"
      for e in entities:
        if e.et.name == "ServiceGroup":
          thisSg = e.data["name"]
          break
      for e in entities:
        if e.et.name == "Node":
          thisNode = e.data["name"]
          values = self.model.instances.get(thisNode, None)
          if not values:
            continue
          sgs = values.data.get(tagName, None)
          if sgs:
            if added:
              if sgs.find(thisSg) == -1:
                sgs += " "+thisSg
                values.data[tagName] = sgs
            else:
              # exclude the this sg from the node.instances
              sgs = sgs.split()
              temp = sgs[0] if sgs[0]!=thisSg else ""
              #temp = ""
              for i in range(1,len(sgs)):
                if sgs[i]!=thisSg:
                  temp+=" "+sgs[i]
              values.data[tagName]=temp
          else:
            if added:
              values.data[tagName] = thisSg            
          break

    def removeGrayCells(self, e):
      if e.entity.et.name != "ServiceGroup" and e.entity.et.name != "Node":
        return
      y = ROW_MARGIN+ROW_SPACING
      cell=None
      for e1 in self.rows:        
        x = COL_MARGIN+COL_SPACING
        for e2 in self.columns:          
          pos = (x,y)
          cell = self.grid.getAnyCell(pos)
          assert(cell)
          if e1==e or e2==e:
            print 'removeGrayCells: remove gray cell at pos: : %s' % str(pos)
            if cell.bound in self.grayCells:
              del self.grayCells[cell.bound]
          x=COL_SPACING+cell.bound[2]
        if cell:
          y=ROW_SPACING+cell.bound[3]
 
    def setSUAssignment(self, inst):
      su = None
      for ca in inst.containmentArrows:        
        if ca.contained.et.name == "ServiceUnit":
          su = ca.contained
          break
      if not su:
        print 'setSUAssignment: SU is not found. Nothing to do'
        return
      y = ROW_MARGIN+ROW_SPACING
      for e1 in self.rows:        
        x = COL_MARGIN+COL_SPACING
        cell = None
        for e2 in self.columns:
          su.pos = (x,y)
          ok = self.grid.reposition(su, self, inst)
          if ok:
            return
          cell = self.grid.getAnyCell(su.pos)
          assert(cell)
          x=COL_SPACING+cell.bound[2]
        if cell:
          y=ROW_SPACING+cell.bound[3]

    def getUserDefinedNodeTypes(self):
      userDefinedNodeTypes = []
      for e in self.columns:
        thisNode = e.data["name"]
        values = self.model.instances.get(thisNode, None)
        if not values:
          continue
        canBeInherited = bool(values.data.get("canBeInherited", None))
        if canBeInherited:
          userDefineType = values.data.get("userDefinedType", None)
          if userDefineType and len(userDefineType)>0:
            userDefinedNodeTypes.append(userDefineType)
      return userDefinedNodeTypes

    def updateMenuUserDefineNodeTypes(self, ent, oldValue=None,flag=None):
      userDefineType = ent.data.get("userDefinedType", None)
      print 'updateMenuUserDefineNodeTypes: userDefinedType: %s' % userDefineType
      if not flag:
        flag = ent.data.get("canBeInherited", None)
      menuItems = self.menuUserDefineNodeTypes.GetMenuItems()
      for item in menuItems:
        if item.GetText() == userDefineType: # the user-defined node type has been already here
          if flag:# True is to add new user-defined node type but it exists, nothing to do            
            return
          else: # False is to delete an existing one from the menu            
            self.menuUserDefineNodeTypes.Delete(item.Id)
            return
        elif oldValue and item.GetText()==oldValue:
          self.menuUserDefineNodeTypes.Delete(item.Id)
          break
      if flag:
        # Add the new one
        if userDefineType and len(userDefineType)>0:
          i = self.menuUserDefineNodeTypes.Append(wx.NewId(), userDefineType)
          self.Bind(wx.EVT_MENU, self.OnMenuNodeInstCreate, i)

    def getNodeInst(self, usrDefinedType):
      for e in self.columns:
        thisNode = e.data["name"]
        values = self.model.instances.get(thisNode, None)
        if not values:
          continue
        canBeInherited = bool(values.data.get("canBeInherited", None))
        if canBeInherited:
          if usrDefinedType==values.data.get("userDefinedType", None):
            return values
      return None

    def createNodeTypeMenu(self):
      usrDefinedNodeTypes = self.getUserDefinedNodeTypes()      
      for t in usrDefinedNodeTypes:
        i = self.menuUserDefineNodeTypes.Append(wx.NewId(), t)
        self.Bind(wx.EVT_MENU, self.OnMenuNodeInstCreate, i)

    def duplicateNode(self):
      if self.userSelectionNode:
        name = NameCreator(self.userSelectionNode.entity.data["name"])
        dupNode = self.userSelectionNode.duplicate(name)
        if dupNode:
          self.model.instances[name] = dupNode
          self.columns.append(dupNode)
          # duplicate containment arrows
          for ca in self.userSelectionNode.containmentArrows:
            print 'duplicateNode: ca [%s]' % ca.contained.data["name"]
            inst = self.model.recursiveDuplicateInst(ca.contained)
            if inst.et.name == "ServiceUnit":
              inst.childOf.add(dupNode)
              dupNode.createContainmentArrowTo(inst)
          return True
      return False

    def updateInstDetails(self):
      if share.instanceDetailsPanel:
        for (name, ent) in filter(lambda (name, ent): not ent.et.name in (self.ignoreEntities),self.model.instances.items()):
          share.instanceDetailsPanel.createTreeItemEntity(name, ent)

    def CreateNewInstance(self,entity,position,size=None, name=None):
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
        inst = entity.createInstance(position, size, name=name, parent=self)
        inst.instanceLocked = copy.deepcopy(entity.instanceLocked)

        self.model.instances[inst.data["name"]] = inst

        if placement == "row":
          self.rows.append(inst)  # TODO: calculate an insertion position based on the mouse position and the positions of the other entities
        if placement == "column":
          self.columns.append(inst)  # TODO: calculate an insertion position based on the mouse position and the positions of the other entities

        # Put all childs instances into hyperlisttree
        if share.instanceDetailsPanel:
          for (name, ent) in filter(lambda (name, ent): not ent.et.name in (self.ignoreEntities),self.model.instances.items()):
            share.instanceDetailsPanel.createTreeItemEntity(name, ent)
        self.Refresh()

      self.UpdateVirtualSize()
      self.layout()
      if entity.et.name == "ServiceGroup":
        self.setSUAssignment(inst)
        #self.layout()
        self.Refresh()

#  Function does not exist
#      if entity.et.name == "Node":
#        self.updateNodeTypeMenu()
      return True 

    def GetBoundingBox(self):
      """Calculate the bounding box -- the smallest box that will fit around all the elements in this panel"""
      virtRct = wx.Rect(0,0, ((COL_MARGIN + (len(self.columns) +1)*(COL_WIDTH+COL_SPACING) + COL_SPACING) * self.scale), (ROW_MARGIN + (len(self.rows)+1)*(ROW_WIDTH+ROW_SPACING) + ROW_SPACING) * self.scale)
      return virtRct
 
    def UpdateVirtualSize(self):
      virtRct = self.GetBoundingBox()
      # We need to shift the client rectangle to take into account
      # scrolling, converting device to logical coordinates
      self.SetVirtualSize((virtRct.x + virtRct.Width, virtRct.y + virtRct.Height))
 
    def repositionRow(self, e, pos):
      if len(self.rows) > 0:
        self.rows.remove(e)
      i = 0
      length = len(self.rows)
      while i<length:
        tmp = self.rows[i]
        # print "%d < %d" % (e.pos[1], tmp.pos[1])
        if e.pos[1] < tmp.pos[1]:
          break
        i+=1
      self.rows.insert(i,e)
     # print [ x.data["name"] for x in self.rows]
      self.layout()
      self.Refresh()      


    def repositionColumn(self,e,pos):
      if len(self.columns) > 0:
        self.columns.remove(e)
      i = 0
      length = len(self.columns)
      while i<length:
        tmp = self.columns[i]
        #print "%d < %d" % (e.pos[0], tmp.pos[0])
        if e.pos[0] < tmp.pos[0]:
          break
        i+=1
      self.columns.insert(i,e)
      #print [ x.data["name"] for x in self.rows]
      self.layout()
      self.Refresh()      
      

    def layout(self):
        """place all the objects in their appropriate locations"""
        rowSet = set(self.rows)
        columnSet = set(self.columns)
        self.renderArrow = {}  # We will recalculate which arrows need rendering
        colCount = len(self.columns)
        panelSize = self.GetVirtualSize()
        scaledExtents = (panelSize.x/self.scale,panelSize.y/self.scale)
        x = COL_MARGIN
        self.renderOrder = []
        for i in self.columns:
          width = max(COL_WIDTH,i.size[0])  # Pick the minimum width or the user's adjustment, whichever is bigger
          i.pos = (x,0)
          size = (width,scaledExtents[1])
          if size != i.size:
            i.size = size
            i.recreateBitmap()
          x+=width+COL_SPACING
          self.renderOrder.append(i)

        y = ROW_MARGIN
        
        for i in self.rows:
          width = max(ROW_WIDTH,i.size[1])  # Pick the minimum width or the user's adjustment, whichever is bigger
          i.pos = (0,y)
          size = (scaledExtents[0],width)
          if size != i.size:
            i.size = size
            i.recreateBitmap()
          y+=width+ROW_SPACING
          self.renderOrder.append(i)
        
        rows = [Margin((0,0),(panelSize[0]/self.scale,ROW_MARGIN))]
        rows += self.rows
 
        cols = [Margin((0,0),(COL_MARGIN,panelSize[1]/self.scale))]
        cols += self.columns

        self.grid = GridEntityLayout(rows, cols)  # Create a 2 dimensional array of sets of objects

        # Re-calculate intersect rectangle
        del self.intersects[0:]
        for n in self.columns:
          nRect = getRectInstance(n, self.scale) 
          # Get SGs
          for sg in self.rows:
            sgRect = getRectInstance(sg, self.scale)
            if sgRect.Intersects(nRect):
              self.intersects.append(sgRect.Intersect(nRect))
       
        for (name,i) in self.model.instances.items():
          if i in self.columns or i in self.rows: continue  # Already laid out
          t = len(i.childOf)
          if t > 0:
             # Find out which rows and columns this object is in
            inRow = i.childOf & rowSet
            inCol = i.childOf & columnSet
            rowParent = None
            colParent = None
            cell = self.grid.nogrid
            if len(inRow) and len(inCol):
              rowParent = next(iter(inRow))
              colParent = next(iter(inCol))
              #bound = boxIntersect(rowParent.pos, rowParent.size, colParent.pos, colParent.size)
              cell = self.grid.idx(rowParent,colParent)
            elif len(inRow):
              rowParent = next(iter(inRow))
              #bound = boxIntersect(rowParent.pos, rowParent.size, (0,0), (COL_MARGIN,panelSize.y))
              cell = self.grid.idx(rowParent,0)
            elif len(inCol):            
              colParent = next(iter(inCol))
              #bound = boxIntersect(colParent.pos, colParent.size, (0,0), (panelSize.x, ROW_MARGIN))
              cell = self.grid.idx(0,colParent)
            else:  # its not drawn contained
              #bound = (0,0,panelSize.x, panelSize.y)
              cell = self.grid.nogrid
            # print "addentity %s %s" % (i.et.name, i.data['name'])
            cell.addEnt(i)
            #cell.bound = bound

            if rowParent:
              self.renderArrow[(rowParent,i)] = False # object is drawn inside, so it is unnecessary to render the containment arrow
            if colParent:
              self.renderArrow[(colParent,i)] = False # object is drawn inside, so it is unnecessary to render the containment arrow 
          else:
            pass
        self.grid.layout()


    def render(self, dc):
        """Put the entities on the screen"""
        ctx = wx.lib.wxcairo.ContextFromDC(dc)
        # Now draw the graph
        ctx.save()
        # First position the screen's view port into the larger document
        ctx.translate(*self.location)
        ctx.rotate(self.rotate)
        ctx.scale(self.scale, self.scale)
        # Now draw the links
        # Now draw the entites

        # Draw the baseline row and column entities
        for e in filter(lambda entInt: entInt.et.name in (self.columnTypes + self.rowTypes), self.model.instances.values()):
          svg.blit(
          ctx,
          e.bmp,
          (e.pos[0]+self.translating['horizontal'], 
          e.pos[1]+self.translating['vertical']),
          e.scale,
          e.rotate)

        self.addButtons = {}

        # Draw the other instances on top
        for e in filter(lambda entInt: not entInt.et.name in (self.columnTypes + self.rowTypes ), self.model.instances.values()):
          if e.size != (0,0):  # indicate that the object is hidden
            svg.blit(
            ctx,
            e.bmp,
            (e.pos[0]+self.translating['horizontal'], 
            e.pos[1]+self.translating['vertical']),
            e.scale,
            e.rotate)
            
            self.addButtons[
            (e.pos[0]+e.size[0]-self.addBmp.get_width()+self.translating['horizontal'], 
            e.pos[1]+e.size[1]-self.addBmp.get_height()+self.translating['vertical'], 
            e.pos[0]+e.size[0]+self.translating['horizontal'], 
            e.pos[1]+e.size[1]+self.translating['vertical'])] = e
            
            # Show "component/csi" children of "SUs/SIs" inside the graphical box which is the SU/SI
            if e.et.name in ("ServiceInstance", "ServiceUnit",):
              childs = []
              for a in e.containmentArrows:
                childs.append(a.contained)

              # make children's Y size be smaller then the parent by 30 at the bottom
              bound = e.pos + (e.pos[0]+e.size[0], e.pos[1]+max(5,e.size[1]-(30.0/e.scale[1])))
              for ch,place in zip(childs,partition(len(childs), bound)):
                ch.pos = (place[0]- 5,place[1] - (bound[3] - bound[1])/5) #
                tmp = ((place[2]-place[0])+10, (place[3]-place[1])+(bound[3] - bound[1])/5+5) # (min(cell.bound[2]-30,64),min(cell.bound[3]-30,64))
                if tmp[0]<3: tmp[0]=3
                if tmp[1]<3: tmp[1]=3
                if tmp != ch.size:
                  ch.size = tmp
                  ch.recreateBitmap()
                svg.blit(
                ctx,
                ch.bmp,
                (ch.pos[0]+self.translating['horizontal'], 
                ch.pos[1]+self.translating['vertical']),
                ch.scale,
                ch.rotate)
                
                # Render copy button (duplicate, similar: selected entity and type ctrl+'v')
                self.addButtons[
                (ch.pos[0]+ch.size[0]-self.addBmp.get_width()+self.translating['horizontal'],
                ch.pos[1]+ch.size[1]-self.addBmp.get_height()+self.translating['vertical'], 
                ch.pos[0]+ch.size[0]+self.translating['horizontal'], 
                ch.pos[1]+ch.size[1]+self.translating['vertical'])] = ch
                
        # Draw "add" icon on top of entity
        # Render copy button (duplicate, similar: selected entity and type ctrl+'v')
        for (rect,e) in self.addButtons.items():
          svg.blit(
          ctx,
          self.addBmp,
          (rect[0],rect[1]),
          e.scale,
          e.rotate)

        # Now draw the containment arrows on top
#        pdb.set_trace()
        if 0:
         for (name,e) in self.model.instances.items():
          for a in e.containmentArrows:
            st = a.container.pos
            end = a.contained.pos
            if self.renderArrow.get((a.container,a.contained),True):  # Check to see if there's an directive whether to render this arrow or not.  If there is not one, render it by default
              # pdb.set_trace()
              drawCurvyArrow(
              ctx, 
              (st[0] + a.beginOffset[0]+self.translating['horizontal'],
              st[1] + a.beginOffset[1]+self.translating['vertical']),
              (end[0] + a.endOffset[0]+self.translating['horizontal'],
              end[1] + a.endOffset[1]+self.translating['vertical']),
              a.midpoints, 
              linkNormalLook)
        
        self.renderGrayCells(ctx)
        ctx.restore()

        # These are non-model based transient elements that need to be drawn like selection boxes
        for e in self.drawers:
          e.render(ctx)

        for idx in self.idLookup:
          tool = self.idLookup[idx]
          if isinstance(tool, SelectTool) and not self.drawSelectionBox:
            continue
          tool.render(ctx)

        for rect in self.intersects:
          drawIntersectRect(ctx, rect)

    def findEntitiesAt(self,pos, filterOut = []):
      """Returns the entity located at the passed position """
      # TODO handle the viewscope's translation, rotation, scaling
      # Real pos
      pos = convertToRealPos(pos, self.scale)
      ret = set()
      for (name, e) in self.model.instances.items():
        if e.et.name in filterOut:
          continue

        furthest= (
        e.pos[0] + e.size[0]*e.scale[0]+ self.translating['horizontal'], 
        e.pos[1] + e.size[1]*e.scale[1]+ self.translating['vertical'])
        #print e.data["name"], ": ", pos, " inside: ", e.pos, " to ", furthest
        if inBox(
        pos,
        (e.pos[0]+ self.translating['horizontal'], 
        e.pos[1]+ self.translating['vertical'],
        furthest[0],
        furthest[1])): # mouse is in the box formed by the entity
          ret.add(e)
      return ret

    def findObjectsAt(self,pos, filter = []):
      return self.findInstancesAt(pos).union(self.findEntitiesAt(pos))

    def findEntitiesTouching(self,rect, filterOut = []):
      """Returns all entities touching the passed rectangle """
      # TODO handle the viewscope's translation, rotation, scaling
      # Real rectangle
      rect = convertToRealPos(rect, self.scale)
      ret = set()
      for (name, e) in self.model.instances.items():
        if e.et.name in filterOut:
          continue

        furthest= (e.pos[0] + e.size[0]*e.scale[0],e.pos[1] + e.size[1]*e.scale[1])
        if rectOverlaps(rect,(e.pos[0],e.pos[1],furthest[0],furthest[1])):  # mouse is in the box formed by the entity
          ret.add(e)
      return ret

    def findAddButtonAt(self, pos):
      pos = convertToRealPos(pos, self.scale)
      for (rect,e) in self.addButtons.items():
        if inBox(pos, rect):
          return e
      return None

    def notifyValueChange(self, ent, key, query, newValue):
      for (name, e) in self.model.instances.items():
        if e == ent:
          iterKeys = iter(key.split("_"))
          token  = next(iterKeys)
          d = e.data
          while 1:
            try:
              token  = next(iterKeys)
              if type(d[token]) is types.DictType:
                #d[token] = newValue
                d = d[token]
              else:
                # token is the last key
                break
            except StopIteration:
              break
          d[token] = newValue          
          # set the name (as a key) of self.model.instances dict
          validator = query.GetValidator()
          if token == "name":
            if validator:
              self.model.deleteInstanceFromMicrodom(validator.currentValue)
            self.model.instances[newValue] = self.model.instances.pop(name)
          if validator:
            validator.currentValue = newValue
          else:
            print 'instanceEditor::notifyValueChange: validator is null'
          e.recreateBitmap()

          if token=="canBeInherited":
            self.updateMenuUserDefineNodeTypes(ent=ent, oldValue=None, flag=d["canBeInherited"])

      self.Refresh()

    def SetSashPosition(self, position):
      if isinstance(self.GetParent(), wx.SplitterWindow):
        # Manual resize sash position
        width = self.GetParent().GetSashPosition()
        self.GetParent().SetSashPosition(-1)
        if width > 10:
          self.GetParent().SetSashPosition(width)
        else:
          self.GetParent().SetSashPosition(position)
        self.GetParent().UpdateSize()

    def updateDefineNodeTypes(self, ents):
      """Update define node type after intances is deleted """
      for ent in ents:
        itemId = None
        cntIntance = 0
        cntViableType = 0
        userDefineType = ent.data.get("userDefinedType", None)
        if userDefineType is not None:
          for (name, e) in self.model.instances.items():
            if e.data.get("userDefinedType", None) == userDefineType:
              if e.data.get("canBeInherited", None):
                cntIntance += 1
          if ent.data.get("canBeInherited", None):
            menuItems = self.menuUserDefineNodeTypes.GetMenuItems()
            for item in menuItems:
              if item.GetText() == userDefineType: # the user-defined node type has been already here
                itemId = item.Id
                cntViableType += 1

          if cntIntance >= 2 and cntViableType == 1:
            pass
          elif itemId is not None:
            self.menuUserDefineNodeTypes.Delete(itemId)

    def deleteEntities(self, ents, deleteFromModel=True):
      #remove columns/rows
      self.drawSelectionBox = False
      self.selectedEntities = None
      for ent in ents:
        self.removeGrayCells(ent)
        try:
          self.columns.remove(ent);
        except:
          pass
        
        try:
          self.rows.remove(ent);
        except:
          pass

      if share.instanceDetailsPanel:
        share.instanceDetailsPanel.deleteTreeItemEntities(ents)
     
      if deleteFromModel:
        self.updateDefineNodeTypes(ents)
        self.model.delete(ents)
      self.Refresh()
      self.layout()

    def setIntanceChange(self):
      '''
      @summary    : add character '*' if intance is change
      '''
      self.guiPlaces.frame.modelChange()

model = None

class Reminder:
  def __init__(self):
    pass
  def Dialog(self, fileName):
    self.dlg = wx.Dialog(None, title = "Merge source code ?", size=(620, 320))
    vBox = wx.BoxSizer(wx.VERTICAL)
    hBox = wx.BoxSizer(wx.HORIZONTAL)
    parentFrame = share.instancePanel.guiPlaces.frame 
    lablel1 = wx.StaticText(self.dlg, label="Source code already exists in : ")
    lablel2 = wx.StaticText(self.dlg, label=fileName)
    lablel3 = wx.StaticText(self.dlg, label="Would you like to merge or overwrite the source code:")
    self.checkBox = wx.CheckBox(self.dlg, id=wx.ID_ANY, label="Never show this dialog again")
    self.checkBox.Bind(wx.EVT_CHECKBOX, self.onChecked) 
    lablel5 = wx.StaticText(self.dlg, label="(Dialog can be reenable through Project Properties)")

    noBtn = wx.Button(self.dlg, label="Merge")
    noBtn.Bind(wx.EVT_BUTTON, self.onClickMergeNoBtn)
    cancelBtn = wx.Button(self.dlg, label="Cancel")
    cancelBtn.Bind(wx.EVT_BUTTON, self.onClickMergeCancelBtn)
    yesBtn = wx.Button(self.dlg, label="Overwrite")
    yesBtn.Bind(wx.EVT_BUTTON, self.onClickMergeYesBtn)

    vBox.Add(lablel1, 0, wx.ALL, 5)
    vBox.Add(lablel2, 0, wx.ALL|wx.LEFT, 15)
    vBox.Add(lablel3, 0, wx.ALL, 5)
    vBox.Add(self.checkBox, 0, wx.ALL, 5)
    vBox.Add(lablel5, 0, wx.ALL, 5)

    hBox.Add(noBtn, 0, wx.CENTER, 0)
    hBox.Add(cancelBtn, 0, wx.CENTER, 0)
    hBox.Add(yesBtn, 0, wx.CENTER|wx.RIGHT, 15)
    vBox.Add(hBox, 0, wx.TOP|wx.ALIGN_RIGHT, 30)

    self.dlg.SetSizer(vBox)

    result = self.dlg.ShowModal()
    return result

  def onClickMergeNoBtn(self, event):
    '''Do not merge and never show this dialog again'''
    if self.checkBox.GetValue():
      parentFrame = share.instancePanel.guiPlaces.frame
      parentFrame.project.prjProperties['mergeMode'] = "never"
      parentFrame.project.savePrjProperties()
    self.dlg.EndModal(1)
  
  def onClickMergeCancelBtn(self, event):
    '''Cancle'''
    self.dlg.EndModal(0)

  def onClickMergeYesBtn(self, event):
    '''Merge and do not show this dialog again'''
    if self.checkBox.GetValue():
      parentFrame = share.instancePanel.guiPlaces.frame
      parentFrame.project.prjProperties['mergeMode'] = "always"
      parentFrame.project.savePrjProperties()
    self.dlg.EndModal(2)

  def onChecked(self, event):
    pass

def Test():
  import time
  import pyGuiWrapper as gui
  global model
  model = Model()
  model.load("testModel.xml")

  gui.go(lambda parent,menu,tool,status,m=model: Panel(parent,menu,tool,status, m))
  #gui.start(lambda parent,menu,tool,status,m=model: Panel(parent,menu,tool,status, m))
  return model

# Notes
#  test automation panel.WarpPointer(self,x,y)  "Move pointer"

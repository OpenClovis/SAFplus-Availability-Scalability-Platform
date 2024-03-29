import pdb
import math
import time
from types import *

import wx
import wx.lib.wxcairo
import cairo
import gi
gi.require_version('Rsvg', '2.0')
from gi.repository import Rsvg
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
from gfxtoolkit import *
from gfxmath import *

import share
 
ENTITY_TYPE_BUTTON_START = wx.NewId()
SAVE_BUTTON = wx.NewId()
ZOOM_BUTTON = wx.NewId()
CONNECT_BUTTON = wx.NewId()
SELECT_BUTTON = wx.NewId()
DELETE_BUTTON = wx.NewId()

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

def convertToRealPos(pos, scale):
  if pos == None: return pos
  if isinstance(pos, wx.Point):
    return wx.Point(round(pos.x/scale), round(pos.y/scale))
  elif isinstance(pos, tuple):
    return [convertToRealPos(x, scale) for x in pos]
  else:
    return round(pos/scale)

      


class EntityTypeTool(Tool):
  def __init__(self, panel,entityType):
    self.entityType = entityType
    Tool.__init__(self,panel)
    self.box = BoxGesture()

  def OnSelect(self, panel,event):
    panel.statusBarText.SetLabel("Click to create a new %s" % self.entityType.name);
    return True

  def OnUnselect(self,panel,event):
    print("Unselected %s" % self.entityType.name)
    return True

  def OnEditEvent(self,panel, event):
    pos = panel.CalcUnscrolledPosition(event.GetPosition())
    ret = False
    if isinstance(event,wx.MouseEvent):
      if event.ButtonDown(wx.MOUSE_BTN_LEFT):  # Select
        self.box.start(panel,pos)
        ret = True
      elif event.Dragging():
        self.box.change(panel,event)
        ret = True
      elif event.ButtonUp(wx.MOUSE_BTN_LEFT):
        if self.entityType.name == 'Cluster':
          for name,e in list(share.umlEditorPanel.model.entities.items()):
            if e.et.name == 'Cluster':
              panel.statusBarText.SetLabel("Model can't contain more than one Cluster");
              panel.drawers.discard(self.box)
              panel.Refresh()
              return ret
        rect = self.box.finish(panel,pos)
        # Real point rectangle
        rect = convertToRealPos(rect, panel.scale)
        size = (rect[2]-rect[0],rect[3]-rect[1])
        if size[0] < 15 or size[1] < 15:  # its so small it was probably an accidental drag rather then a deliberate sizing
          size = None
        ret = self.CreateNewInstance(panel,(rect[0],rect[1]),size)
      # in pointer mode scroll wheel zooms
      #elif event.GetWheelRotation() > 0:
      #  panel.SetScale(panel.scale*1.1,pos)
      #elif event.GetWheelRotation() < 0:
      #  panel.SetScale(panel.scale*.9,pos)

    return ret
    
  def CreateNewInstance(self,panel,position,size=None):
    """Create a new instance of this entity type at this position"""
    panel.statusBarText.SetLabel("Created %s" % self.entityType.name);
    
    ent = self.entityType.createEntity((list(position)[0]-self.panel.translating['horizontal'], list(position)[1]-self.panel.translating['vertical']), size)
    if(ent.data["entityType"] == "ComponentServiceInstance"):
      ent.data["type"] = ent.data["name"]
    
    
    panel.entities[ent.data["name"]] = ent
    panel.Refresh()

    if share.instancePanel:
      share.instancePanel.addEntityTool(ent)

    if share.detailsPanel:
      share.detailsPanel.createTreeItemEntity(ent.data["name"], ent)

    return True
 

class LinkTool(Tool):
  def __init__(self, panel):
    Tool.__init__(self,panel)
    self.line =   LazyLineGesture(color=(0,0,0,.9),circleRadius=5,linethickness=4,arrowlen=15, arrowangle=PI/8)
    self.startEntity = None
    self.err = None

  def OnSelect(self, panel,event):
    panel.statusBarText.SetLabel("Click on an entity and drag to another entity to create a relationship.");
    return True

  def OnUnselect(self,panel,event):
    return True

  def OnEditEvent(self,panel, event):
    pos = panel.CalcUnscrolledPosition(event.GetPosition())
    ret = False
    if isinstance(event,wx.MouseEvent):
      if event.ButtonDown(wx.MOUSE_BTN_LEFT):  # Select
        entities = panel.findEntitiesAt(pos)
        if len(entities) != 1:
          panel.statusBarText.SetLabel("You must choose a single starting entity!");
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
            panel.statusBarText.SetLabel("You must choose a single ending entity!");
            self.err = FadingX(panel,pos,1)
          else:
            self.endEntity = entities.pop()
            rc = self.startEntity.canContain(self.endEntity)
            if rc != RLS_OK:
              panel.statusBarText.SetLabel("Relationship is not allowed.  Most likely %s entities cannot contain %s entities.  Or maybe you've exceeded the number of containments allowed" % (self.startEntity.et.name, self.endEntity.et.name));
              if rc!=RLS_ERR_EXISTS:
                self.err = FadingX(panel,pos,1)
            else:
              rc = self.endEntity.canBeContained(self.startEntity)
              if rc != RLS_OK:
                panel.statusBarText.SetLabel("Relationship is not allowed.  Most likely %s entities can only be contained by one %s entity."  % (self.endEntity.et.name, self.startEntity.et.name));
                self.err = FadingX(panel,pos,1)
              else:
                # Add the arrow into the model.  the arrow's location is relative to the objects it connects
                #print 'starten: %s, endE: %s' % (self.startEntity.data['name'],self.endEntity.data['name'])
                ca = ContainmentArrow(
                self.startEntity, 
                (line[0][0] - self.startEntity.pos[0] - self.panel.translating['horizontal'],
                line[0][1] - self.startEntity.pos[1]  - self.panel.translating['vertical']), 
                self.endEntity, 
                (line[1][0]-self.endEntity.pos[0]     - self.panel.translating['horizontal'],
                line[1][1]-self.endEntity.pos[1]      - self.panel.translating['vertical']), 
                [line[2]] if len(line)>1 else None)
                self.startEntity.containmentArrows.append(ca)
                if (self.endEntity.data['entityType'] == 'Component' or self.endEntity.data['entityType'] == 'NonSafComponent') and self.startEntity.data['entityType']=='ComponentServiceInstance':
                   self.endEntity.data['csiTypes'].add(self.startEntity.data['name'])
                   self.updateProxyCSI(self.endEntity)
                if self.startEntity.data['entityType'] == 'Component' and self.endEntity.data['entityType']=='NonSafComponent':
                   self.endEntity.data['proxyCSI'].add(self.startEntity.data['csiTypes'])
                #print str(self.startEntity.data)
                #print '\n\n'
                #print str(self.endEntity.data)

    return ret
    
  def CreateNewInstance(self,panel,position,size=None):
    """Create a new instance of this entity type at this position"""
    panel.statusBarText.SetLabel("Created %s" % self.entityType.name);
    panel.entities.append(self.entityType.createEntity(position, size))
    panel.Refresh()
    return True

  def updateProxyCSI(self, proxyComp):
    for arrow in proxyComp.containmentArrows:
       proxiedComp = arrow.contained
       for name,e in list(share.umlEditorPanel.model.entities.items()):
          if proxiedComp.data['name'] == e.data['name']:
             proxiedComp.data['proxyCSI'] = proxyComp.data['csiTypes']
             break

class SelectTool(Tool):
  def __init__(self, panel):
    self.panel = panel
    self.defaultStatusText = "Click to edit configuration.  Double click to expand/contract.  Drag to move.  Del key removes."
    self.selected = set()  # This is everything that is currently selected... using shift or ctrl click may mean the more is selected then currently touching
    self.touching = set()  # This is everything that the cursor is currently touching
    self.rect = None       # Will be something if a rectangle selection is being used
    #self.downPos = None    # Where the left mouse button was pressed
    self.boxSel = BoxGesture()

  def OnSelect(self, panel,event):
    panel.statusBarText.SetLabel(self.defaultStatusText);
    self.touching.clear()
    self.selected.clear()
    return True

  def OnUnselect(self,panel,event):
    pass

  def render(self,ctx):
    # Draw mini rectangle at top left corner
    if self.selected:
      for e in self.selected:
        if e.data["name"] in self.panel.entities:  # Don't show if its deleted.
          pos = (e.pos[0] * share.umlEditorPanel.scale,e.pos[1] * share.umlEditorPanel.scale) 
          ctx.set_line_width(2)
          ctx.rectangle(pos[0] + self.panel.scale*self.panel.translating['horizontal'], pos[1] + self.panel.scale*self.panel.translating['vertical'], 20*self.panel.scale, 20*self.panel.scale)
          ctx.set_source_rgba(0, 0, 1, 1)
          ctx.fill()

  def OnEditEvent(self,panel, event):
    panel.drawSelectionBox = True
    pos = panel.CalcUnscrolledPosition(event.GetPosition())
    if isinstance(event,wx.MouseEvent):
      # Single select
      if event.ButtonDown(wx.MOUSE_BTN_LEFT) or event.LeftDClick():  
        entities = panel.findEntitiesAt(pos)
        self.dragPos = pos
        if not entities:
          panel.statusBarText.SetLabel(self.defaultStatusText);
          self.touching = set()
          self.boxSel.start(panel,pos)
          return False
        print("Touching %s" % ", ".join([ e.data["name"] for e in entities]))
        panel.statusBarText.SetLabel("Touching %s" % ", ".join([ e.data["name"] for e in entities]));

        self.touching = set(entities)
        # If the control key is down, then add to the currently selected group, otherwise replace it.
        if event.ControlDown():
          self.selected = self.selected.union(self.touching)
          #panel.Refresh()
        else:
          # If you touch something else, your touching set changes.  But if you touch something in your current touch group then nothing changes
          # This enables behavior like selecting a group of entities and then dragging them (without using the ctrl key)
          if not elemsInSet(entities,self.selected):
            self.selected = self.touching.copy()
            #panel.Refresh()
        if event.LeftDClick():
          self.updateSelected()
        panel.Refresh()
        return True
        
      if event.Dragging():
        # if you are touching anything, then drag everything
        if self.touching and self.dragPos:
          delta = ((pos[0]-self.dragPos[0])/self.panel.scale, (pos[1] - self.dragPos[1])/self.panel.scale)
          if delta[0] != 0 or delta[1] != 0:
            for e in self.selected:
              e.pos = (e.pos[0] + delta[0], e.pos[1] + delta[1])  # move all the touching objects by the amount the mouse moved
          self.dragPos = pos
          panel.Refresh()
        else:  # touching nothing, this is a selection rectangle
          self.boxSel.change(panel,event)

      elif event.ButtonUp(wx.MOUSE_BTN_LEFT):
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
      
      # in pointer mode scroll wheel zooms
      #elif event.GetWheelRotation() > 0:
      #  panel.SetScale(panel.scale*1.1,pos)
      #elif event.GetWheelRotation() < 0:
      #  panel.SetScale(panel.scale*.9,pos)

    if isinstance(event,wx.KeyEvent):      
      if event.GetEventType() == wx.EVT_KEY_DOWN.typeId and (event.GetKeyCode() ==  wx.WXK_DELETE or event.GetKeyCode() ==  wx.WXK_NUMPAD_DELETE):
        if self.touching:
          self.panel.deleteEntities(self.touching)
          #self.touching.clear()
        elif self.selected:
          self.panel.deleteEntities(self.selected)
          #self.selected.clear()
        self.touching.clear()
        self.selected.clear()
        #panel.Refresh()
        return True # I consumed this event
      else:
        return False # Give this key to someone else

    #self.updateSelected()
  
  def updateSelected(self):
    if len(self.selected) == 1:
      parentFrame = share.umlEditorPanel.guiPlaces.frame
      if share.detailsPanel:
        share.detailsPanel.showEntity(next(iter(self.selected)))
        index = parentFrame.tab.GetPageIndex(share.detailsPanel)
        parentFrame.tab.SetSelection(index)
      else:
        parentFrame.insertPage(1)
        share.detailsPanel.showEntity(next(iter(self.selected)))    

class ZoomTool(Tool):
  def __init__(self, panel):
    self.panel = panel
    self.defaultStatusText = "Click (+) to zoom in.  Right click (-) to zoom out."
    self.rect = None       # Will be something if a rectangle selection is being used
    #self.downPos = None    # Where the left mouse button was pressed
    self.boxSel = BoxGesture()
    #self.scale = 1
    self.scaleRange = 0.1
    self.minScale = 0.2
    self.maxScale = 10
    self.handBmp = wx.Bitmap(common.fileResolver("hand.png"))

  def OnSelect(self, panel,event):
    panel.statusBarText.SetLabel(self.defaultStatusText);
    zoomImg =  self.ScaleBitmap(self.handBmp, 16*self.panel.scale, 25*self.panel.scale)
    cursor = wx.CursorFromImage(zoomImg)
    self.panel.SetCursor(cursor)

    return True

  def OnUnselect(self,panel,event):
    self.panel.SetCursor(wx.StockCursor(wx.CURSOR_ARROW))
    pass

  def render(self,ctx):
    pass

  def OnEditEvent(self,panel, event):
    pos = panel.CalcUnscrolledPosition(event.GetPosition())
    scale = self.panel.scale
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

        if (scale + self.scaleRange) < self.maxScale:
          scale += self.scaleRange

      elif event.ButtonUp(wx.MOUSE_BTN_RIGHT):
        rect = self.boxSel.finish(panel,pos)
        delta = (rect[2]-rect[0],rect[3]-rect[1])
        # Center rect selected
        pos[0] = rect[0] + delta[0]/2
        pos[1] = rect[1] + delta[1]/2

        if (scale - self.scaleRange) > self.minScale:
          scale -= self.scaleRange

      elif event.ControlDown(): 
        if event.GetWheelRotation() > 0:
            if (scale + self.scaleRange) < self.maxScale:
              scale += self.scaleRange
        elif event.GetWheelRotation() < 0:
            if (scale - self.scaleRange) > self.minScale:
              scale -= self.scaleRange

    if isinstance(event, wx.KeyEvent):
      if event.ControlDown(): 
        if event.GetKeyCode() == wx.WXK_NUMPAD_ADD:
          if (scale + self.scaleRange) < self.maxScale:
            scale += self.scaleRange
        elif event.GetKeyCode() == wx.WXK_NUMPAD_SUBTRACT:
          if (scale - self.scaleRange) > self.minScale:
            scale -= self.scaleRange
        elif event.GetKeyCode() == wx.WXK_NUMPAD0:
          scale = 1

    self.SetScale(scale, pos)

  def SetScale(self, newscale, pos):
    if newscale != self.panel.scale:

      # Update scale for panel
      self.panel.scale = newscale

      # TODO: scroll wrong??? 
      scrollx, scrolly = self.panel.GetScrollPixelsPerUnit();
      size = self.panel.GetClientSize()
      self.panel.Scroll((pos[0]*newscale - size.x/2)/scrollx, (pos[1]*newscale - size.y/2)/scrolly)  # multiply by scale because I am converting from scaled to screen coordinates

      self.panel.Refresh()

      zoomImg =  self.ScaleBitmap(self.handBmp, 16*newscale, 16*newscale)
      cursor = wx.CursorFromImage(zoomImg)
      self.panel.SetCursor(cursor)

  def ScaleBitmap(self, bitmap, width, height):
      image = wx.ImageFromBitmap(bitmap)
      image = image.Scale(width, height, wx.IMAGE_QUALITY_HIGH)
      return image


class SaveTool(Tool):
  def __init__(self, panel):
    self.panel = panel
  
  def OnSelect(self, panel,event):
    dlg = wx.FileDialog(panel, "Save model as...", os.getcwd(), style=wx.SAVE | wx.OVERWRITE_PROMPT, wildcard="*.xml")
    if dlg.ShowModal() == wx.ID_OK:
      filename = dlg.GetPath()
      self.panel.model.save(filename)
      # TODO: Notify (IPC) to GUI instances to change, using pyinotify(python), inotify (c++)
      # Trigger re-create instance dialog from standalone

    return False

class DeleteTool(Tool):
  def __init__(self, panel):
    self.panel = panel
    self.touching = set()  # This is everything that the cursor is currently touching
    self.wireCheck = False
    self.rect = None       # Will be something if a rectangle selection is being used
    self.boxSel = BoxGesture()
    self.entOrder = ["Component", "ComponentServiceInstance", "ServiceUnit", "ServiceInstance", "Node", "ServiceGroup"] # Select layer entity by order: Component->csi->su->si->node->sg
    self.selectMultiple = False

  def OnSelect(self, panel,event):
    return False

  def getDistance(self, pos_1, pos_2):
    vector = (pos_2[0] - pos_1[0], pos_2[1] - pos_1[1])
    return math.sqrt(vector[0]**2 + vector[1]**2)

  def wireDelete(self, pos):
    '''
    @summary    : detect wire by pos then delete wire(s)/connect(s)
    '''
    model = share.detailsPanel.model
    pos = convertToRealPos(pos, self.panel.scale)
    for (name, e) in list(model.entities.items()):
      for arrow in e.containmentArrows:
        st = arrow.container.pos
        end = arrow.contained.pos
        pos_1 = (st[0] + arrow.beginOffset[0], st[1] + arrow.beginOffset[1])
        pos_2 = (end[0] + arrow.endOffset[0], end[1] + arrow.endOffset[1])
        l = self.getDistance(pos_1, pos_2)
        l1 = self.getDistance(pos_1, pos)
        l2 = self.getDistance(pos_2, pos)
        if math.fabs(l1+l2-l) < 0.08:          
          if (arrow.contained.data['entityType'] == 'Component' or arrow.contained.data['entityType'] == 'NonSafComponent') and arrow.container.data['entityType']=='ComponentServiceInstance':
              if arrow.container.data['type'] in arrow.contained.data['csiTypes']:
                arrow.contained.data['csiTypes'].remove(arrow.container.data['type'])
              self.removeProxyCSI(arrow.contained)
          if arrow.container.data['entityType'] == 'Component' and arrow.contained.data['entityType']=='NonSafComponent':
              if arrow.container.data['type'] in arrow.contained.data['proxyCSI']:
                arrow.contained.data['proxyCSI'].remove(arrow.container.data['type'])
          e.containmentArrows.remove(arrow)
          model.deleteWireFromMicrodom(name, arrow.contained.data['name'])

  def OnEditEvent(self,panel,event):
    pos = panel.CalcUnscrolledPosition(event.GetPosition())
    if isinstance(event, wx.MouseEvent):
      if event.ButtonDown(wx.MOUSE_BTN_LEFT):  # Select
        self.selectMultiple = False
        entities = panel.findEntitiesAt(pos)
        self.dragPos = pos
        if not entities:
          self.wireCheck = True
          self.touching = set()
          self.boxSel.start(panel,pos)
          return False
        self.wireCheck = False
        # If you touch something else, your touching set changes.  But if you touch something in your current touch group then nothing changes
        # This enables behavior like selecting a group of entities and then dragging them (without using the ctrl key)
        if not entities.issubset(self.touching) or (len(self.touching) != len(entities)):
          self.touching = set(entities)

      if event.Dragging():
          # Touching nothing, this is a selection rectangle
          self.wireCheck = False
          self.boxSel.change(panel,event)
  
      if event.ButtonUp(wx.MOUSE_BTN_LEFT):
        # Or find everything inside a selection box
        if self.boxSel.active:
          rect = self.boxSel.rect
          delta = (rect[0]-rect[2], rect[1] - rect[3])
          if delta[0] != 0 or delta[1] != 0:
            # Multi select to delete
            self.selectMultiple = True

          self.touching = panel.findEntitiesTouching(self.boxSel.finish(panel,pos))

        if self.touching:
          print("Delete(s): %s" % ", ".join([ e.data["name"] for e in self.touching]))
          self.deleteEntities(self.touching)
          self.touching.clear()
        elif self.wireCheck:
          self.wireDelete(pos)
        panel.Refresh()

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

  def removeProxyCSI(self, proxyComp):    
    for arrow in proxyComp.containmentArrows:
       proxiedComp = arrow.contained
       for name,e in list(share.umlEditorPanel.model.entities.items()):
          if proxiedComp.data['name'] == e.data['name']:
             proxiedComp.data['proxyCSI'] = ''
             break
# Global of this panel for debug purposes only.  DO NOT USE IN CODE
dbgUep = None

class Panel(scrolled.ScrolledPanel):
    def __init__(self, parent,guiPlaces,model,**cfg):
      scrolled.ScrolledPanel.__init__(self, parent, style = wx.TAB_TRAVERSAL|wx.SUNKEN_BORDER)
      self.guiPlaces = guiPlaces
      self.SetupScrolling(True, True, 10, 10, True, False)
      self.Bind(wx.EVT_SIZE, self.OnReSize)
      self.Bind(wx.EVT_PAINT, self.OnPaint)

      share.umlEditorPanel = self

      dbgUep = self 
      self.menuBar = self.guiPlaces.menubar
      self.toolBar = self.guiPlaces.toolbar
      self.statusBar = self.guiPlaces.statusbar
      self.statusBarText = self.guiPlaces.statusBarText
      self.model=model
      self.tool = None  # The current tool
      self.drawers = set()

      # The position of the panel's viewport within the larger drawing
      self.location = (0,0)
      self.rotate = 0.0
      self.scale = 0.5

      self.undoData = []
      self.redoData = []
      self.UNDO_MAX = 100
      self.undoAction = False
      self.data_before = None

      # Buttons and other IDs that are registered may need to be looked up to turn the ID back into a python object
      self.idLookup={}  
      self.drawSelectionBox = False
      # The list of entities that exist in this model, extracted from the model for easy display
      self.entities = self.model.entities
      # TODO load entities from the model

      # Add toolbar buttons

      # first get the toolbar    
      if not self.toolBar:
        self.toolBar = wx.ToolBar(self,-1)
        self.toolBar.SetToolBitmapSize((24,24))

      #self.addCommonTools()

      #self.entityToolIds = [] # stores entity tool id
      # Add the custom entity creation tools as specified by the model's YANG
      #self.addEntityTools()
      self.addTools()
      # Set up to handle tool clicks
      #self.toolBar.Bind(wx.EVT_TOOL, self.OnToolClick, id=ENTITY_TYPE_BUTTON_START. id2=self.idLookup[len(self.idLookup)-1])  # id=start, id2=end to bind a range
      #self.toolBar.Bind(wx.EVT_TOOL_RCLICKED, self.OnToolRClick)

      # Set up events that tools may be interested in
      toolEvents = [ wx.EVT_LEAVE_WINDOW, wx.EVT_LEFT_DCLICK, wx.EVT_LEFT_DOWN , wx.EVT_LEFT_UP, wx.EVT_MOUSEWHEEL , wx.EVT_MOVE,wx.EVT_MOTION , wx.EVT_RIGHT_DCLICK, wx.EVT_RIGHT_DOWN , wx.EVT_RIGHT_UP, wx.EVT_KEY_DOWN, wx.EVT_KEY_UP]
      for t in toolEvents:
        self.Bind(t, self.OnToolEvent)
      self.toolIds = [CONNECT_BUTTON,SELECT_BUTTON,ZOOM_BUTTON,DELETE_BUTTON]
      
      self.translating = {'vertical':0, 'horizontal':0}

    def SetScale(self, newscale, pos):
      if newscale != self.scale:

        # Update scale for panel
        self.scale = newscale
        # TODO: scroll wrong??? 
        scrollx, scrolly = self.GetScrollPixelsPerUnit();
        size = self.GetClientSize()
        self.Scroll((pos[0]*newscale - size.x/2)/scrollx, (pos[1]*newscale - size.y/2)/scrolly)  # multiply by scale because I am converting from scaled to screen coordinates
        self.Refresh()

    def Undo(self):
      if len(self.undoData) == 0:
        return
      self.undoAction = True
      data = self.undoData.pop()
      self.model.setEntitiesAndInfos(data[0])
      frame = share.umlEditorPanel.guiPlaces.frame
      model = frame.model
      model.uml.setModelData(model.model)
      model.instance.deleteMyTools()
      model.instance.addTools()
      self.entities = share.detailsPanel.model.entities
      self.refresh()
      self.modellingChange()
      frame.enableTools(frame.getCurrentPageText(0))
      self.redoData.append(data)
      while len(self.redoData) > self.UNDO_MAX:
        self.redoData.remove(self.redoData[0])

    def Redo(self):
      if len(self.redoData) == 0:
        return
      self.undoAction = True
      data = self.redoData.pop()
      self.model.setEntitiesAndInfos(data[1])
      frame = share.umlEditorPanel.guiPlaces.frame
      model = frame.model
      model.uml.setModelData(model.model)
      model.instance.deleteMyTools()
      model.instance.addTools()
      self.entities = share.detailsPanel.model.entities
      self.refresh()
      self.modellingChange()
      frame.enableTools(frame.getCurrentPageText(0))
      self.undoData.append(data)
      while len(self.undoData) > self.UNDO_MAX:
        self.undoData.remove(self.undoData[0])

    def resetDataMembers(self):
      self.location = (0,0)
      self.rotate = 0.0
      self.scale = 1.0
      self.tool = None
      self.drawSelectionBox = False
      #self.entityToolIds = []

    def setModelData(self, model):
      print('set model data')
      self.model = model
      self.entities = self.model.entities

    def OnShow(self,event):
      print("ON SHOW")
      if event.IsShown():
        enable = True
      else:
        enable = False
      for i in self.toolIds:  # Enable/disable the tools when the tab is shown/removed
        print(i)
        #self.toolBar.EnableTool(i,enable)
        tool = self.toolBar.FindById(i)
        tool.Enable(enable)
      self.toolBar.Refresh()

    def OnReSize(self, event):
      self.UpdateVirtualSize()
      self.Refresh(False)
      event.Skip()
    
    def addEntityTools(self):
      """Iterate through all the entity types, adding them as tools"""
      # print 'addEntityTools'
      tsize = self.toolBar.GetToolBitmapSize()

      sortedEt = list(self.model.entityTypes.items())  # Do this so the buttons always appear in the same order
      sortedEt.sort()
      #buttonIdx = ENTITY_TYPE_BUTTON_START
      menu = self.guiPlaces.menu.get("Modelling",None)
      for et in sortedEt:
        buttonIdx = wx.NewId()
        name = et[0]
        bitmap = et[1].buttonSvg.bmp(tsize, { "name":name[0:3] }, BAR_GREY)  # Use the first 3 letters of the name as the button text if nothing
        bitmapDisabled = et[1].buttonSvg.disabledButton(tsize)
        shortHelp = et[1].data.get("shortHelp",None)
        longHelp = et[1].data.get("help",None)
        #self.toolBar.AddLabelTool(11, et[0], bitmap, shortHelp=shortHelp, longHelp=longHelp)
        self.toolBar.AddRadioTool(buttonIdx, "", bitmap, bitmapDisabled, shortHelp=et[0], longHelp=longHelp,clientData=et)
        self.idLookup[buttonIdx] = EntityTypeTool(self,et[1])  # register this button so when its clicked we know about it
        #buttonIdx+=1
        if menu: # If there's a menu for these entity tools, then add the object to the menu as well
          menu.Append(buttonIdx, name, name)
          menu.Bind(wx.EVT_MENU, self.OnToolMenu, id=buttonIdx)

        et[1].buttonIdx = buttonIdx
        #self.entityToolIds.append(buttonIdx)
      self.toolBar.Realize()
      

    def addTools(self):
     self.addCommonTools()
     self.addEntityTools()
     #self.toolBar.Bind(wx.EVT_TOOL, self.OnToolClick, id=ENTITY_TYPE_BUTTON_START, id2=wx.NewId())
     #self.toolBar.Bind(wx.EVT_TOOL_RCLICKED, self.OnToolRClick)
     for toolId in self.idLookup:
       self.toolBar.Bind(wx.EVT_TOOL, self.OnToolClick, id=toolId)
     self.toolBar.Bind(wx.EVT_TOOL_RCLICKED, self.OnToolRClick)

    def addCommonTools(self):
      tsize = self.toolBar.GetToolBitmapSize()

      # example of adding a standard button
      #new_bmp =  wx.ArtProvider.GetBitmap(wx.ART_NEW, wx.ART_TOOLBAR, tsize)
      #self.toolBar.AddLabelTool(10, "New", new_bmp, shortHelp="New", longHelp="Long help for 'New'")

      # TODO: add disabled versions to these buttons

      if 0:  # Save is handled at the project level
        bitmap1 = svg.SvgFile("save_as.svg").bmp(tsize, { }, (222,222,222,wx.ALPHA_OPAQUE))
        self.toolBar.AddTool(SAVE_BUTTON, "", bitmap1, wx.NullBitmap, shortHelp="save", longHelp="Save model as...")
        self.idLookup[SAVE_BUTTON] = SaveTool(self)

      # Add the umlEditor's standard tools
      self.toolBar.AddSeparator()

      s = svg.SvgFile("connect.svg")
      bitmapDisabled = s.disabledButton(tsize)
      bitmap = s.bmp(tsize, { }, BAR_GREY)
      self.toolBar.AddRadioTool(CONNECT_BUTTON, "", bitmap, bitmapDisabled, shortHelp="connect", longHelp="Draw relationships between entities")
      self.idLookup[CONNECT_BUTTON] = LinkTool(self)

      s = svg.SvgFile("pointer.svg")
      bitmapDisabled = s.disabledButton(tsize)
      bitmap = s.bmp(tsize, { }, BAR_GREY)
      self.toolBar.AddRadioTool(SELECT_BUTTON, "", bitmap, bitmapDisabled, shortHelp="select", longHelp="Select one or many entities.  Click entity to edit details.  Double click to expand/contract.")
      self.idLookup[SELECT_BUTTON] = self.selectTool = SelectTool(self)

      s = svg.SvgFile("zoom.svg")
      bitmapDisabled = s.disabledButton(tsize)
      bitmap = s.bmp(tsize, { }, BAR_GREY)
      self.toolBar.AddRadioTool(ZOOM_BUTTON, "", bitmap, bitmapDisabled, shortHelp="zoom", longHelp="Left click (+) to zoom in. Right click (-) to zoom out.")
      self.idLookup[ZOOM_BUTTON] = ZoomTool(self)

      s = svg.SvgFile("remove.svg")
      bitmapDisabled = s.disabledButton(tsize)
      bitmap = s.bmp(tsize, { }, BAR_GREY)
      self.toolBar.AddRadioTool(DELETE_BUTTON, "", bitmap, bitmapDisabled, shortHelp="Delete entity/entities", longHelp="Select one or many entities. Click entity to delete.")
      self.idLookup[DELETE_BUTTON] = DeleteTool(self)
      # setting the default tool
      self.tool = self.idLookup[CONNECT_BUTTON]

    def deleteTools(self):   
      self.toolBar.DeletePendingEvents()
      #self.toolBar.Unbind(wx.EVT_TOOL)
      self.toolBar.ClearTools()
      self.deleteMenuItems()
      if share.instancePanel:
        share.instancePanel.deleteMenuItems()
      self.idLookup.clear()

    def deleteMyTools(self):
      for toolId in self.idLookup:
        self.toolBar.DeleteTool(toolId)
      self.deleteMenuItems()
      self.idLookup.clear()

    def deleteMenuItems(self):
      menu = self.guiPlaces.menu.get("Modelling",None)
      menuItems = menu.GetMenuItems()
      for item in menuItems:
        menu.Delete(item.Id)

    def enableMenuItems(self, enable):
      menu = self.guiPlaces.menu.get("Modelling",None)
      menuItems = menu.GetMenuItems()
      for item in menuItems:
        menu.Enable(item.Id, enable)

    def enableTools(self, enable):      
      for toolId in self.idLookup:
        self.toolBar.EnableTool(toolId, enable)

    def OnToolEvent(self,event):      
      # Reset to SelectTool after press ESC button
      if isinstance(event, wx.KeyEvent):
        if wx.WXK_ESCAPE == event.GetUnicodeKey():
          self.toolBar.ToggleTool(SELECT_BUTTON, True)
          tool = self.idLookup[SELECT_BUTTON]
          if self.tool:
            self.tool.OnUnselect(self,event)
            self.tool = None
          if tool:
            tool.OnSelect(self,event)
            self.tool = tool

      self.recordStartChange(event)
      handled = False
      if self.tool:
        handled = self.tool.OnEditEvent(self, event)
      event.Skip(not handled)  # if you pass false, event will not be processed anymore
      self.recordEndChange(event)
      
    def recordStartChange(self, event):
      isStartChange = False
      if isinstance(event,wx.MouseEvent):
        if event.ButtonDown(wx.MOUSE_BTN_LEFT):
          isStartChange = True
      if isinstance(event,wx.KeyEvent):      
        if event.GetEventType() == wx.EVT_KEY_DOWN.typeId and (event.GetKeyCode() ==  wx.WXK_DELETE or event.GetKeyCode() ==  wx.WXK_NUMPAD_DELETE):
          isStartChange = True
      if isStartChange:
        self.model.updateMicrodom()
        self.data_before = self.model.getEntitiesAndInfos()

    def modellingChange(self):
      '''
      @summary    : add character '*' if modelling is change
      '''
      share.umlEditorPanel.guiPlaces.frame.modelChange()

    def recordEndChange(self, event, isEndChange=False):
      if not self.data_before: return
      if isinstance(event,wx.MouseEvent):
        if event.ButtonUp(wx.MOUSE_BTN_LEFT):
          isEndChange = True
      if isinstance(event,wx.KeyEvent):      
        if event.GetEventType() == wx.EVT_KEY_DOWN.typeId and (event.GetKeyCode() ==  wx.WXK_DELETE or event.GetKeyCode() ==  wx.WXK_NUMPAD_DELETE):
          isEndChange = True
      if isEndChange:
        self.model.updateMicrodom()
        data_after = self.model.getEntitiesAndInfos()
        if self.data_before[1] != data_after[1] or self.data_before[0] != data_after[0]:
          data = (self.data_before, data_after)
          self.undoData.append(data)
          while len(self.undoData) > self.UNDO_MAX:
            self.undoData.remove(self.undoData[0])
          self.data_before = data_after
          self.modellingChange()

    def OnToolMenu(self,event):
      print("On Tool Menu")
      self.toolBar.ToggleTool(event.GetId(), True)
      self.OnToolClick(event)

    def OnToolClick(self,event):
      co = event.GetClientObject()
      cd = event.GetClientData()
      id = event.GetId()
      print("Tool Clicked %d %s %s" % (id, str(co), str(cd)))
      try:
        tool = self.idLookup[id]
        if not isinstance(tool, SelectTool) and self.drawSelectionBox:
          self.drawSelectionBox = False
          self.Refresh()

        if self.tool:
          self.tool.OnUnselect(self,event)
          self.tool = None

        if tool:
          tool.OnSelect(self,event)
          self.tool = tool
      except KeyError as e:
        event.Skip()
        pass # Not one of my tools
      

    def OnToolRClick(self,event):
      co = event.GetClientObject()
      cd = event.GetClientData()
      id = event.GetId()
      print("Tool Right Clicked %d %s %s" % (id, str(co), str(cd)))      
      try:
        tool = self.idLookup[id]
        if tool:
          tool.OnRightClick(self,event)
      except KeyError as e:
        event.Skip()
        pass # Not one of my tools


    def OnPaint(self, event):
      dc = wx.BufferedPaintDC(self, style=wx.BUFFER_VIRTUAL_AREA)
      dc.SetBackground(wx.Brush('white'))
      dc.Clear()
      self.UpdateVirtualSize()
      self.render(dc)

    def GetBoundingBox(self):
      # Calculate bounding box
      virtRct = wx.Rect()
      if len(self.entities) > 0:
        elst = list(self.entities.items())
        first = elst[0]
        for (name,e) in elst:
          if e == first:
            virtRct = wx.Rect(e.pos[0]*self.scale, e.pos[1]*self.scale, e.size[0]*e.scale[0]*self.scale, e.size[1]*e.scale[1]*self.scale)
          else:
            virtRct.Union(wx.Rect(e.pos[0]*self.scale, e.pos[1]*self.scale, e.size[0]*e.scale[0]*self.scale, e.size[1]*e.scale[1]*self.scale))
      return virtRct
 
    def UpdateVirtualSize(self):
      virtRct = self.GetBoundingBox()
      # We need to shift the client rectangle to take into account
      # scrolling, converting device to logical coordinates
      self.SetVirtualSize((virtRct.x + virtRct.Width, virtRct.y + virtRct.Height))

    def render(self, dc):
        """Put the entities on the screen"""
        #print 'enter render'
        ctx = wx.lib.wxcairo.ContextFromDC(dc)
        # Now draw the graph
        ctx.save()
        # First position the screen's view port into the larger document
        ctx.translate(self.location[0],self.location[1])
        # ctx.rotate(self.rotate)
        ctx.scale(self.scale, self.scale)
        # Now draw the links
        # Now draw the entites
        for (name,e) in list(self.entities.items()):
          svg.blit(ctx, e.bmp, (e.pos[0]+self.translating['horizontal'], e.pos[1]+self.translating['vertical']), e.scale, e.rotate)
        # Now draw the containment arrows on top
        for (name,e) in list(self.entities.items()):
          for a in e.containmentArrows:
            #print 'UpdatedMidpoints = ', [(list(a.midpoints[0])[0]+self.translating['horizontal'], list(a.midpoints[0])[1]+self.translating['vertical'])]
            st = a.container.pos
            end = a.contained.pos
            drawCurvyArrow(ctx, (st[0] + a.beginOffset[0],st[1] + a.beginOffset[1]),(end[0] + a.endOffset[0],end[1] + a.endOffset[1]),a.midpoints, linkNormalLook)
        ctx.restore()

        # These are non-model based transient elements that need to be drawn like selection boxes
        for e in self.drawers:
          e.render(ctx)

        for idx in self.idLookup:
          tool = self.idLookup[idx]
          if isinstance(tool, SelectTool) and not self.drawSelectionBox:
            continue
          tool.render(ctx)

    def findEntitiesAt(self,pos):
      """Returns the entity located at the passed position """
      # TODO handle the viewscope's translation, rotation, scaling
      # Real pos
      pos = convertToRealPos(pos, self.scale)
      ret = set()
      for (name, e) in list(self.entities.items()):
        furthest= (e.pos[0] + e.size[0]*e.scale[0] + self.translating['horizontal'], e.pos[1] + e.size[1]*e.scale[1] + self.translating['vertical'])
        #print e.data["name"], ": ", pos, " inside: ", e.pos, " to ", furthest
        if pos[0] >= (e.pos[0] + self.translating['horizontal']) and pos[1] >= (e.pos[1] + self.translating['vertical']) and pos[0] <= furthest[0] and pos[1] <= furthest[1]:  # mouse is in the box formed by the entity
          ret.add(e)
      return ret

    def findEntitiesTouching(self,rect):
      """Returns all entities touching the passed rectangle """
      # TODO handle the viewscope's translation, rotation, scaling
      # Real rectangle
      rect = convertToRealPos(rect, self.scale)
      ret = set()
      for (name, e) in list(self.entities.items()):
        furthest= (e.pos[0] + e.size[0]*e.scale[0] + self.translating['horizontal'], e.pos[1] + e.size[1]*e.scale[1] + self.translating['vertical'])
        if rectOverlaps(rect,(e.pos[0] + self.translating['horizontal'], e.pos[1] + self.translating['vertical'], furthest[0], furthest[1])):  # mouse is in the box formed by the entity
          ret.add(e)
      return ret

    def notifyValueChange(self, ent, key, query, newValue):      
      names = list()
      for name, value in self.entities.items():
        if value != ent:
          names.append(name)
      validator = query.GetValidator()
      if (key.split("_")[-1] == "name") and (newValue in names):
        self.statusBarText.SetWarning("Entity name [%s] existed. Name [%s] is being used" % (newValue, validator.currentValue))
        query.ChangeValue(validator.currentValue)
        return False
      elif (key.split("_")[-1] == "name") and (newValue == ""):
        self.statusBarText.SetWarning("Entity name must not be empty.")
        query.ChangeValue(validator.currentValue)
        return False
      elif (key.split("_")[-1] != "name") and (newValue.isdigit() == False):
        self.statusBarText.SetWarning("Invalid input, must contains numbers only")
        query.ChangeValue(validator.currentValue)
        return False
      self.statusBarText.SetLabel(" ")
      if share.instancePanel:
        share.instancePanel.modifyEntityTool(ent, newValue)
      for (name, e) in list(self.entities.items()):
        if e == ent:
          iterKeys = iter(key.split("_"))
          token  = next(iterKeys)
          d = e.data
          while 1:
            try:
              token  = next(iterKeys)
              if type(d[token]) is dict:
                #d[token] = newValue
                d = d[token]
              else:
                # token is the last key
                break
            except StopIteration:
              break
          d[token] = newValue
          if token == "name":
            self.statusBarText.SetLabel("Entity name changed from [%s] to [%s]." % (validator.currentValue, newValue))
            if validator:              
              self.model.deleteEntityFromMicrodom(validator.currentValue, e.et.name)
            self.entities[newValue] = self.entities.pop(name)          
          if validator:            
            validator.currentValue = newValue
          else:
            print('umlEditor::notifyValueChange: validator is null')
          e.recreateBitmap()
          break

      self.Refresh()
      return True

    def deleteEntities(self, ents):
      self.drawSelectionBox = False
      if share.instancePanel:
        share.instancePanel.deleteEntityTool(ents)
      if share.detailsPanel:
        share.detailsPanel.deleteTreeItemEntities(ents)
      self.model.delete(ents)
      self.Refresh()

    def refresh(self):
      print('refresh window')
      self.Refresh()

model = None

def Test():
  import time
  import pyGuiWrapper as gui
  global model
  model = Model()
  try:
    model.load("testModel.xml")
  except IOError as e:
    if e.errno != 2: # no such file
      raise

  gui.go(lambda parent,menu,tool,status,m=model: Panel(parent,menu,tool,status, m))
  #gui.start(lambda parent,menu,tool,status,m=model: Panel(parent,menu,tool,status, m))
  return model

# Notes
#  test automation panel.WarpPointer(self,x,y)  "Move pointer"

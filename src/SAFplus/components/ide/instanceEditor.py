import pdb
import math
import time
from types import *

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

import dot
from common import *
from module import Module
from entity import *
from model import Model

from gfxmath import *
from gfxtoolkit import *

import share
 
ENTITY_TYPE_BUTTON_START = 100
SAVE_BUTTON = 99
ZOOM_BUTTON = 98
CONNECT_BUTTON = 97
SELECT_BUTTON = 96
CODEGEN_BUTTON = 95
CODEGEN_LANG_C = 94
CODEGEN_LANG_CPP = 93
CODEGEN_LANG_PYTHON = 92
CODEGEN_LANG_JAVA = 91

COL_MARGIN = 250
COL_SPACING = 2
COL_WIDTH = 250

ROW_MARGIN = 100
ROW_SPACING = 2
ROW_WIDTH   = 120

linkNormalLook = dot.Dot({ "color":(0,0,.8,.75), "lineThickness": 4, "buttonRadius": 6, "arrowLength":15, "arrowAngle": PI/8 })

class TemplateMgr:
  """ This class caches template files for later use to reduce disk access"""
  def __init__(self):
    self.cache = {}
  
  def loadPyTemplate(self,fname):
    """Load a template file and return it as a string"""
    if self.cache.has_key(fname): return self.cache[fname]
    
    f = open(fname,"r")
    s = f.read()
    t = Template(s)
    self.cache[fname] = t
    return t

  def loadKidTemplate(self,fname):
    pass

# The global template manager
templateMgr = TemplateMgr()

#TODO: Get relative directiry with model.xml
myDir = os.getcwd()
TemplatePath = myDir + "/codegen/templates/"

class Output:
  """This abstract class defines how the generated output is written to the file system
     or tarball.

     In these routines if you tell it to write a file or template to a nonexistant path
     then that path is created.  So you no longer have to worry about creating directories.
     If you call writes("/foo/bar/yarh/myfile.txt","contents") then the directory hierarchy /foo/bar/yarh is automatically created.  The file "myfile.txt" is created and written with "contents".
  """
  def write(self, filename, data):
    if type(data) in types.StringTypes:
      self.writes(filename, data)
    else: self.writet(filename,data)
        
  def writet(self, filename, template):
    pass
  def writes(self, filename, template):
    pass
   

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
        size = None
        ret = self.panel.CreateNewInstance(self.entity,(pos[0],pos[1]),size)
            
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

  def OnSelect(self, panel,event):
    panel.statusBar.SetStatusText(self.defaultStatusText,0);
    return True

  def OnUnselect(self,panel,event):
    pass

  def render(self,ctx):
    # Draw mini rectangle at left right top bottom corner
    if len(self.selected) > 0:
      for e in self.selected:
        svg.blit(ctx,self.rectBmp,e.pos,e.scale,e.rotate)

  def OnEditEvent(self,panel, event):
    ret = False
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
    
        # Remove the background entities; you can't move them
        ent = set()
        rcent = set()
        for e in entities:
          if e.et.name in panel.rowTypes or e.et.name in panel.columnTypes:
            rcent.add(e)
          else:
            ent.add(e)

        # I'm going to let you select row/column entities OR contained entities but not both
        if ent:
          entities = ent
        else:
          entities = rcent
        print "Touching %s" % ", ".join([ e.data["name"] for e in entities])

        panel.statusBar.SetStatusText("Touching %s" % ", ".join([ e.data["name"] for e in entities]),0);

        # If you touch something else, your touching set changes.  But if you touch something in your current touch group then nothing changes
        # This enables behavior like selecting a group of entities and then dragging them (without using the ctrl key)
        if not entities.issubset(self.touching) or (len(self.touching) != len(entities)):
          self.touching = set(entities)
          self.selected = self.touching.copy()
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
          if delta[0] != 0 or delta[1] != 0:
            for e in self.selected:
              e.pos = (e.pos[0] + delta[0], e.pos[1] + delta[1])  # move all the touching objects by the amount the mouse moved
          self.dragPos = pos
          panel.Refresh()
        else:  # touching nothing, this is a selection rectangle
          self.boxSel.change(panel,event)

      if event.ButtonUp(wx.MOUSE_BTN_LEFT):
        # TODO: Move the data in this entity to the configuration editing sidebar, and expand it if its minimized.
        if self.touching and self.dragPos:
          # print "drop!"
          for e in self.selected:
             if e.et.name in panel.rowTypes:
               panel.repositionRow(e,pos)
             if e.et.name in panel.columnTypes:
               panel.repositionColumn(e,pos)
             else:
               panel.grid.reposition(e)
          self.touching = set()
          panel.layout()
          panel.Refresh()

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
        entities = panel.findEntitiesAt(pos)
        if not entities: return False

        # TODO: show instance details
        if len(entities) == 2:
          share.instanceDetailsPanel.showEntities(*entities)
          self.panel.SetSashPosition(self.panel.GetParent().GetClientSize().x/4)

    if isinstance(event,wx.KeyEvent):
      if event.GetEventType() == wx.EVT_KEY_DOWN.typeId:
        try:
          character = chr(event.GetKeyCode())
        except ValueError:
          character  = None
        print "key code: ", character
        if event.GetKeyCode() ==  wx.WXK_DELETE or event.GetKeyCode() ==  wx.WXK_NUMPAD_DELETE:
          if self.touching:
            self.panel.deleteEntities(self.touching)
            self.touching.clear()
          elif self.selected:
            self.panel.deleteEntities(self.selected)
            self.selected.clear()
          self.panel.Refresh()
          ret=True # I consumed this event
        elif event.ControlDown() and character ==  'V':
          print "Copy selected instances"
          (self.selected,addtl) = self.panel.model.duplicate(self.selected,recursive=True)

          ret=True
      else:
        return False # Give this key to someone else

    self.updateSelected()
    return ret
  
  def updateSelected(self):
    if len(self.selected) == 1:
      if share.instanceDetailsPanel:
        share.instanceDetailsPanel.showEntity(next(iter(self.selected)))
        self.panel.SetSashPosition(self.panel.GetParent().GetClientSize().x/4)

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
    return False

class FilesystemOutput(Output):
  """Write to a normal file system"""

  def writet(self, filename, template):
    if not os.path.exists(os.path.dirname(filename)):
      os.makedirs(os.path.dirname(filename))			
    template.write(filename, "utf-8",False,"xml")
    
  def writes(self, filename, string):
    if not os.path.exists(os.path.dirname(filename)):
      os.makedirs(os.path.dirname(filename))			
    f = open(filename,"w")
    f.write(string)
    f.close()

class GenerateTool(Tool):
  def __init__(self, panel):
    self.panel = panel
    self.dictGenFunc = {CODEGEN_LANG_C:self.generateC, CODEGEN_LANG_CPP:self.generateCpp, CODEGEN_LANG_PYTHON:self.generatePython, CODEGEN_LANG_JAVA:self.generateJava}

  def OnSelect(self, panel, event):
    if event.GetId() == CODEGEN_BUTTON:
      mnuGenerateCode = wx.Menu()
      mnuItemC = mnuGenerateCode.Append(CODEGEN_LANG_C, "C")
      panel.Bind(wx.EVT_MENU, panel.OnToolClick, mnuItemC)
  
      mnuItemCpp = mnuGenerateCode.Append(CODEGEN_LANG_CPP, "CPP")
      panel.Bind(wx.EVT_MENU, panel.OnToolClick, mnuItemCpp)
  
      mnuItemPython = mnuGenerateCode.Append(CODEGEN_LANG_PYTHON, "PYTHON")
      panel.Bind(wx.EVT_MENU, panel.OnToolClick, mnuItemPython)
  
      mnuItemJava = mnuGenerateCode.Append(CODEGEN_LANG_JAVA, "JAVA")
      panel.Bind(wx.EVT_MENU, panel.OnToolClick, mnuItemJava)
  
      panel.PopupMenu(mnuGenerateCode)
    else:
      output = FilesystemOutput()
      comps = filter(lambda inst: inst.entity.et.name == 'Component', panel.model.instances.values())

      #TODO: Get relative directiry with model.xml
      srcDir = "/".join([myDir, "langTest"])

      # Generate makefile for the "app" directory
      mkSubdirTmpl = templateMgr.loadPyTemplate(TemplatePath + "Makefile.subdir.ts")
      s = mkSubdirTmpl.safe_substitute(subdirs=" ".join([c.data["name"] for c in comps]))
      output.write(srcDir + os.sep + "app" + os.sep + "Makefile", s)

      for c in comps:
        self.dictGenFunc[event.GetId()](output, srcDir, c, c.data)
    return False


  def generateC(self, output, srcDir, comp,ts_comp):
    compName = str(comp.data["name"])
  
    # Create main
    cpptmpl = templateMgr.loadPyTemplate(TemplatePath + "main.c.ts")
    s = cpptmpl.safe_substitute(addmain=False)
    output.write(srcDir + os.sep + "app" + os.sep + compName + os.sep + "main.c", s)
    # Create cfg  -- not needed anymore
    #tmpl = templateMgr.loadPyTemplate(TemplatePath + "cfg.c.ts")
    #s = tmpl.safe_substitute(**ts_comp)
    #output.write(srcDir + os.sep + "app" + os.sep + compName + os.sep + "cfg.c", s)
  
    # Create Makefile
    tmpl = templateMgr.loadPyTemplate(TemplatePath + "Makefile.c.ts")
    s = tmpl.safe_substitute(**ts_comp)
    output.write(srcDir + os.sep + "app" + os.sep + compName + os.sep + "Makefile", s)
  
  def generateCpp(self, output, srcDir, comp,ts_comp):
    # Create main
    compName = str(comp.data["name"])
    cpptmpl = templateMgr.loadPyTemplate(TemplatePath + "main.cpp.ts")
    s = cpptmpl.safe_substitute(addmain=False)
    output.write(srcDir + os.sep + "app" + os.sep + compName + os.sep + "main.cpp", s)
  
    # Create cfg -- no longer needed
    #tmpl = templateMgr.loadPyTemplate(TemplatePath + "cfg.cpp.ts")
    #s = tmpl.safe_substitute(**ts_comp)
    #output.write(srcDir + os.sep + "app" + os.sep + compName + os.sep + "cfg.cpp", s)
  
    # Create Makefile
    tmpl = templateMgr.loadPyTemplate(TemplatePath + "Makefile.cpp.ts")
    s = tmpl.safe_substitute(**ts_comp)
    output.write(srcDir + os.sep + "app" + os.sep + compName + os.sep + "Makefile", s)


  def generatePython(self, output, srcDir, comp,ts_comp):
    compName = str(comp.data["name"])
  
    # Create src files
    for (inp,outp) in [("main.py.ts",str(comp.data[instantiateCommand]) + ".py"),("pythonexec.sh",str(comp.data[instantiateCommand]))]:
      tmpl = templateMgr.loadPyTemplate(TemplatePath + inp)
      s = tmpl.safe_substitute(**ts_comp)
      output.write(srcDir + os.sep + "app" + os.sep + compName + os.sep + outp, s)
    
  
    # Create Makefile
    tmpl = templateMgr.loadPyTemplate(TemplatePath + "Makefile.python.ts")
    s = tmpl.safe_substitute(**ts_comp)
    output.write(srcDir + os.sep + "app" + os.sep + compName + os.sep + "Makefile", s)  


  def generateJava(self, output, srcDir, comp,ts_comp):
    compName = str(comp.data["name"])
    # Create Makefile
    tmpl = templateMgr.loadPyTemplate(TemplatePath + "Makefile.java.ts")
    s = tmpl.safe_substitute(**ts_comp)
    output.write(srcDir + os.sep + "app" + os.sep + compName + os.sep + "Makefile", s)  

# Global of this panel for debug purposes only.  DO NOT USE IN CODE
dbgPanel = None

class GridObject:
  def __init__(self):
    self.bound = None
    self.entities = set()
    self.row = None
    self.col = None

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

  def reposition(self,instance):
    """Move an instance somewhere else -- its physical movement may change its parent relationships"""
    pos = instance.pos
    for row in self.grid:
      for cell in row:
        # print cell.bound
        if inBox(pos,cell.bound):
          # Remove the containment arrows (if they exist)
          for i in instance.childOf:  
            i.deleteContainmentArrowTo(instance)
          # Rewrite the back pointers
          instance.childOf = set([cell.row,cell.col])
          # Create the new containment arrows
          for i in instance.childOf:  
            i.createContainmentArrowTo(instance)

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
    def __init__(self, parent,menubar,toolbar,statusbar,model):
      global dbgPanel
      dbgPanel = self
      scrolled.ScrolledPanel.__init__(self, parent, style = wx.TAB_TRAVERSAL|wx.SUNKEN_BORDER)
      share.instancePanel = self
      # self.displayGraph = networkx.Graph()
      # These variables define what types can be instantiated in the outer row/columns of the edit tool
      self.rowTypes = ["ServiceGroup","Application"]
      self.columnTypes = ["Node"]
      for (name,m) in model.entities.items():
        if m.et.name == "ServiceGroup":
          m.customInstantiator = lambda entity,pos,size,children,name,pnl=self: pnl.sgInstantiator(entity, pos,size,children,name)

      self.SetupScrolling(True, True)
      self.SetScrollRate(10, 10)
      self.Bind(wx.EVT_SIZE, self.OnReSize)

      self.Bind(wx.EVT_PAINT, self.OnPaint)

      self.menuBar = menubar
      self.toolBar = toolbar
      self.statusBar = statusbar
      self.model=model
      self.tool = None  # The current tool
      self.drawers = set()
      self.renderArrow = {}  # This dictionary indicates whether an arrow needs to be rendered or whether the parent child relationship is expressed in a different fashion (containment)

      # The position of the panel's viewport within the larger drawing
      self.location = (0,0)
      self.rotate = 0.0
      self.scale = 1.0

      # Buttons and other IDs that are registered may need to be looked up to turn the ID back into a python object
      self.idLookup={}  

      # Ordering of instances in the GUI display, from the upper left
      self.columns = []
      self.rows = []

      self.intersects = []

      # Add toolbar buttons

      # first get the toolbar    
      if not self.toolBar:
        self.toolBar = wx.ToolBar(self,-1)
        self.toolBar.SetToolBitmapSize((24,24))

      tsize = self.toolBar.GetToolBitmapSize()

      # example of adding a standard button
      #new_bmp =  wx.ArtProvider.GetBitmap(wx.ART_NEW, wx.ART_TOOLBAR, tsize)
      #self.toolBar.AddLabelTool(10, "New", new_bmp, shortHelp="New", longHelp="Long help for 'New'")

      bitmap = svg.SvgFile("save_as.svg").bmp(tsize, { }, (222,222,222,wx.ALPHA_OPAQUE))
      self.toolBar.AddTool(SAVE_BUTTON, bitmap, wx.NullBitmap, shortHelpString="save", longHelpString="Save model as...")
      self.idLookup[SAVE_BUTTON] = SaveTool(self)

      bitmap = svg.SvgFile("generate.svg").bmp(tsize, { }, (222,222,222,wx.ALPHA_OPAQUE))
      self.toolBar.AddTool(CODEGEN_BUTTON, bitmap, wx.NullBitmap, shortHelpString="Generate Source Code", longHelpString="Generate source code ...")
      idMnuGenerateCode = [CODEGEN_BUTTON, CODEGEN_LANG_C, CODEGEN_LANG_CPP, CODEGEN_LANG_PYTHON, CODEGEN_LANG_JAVA]
      for idx in idMnuGenerateCode:
        self.idLookup[idx] = GenerateTool(self)

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

    def sgInstantiator(self,ent,pos,size,children,name):
      """Custom instantiator for service groups"""
      (top, all_created) = self.model.recursiveInstantiation(ent)
      return top
#      newdata = copy.deepcopy(ent.data)
#      ret = Instance(ent, newdata,pos,size,name=name)
#      #Actually merge data from entity
#      ret.updateDataFields(newdata)
#      return ret


    def OnReSize(self, event):
      self.layout()
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
          self.toolBar.AddRadioLabelTool(buttonIdx, e[0], bitmap, shortHelp=shortHelp, longHelp=longHelp)
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
        inst = entity.createInstance(position, size, name=name)
        inst.instanceLocked = copy.deepcopy(entity.instanceLocked)

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
        elst = filter(lambda (name, entInt): entInt.et.name in (self.columnTypes + self.rowTypes), self.model.instances.items())
        first = elst[0]
        for (name,e) in elst:
          if e == first:
            virtRct = wx.Rect(e.pos[0]*self.scale, e.pos[1]*self.scale, e.size[0]*e.scale[0]*self.scale, e.size[1]*e.scale[1]*self.scale)
          else:
            virtRct.Union(wx.Rect(e.pos[0]*self.scale, e.pos[1]*self.scale, e.size[0]*e.scale[0]*self.scale, e.size[1]*e.scale[1]*self.scale))
      return virtRct
 
    def UpdateVirtualSize(self, dc):
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
        rowSet = set(self.rows)
        columnSet = set(self.columns)
        self.renderArrow = {}  # We will recalculate which arrows need rendering
        colCount = len(self.columns)
        panelSize = self.GetVirtualSize()
        x = COL_MARGIN
        self.renderOrder = []
        for i in self.columns:
          width = max(COL_WIDTH,i.size[0])  # Pick the minimum width or the user's adjustment, whichever is bigger
          i.pos = (x,0)
          size = (width,panelSize.y)
          if size != i.size:
            i.size = size
            i.recreateBitmap()
          x+=width+COL_SPACING
          self.renderOrder.append(i)

        y = ROW_MARGIN
        
        for i in self.rows:
          width = max(ROW_WIDTH,i.size[1])  # Pick the minimum width or the user's adjustment, whichever is bigger
          i.pos = (0,y)
          size = (panelSize.x,width)
          if size != i.size:
            i.size = size
            i.recreateBitmap()
          y+=width+ROW_SPACING
          self.renderOrder.append(i)
        
        rows = [Margin((0,0),(panelSize[0],ROW_MARGIN))]
        rows += self.rows
 
        cols = [Margin((0,0),(COL_MARGIN,panelSize[1]))]
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
          svg.blit(ctx,e.bmp,e.pos,e.scale,e.rotate)

        # Draw the other instances on top
        for e in filter(lambda entInt: not entInt.et.name in (self.columnTypes + self.rowTypes), self.model.instances.values()):
          if e.size != (0,0):  # indicate that the object is hidden
            svg.blit(ctx,e.bmp,e.pos,e.scale,e.rotate)

        # Now draw the containment arrows on top
#        pdb.set_trace()
        if 0:
         for (name,e) in self.model.instances.items():
          for a in e.containmentArrows:
            st = a.container.pos
            end = a.contained.pos
            if self.renderArrow.get((a.container,a.contained),True):  # Check to see if there's an directive whether to render this arrow or not.  If there is not one, render it by default
              # pdb.set_trace()
              drawCurvyArrow(ctx, (st[0] + a.beginOffset[0],st[1] + a.beginOffset[1]),(end[0] + a.endOffset[0],end[1] + a.endOffset[1]),a.midpoints, linkNormalLook)

        ctx.restore()

        # These are non-model based transient elements that need to be drawn like selection boxes
        for e in self.drawers:
          e.render(ctx)

        for idx in self.idLookup:
          self.idLookup[idx].render(ctx)

        for rect in self.intersects:
          drawIntersectRect(ctx, rect)

    def findEntitiesAt(self,pos):
      """Returns the entity located at the passed position """
      # TODO handle the viewscope's translation, rotation, scaling
      # Real pos
      pos = convertToRealPos(pos, self.scale)
      ret = set()
      for (name, e) in self.model.instances.items():
        furthest= (e.pos[0] + e.size[0]*e.scale[0],e.pos[1] + e.size[1]*e.scale[1])
        #print e.data["name"], ": ", pos, " inside: ", e.pos, " to ", furthest
        if inBox(pos,(e.pos[0],e.pos[1],furthest[0],furthest[1])): # mouse is in the box formed by the entity
          ret.add(e)
      return ret

    def findObjectsAt(self,pos):
      return self.findInstancesAt(pos).union(self.findEntitiesAt(pos))

    def findEntitiesTouching(self,rect):
      """Returns all entities touching the passed rectangle """
      # TODO handle the viewscope's translation, rotation, scaling
      # Real rectangle
      rect = convertToRealPos(rect, self.scale)
      ret = set()
      # hoang for (name, e) in self.model.instances.items():
      for (name, e) in self.entities.items():
        furthest= (e.pos[0] + e.size[0]*e.scale[0],e.pos[1] + e.size[1]*e.scale[1])
        if rectOverlaps(rect,(e.pos[0],e.pos[1],furthest[0],furthest[1])):  # mouse is in the box formed by the entity
          ret.add(e)
      return ret

    def notifyNameValueChange(self, ent, newValue):
      self.notifyValueChange(ent, 'name', newValue)

    def notifyValueChange(self, ent, key, newValue):
      for (name, e) in self.model.instances.items():
        if e == ent:
          e.data[key] = newValue
          e.recreateBitmap()
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

    def deleteEntities(self, ents):
      #remove columns/rows
      for ent in ents:
        try:
          self.columns.remove(ent);
        except:
          pass
        
        try:
          self.rows.remove(ent);
        except:
          pass
  
      self.model.delete(ents)
      self.Refresh()
      self.layout()


model = None

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

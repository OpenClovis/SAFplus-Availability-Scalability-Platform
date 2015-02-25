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

import module
import share
from model import *
from entity import *
import wx.lib.scrolledpanel as scrolled

try:
    from agw import hypertreelist as HTL
except ImportError: # if it's not there locally, try the wxPython lib.
    import wx.lib.agw.hypertreelist as HTL

LOCK_BUTTON_ID = 3482
HELP_BUTTON_ID = 4523
TEXT_ENTRY_ID = 7598

StandaloneDev = False

# Min and Max values taken from rfc6020 Section 9.2
MinValFor = { 'int8':-128,'uint8':0  , 'int16': -32768, 'uint16':0,   'int32': -2147483648,'uint32':0,'int64':-9223372036854775808,'uint64':0}
MaxValFor = { 'int8': 127,'uint8':255, 'int16': 32767, 'uint16':65535,'int32': 2147483647, 'uint32':4294967295,'int64':9223372036854775807,'uint64':18446744073709551615} 

YangIntegerTypes = ['int8','uint8','int16', 'uint16','int32','uint32','int64','uint64']

class Panel(scrolled.ScrolledPanel):
    def __init__(self, parent,menubar,toolbar,statusbar,model):
        global thePanel
        scrolled.ScrolledPanel.__init__(self, parent, style = wx.TAB_TRAVERSAL|wx.SUNKEN_BORDER)
        self.SetupScrolling(True, True)
        self.SetScrollRate(10, 10)

        self.model = model
        self.lockedSvg = svg.SvgFile("locked.svg") 
        self.unlockedSvg = svg.SvgFile("unlocked.svg") 
        self.helpSvg = svg.SvgFile("help.svg") 
        self.lockedBmp = self.lockedSvg.bmp((24,24))
        self.unlockedBmp = self.unlockedSvg.bmp((24,24))
        self.helpBmp = self.helpSvg.bmp((12,12))
        self.entity=None
        self.sizer =None
        self.lookup = {}  # find the object from its windowing ID
        self.numElements = 0
        self.row = 0
        self.font = wx.Font(10, wx.MODERN, wx.NORMAL, wx.BOLD)
        # Event handlers
        #self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_BUTTON, self.OnButtonClick)
        self.tree = None
  
        # for the data entry
        # self.Bind(wx.EVT_TEXT, self.EvtText)

        share.detailsPanel = self
        if StandaloneDev:
          e = model.entities["app"]
          self.showEntity(e)
        self.SetSashPosition(10)


    def partialDataValidate(self, proposedPartialValue, fieldData):
      """Return True if the passed data could be part of a valid entry for this field -- that is, user might enter more text"""
      # TODO: fieldData contains information about the expected type of this field.
      return True

    def dataValidate(self, proposedValue, fieldData):
      """Return True if the passed data is a valid entry for this field"""
      # TODO: fieldData contains information about the expected type of this field.
      return True

    def EvtText(self,event):
      id = event.GetId()
      if id>=TEXT_ENTRY_ID and id < TEXT_ENTRY_ID+self.numElements:
        idx = id - TEXT_ENTRY_ID
        obj = self.lookup[idx]
        query = obj[1]
        if not isinstance(query, wx._core._wxPyDeadObject):
          proposedValue = query.GetValue()
          # print "evt text ", obj
          if not self.partialDataValidate(proposedValue, obj[0]):          
            # TODO: Print a big red warning in the error area
            pass
  
          # TODO: handle only dirty (actually value changed) entity
          self.ChangedValue(proposedValue, obj[0][0])
      else:
        # Notify name change to umlEditor to validate and render
        self.ChangedValue(event.GetEventObject().GetValue())

    def ChangedValue(self, proposedValue, obj = None):
      if obj == None: obj = 'name'

      if isinstance(self.entity,entity.Instance):
        if share.instancePanel:
          share.instancePanel.notifyValueChange(self.entity, obj, proposedValue)
      else:
        if share.umlEditorPanel:
          share.umlEditorPanel.notifyValueChange(self.entity, obj, proposedValue)


    def OnUnfocus(self,event):
      id = event.GetId()
      if id>=TEXT_ENTRY_ID and id < TEXT_ENTRY_ID+self.numElements:
        idx = id - TEXT_ENTRY_ID
        obj = self.lookup[idx]
        name = obj[0][0]
        query = obj[1]

        if not isinstance(query, wx._core._wxPyDeadObject):
          proposedValue = query.GetValue()
          if not self.dataValidate(proposedValue, obj[0]):
            # TODO: Print a big red warning in the error area
            pass
          # TODO: model consistency check -- test the validity of the whole model given this change
          else:
            # TODO: handle only dirty (actually value changed) entity
            self.ChangedValue(proposedValue, obj[0][0])

      else:
        # Notify name change to umlEditor to validate and render
        self.ChangedValue(event.GetEventObject().GetValue())

    def OnButtonClick(self,event):
      id = event.GetId()
      if id>=LOCK_BUTTON_ID and id < HELP_BUTTON_ID:
        idx = id - LOCK_BUTTON_ID
      elif id>=HELP_BUTTON_ID and id < TEXT_ENTRY_ID:
        idx = id - HELP_BUTTON_ID
        return
      else:
        print "unknown button! %d" % id
        return
        pdb.set_trace()
      obj = self.lookup[idx]
      lockButton = obj[2]
      # Toggle the locked state, create it unlocked (then instantly lock it) if it is not already set

      #TODO: handle for children controls not itself if it has child. If it being locked/unlocked, mean all children affected

      self.entity.instanceLocked[obj[0][0]] = not self.entity.instanceLocked.get(obj[0][0],False)
      # set the button appropriately
      if self.entity.instanceLocked[obj[0][0]]:
        lockButton.SetBitmap(self.lockedBmp);
        lockButton.SetBitmapSelected(self.unlockedBmp)
      else:
        lockButton.SetBitmap(self.unlockedBmp);
        lockButton.SetBitmapSelected(self.lockedBmp)

    def createControl(self,id,item, value):
      """Create the best GUI control for this type of data"""
      # pdb.set_trace()
      typeData = item[1]
      if type(typeData) is DictType and typeData.has_key("type"):
        if typeData["type"] == "boolean":
          query = wx.CheckBox(self,id,"")
        elif typeData["type"] in YangIntegerTypes:
          v = 0
          try:
            v = int(value)
          except:
            default = typeData["default"]
            if default:
              v = int(default)
          rang = typeData["range"]
          if not rang:
            rang=[0,100]
          else:
            rang=[rang[0][0], rang[-1][-1]]  # get the first and last element of this list of lists.  We are ignoring any gaps (i.e. [[1,3],[5,10]] because the slider can't handle them anyway. It might be better to just fall back to a text entry box if range is funky.
            if rang[0] == "min": rang[0] = MinValFor[typeData["type"]]
            if rang[1] == "max": rang[1] = MaxValFor[typeData["type"]]
          query = wx.Slider(self,id,v,rang[0],rang[1],size=(200,60), style = wx.SL_HORIZONTAL | wx.SL_AUTOTICKS | wx.SL_LABELS)
          query.SetTickFreq(5)
          # TODO create a control that contains both a slider and a small text box
          # TODO size the slider properly using min an max hints 
        elif self.model.dataTypes.has_key(typeData["type"]):
          typ = self.model.dataTypes[typeData["type"]]
          if typ["type"] == "enumeration":
            vals = typ["values"].items()
            vals = sorted(vals, module.DataTypeSortOrder)
            choices = [x[0] for x in vals]
            if not value in choices:  # OOPS!  Either initial case or the datatype was changed
              value = choices[0]  # so set the value to the first one TODO: set to default one
            query = wx.ComboBox(self,id,value=value,choices=[x[0] for x in vals])
          else:
            # TODO other datatypes 
            query  = wx.TextCtrl(self, id, value,style = wx.BORDER_SIMPLE)
            query.Bind(wx.EVT_KILL_FOCUS, self.OnUnfocus)
        else:  # Default control is a text box
          query  = wx.TextCtrl(self, id, str(value),style = wx.BORDER_SIMPLE)
          # Works: query.SetToolTipString("test")
          query.Bind(wx.EVT_KILL_FOCUS, self.OnUnfocus)
        
        # Bind to handle event on change
        self.BuildChangeEvent(query)
        
      else:
        # TODO do any of these need to be displayed?
        query = None
      return query


    def showEntity(self,ent):
      if self.entity == ent: return  # Its already being shown

      if self.sizer:
        if self.tree:
          self.sizer.Detach(self.tree)
          self.tree.Destroy()
          del self.tree
      else:
        self.sizer = wx.BoxSizer(wx.VERTICAL)

      self.tree = HTL.HyperTreeList(self, id=wx.ID_ANY, pos=wx.DefaultPosition, size=wx.DefaultSize, agwStyle=HTL.TR_NO_HEADER | wx.TR_HAS_BUTTONS | wx.TR_HAS_VARIABLE_ROW_HEIGHT)
      self.tree.AddColumn("Main column")
      self.tree.AddColumn("Col 1")
      self.tree.AddColumn("Col 2")
      self.tree.AddColumn("Col 3")
      self.tree.SetMainColumn(0) # the one with the tree in it...
      self.tree.SetColumnWidth(0, 185)
      self.tree.SetColumnWidth(1, 35)
      self.tree.SetColumnWidth(2, 225)
      self.tree.SetColumnWidth(3, 50)

      self.sizer.Add(self.tree, -1, wx.EXPAND)

      self.entity = ent
      items = ent.et.data.items()
      items = sorted(items,EntityTypeSortOrder)

      self.treeRoot = self.tree.AddRoot(ent.data["name"])

      # Put the name at the top
      query  = wx.TextCtrl(self, -1, "")
      query.ChangeValue(ent.data["name"])
      query.Bind(wx.EVT_KILL_FOCUS, self.OnUnfocus)
      
      # Binding name wx control
      self.BuildChangeEvent(query)
      
      b = wx.BitmapButton(self, -1, self.unlockedBmp,style = wx.NO_BORDER )
      b.SetToolTipString("If unlocked, instances can change this field")

      child = self.tree.AppendItem(self.treeRoot, "Name:")
      self.tree.SetItemWindow(child, query, 2)
      self.tree.SetItemWindow(child, b, 3)
      b.Disable()
      self.row += 1

      # Create all controls of entity
      self.createChildControls(self.treeRoot, items, ent.data)

      self.numElements =  self.row
      self.tree.ExpandAll()

      self.SetSizer(self.sizer)
      self.sizer.Layout()
      self.Refresh()
      self.SetSashPosition(self.GetParent().GetClientSize().x/4)

    def createChildControls(self, treeItem, items, values):
      for item in filter(lambda item: item[0] != "name", items):
        name = item[0]
        if item[0] == "contains":
          pass
        elif type(item[1]) is DictType:
          # figure out the prompt; its either the name of the item or the prompt override
          s = item[1].get("prompt",name)
          if not s: s = name

          if self.entity.isContainer(item[1]):
            child = self.tree.AppendItem(treeItem, s)
            self.createChildControls(child, item[1].items(), values.get(item[0], None))
          else:
            #pdb.set_trace()
            query = self.createControl(self.row + TEXT_ENTRY_ID, item, values.get(item[0], None))
            if not query: continue  # If this piece of data has no control, it is not user editable

            #Create tree item
            child = self.tree.AppendItem(treeItem, s)

            #Add control into tree item
            self.tree.SetItemWindow(child, query, 2)

            # Now create the lock/unlock button
            b = wx.BitmapButton(self, self.row + LOCK_BUTTON_ID, self.unlockedBmp,style = wx.NO_BORDER )
            b.SetToolTipString("If unlocked, instances can change this field")
            
            #Add control into tree item
            self.tree.SetItemWindow(child, b, 3)

            if self.entity.instanceLocked.get(name, False):  # Set its initial state
              b.SetBitmap(self.lockedBmp);
              b.SetBitmapSelected(self.unlockedBmp)

              # Not allow to change this value from instance
              query.Enable(isinstance(self.entity,entity.Instance) == False)
            else:
              b.SetBitmap(self.unlockedBmp);
              b.SetBitmapSelected(self.lockedBmp)

            b.SetBitmapSelected(self.lockedBmp)

            # Devide from entity type, not allow to 'Lock' this from instance
            b.Enable(isinstance(self.entity,entity.Instance) == False)

            # Next add the extended help button
            h = None
            if item[1].has_key("help") and item[1]["help"]:
              h = wx.BitmapButton(self, self.row + HELP_BUTTON_ID, self.helpBmp,style = wx.NO_BORDER )
              h.SetToolTipString(item[1]["help"])
            #else: 
            #  h = wx.StaticText(self,-1,"")  # Need a placeholder or gridbag sizer screws up

            #Add control into tree item
            if h:
              self.tree.SetItemWindow(child, h, 1)
            
            self.lookup[self.row] = [item,query,b,h]
            self.row+=1

    def OnPaint(self, evt):
        if self.IsDoubleBuffered():
            dc = wx.PaintDC(self)
        else:
            dc = wx.BufferedPaintDC(self)
        dc.SetBackground(wx.Brush('blue'))
        dc.Clear()
        
        #self.Render(dc)

    def SetSashPosition(self, width = -1):
      #Workaround to show scrollbar
      if isinstance(self.GetParent(), wx.SplitterWindow):
        self.GetParent().SetSashPosition(-1)
        self.GetParent().SetSashPosition(width)
        self.GetParent().UpdateSize()

    def BuildChangeEvent(self, ctrl):
        mapEvents = {
            'text': [wx.EVT_TEXT, wx.EVT_TEXT_ENTER],
            'check': [wx.EVT_CHECKBOX],
            'slider': [wx.EVT_SCROLL, wx.EVT_SLIDER, wx.EVT_SCROLL_CHANGED, ],
            'comboBox': [wx.EVT_COMBOBOX, wx.EVT_TEXT, wx.EVT_TEXT_ENTER, wx.EVT_COMBOBOX_DROPDOWN, wx.EVT_COMBOBOX_CLOSEUP],
            # TBD 
        }

        for t in mapEvents[ctrl.GetName()]:
          ctrl.Bind(t, self.EvtText)

def Test():
  global StandaloneDev
  import time
  import pyGuiWrapper as gui

  mdl = Model()
  mdl.load("testModel.xml")

  #sgt = mdl.entityTypes["ServiceGroup"]
  #sg = mdl.entities["MyServiceGroup"] = Entity(sgt,(0,0),(100,20))
  StandaloneDev = True
  gui.go(lambda parent,menu,tool,status,m=mdl: Panel(parent,menu,tool,status, m))
  #time.sleep(2)
  #thePanel.showEntity(sg)


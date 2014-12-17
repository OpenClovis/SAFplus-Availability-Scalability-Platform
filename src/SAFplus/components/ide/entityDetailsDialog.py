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

LOCK_BUTTON_ID = 3482
HELP_BUTTON_ID = 4523
TEXT_ENTRY_ID = 7598

class Panel(wx.Panel):
    def __init__(self, parent,menubar,toolbar,statusbar,model):
        global thePanel
        wx.Panel.__init__(self, parent, -1, style=wx.SUNKEN_BORDER)
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
        # Event handlers
        #self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_BUTTON, self.OnButtonClick)
  
        # for the data entry
        self.Bind(wx.EVT_TEXT, self.EvtText)

        share.detailsPanel = self
        #e = model.entities["MyServiceGroup"]
        #self.showEntity(e)


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
        query = obj[2]
        proposedValue = query.GetValue()
        # print "evt text ", obj
        if not self.partialDataValidate(proposedValue, obj[0]):          
          # TODO: Print a big red warning in the error area
          pass

    def OnUnfocus(self,event):
      id = event.GetId()
      if id>=TEXT_ENTRY_ID and id < TEXT_ENTRY_ID+self.numElements:
        idx = id - TEXT_ENTRY_ID
        obj = self.lookup[idx]
        name = obj[0][0]
        query = obj[2]
        proposedValue = query.GetValue()
        if self.dataValidate(proposedValue, obj[0]):
          self.entity.data["name"] = proposedValue
        else:
          # TODO: Print a big red warning in the error area
          pass
        # TODO: model consistency check -- test the validity of the whole model given this change

    def OnButtonClick(self,event):
      id = event.GetId()
      if id>=LOCK_BUTTON_ID and id < HELP_BUTTON_ID:
        idx = id - LOCK_BUTTON_ID
      obj = self.lookup[idx]
      lockButton = obj[3]
      # Toggle the locked state, create it unlocked (then instantly lock it) if it is not already set
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
      if typeData.has_key("type"):
        if typeData["type"] == "boolean":
          query = wx.CheckBox(self,id,"")
        elif typeData["type"] == "uint32":
          v = 0
          try:
            v = int(value)
          except:
            pass
          query = wx.Slider(self,id,v,0,100,style = wx.SL_HORIZONTAL | wx.SL_AUTOTICKS | wx.SL_LABELS)
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
          query  = wx.TextCtrl(self, id, value,style = wx.BORDER_SIMPLE)
          # Works: query.SetToolTipString("test")
          query.Bind(wx.EVT_KILL_FOCUS, self.OnUnfocus)
      else:
        # TODO do any of these need to be displayed?
        query = None
      return query


    def showEntity(self,ent):
      if self.entity == ent: return  # Its already being shown
      if self.sizer:
        self.sizer.Clear(True)
        created = False
      else:
        self.sizer = wx.GridBagSizer(2,1)
        self.sizer.SetFlexibleDirection(wx.HORIZONTAL | wx.VERTICAL)
        created = True

      sizer = self.sizer   

      self.entity = ent
      items = ent.et.data.items()
      items = sorted(items,EntityTypeSortOrder)
      font = wx.Font(10, wx.MODERN, wx.NORMAL, wx.BOLD)

      # Put the name at the top
      row=0
      prompt = wx.StaticText(self,-1,"Name:",style =wx.ALIGN_RIGHT)
      query  = wx.TextCtrl(self, -1, "")
      query.ChangeValue(ent.data["name"])
      query.Bind(wx.EVT_KILL_FOCUS, self.OnUnfocus)
      prompt.SetFont(font)
      sizer.Add(prompt,(row,0),(1,1),wx.ALIGN_CENTER)
      sizer.Add(query,(row,2),flag=wx.EXPAND)
      bmp = wx.StaticBitmap(self, -1, self.unlockedBmp)
      sizer.Add(bmp,(row,3),(1,1),wx.ALIGN_CENTER | wx.ALL)
      row += 1
      for item in items:
        name = item[0]
        if type(item[1]) is DictType:

          # Create the data entry field
          query = self.createControl(row + TEXT_ENTRY_ID,item,ent.data[name])
          if not query: continue  # If this piece of data has not control, it is not user editable

          # figure out the prompt; its either the name of the item or the prompt override
          s = item[1].get("prompt",name)
          if not s: s = name
          prompt = wx.StaticText(self,-1,s,style=wx.ALIGN_RIGHT)
          prompt.SetFont(font)  # BUG: Note, setting the font can mess up the layout; resize the window to recalc
          prompt.Fit()

          # Now create the lock/unlock button
          b = wx.BitmapButton(self, row + LOCK_BUTTON_ID, self.unlockedBmp,style = wx.NO_BORDER )
          b.SetToolTipString("If unlocked, instances can change this field")
          if self.entity.instanceLocked.get(name, False):  # Set its initial state
            b.SetBitmap(self.lockedBmp);
            b.SetBitmapSelected(self.unlockedBmp)
          else:
            b.SetBitmap(self.unlockedBmp);
            b.SetBitmapSelected(self.lockedBmp)

          b.SetBitmapSelected(self.lockedBmp)

          # Next add the extended help button
          h = None
          if item[1].has_key("help") and item[1]["help"]:
            h = wx.BitmapButton(self, row + HELP_BUTTON_ID, self.helpBmp,style = wx.NO_BORDER )
            h.SetToolTipString(item[1]["help"])
          #else: 
          #  h = wx.StaticText(self,-1,"")  # Need a placeholder or gridbag sizer screws up

          # OK fill up the sizer
          sizer.Add(prompt,(row,0),(1,1),wx.ALIGN_CENTER)
          if h: sizer.Add(h,(row,1),(1,1),wx.ALIGN_CENTER)
          sizer.Add(query,(row,2),flag=wx.EXPAND)
          sizer.Add(b,(row,3))

          # Update the lookup dictionary so when an action occurs we can get the objects from the window ID
          self.lookup[row] = [item,prompt,query,b,h]
          row +=1
      self.numElements = row
      if created: sizer.AddGrowableCol(2)
      self.SetSizer(sizer)
      sizer.Layout()
      self.Refresh()

    def OnPaint(self, evt):
        if self.IsDoubleBuffered():
            dc = wx.PaintDC(self)
        else:
            dc = wx.BufferedPaintDC(self)
        dc.SetBackground(wx.Brush('blue'))
        dc.Clear()
        
        #self.Render(dc)


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

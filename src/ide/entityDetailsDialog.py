import pdb
from collections import namedtuple

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


if int(wx.__version__[0]) >= 3:
  try:
    from agw import hypertreelist as HTL
    from agw import infobar as IB
  except ImportError: # if it's not there locally, try the wxPython lib.
    import wx.lib.agw.hypertreelist as HTL
    import wx.lib.agw.infobar as IB
else:
  try:
    from agw import hypertreelist as HTL
    #import infobar as IB
  except ImportError: # if it's not there locally, try the wxPython lib.
    import wx.lib.agw.hypertreelist as HTL
    #import infobar as IB

  class FakeIB:
    def __init__(self):
      pass
    def InfoBar(self,parent):
      return InfoBar(parent)

  class InfoBar(wx.Panel):
    def __init__(self,parent):
      wx.Panel.__init__(self,parent)
      self.parent=parent
    def ShowMessage(self,msg, severity):
      pass
    def Dismiss(self):
      pass

  IB = FakeIB()


Gui2Obj=namedtuple('Gui2Obj','fielddef widget lock help item entity')

LOCK_BUTTON_ID = 3482
HELP_BUTTON_ID = 4523
TEXT_ENTRY_ID = 7598

StandaloneDev = False

# Min and Max values taken from rfc6020 Section 9.2
MinValFor = { 'int8':-128,'uint8':0  , 'int16': -32768, 'uint16':0,   'int32': -2147483648,'uint32':0,'int64':-9223372036854775808,'uint64':0}
MaxValFor = { 'int8': 127,'uint8':255, 'int16': 32767, 'uint16':65535,'int32': 2147483647, 'uint32':4294967295,'int64':9223372036854775807,'uint64':18446744073709551615} 

YangIntegerTypes = ['int8','uint8','int16', 'uint16','int32','uint32','int64','uint64']

# Work-around to initialize wxPython from standalone
app = wx.App(0)

class SliderCustom(wx.PyControl):
  def __init__(self, parent, id, v, rang, style = wx.BORDER_NONE, name = ''):
    wx.PyControl.__init__(self, parent, id, style = style)

    self.sliderText = wx.TextCtrl(self, id, str(v), style = wx.BORDER_SIMPLE, name=name)
    self.slider = wx.Slider(self,id,v,rang[0],rang[1],size=(200,60), style = wx.SL_HORIZONTAL) #  | wx.SL_AUTOTICKS | wx.SL_LABELS)

    self.tickFreq = 5
    self.slider.SetTickFreq(self.tickFreq)

    sizer = wx.BoxSizer(wx.HORIZONTAL)
    sizer.Add(self.sliderText, 0, wx.ALIGN_CENTER | wx.FIXED_MINSIZE | wx.ALL, 0)
    sizer.Add(self.slider, 1, wx.EXPAND | wx.ALL, 0)

    self.slider.Bind(wx.EVT_SLIDER, self.sliderHandler)
    self.sliderText.Bind(wx.EVT_TEXT, self.sliderTextHandler)
    self.sliderText.Bind(wx.EVT_TEXT_ENTER, self.sliderTextHandler)

    self.sizer = sizer
    self.SetSizer(sizer)
    self.Fit()

  def sliderHandler(self, evt):
    value = evt.GetInt()
    self.sliderText.SetValue(str(value))
    evt.Skip()

  def sliderTextHandler(self, evt):
    try:
      value = int(self.sliderText.GetValue())
      self.slider.SetValue(value)
      self.sliderText.GetValidator().clearError()
    except:
      value = 0

    evt.Skip()

  def GetValue(self):
    return self.sliderText.GetValue()

  def GetName(self):
    return self.sliderText.GetName()

  def SetName(self, name):
    return self.sliderText.SetName(name)

  def SetValidator(self, validator):
    self.sliderText.SetValidator(validator)

  def GetValidator(self):
    return self.sliderText.GetValidator()

class GenericObjectValidator(wx.PyValidator):
    """ This validator is used to ensure that the user has entered something
        into the text object editor dialog's text field.
    """
    def __init__(self):
      """ Standard constructor.
      """
      wx.PyValidator.__init__(self)
      self.Bind(wx.EVT_CHAR, self.OnChar)

      # Error msg, this can get from yang leaf
      self.errorStr = ''

      # Mark the field if error
      self.isError  = False
      self.currentValue = None

    def Clone(self):
      """ Standard cloner.
          Note that every validator must implement the Clone() method.
      """
      return GenericObjectValidator()


    def Validate(self):
      """ Validate the contents of the given text control.
      """
      self.isError = False
      return True

    def setError(self, errorStr = ''):
      textCtrl = self.GetWindow()
      textCtrl.SetBackgroundColour("red")
      textCtrl.Refresh()

      self.isError = True
      self.errorStr = errorStr

    def clearError(self):
      textCtrl = self.GetWindow()
      textCtrl.SetBackgroundColour(wx.SystemSettings_GetColour(wx.SYS_COLOUR_WINDOW))
      textCtrl.Refresh()

      self.isError = False
      self.errorStr = ''

    def getModified(self, modify):
      textCtrl = self.GetWindow()
      return (self.currentValue != textCtrl.GetValue())

    def TransferToWindow(self):
      return True

    def TransferFromWindow(self):
      textCtrl = self.GetWindow()
      self.currentValue = textCtrl.GetValue()
      return True

    def OnChar(self, evt):
      evt.Skip()
      
    def GetErrorMsg(self):
      return self.errorStr
    

class NameObjectValidator(GenericObjectValidator):
    def __init__(self, dictItems, currentValue):
      """ Standard constructor.
      """
      GenericObjectValidator.__init__(self)
      self.dictItems = dictItems
      self.length = (1,255)
      self.currentValue = currentValue


    def Clone(self):
        """ Standard cloner.
            Note that every validator must implement the Clone() method.
        """
        return NameObjectValidator(self.dictItems, self.currentValue)

    def Validate(self):
      textCtrl = self.GetWindow()
      value = textCtrl.GetValue()

      if len(value) < self.length[0] or len(value) > self.length[1]:
        self.setError("Length value should be [%d..%d]" %(self.length[0], self.length[1]))

      if (value != self.currentValue) and (value in self.dictItems.keys()):
        self.setError("Name value of entity should be unique")

      return (not self.isError)

    def OnChar(self, evt):
      self.clearError()
      evt.Skip()


class NumberObjectValidator(GenericObjectValidator):
    def __init__(self, rang=None, mask=None):
        """ Standard constructor.
        """
        GenericObjectValidator.__init__(self)
        self.rang=rang
        self.mask=mask

    def Clone(self):
        """ Standard cloner.
            Note that every validator must implement the Clone() method.
        """
        return NumberObjectValidator(self.rang, self.mask)


    def Validate(self):
        """ Validate the contents of the given text control.
        """
        textCtrl = self.GetWindow()
        value = textCtrl.GetValue()
        num = 0
        try:
          num = int(value)
        except:
          self.setError("Invalid number [0-9]")
          return (not self.isError)

        # TODO: check in mask
        if self.rang:
          if num>self.rang[1] or num<self.rang[0]:
            self.setError("Value should be in range [%d..%d]" %(self.rang[0],self.rang[1]))

        return (not self.isError)

    def OnChar(self, evt):
      self.clearError()
      evt.Skip()


class Panel(scrolled.ScrolledPanel):
    def __init__(self, parent,guiPlaces,model, **kw):
        global thePanel
        scrolled.ScrolledPanel.__init__(self, parent, style = wx.TAB_TRAVERSAL|wx.SUNKEN_BORDER)
        
        self.usrDefinedNodeTypeCtrls = {}
        

        #? Set to True if you want to only show the config items, set to false if you want to show config and runtime items
        self.configOnly = True

        #? Set to the list of types that should not appear in the panel.  The instance-identifier is not shown because it is constructed using the UML diagram
        self.noShowTypes = ["instance-identifier"]

        self.SetupScrolling(True, True)
        self.SetScrollRate(10, 10)

        self.model = model
        self.guiPlaces = guiPlaces
        self.lockedSvg = svg.SvgFile("locked.svg") 
        self.unlockedSvg = svg.SvgFile("unlocked.svg") 
        self.helpSvg = svg.SvgFile("help.svg") 
        self.lockedBmp = self.lockedSvg.bmp((24,24))
        self.unlockedBmp = self.unlockedSvg.bmp((24,24))
        self.helpBmp = self.helpSvg.bmp((12,12))
        self.entity=None
        self.sizer = wx.BoxSizer(wx.VERTICAL)
        self.lookup = {}  # find the object from its windowing ID
        self.numElements = 0
        self.row = 0
        self.font = wx.Font(10, wx.MODERN, wx.NORMAL, wx.BOLD)
        # Event handlers
        #self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_BUTTON, self.OnButtonClick)
        self.tree = None
        self.treeItemSelected = None
  
        # for the data entry
        # self.Bind(wx.EVT_TEXT, self.EvtText)
        self.isDetailInstance = kw.get("isDetailInstance",False)
        self.entityTreeTypes = ["ServiceGroup", "Node", "ServiceUnit", "ServiceInstance"]

        # Create the InfoBar to show error message
        self.info = IB.InfoBar(self)
        self.sizer.Add(self.info, 0, wx.EXPAND)

        # Creates hyperlisttree 
        self.eventDictTree = {wx.EVT_TREE_ITEM_EXPANDED:self.OnTreeSelExpanded, wx.EVT_TREE_SEL_CHANGING: self.OnTreeSelChanged, wx.EVT_TREE_SEL_CHANGED: self.OnTreeSelChanged}
        self._createTreeEntities()

        if (self.isDetailInstance):
          share.instanceDetailsPanel = self
        else:
          share.detailsPanel = self

        if StandaloneDev:
          e = model.entities["app"]
          self.showEntity(e)

        # Does not work because this panel does not have a size yet (parent has not laid it out)
        #sz = self.GetParent().GetClientSize()
        #pdb.set_trace()
        #self.SetSashPosition(self.GetParent().GetClientSize().x/4)

    def setModelData(self, model):
      self.model = model

    def partialDataValidate(self, proposedPartialValue, fieldData):
      """Return True if the passed data could be part of a valid entry for this field -- that is, user might enter more text"""
      # TODO: fieldData contains information about the expected type of this field.
      validator = fieldData.GetValidator()
      isValid = True
      if validator:
        isValid = validator.Validate()

      # Print a big red warning in the error area
      if not isValid:
        self.info.ShowMessage(validator.GetErrorMsg(), wx.ICON_ERROR)
        fieldData.SetFocus()
      else:
        # Go through ctrl to validate
        for obj in self.lookup.values():
          query = obj[1]
          validator = query.GetValidator()
          if validator:
            isValid &= validator.Validate()
            msg = ''
            if not isValid:
              if obj[0]:
                # Error in one of field
                msg = "%s - %s: %s" %(self.tree.GetItemText(obj[4].GetParent()), self.tree.GetItemText(obj[4]), validator.GetErrorMsg())
              else:
                # Error in name field
                msg = "%s - Name: %s" %(self.tree.GetItemText(obj[4]), validator.GetErrorMsg())
              self.info.ShowMessage(msg, wx.ICON_ERROR)
              break

      return isValid

    def dataValidate(self, proposedValue, fieldData):
      """Return True if the passed data is a valid entry for this field"""
      # TODO: fieldData contains information about the expected type of this field.
      return self.partialDataValidate(proposedValue, fieldData)

    def EvtText(self,event):
      self.info.Dismiss()
      id = event.GetId()
      if id>=TEXT_ENTRY_ID and id < TEXT_ENTRY_ID+self.numElements:
        idx = id - TEXT_ENTRY_ID
        obj = self.lookup[idx]
        query = obj[1]
        self.treeItemSelected = obj[4]

        if not isinstance(query, wx._core._wxPyDeadObject):
          proposedValue = query.GetValue()
          # print "evt text ", obj
          if not self.partialDataValidate(proposedValue, query):
            # TODO: Disable "SAVE_BUTTON"
            return

          # TODO: Enable "SAVE_BUTTON" if mark dirty and handle only dirty (actually value changed) entity
          self.ChangedValue(proposedValue, event.GetEventObject(), obj[0])
      else:
        # Notify name change to umlEditor to validate and render
        # TODO: Enable "SAVE_BUTTON"
        self.ChangedValue(event.GetEventObject().GetValue(), event.GetEventObject())

    def ChangedValue(self, proposedValue, query, obj = None):
      if not self.treeItemSelected: return

      if self.treeItemSelected:
        self.entity = self.tree.GetPyData(self.treeItemSelected)

      name = query.GetName()
      if obj == None:
        self.tree.SetItemText(self.treeItemSelected, proposedValue)

      if isinstance(self.entity,entity.Instance):
        if share.instancePanel:
          share.instancePanel.notifyValueChange(self.entity, name, query, proposedValue)              
      else:
        if share.umlEditorPanel:          
          share.umlEditorPanel.notifyValueChange(self.entity, name, query, proposedValue)

    def OnUnfocus(self,event):
      # TODO: got wrong entity value change
      id = event.GetId()
      if id in self.usrDefinedNodeTypeCtrls:
        idx = id - TEXT_ENTRY_ID
        obj = self.lookup[idx]
        query = obj[1]
        unfocusTreeItem = obj[4]
        entity = self.tree.GetPyData(unfocusTreeItem)
        if share.instancePanel:
          share.instancePanel.updateMenuUserDefineNodeTypes(entity, self.usrDefinedNodeTypeCtrls[id])
      return
      id = event.GetId()
      if id>=TEXT_ENTRY_ID and id < TEXT_ENTRY_ID+self.numElements:
        idx = id - TEXT_ENTRY_ID
        obj = self.lookup[idx]
        query = obj[1]

        if not isinstance(query, wx._core._wxPyDeadObject):
          proposedValue = query.GetValue()
          if not self.dataValidate(proposedValue, query):
            # TODO: Disable "SAVE_BUTTON"
            return

          # TODO: model consistency check -- test the validity of the whole model given this change
          # TODO: handle only dirty (actually value changed) entity
          # TODO: Enable "SAVE_BUTTON" button if mark dirty
          self.ChangedValue(proposedValue, obj[0])
      else:
        # Notify name change to umlEditor to validate and render
        # TODO: Enable "SAVE_BUTTON" button if mark dirty
        self.ChangedValue(event.GetEventObject().GetValue())

    def OnFocus(self,event):
      id = event.GetId()
      if id>=TEXT_ENTRY_ID and id < TEXT_ENTRY_ID+self.numElements:
        idx = id - TEXT_ENTRY_ID
        obj = self.lookup[idx]
        self.treeItemSelected = obj[4]
        query = obj[1]
        if id in self.usrDefinedNodeTypeCtrls:
          self.usrDefinedNodeTypeCtrls[id] = query.GetValue()

        if self.treeItemSelected:
          self.tree.SelectItem(self.treeItemSelected)
      event.Skip()

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
      entity = obj.entity
      entity.instanceLocked[obj[0][0]] = not entity.instanceLocked.get(obj[0][0],False)
      # set the button appropriately
      if entity.instanceLocked[obj[0][0]]:
        lockButton.SetBitmap(self.lockedBmp);
        lockButton.SetBitmapSelected(self.unlockedBmp)
      else:
        lockButton.SetBitmap(self.unlockedBmp);
        lockButton.SetBitmapSelected(self.lockedBmp)

    def createControl(self,id,item, value, nameCtrl):
      """Create the best GUI control for this type of data"""
      # pdb.set_trace()
      typeData = item[1]
      if type(typeData) is DictType and typeData.has_key("type"):
        if typeData["type"] == "boolean":
          query = wx.CheckBox(self.tree.GetMainWindow(),id,"")
          checked = bool(value)
          query.SetValue(checked)
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
            rang=[0,100000]
          else:
            rang=[rang[0][0], rang[-1][-1]]  # get the first and last element of this list of lists.  We are ignoring any gaps (i.e. [[1,3],[5,10]] because the slider can't handle them anyway. It might be better to just fall back to a text entry box if range is funky.
            if rang[0] == "min": rang[0] = MinValFor[typeData["type"]]
            if rang[1] == "max": rang[1] = MaxValFor[typeData["type"]]

          # TODO: default 'v' > range ???
          if v > rang[1]:
            query  = wx.TextCtrl(self.tree.GetMainWindow(),id,str(v),style = wx.BORDER_SIMPLE)
            query.SetValidator(NumberObjectValidator())
          else:
            query = SliderCustom(self.tree.GetMainWindow(),id, v, rang)
            # TODO: getcustom validator from yang
            query.SetValidator(NumberObjectValidator(rang))

          # TODO create a control that contains both a slider and a small text box
          # TODO size the slider properly using min an max hints 
        elif typeData["type"] == 'enumeration':  # This handles enums inside other lists or containers
          vals = typeData["choice"].choices.items()
          vals = sorted(vals, module.DataTypeSortOrder)
          choices = [x[0] for x in vals]
          if not value in choices:  # OOPS!  Either initial case or the datatype was changed
              value = choices[0]  # so set the value to the first one TODO: set to default one
          query = wx.ComboBox(self.tree.GetMainWindow(),id,value=value,choices=[x[0] for x in vals])

        elif self.model.dataTypes.has_key(typeData["type"]):
          typ = self.model.dataTypes[typeData["type"]]
          if typ["type"] == "enumeration":  # This is a typedef enum
            vals = typ["values"].items()
            vals = sorted(vals, module.DataTypeSortOrder)
            choices = [x[0] for x in vals]
            if not value in choices:  # OOPS!  Either initial case or the datatype was changed
              value = choices[0]  # so set the value to the first one TODO: set to default one
            query = wx.ComboBox(self.tree.GetMainWindow(),id,value=value,choices=[x[0] for x in vals])
          else:
            # TODO other datatypes 
            query  = wx.TextCtrl(self.tree.GetMainWindow(), id, value,style = wx.BORDER_SIMPLE)
            query.Bind(wx.EVT_KILL_FOCUS, self.OnUnfocus)
        else:  # Default control is a text box
          query  = wx.TextCtrl(self.tree.GetMainWindow(), id, str(value),style = wx.BORDER_SIMPLE)
          # Works: query.SetToolTipString("test")
          query.Bind(wx.EVT_KILL_FOCUS, self.OnUnfocus)
        
        # Bind to handle event on change
        self.BuildChangeEvent(query)
        query.SetName(nameCtrl)

      else:
        # TODO do any of these need to be displayed?
        query = None
      if 'userDefinedType' in item:
        self.usrDefinedNodeTypeCtrls[id] = ""
      elif 'disableAssignmentOn' in item:
        query.SetEditable(False)

      return query

    def findEntityRecursive(self, ent, parent = None):
      # get root if on item
      if not parent:
        parent = self.tree.GetRootItem()

      if (self.tree.GetPyData(parent) == ent):
        return parent

      for treeItem in parent.GetChildren():
        item = self.findEntityRecursive(ent, treeItem)
        if item:
          return item

      return None

    def showEntity(self,ent):
      #if self.entity == ent: return  # Its already being shown
      self.entity = ent

      # Collapse all tree items
      self.CollapseAll()
      treeItem = self.createTreeItemEntity(ent.data["name"], ent)
      if not self.tree.IsExpanded(treeItem):
        self.tree.Expand(treeItem)
      self.tree.SelectItem(treeItem)

    def OnTreeSelExpanded(self, event):
      self.treeItemSelected = event.GetItem()
      event.Skip()

    def OnTreeSelChanged(self, event):
      self.treeItemSelected = event.GetItem()
      event.Skip()

    def CollapseAll(self):
      #self.tree.SelectAll()
      #for item in filter(lambda item: item != self.treeRoot, self.tree.GetSelections()):
      #  self.tree.Collapse(item)
      #self.tree.UnselectAll()
      self.tree.Unselect()

    # Create controls for all entities
    def _createTreeEntities(self):
      if self.sizer:
        if self.tree:
          self.sizer.Detach(self.tree)
          self.tree.Destroy()
          del self.tree

      self.tree = HTL.HyperTreeList(self, id=wx.ID_ANY, pos=wx.DefaultPosition, style=wx.BORDER_NONE, size=(5,5), agwStyle=HTL.TR_NO_HEADER | wx.TR_HAS_BUTTONS | wx.TR_HAS_VARIABLE_ROW_HEIGHT | HTL.TR_HIDE_ROOT | wx.TR_SINGLE) # wx.TR_MULTIPLE)
      win = self.tree.GetMainWindow()
      # win.SetBackgroundStyle(None)
      # win._hilightBrush = wx.Brush(wx.Colour(255,255,255,255))

      # workaround: the constructor of customtreectrl makes this very black which looks ugly when you click on a node in the tree:
      bkcol = win.GetBackgroundColour()
      win._hilightUnfocusedBrush = wx.Brush(bkcol)
     # self.tree.ShouldInheritColors(True)
      self.tree.AddColumn("Main column")
      self.tree.AddColumn("Col 1")
      self.tree.AddColumn("Col 2")
      self.tree.AddColumn("Col 3")
      self.tree.SetMainColumn(0) # the one with the tree in it...
      self.tree.SetColumnWidth(0, 185)
      self.tree.SetColumnWidth(1, 35)
      self.tree.SetColumnWidth(2, 50)
      self.tree.SetColumnWidth(3, 225)
      self.row = 0

      for (evt, func) in self.eventDictTree.items():
        self.tree.Bind(evt, func)

      self.sizer.Add(self.tree, -1, wx.ALL | wx.EXPAND)

      self.treeRoot = self.tree.AddRoot("entityDetails")

      # This tree to show details for entity instantiate
      if (self.isDetailInstance):
        for (name, ent) in filter(lambda (name, ent): ent.et.name in (self.entityTreeTypes),self.model.instances.items()):
          self.createTreeItemEntity(name, ent)
      else:
        for (name, ent) in self.model.entities.items():
          self.createTreeItemEntity(name, ent)

      self.SetSizer(self.sizer)
      self.sizer.Layout()
      self.Refresh()

    def deleteEntFromLookup(self, ent):      
      deletedItems = [k for (k,v) in self.lookup.items() if v[5] == ent]
      for k in deletedItems:
        query = self.lookup[k][1]
        if query:
          query.Destroy()
        b = self.lookup[k][2]
        if b:
          b.Destroy()
        h = self.lookup[k][3]
        if h:
          h.Destroy()
        del self.lookup[k]

    def deleteTreeItemEntities(self, ents, tmpList = None):
      if tmpList == None:
        tmpList = ents

      for ent in ents:
          treeItem = self.findEntityRecursive(ent)

          if treeItem:
            if 1: # TODO: recursive deleting???
              if ent.et.name in self.entityTreeTypes:
                for a in ent.containmentArrows:
                    self.deleteTreeItemEntities(set([a.contained]), tmpList)

            # Cascade delete csi/component entity instance
            #if not ent in tmpList:
            #  try:
            #    self.model.delete(ent)
            #  except:
            #    pass
            # delete associated items with this entity from self.lookup dict
            self.deleteEntFromLookup(ent)
            self.tree.Delete(treeItem)

    # Create controls for an entity
    def createTreeItemEntity(self, name, ent, parentItem = None):

      if parentItem is None:
        parentItem = self.treeRoot

      # Check and return if item exists
      treeItem = self.findEntityRecursive(ent)
      if treeItem: return treeItem

      treeItem = self.tree.AppendItem(parentItem, name)
      self.tree.SetPyData(treeItem, ent)

      items = ent.et.data.items()
      items = sorted(items,EntityTypeSortOrder)

      # Put the name at the top
      query  = wx.TextCtrl(self.tree.GetMainWindow(), self.row + TEXT_ENTRY_ID, "")
      query.ChangeValue(ent.data["name"])
      query.Bind(wx.EVT_KILL_FOCUS, self.OnUnfocus)
      
      # Binding name wx control
      self.BuildChangeEvent(query)
      query.SetName("%s_name"%name)

      # TODO: getcustom validator from yang
      query.SetValidator(NameObjectValidator(self.model.instances if self.isDetailInstance else self.model.entities, ent.data["name"]))

      b = wx.BitmapButton(self.tree.GetMainWindow(), self.row + LOCK_BUTTON_ID, self.unlockedBmp,style = wx.NO_BORDER )
      b.SetToolTipString("If unlocked, instances can change this field")

      child = self.tree.AppendItem(treeItem, "Name:")
      self.tree.SetPyData(child, ent)

      self.tree.SetItemWindow(child, query, 3)
      self.tree.SetItemWindow(child, b, 2)
      b.Disable()
      self.lookup[self.row] = Gui2Obj(None,query,b,None,treeItem,ent)
      self.row += 1

      # Create all controls of entity
      self.createChildControls(treeItem, ent, items, ent.data, nameCtrl=name)

      # Put comps/csis into SU/SI
      if self.isDetailInstance and ent.et.name in ("ServiceInstance", "ServiceUnit"):
        for a in ent.containmentArrows:
          self.createTreeItemEntity(a.contained.data["name"], a.contained, treeItem)

      # Update num controls
      self.numElements =  self.row

      return treeItem

    def createChildControls(self, treeItem, ent, items, values, nameCtrl):
      for item in filter(lambda item: item[0] != "name", items):
        name = item[0]
        if item[0] == "contains":
          pass
        elif type(item[1]) is DictType:
          if item[1].get("type","untyped") in self.noShowTypes:
            continue
          if self.configOnly and item[1].get("config",True)==False:
            # Skip runtime items
            continue

          # figure out the prompt; its either the name of the item or the prompt override
          s = item[1].get("prompt",name)
          if not s: s = name

          if ent.isContainer(item[1]):
            child = self.tree.AppendItem(treeItem, s)
            self.createChildControls(child, ent, item[1].items(), values.get(item[0], None), nameCtrl="%s_%s"%(nameCtrl, name))
          else:
            query = self.createControl(self.row + TEXT_ENTRY_ID, item, values.get(item[0], None), nameCtrl="%s_%s"%(nameCtrl, name))
            if not query: continue  # If this piece of data has no control, it is not user editable

            #Create tree item
            child = self.tree.AppendItem(treeItem, s)
            # pdb.set_trace()
            # Now create the lock/unlock button
            b = wx.BitmapButton(self, self.row + LOCK_BUTTON_ID, self.unlockedBmp,style = wx.NO_BORDER )
            b.SetToolTipString("If unlocked, instances can change this field")
            
            if ent.instanceLocked.get(name, False):  # Set its initial state
              b.SetBitmap(self.lockedBmp);
              b.SetBitmapSelected(self.unlockedBmp)

              # Not allow to change this value from instance
              query.Enable(isinstance(ent,entity.Instance) == False)
            else:
              b.SetBitmap(self.unlockedBmp);
              b.SetBitmapSelected(self.lockedBmp)

            b.SetBitmapSelected(self.lockedBmp)

            # Devide from entity type, not allow to 'Lock' this from instance
            b.Enable(isinstance(ent,entity.Instance) == False)

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

            self.tree.SetItemWindow(child, query, 3)
            self.tree.SetItemWindow(child, b, 2)
            # self.tree.SetItemBackgroundColor(child,wx.Color(50,50,50))
            self.lookup[self.row] = Gui2Obj(item,query,b,h,child,ent)
            self.row+=1

          self.tree.SetPyData(child, ent)

    def OnPaint(self, evt):
        if self.IsDoubleBuffered():
            dc = wx.PaintDC(self)
        else:
            dc = wx.BufferedPaintDC(self)
        dc.SetBackground(wx.Brush('blue'))
        dc.Clear()
        
        #self.Render(dc)

    def SetSashPosition(self, width = -1):
      # In the test configuration this window is just thrown into a frame so there is no sash...
      if isinstance(self.GetParent(), wx.SplitterWindow):
        self.GetParent().SetSashPosition(-1)
        self.GetParent().SetSashPosition(width)
        self.GetParent().UpdateSize()

    def BuildChangeEvent(self, ctrl):
        mapEvents = {
            'TextCtrl': [wx.EVT_TEXT, wx.EVT_TEXT_ENTER],
            'CheckBox': [wx.EVT_CHECKBOX],
            'Slider': [wx.EVT_SCROLL, wx.EVT_SLIDER, wx.EVT_SCROLL_CHANGED, ],
            'ComboBox': [wx.EVT_COMBOBOX, wx.EVT_TEXT, wx.EVT_TEXT_ENTER ], # , wx.EVT_COMBOBOX_CLOSEUP],  # wx.EVT_COMBOBOX_DROPDOWN,
            'SliderCustom':[wx.EVT_TEXT, wx.EVT_TEXT_ENTER]
            # TBD 
        }

        for t in mapEvents[type(ctrl).__name__]:
          ctrl.Bind(t, self.EvtText)
        
        # Bind this to select treeItem container this control
        ctrl.Bind(wx.EVT_LEFT_DOWN, self.OnFocus)
   
    def refresh(self):      
      self.treeItemSelected = None
      self.lookup.clear()      
      self._createTreeEntities()

def Test():
  global StandaloneDev
  import time
  import pyGuiWrapper as gui

  mdl = Model()
  mdl.load("testModel.xml")

  #sgt = mdl.entityTypes["ServiceGroup"]
  #sg = mdl.entities["MyServiceGroup"] = Entity(sgt,(0,0),(100,20))
  StandaloneDev = True
  gui.go(lambda parent,menu,tool,status,m=mdl: Panel(parent,common.GuiPlaces(parent,menu,tool,status,None,None), m))
  #time.sleep(2)
  #thePanel.showEntity(sg)


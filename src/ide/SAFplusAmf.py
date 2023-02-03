from gfxmath import *
from gfxtoolkit import *
from entity import *
from common import *
import share
import types
import umlEditor

class WizardDialog(wx.Dialog):

    def __init__(self,title, size=(430,340)):
        """Constructor"""
        wx.Dialog.__init__(self, None, title="SAF Application Wizard", size=size)
        self.LabelSize = (150,25) 
        self.EntrySize = (300,25)
        self.main_sizer = wx.BoxSizer(wx.VERTICAL)

  
    def createRow(self,label, ctrl=None, size=None):
      """Generates a GUI row consisting of a horizontal sizer, text description and control"""
      sizer = wx.BoxSizer(wx.HORIZONTAL)
      if label is not None:
        label = wx.StaticText(self, label=label, size=size if size else self.LabelSize)
        if ctrl == 0: 
          sizer.Add(label, 3, wx.ALL| wx.EXPAND, 5)
        else:
          sizer.Add(label, 0, wx.ALL|wx.CENTER, 5)
      if ctrl is None:
        ctrl = wx.TextCtrl(self,size=self.EntrySize)
        sizer.Add(ctrl, 10, wx.ALL | wx.EXPAND, 5)
      elif ctrl == 0:
        pass
      elif type(ctrl) is list:
        if isinstance(ctrl[0], wx.Control):  # Its an object, assume that its a wx control
          for c in ctrl:
            sizer.Add(c, 10, wx.ALL | wx.EXPAND, 5)
        else:
          ctrlValues = ctrl
          ctrl = wx.ComboBox(self,id=wx.ID_ANY,value=ctrlValues[0],choices=ctrlValues,style=wx.CB_DROPDOWN)
          sizer.Add(ctrl, 10, wx.ALL | wx.EXPAND, 5)
      else:
        sizer.Add(ctrl, 10, wx.ALL | wx.EXPAND, 5)

      return(sizer,ctrl)
 
class SAFWizardDialog(WizardDialog):
    """Class to define SAF application wizard dialog"""

    def __init__(self):
        """Constructor"""
        WizardDialog.__init__(self,"SAF Application Generator")
        gelems = []
        gelems.append(self.createRow("Service Group name"))
        self.nameGui = gelems[0][1]

        tmp = self.createRow("Redundancy Mode",["No redundancy","1+1","N+1"])
        self.redGui = tmp[1]
        gelems.append(tmp)

        tmp = self.createRow("Number of processes",[ str(x) for x in range(1,100)])
        self.nProc = tmp[1]
        gelems.append(tmp)

        tmp = self.createRow("Process names (space or comma separated)", size=(150,75))
        self.procNames = tmp[1]
        gelems.append(tmp)

        self.wrkGui = wx.CheckBox(self,-1,"")
        self.wrkGui.SetValue(True)
        gelems.append(self.createRow("Create basic work", self.wrkGui))

        OK_btn = wx.Button(self, label="OK")
        OK_btn.Bind(wx.EVT_BUTTON, self.onBtnHandler)
        cancelBtn = wx.Button(self, label="Cancel")
        cancelBtn.Bind(wx.EVT_BUTTON, self.onBtnHandler)
        gelems.append(self.createRow(None,[OK_btn,cancelBtn]))

        
        for (sizer, ctrl) in gelems:
          if type(ctrl) is list and isinstance(ctrl[0], wx.Button):
            self.main_sizer.Add(sizer, 0, wx.ALL|wx.CENTER, 5)
          else:
            self.main_sizer.Add(sizer, 0, wx.ALL, 5)

        self.SetSizer(self.main_sizer)
        self.main_sizer.Layout()
        for (sizer, ctrl) in gelems:
          sizer.Layout()
        self.main_sizer.Fit(self)
        
    def onBtnHandler(self, event):
        what = event.GetEventObject().GetLabel()
        print('about to %s' % what)  
        # if (what == "OK"):
        self.what = what
        if self.what == "OK":
          if not self.validate():
            return
        self.Close()

    def validate(self):
        if len(self.nameGui.GetValue().strip())==0:
          wx.MessageBox("Service Group name can not be empty", "Validator", wx.OK|wx.ICON_EXCLAMATION, self)
          self.nameGui.SetFocus()
          return False
        if len(self.procNames.GetValue().strip())==0:
          wx.MessageBox("Process names can not be empty", "Validator", wx.OK|wx.ICON_EXCLAMATION, self)
          self.procNames.SetFocus()
          return False
        if not self.nProc.GetValue().isdigit():
          wx.MessageBox("Number of processes can not be string", "Validator", wx.OK|wx.ICON_EXCLAMATION, self)
          self.nProc.SetFocus()
          return False
        procNameStr = self.procNames.GetValue()
        self.procNameList = splitByThisAndWhitespace(",", procNameStr)
        temp = set(self.procNameList)
        if len(self.procNameList) != len(temp):
          wx.MessageBox("Process names can not be duplicated", "Validator", wx.OK|wx.ICON_EXCLAMATION, self)
          self.procNames.SetFocus()
          return False

        return True

class NPNPWizardDialog(WizardDialog):
    """Class to define SAF application wizard dialog"""
 
    def __init__(self):
        """Constructor"""
        WizardDialog.__init__(self, "Non-SAF Application Generator", size=(430,400))

        gelems = []

        info = self.createRow("This dialog creates the expected UML entities for 3rd-party applications.  Define groups of apps that you want to fail over together.",0,size=(100,50))
        info2 = self.createRow("To define several separate failure groups, open this wizard multiple times.",0, size=(100,45))
        self.main_sizer.Add(info[0], 5, wx.ALL|wx.EXPAND,5)
        self.main_sizer.Add(info2[0], 5, wx.ALL|wx.EXPAND,5)
        self.main_sizer.Add(wx.StaticLine(self,), 0, wx.ALL|wx.EXPAND, 5)
        gelems.append(self.createRow("Service Group name"))
        self.nameGui = gelems[0][1]

        tmp = self.createRow("Number of processes",[ str(x) for x in range(1,100)])
        self.nProc = tmp[1]
        gelems.append(tmp)

        tmp = self.createRow("Process names (space or comma separated)", size=(150,75))
        self.procNames = tmp[1]
        gelems.append(tmp)

        OK_btn = wx.Button(self, label="OK")
        OK_btn.Bind(wx.EVT_BUTTON, self.onBtnHandler)
        cancelBtn = wx.Button(self, label="Cancel")
        cancelBtn.Bind(wx.EVT_BUTTON, self.onBtnHandler)
        gelems.append(self.createRow(None,[OK_btn,cancelBtn]))

        
        for (sizer, ctrl) in gelems:
          if type(ctrl) is list and isinstance(ctrl[0], wx.Button):
            self.main_sizer.Add(sizer, 0, wx.ALL|wx.CENTER, 5)
          else:
            self.main_sizer.Add(sizer, 0, wx.ALL, 5)

        self.SetSizer(self.main_sizer)
        self.main_sizer.Layout()
        info[0].Layout()
        info2[0].Layout()
        for (sizer, ctrl) in gelems:
          sizer.Layout()
        self.main_sizer.Fit(self)
    
    def onBtnHandler(self, event):
        what = event.GetEventObject().GetLabel()
        print('about to %s' % what)  
        # if (what == "OK"):
        self.what = what
        if self.what == "OK":
          if not self.validate():
            return
        self.Close()

    def validate(self):
        if len(self.nameGui.GetValue().strip())==0:
          wx.MessageBox("Service Group name can not be empty", "Validator", wx.OK|wx.ICON_EXCLAMATION, self)
          self.nameGui.SetFocus()
          return False
        if len(self.procNames.GetValue().strip())==0:
          wx.MessageBox("Process names can not be empty", "Validator", wx.OK|wx.ICON_EXCLAMATION, self)
          self.procNames.SetFocus()
          return False
        if not self.nProc.GetValue().isdigit():
          wx.MessageBox("Number of processes can not be string", "Validator", wx.OK|wx.ICON_EXCLAMATION, self)
          self.nProc.SetFocus()
          return False
        procNameStr = self.procNames.GetValue()
        self.procNameList = splitByThisAndWhitespace(",", procNameStr)
        temp = set(self.procNameList)
        if len(self.procNameList) != len(temp):
          wx.MessageBox("Process names can not be duplicated", "Validator", wx.OK|wx.ICON_EXCLAMATION, self)
          self.procNames.SetFocus()
          return False

        return True

class ConflictNameWarning(WizardDialog):
  """Class to warning about the name that conflict"""

  def __init__(self):
        """Constructor"""
        WizardDialog.__init__(self, "WARNING", size=(430,400))
        gelems = []
        info = self.createRow("Warning: name already exists",0,size=(100,50))
        self.main_sizer.Add(info[0], 5, wx.ALL|wx.EXPAND,5)
        self.main_sizer.Add(wx.StaticLine(self,), 0, wx.ALL|wx.EXPAND, 5)
        OK_btn = wx.Button(self, label="OK")
        OK_btn.Bind(wx.EVT_BUTTON, self.onBtnHandler)
        gelems.append(self.createRow(None,[OK_btn]))

        for (sizer, ctrl) in gelems:
          if type(ctrl) is list and isinstance(ctrl[0], wx.Button):
            self.main_sizer.Add(sizer, 0, wx.ALL|wx.CENTER, 5)
          else:
            self.main_sizer.Add(sizer, 0, wx.ALL, 5)

        self.SetSizer(self.main_sizer)
        self.main_sizer.Layout()
        info[0].Layout()
        for (sizer, ctrl) in gelems:
          sizer.Layout()
        self.main_sizer.Fit(self)

  def onBtnHandler(self, event):
    what = event.GetEventObject().GetLabel()
    print('about to %s' % what)  
    # if (what == "OK"):
    self.what = what
    if self.what == "OK":
      self.Close()

LevelStep = 150
SpacerStep = 300

class Extensions:
  def __init__(self, model, guiPlaces):
    self.model = model
    self.guiPlaces = guiPlaces
    print("SAFplusAmf extension loaded")
    self.safHAWizardId = wx.NewId()
    self.npnpHAWizardId = wx.NewId()
    menu = self.guiPlaces.menu.get("Modelling",None)
    self.position = (100,100)
    self.posAdjust = (0, 0)
    if menu:
      menu.AppendSeparator()
      menu.Append(self.safHAWizardId, "SAF App Creator","Create all the entities required for a SAF-aware compliant HA application")
      menu.Bind(wx.EVT_MENU, self.OnSAFWizardMenu, id=self.safHAWizardId)
      menu.Append(self.npnpHAWizardId, "App Creator","Create all the entities required for a SAF-unaware compliant HA application")
      menu.Bind(wx.EVT_MENU, self.OnNPNPWizardMenu, id=self.npnpHAWizardId)

  def compNameListCreator(self, sgName, processesName, numProcess):
    result = list(processesName)
    i = n = len(processesName)
    while(n < numProcess):
      i+=1
      procName = sgName + "Comp" + str(i)
      if procName in processesName:
        continue
      else:
        result.append(procName)
        n+=1
    return result

  def isNameConflicted(self, sgName, processesName, numProcess):
    newEntitiesName = [sgName+"SG", sgName+"SU", sgName+"SI"]
    compNameList = self.compNameListCreator(sgName, processesName, numProcess)
    csiNameList = [compName+"CSI" for compName in compNameList]

    newEntitiesName = newEntitiesName + compNameList + csiNameList
    existedNames = list(share.umlEditorPanel.entities.keys())
    
    if any(name in newEntitiesName for name in existedNames):
      return True
    return False
  
  def modelPosition(self, numbProcs):
    positionArray = []
    lArray = {}
    # Transverse through entities postion
    for key in share.umlEditorPanel.entities:
      if share.umlEditorPanel.entities[key].pos[1] < 800:
        positionArray.append(share.umlEditorPanel.entities[key].pos[0])
    # Sort the postion array by x_codinator (first value of position)
    positionArray = [*set(positionArray)]
    tmp = positionArray.copy()
    positionArray.append(0)
    positionArray.sort()
    tmp.append(positionArray[-1]  + 400)
    tmp.sort()
    for x, y in zip(positionArray, tmp):
      lArray.update({x: (y - x)})
    modelLength = 800 if numbProcs <= 2 else numbProcs*450 + 100
    for key in lArray:
      if lArray[key] >= modelLength:
        if key == 0:
          return 100
        else:
          return key + 400
    if (tmp[-1] == 400):
      return 100
    return tmp[-1]

  def OnSAFWizardMenu(self, event):
    dlg = SAFWizardDialog()
    dlg.what = None
    dlg.ShowModal()
    dlg.Destroy()
    warning = ConflictNameWarning()
    warning.what = None
    if dlg.what == "OK":
      name = dlg.nameGui.GetValue()
      procNames = dlg.procNameList
      nProc = int(dlg.nProc.GetValue())
      if self.isNameConflicted(name, procNames, nProc):
        warning.ShowModal()
        warning.Destroy()
        return
      position = (self.modelPosition(nProc), 100)
      size = None
      newEntities = []
      SG = self.model.model.entityTypes["ServiceGroup"].createEntity(position, size,name=name + "SG")
      newEntities.append(SG)
      position = (position[0], position[1] + LevelStep)
      SU = self.model.model.entityTypes["ServiceUnit"].createEntity(position, size,name=name + "SU")
      newEntities.append(SU)

      ca = ContainmentArrow(SG, (50,50), SU, (100,50), None)
      SG.containmentArrows.append(ca)

      createWork = dlg.wrkGui.GetValue()
 
      if createWork:
        SI = self.model.model.entityTypes["ServiceInstance"].createEntity((position[0] + SpacerStep, position[1]), size,name=name + "SI")
        ca = ContainmentArrow(SG, (50,50), SI, (100,50), None)
        SG.containmentArrows.append(ca)
        newEntities.append(SI)

      position = (position[0], position[1] + LevelStep)
      pos = position

      #for n in max(len(procNames), list(range(1,nProc+1))):
      compNameList = self.compNameListCreator(name, procNames, nProc)
      for compName in compNameList:
        comp = self.model.model.entityTypes["Component"].createEntity(pos, size,name=compName)
        ca = ContainmentArrow(SU, (50,50), comp, (100,50), None)
        SU.containmentArrows.append(ca)

        if createWork:
          CSI = self.model.model.entityTypes["ComponentServiceInstance"].createEntity((pos[0] + 60, pos[1] + 60), size,name=compName + "CSI")          
          ca = ContainmentArrow(SI, (50,50), CSI, (100,50), None)
          SI.containmentArrows.append(ca)
          ca = ContainmentArrow(CSI, (50,50), comp, (100,50), None)
          CSI.data["type"] = CSI.data["name"]
          comp.data['csiTypes'] = set()
          comp.data['csiTypes'].add(str(CSI.data['type']))
          CSI.containmentArrows.append(ca)
          newEntities.append(CSI)

        newEntities.append( comp )

        pos = (pos[0] + SpacerStep, pos[1])


      for ent in newEntities:
        if share.instancePanel:
          share.instancePanel.addEntityTool(ent)
        if share.detailsPanel:
          share.detailsPanel.createTreeItemEntity(ent.data["name"], ent)
        if share.umlEditorPanel:
          share.umlEditorPanel.entities[ent.data["name"]] = ent
          share.umlEditorPanel.Refresh() 

      toolBar = self.guiPlaces.toolbar
      unClickAllTools(toolBar)
      toolBar.ToggleTool(umlEditor.SELECT_BUTTON, True)  # Turn on the tool we need

      seltool = share.umlEditorPanel.selectTool
      seltool.OnSelect(share.umlEditorPanel,None)
      seltool.selected = set(newEntities)
      seltool.touching = set(newEntities)
      share.umlEditorPanel.tool = seltool
      
      self.posAdjust = (SpacerStep*(nProc+1)+5,0)
      # self.position = (self.position[0] + self.posAdjust[0], self.position[1] + self.posAdjust[1])

  def OnNPNPWizardMenu(self,event):
    dlg = NPNPWizardDialog()
    dlg.what = None
    dlg.ShowModal()
    dlg.Destroy()
    warning = ConflictNameWarning()
    warning.what = None
    if dlg.what == "OK":
      name = dlg.nameGui.GetValue()
      procNames = dlg.procNameList
      nProc = int(dlg.nProc.GetValue())
      if self.isNameConflicted(name, procNames, nProc):
        warning.ShowModal()
        warning.Destroy()
        return
      position = (self.modelPosition(nProc), 100)
      size = None
      newEntities = []
      SG = self.model.model.entityTypes["ServiceGroup"].createEntity(position, size,name=name + "SG")
      newEntities.append(SG)
      position = (position[0], position[1] + LevelStep)
      SU = self.model.model.entityTypes["ServiceUnit"].createEntity(position, size,name=name + "SU")
      newEntities.append(SU)

      ca = ContainmentArrow(SG, (50,50), SU, (100,50), None)
      SG.containmentArrows.append(ca)

      createWork = True
 
      if createWork:
        SI = self.model.model.entityTypes["ServiceInstance"].createEntity((position[0] + SpacerStep, position[1]), size,name=name + "SI")
        ca = ContainmentArrow(SG, (50,50), SI, (100,50), None)
        SG.containmentArrows.append(ca)
        newEntities.append(SI)

      position = (position[0], position[1] + LevelStep)
      pos = position

      #for n in max(len(procNames), list(range(1,nProc+1))):
      compNameList = self.compNameListCreator(name, procNames, nProc)
      for compName in compNameList:
        comp = self.model.model.entityTypes["Component"].createEntity(pos, size,name=compName)
        ca = ContainmentArrow(SU, (50,50), comp, (100,50), None)
        SU.containmentArrows.append(ca)

        if createWork:
          CSI = self.model.model.entityTypes["ComponentServiceInstance"].createEntity((pos[0] + 60, pos[1] + 60), size,name=compName + "CSI")
          ca = ContainmentArrow(SI, (50,50), CSI, (100,50), None)
          SI.containmentArrows.append(ca)
          ca = ContainmentArrow(CSI, (50,50), comp, (100,50), None)
          CSI.data["type"] = CSI.data["name"]
          comp.data['csiTypes'] = set()
          comp.data['csiTypes'].add(str(CSI.data['type']))
          CSI.containmentArrows.append(ca)
          newEntities.append(CSI)

        newEntities.append( comp )

        pos = (pos[0] + SpacerStep, pos[1])


      for ent in newEntities:
        if share.instancePanel:
          share.instancePanel.addEntityTool(ent)
        if share.detailsPanel:
          share.detailsPanel.createTreeItemEntity(ent.data["name"], ent)
        if share.umlEditorPanel:
          share.umlEditorPanel.entities[ent.data["name"]] = ent
          share.umlEditorPanel.Refresh() 

      toolBar = self.guiPlaces.toolbar
      unClickAllTools(toolBar)
      toolBar.ToggleTool(umlEditor.SELECT_BUTTON, True)  # Turn on the tool we need

      seltool = share.umlEditorPanel.selectTool
      seltool.OnSelect(share.umlEditorPanel,None)
      seltool.selected = set(newEntities)
      seltool.touching = set(newEntities)
      share.umlEditorPanel.tool = seltool

      self.posAdjust = (SpacerStep*(nProc+1)+5 ,0)
      # self.position = (self.position[0] + self.posAdjust[0], self.position[1] + self.posAdjust[1])

ext = None

def init(model, guiPlaces):
   global ext
   ext = Extensions(model, guiPlaces) 

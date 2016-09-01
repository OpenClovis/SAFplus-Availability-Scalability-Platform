from gfxmath import *
from gfxtoolkit import *
from entity import *
import share
import types

  

class SAFWizardDialog(wx.Dialog):
    """Class to define SAF application wizard dialog"""

    def createRow(self,label, ctrl=None):
      """Generates a GUI row consisting of a horizontal sizer, text description and control"""
      sizer = wx.BoxSizer(wx.HORIZONTAL)
      if label is not None:
        label = wx.StaticText(self, label=label, size=self.LabelSize)
        sizer.Add(label, 0, wx.ALL|wx.CENTER, 5)
      if ctrl is None:
        ctrl = wx.TextCtrl(self,size=self.EntrySize)
        sizer.Add(ctrl, 10, wx.ALL | wx.EXPAND, 5)
      elif type(ctrl) is types.ListType:
        if isinstance(ctrl[0], wx.Control):  # Its an object, assume that its a wx control
          for c in ctrl:
            sizer.Add(c, 10, wx.ALL | wx.EXPAND, 5)
        else:
          ctrlValues = ctrl
          ctrl = wx.ComboBox(self,-1,value=ctrlValues[0],pos=None,size=None,choices=ctrlValues,style=wx.CB_DROPDOWN)
          sizer.Add(ctrl, 10, wx.ALL | wx.EXPAND, 5)
      else:
        sizer.Add(ctrl, 10, wx.ALL | wx.EXPAND, 5)

      return(sizer,ctrl)
 
    def __init__(self):
        """Constructor"""
        wx.Dialog.__init__(self, None, title="SAF Application Wizard", size=(430,340))
        self.LabelSize = (100,25) 
        self.EntrySize = (300,25)

        gelems = []
        gelems.append(self.createRow("Service Group name"))
        self.nameGui = gelems[0][1]

        tmp = self.createRow("Redundancy Mode",["No redundancy","1+1","N+1"])
        self.redGui = tmp[1]
        gelems.append(tmp)

        tmp = self.createRow("Number of processes",[ str(x) for x in range(1,20)])
        self.nProc = tmp[1]
        gelems.append(tmp)

        self.wrkGui = wx.CheckBox(self,-1,"")
        self.wrkGui.SetValue(True)
        gelems.append(self.createRow("Create basic work", self.wrkGui))

        OK_btn = wx.Button(self, label="OK")
        OK_btn.Bind(wx.EVT_BUTTON, self.onBtnHandler)
        cancelBtn = wx.Button(self, label="Cancel")
        cancelBtn.Bind(wx.EVT_BUTTON, self.onBtnHandler)
        gelems.append(self.createRow(None,[OK_btn,cancelBtn]))

        main_sizer = wx.BoxSizer(wx.VERTICAL)
        for (sizer, ctrl) in gelems:
          main_sizer.Add(sizer, 0, wx.ALL, 5)

        self.SetSizer(main_sizer)
        main_sizer.Layout()
        for (sizer, ctrl) in gelems:
          sizer.Layout()
        
    def onBtnHandler(self, event):
        what = event.GetEventObject().GetLabel()
        print 'about to %s' % what  
        # if (what == "OK"):
        self.what = what
        self.Close()


class NPNPWizardDialog(wx.Dialog):
    """Class to define SAF application wizard dialog"""

    def createRow(self,label, ctrl=None):
      """Generates a GUI row consisting of a horizontal sizer, text description and control"""
      sizer = wx.BoxSizer(wx.HORIZONTAL)
      if label is not None:
        label = wx.StaticText(self, label=label, size=self.LabelSize)
        sizer.Add(label, 0, wx.ALL|wx.CENTER, 5)
      if ctrl is None:
        ctrl = wx.TextCtrl(self,size=self.EntrySize)
        sizer.Add(ctrl, 10, wx.ALL | wx.EXPAND, 5)
      elif type(ctrl) is types.ListType:
        if isinstance(ctrl[0], wx.Control):  # Its an object, assume that its a wx control
          for c in ctrl:
            sizer.Add(c, 10, wx.ALL | wx.EXPAND, 5)
        else:
          ctrlValues = ctrl
          ctrl = wx.ComboBox(self,-1,value=ctrlValues[0],pos=None,size=None,choices=ctrlValues,style=wx.CB_DROPDOWN)
          sizer.Add(ctrl, 10, wx.ALL | wx.EXPAND, 5)
      else:
        sizer.Add(ctrl, 10, wx.ALL | wx.EXPAND, 5)

      return(sizer,ctrl)
 
    def __init__(self):
        """Constructor"""
        wx.Dialog.__init__(self, None, title="Non-SAF Application Wizard", size=(430,340))
        self.LabelSize = (100,25) 
        self.EntrySize = (300,25)

        gelems = []
        gelems.append(self.createRow("Service Group name"))
        self.nameGui = gelems[0][1]

        tmp = self.createRow("Number of processes",[ str(x) for x in range(1,20)])
        self.nProc = tmp[1]
        gelems.append(tmp)

        OK_btn = wx.Button(self, label="OK")
        OK_btn.Bind(wx.EVT_BUTTON, self.onBtnHandler)
        cancelBtn = wx.Button(self, label="Cancel")
        cancelBtn.Bind(wx.EVT_BUTTON, self.onBtnHandler)
        gelems.append(self.createRow(None,[OK_btn,cancelBtn]))

        main_sizer = wx.BoxSizer(wx.VERTICAL)
        for (sizer, ctrl) in gelems:
          main_sizer.Add(sizer, 0, wx.ALL, 5)

        self.SetSizer(main_sizer)
        main_sizer.Layout()
        for (sizer, ctrl) in gelems:
          sizer.Layout()
        
    def onBtnHandler(self, event):
        what = event.GetEventObject().GetLabel()
        print 'about to %s' % what  
        # if (what == "OK"):
        self.what = what
        self.Close()





LevelStep = 150
SpacerStep = 300

class Extensions:
  def __init__(self, model, guiPlaces):
    self.model = model
    self.guiPlaces = guiPlaces
    print "SAFplusAmf extension loaded"
    self.safHAWizardId = wx.NewId()
    self.npnpHAWizardId = wx.NewId()
    menu = self.guiPlaces.menu.get("Modelling",None)
    if menu:
      menu.AppendSeparator()
      menu.Append(self.safHAWizardId, "SAF App Creator","Create all the entities required for a SAF-aware compliant HA application")
      menu.Bind(wx.EVT_MENU, self.OnSAFWizardMenu, id=self.safHAWizardId)
      menu.Append(self.npnpHAWizardId, "App Creator","Create all the entities required for a SAF-unaware compliant HA application")
      menu.Bind(wx.EVT_MENU, self.OnNPNPWizardMenu, id=self.npnpHAWizardId)

  def OnSAFWizardMenu(self, event):
    dlg = SAFWizardDialog()
    dlg.what = None
    dlg.ShowModal()
    dlg.Destroy()
    if dlg.what == "OK":
      position = (100,100)
      size = None
      name = dlg.nameGui.GetValue()

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
      nProc = int(dlg.nProc.GetValue())
      pos = position


      for n in range(0,nProc):
        comp = self.model.model.entityTypes["Component"].createEntity(pos, size,name=name + "Comp" + str(n))
        ca = ContainmentArrow(SU, (50,50), comp, (100,50), None)
        SU.containmentArrows.append(ca)

        if createWork:
          CSI = self.model.model.entityTypes["ComponentServiceInstance"].createEntity((pos[0] + 60, pos[1] + 60), size,name=name + "CSI" + str(n))
          ca = ContainmentArrow(SI, (50,50), CSI, (100,50), None)
          SI.containmentArrows.append(ca)
          ca = ContainmentArrow(CSI, (50,50), comp, (100,50), None)
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

  def OnNPNPWizardMenu(self,event):
    dlg = NPNPWizardDialog()
    dlg.what = None
    dlg.ShowModal()
    dlg.Destroy()
    if dlg.what == "OK":
      position = (100,100)
      size = None
      name = dlg.nameGui.GetValue()

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
      nProc = int(dlg.nProc.GetValue())
      pos = position


      for n in range(0,nProc):
        comp = self.model.model.entityTypes["Component"].createEntity(pos, size,name=name + "Comp" + str(n))
        ca = ContainmentArrow(SU, (50,50), comp, (100,50), None)
        SU.containmentArrows.append(ca)

        if createWork:
          CSI = self.model.model.entityTypes["ComponentServiceInstance"].createEntity((pos[0] + 60, pos[1] + 60), size,name=name + "CSI" + str(n))
          ca = ContainmentArrow(SI, (50,50), CSI, (100,50), None)
          SI.containmentArrows.append(ca)
          ca = ContainmentArrow(CSI, (50,50), comp, (100,50), None)
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

    pass

ext = None

def init(model, guiPlaces):
   global ext
   ext = Extensions(model, guiPlaces) 

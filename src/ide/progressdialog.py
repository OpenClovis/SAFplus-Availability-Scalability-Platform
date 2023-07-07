import wx

class ProgressDialog(wx.Dialog):
    def __init__(self, parent, currentRunningThread, parentDialog):
        if parentDialog:
            wx.Dialog.__init__(self, parentDialog, wx.ID_ANY, "Progress information", size= (610,222))
        else:
            wx.Dialog.__init__(self, parent, wx.ID_ANY, "Progress information", size= (610,222))
        self.parent = parent
        self.parent.setCancelProgress(False)
        self.thread = currentRunningThread
        # self.panel = wx.Panel(self,wx.ID_ANY)
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        self.gauge = wx.Gauge(self,size=(582,20),pos=(12,80), style=wx.GA_HORIZONTAL)
        self.textLabel = wx.StaticText(self, label="Operation in progress...", pos=(12,20))
        self.cancelButton =wx.Button(self, size=(105,27), label="Cancel", pos=(490,155))

        mainSizer.Add(self.textLabel, 0, wx.LEFT | wx.TOP, 15)
        mainSizer.Add(self.gauge, 0, wx.ALL | wx.ALIGN_CENTER_HORIZONTAL, 15)
        mainSizer.Add(self.cancelButton, 0, wx.ALL | wx.ALIGN_RIGHT, 15)

        self.cancelButton.Bind(wx.EVT_BUTTON, self.OnQuit)
        self.Bind(wx.EVT_CLOSE, self.OnQuit)
        self.timer = wx.Timer(self)
        self.Bind(wx.EVT_TIMER,self.OnTimer, self.timer)
        self.timer.Start(30)
        self.gauge.SetRange(500)
        self.gauge.SetValue(0)
        self.step = 1
        self.SetSizerAndFit(mainSizer)
        self.Show()

    def OnTimer(self, evt):
        if self.thread.is_alive():
            x = int(self.gauge.GetValue())
            if x >= 500:
                x = 0
            x += self.step
            self.gauge.SetValue(x)
        else:
            self.OnQuit(None)

    def OnQuit(self, event):
        if self.thread.is_alive():
            self.parent.setCancelProgress(True)
            self.parent.stopCurrentProcess()
            self.thread.join()
        self.timer.Stop()
        self.Destroy()


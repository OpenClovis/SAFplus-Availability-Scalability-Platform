import wx

class ProgressDialog(wx.Dialog):
    def __init__(self, parent, currentRunningThread):
        wx.Dialog.__init__(self, parent, wx.ID_ANY, "Progress information", size= (610,222))
        self.parent = parent
        self.parent.setCancelProgress(False)
        self.thread = currentRunningThread
        self.panel = wx.Panel(self,wx.ID_ANY)
        self.gauge = wx.Gauge(self.panel,size=(582,20),pos=(12,80), style=wx.GA_HORIZONTAL)
        self.textLabel = wx.StaticText(self.panel, label="Operation in progress...", pos=(12,20))
        self.cancelButton =wx.Button(self.panel, size=(105,27), label="Cancel", pos=(490,155))
        self.cancelButton.Bind(wx.EVT_BUTTON, self.OnQuit)
        self.Bind(wx.EVT_CLOSE, self.OnQuit)
        self.timer = wx.Timer(self)
        self.Bind(wx.EVT_TIMER,self.OnTimer, self.timer)
        self.timer.Start(30)
        self.gauge.SetRange(500)
        self.gauge.SetValue(0)
        self.step = 1
        self.Show()

    def OnTimer(self, evt):
        if self.thread.isAlive():
            x = int(self.gauge.GetValue())
            if x >= 500:
                x = 0
            x += self.step
            self.gauge.SetValue(x)
        else:
            self.OnQuit(None)

    def OnQuit(self, event):
        if self.thread.isAlive():
            self.parent.setCancelProgress(True)
            self.parent.stopCurrentProcess()
            self.thread.join()
        self.timer.Stop()
        self.Destroy()


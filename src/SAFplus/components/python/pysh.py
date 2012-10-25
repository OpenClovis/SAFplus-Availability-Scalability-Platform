import wx
import  wx.py   as  py
import  wx.py.crust as crust
import sys
import thread
import time

InjectedCmdEvent = wx.NewEventType()

app = None

class CrustFrameWithCmdInjection(crust.CrustFrame):
    def __init__(self, parent=None, id=-1, title='Python Interactive Shell',
                 pos=wx.DefaultPosition, size=wx.DefaultSize,
                 style=wx.DEFAULT_FRAME_STYLE,
                 rootObject=None, rootLabel=None, rootIsNamespace=True,
                 locals=None, InterpClass=None,
                 config=None, dataDir=None,
                 *args, **kwds):
      crust.CrustFrame.__init__(self, parent, id, title,
                 pos, size,
                 style,
                 rootObject, rootLabel, rootIsNamespace,
                 locals, InterpClass,
                 config, dataDir,
                 *args, **kwds)
      # Connect my custom event
      self.Connect(-1, -1, InjectedCmdEvent, self.OnInjectCmd)

    def OnInjectCmd(self,evt):
      cmds = evt.GetClientObject()
      if type(cmds) is type(""): cmds = [cmds]

      for cmd in cmds:
        sys.stderr.write("Command received: %s\n" % str(cmd))   
        self.shell.run(cmd)
        sys.stderr.write("Command pushed: %s\n" % str(cmd))
#      self.shell.push(cmd)


    def RunCmd(self,cmd):
      event = wx.CommandEvent(InjectedCmdEvent)
      event.SetEventObject(self)
      event.SetClientObject(cmd)
      wx.PostEvent(self, event)


class App(wx.App):
    """Python shell"""

    def OnInit(self):
        import os
        import wx
        from wx import py
        
        self.SetAppName("pythonShell")
        confDir = wx.StandardPaths.Get().GetUserDataDir()
        if not os.path.exists(confDir):
            os.mkdir(confDir)
        fileName = os.path.join(confDir, 'config')
        self.config = wx.FileConfig(localFilename=fileName)
        self.config.SetRecordDefaults(True)
        
        self.frame = CrustFrameWithCmdInjection(config=self.config, dataDir=confDir)
##        self.frame.startupFileName = os.path.join(confDir,'pycrust_startup')
##        self.frame.historyFileName = os.path.join(confDir,'pycrust_history')
        self.frame.Show()
        self.SetTopWindow(self.frame)
        return True


def Run():
    app.MainLoop()

if __name__ == '__main__':
    app = App(0)
    thread.start_new_thread(Run, ())

    for i in range(1,10):
      app.frame.RunCmd('print "hi"')
      time.sleep(10)


import wx

class Panel(wx.Panel):
    def __init__(self, parent,menubar, toolbar, statusbar, renderFn):
      wx.Panel.__init__(self, parent, -1, style=wx.SUNKEN_BORDER)
      self.Bind(wx.EVT_PAINT, self.OnPaint)
      self.renderFn = renderFn
      self.menuBar = menubar
      self.toolBar = toolbar
      self.statusBar = statusbar

    def OnPaint(self, evt):
        dc = wx.BufferedPaintDC(self)
        dc.SetBackground(wx.Brush('white'))
        dc.Clear()        
        ctx = wx.lib.wxcairo.ContextFromDC(dc)
        self.renderFn(ctx,dc)


class MyFrame(wx.Frame):
    """
    This is MyFrame.  It just shows a few controls on a wxPanel,
    and has a simple menu.
    """
    def __init__(self, parent, title,panelFactory):
        wx.Frame.__init__(self, parent, -1, title, pos=(150, 150), size=(800, 600))

        # Create the menubar
        menuBar = wx.MenuBar()
        # and a menu 
        menu = wx.Menu()
        # and a toolbar
        tb = self.CreateToolBar()
        tb.SetToolBitmapSize((24,24))
        # add an item to the menu, using \tKeyName automatically
        # creates an accelerator, the third param is some help text
        # that will show up in the statusbar
        menu.Append(wx.ID_EXIT, "E&xit\tAlt-X", "Exit this simple sample")

        # bind the menu event to an event handler
        self.Bind(wx.EVT_MENU, self.OnTimeToClose, id=wx.ID_EXIT)

        # and put the menu on the menubar
        menuBar.Append(menu, "&File")
        self.SetMenuBar(menuBar)

        sb = self.CreateStatusBar()
        

        # Now create the Panel to put the other controls on.
        panel = self.panel = panelFactory(self,menuBar,tb,sb) # wx.Panel(self)

        # and a few controls
        if 0:
          text = wx.StaticText(panel, -1, "Hello World!")
          text.SetFont(wx.Font(14, wx.SWISS, wx.NORMAL, wx.BOLD))
          text.SetSize(text.GetBestSize())
          btn = wx.Button(panel, -1, "Close")
          funbtn = wx.Button(panel, -1, "Just for fun...")

          # bind the button events to handlers
          self.Bind(wx.EVT_BUTTON, self.OnTimeToClose, btn)
          self.Bind(wx.EVT_BUTTON, self.OnFunButton, funbtn)

          # Use a sizer to layout the controls, stacked vertically and with
          # a 10 pixel border around each
          sizer = wx.BoxSizer(wx.VERTICAL)
          sizer.Add(text, 0, wx.ALL, 10)
          sizer.Add(btn, 0, wx.ALL, 10)
          sizer.Add(funbtn, 0, wx.ALL, 10)
          panel.SetSizer(sizer)
          panel.Layout()

        # And also use a sizer to manage the size of the panel such
        # that it fills the frame
        sizer = wx.BoxSizer()
        sizer.Add(panel, 1, wx.EXPAND)
        self.SetSizer(sizer)
        

    def OnTimeToClose(self, evt):
        """Event handler for the button click."""
        print("See ya later!")
        self.Close()

    def OnFunButton(self, evt):
        """Event handler for the button click."""
        print("Having fun yet?")


class MyApp(wx.App):
    def __init__(self, panelFactory, redirect):
      self.panelFactory = panelFactory
      wx.App.__init__(self, redirect=redirect)
    def OnInit(self):
      self.frame = MyFrame(None, "Simple wxPython App",self.panelFactory)
      self.SetTopWindow(self.frame)

      print("Print statements go to this stdout window by default.")
      self.frame.Show(True)
      return True
        
app       = None

def go(panelFactory):
  global app
  app = MyApp(panelFactory, redirect=False)
  app.MainLoop()

import threading

guithread = None

def start(panelFactory):
  global guithread
  args = panelFactory, 
  guithread = threading.Thread(group=None, target=go, name="GUI",args = args)
  guithread.start()


def start1(panelFactory):
  global app
  global guithread

  app = MyApp(panelFactory, redirect=False)
  guithread = threading.Thread(group=None, target=app.MainLoop, name="GUI")
  guithread.start()


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

class MyPanel(wx.Panel):
    def __init__(self, parent):
        wx.Panel.__init__(self, parent, -1, style=wx.SUNKEN_BORDER)

        self.Bind(wx.EVT_PAINT, self.OnPaint)

        text = wx.StaticText(self, -1,
                            "Everything on this side of the splitter comes from Python.")
        
        intro = 'Welcome To PyCrust %s - The Flakiest Python Shell' % 1234  # version.VERSION
        pycrust = wx.TextCtrl(self, -1, intro)

        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(text, 0, wx.EXPAND|wx.ALL, 10)
        sizer.Add(pycrust, 0, wx.EXPAND|wx.BOTTOM|wx.LEFT|wx.RIGHT, 10)
        self.SetSizer(sizer)


    def OnPaint(self, evt):
        if self.IsDoubleBuffered():
            dc = wx.PaintDC(self)
        else:
            dc = wx.BufferedPaintDC(self)
        dc.SetBackground(wx.Brush('white'))
        dc.Clear()
        
        self.Render(dc)


    def Render(self, dc):
        # now draw something with cairo
        ctx = wx.lib.wxcairo.ContextFromDC(dc)
        ctx.set_line_width(15)
        ctx.move_to(125, 25)
        ctx.line_to(225, 225)
        ctx.rel_line_to(-200, 0)
        ctx.close_path()
        ctx.set_source_rgba(0, 0, 0.5, 1)
        ctx.stroke()

        # and something else...
        ctx.arc(200, 200, 80, 0, math.pi*2)
        ctx.set_source_rgba(0, 1, 1, 0.5)
        ctx.fill_preserve()
        ctx.set_source_rgb(1, 0.5, 0)
        ctx.stroke()

        # Draw some text
        face = wx.lib.wxcairo.FontFaceFromFont(
            wx.FFont(10, wx.SWISS, wx.FONTFLAG_BOLD))
        ctx.set_font_face(face)
        ctx.set_font_size(60)
        ctx.move_to(360, 180)
        ctx.set_source_rgb(0, 0, 0)
        ctx.show_text("Hello")

        # Text as a path, with fill and stroke
        ctx.move_to(400, 220)
        ctx.text_path("World")
        ctx.set_source_rgb(0.39, 0.07, 0.78)
        ctx.fill_preserve()
        ctx.set_source_rgb(0,0,0)
        ctx.set_line_width(2)
        ctx.stroke()

        #Render svg file
        cairo_surface = self.SvgToCairo('comp')
        ctx.translate(100,200)
        ctx.scale(0.7, 0.7)
        ctx.rotate(0)
        ctx.set_source_surface(cairo_surface, 100, 200)
        ctx.identity_matrix()
        ctx.paint()

        cairo_surface = self.SvgToCairo('csi')
        ctx.translate(100,200)
        ctx.scale(0.7, 0.7)
        ctx.rotate(0)
        ctx.set_source_surface(cairo_surface, 300, 200)
        ctx.identity_matrix()
        ctx.paint()

        cairo_surface = self.SvgToCairo('node')
        ctx.translate(100,200)
        ctx.scale(0.7, 0.7)
        ctx.rotate(0)
        ctx.set_source_surface(cairo_surface, 500, 200)
        ctx.identity_matrix()
        ctx.paint()

    def SvgToCairo(self, iconType, config = {}):
        dataFileSvg = svg.loadSvgIcon(iconType, config);

        svgFile = rsvg.Handle(data = dataFileSvg)
        svgwidth = svgFile.get_property('width')
        svgheight = svgFile.get_property('height')

        img = cairo.ImageSurface(cairo.FORMAT_ARGB32, svgwidth,svgheight)
        ictx = cairo.Context(img)
        ictx.set_operator(cairo.OPERATOR_SOURCE)
        ictx.set_operator(cairo.OPERATOR_OVER)
        ictx.set_source_rgba(0,1,0,0) #Transparent
        ictx.paint()
        svgFile.render_cairo(ictx)
        return img


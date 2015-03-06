#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>
#include <wx/defs.h>

#include "SAFplus7ScrolledWindow.h"

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <cairo.h>

#ifdef STANDALONE
#define ASSETLOC "../"
#else
#define ASSETLOC
#endif // STANDALONE

void cairoTestDraw(cairo_t *cr);

BEGIN_EVENT_TABLE(SAFplus7ScrolledWindow, wxScrolledWindow)
// some useful events
    EVT_PAINT(SAFplus7ScrolledWindow::paintEvent)
    EVT_MOTION(SAFplus7ScrolledWindow::mouseMoved)
    EVT_LEFT_DOWN(SAFplus7ScrolledWindow::mouseDown)
    EVT_LEFT_UP(SAFplus7ScrolledWindow::mouseReleased)
    EVT_RIGHT_DOWN(SAFplus7ScrolledWindow::rightClick)
    EVT_LEAVE_WINDOW(SAFplus7ScrolledWindow::mouseLeftWindow)
    EVT_KEY_DOWN(SAFplus7ScrolledWindow::keyPressed)
    EVT_KEY_UP(SAFplus7ScrolledWindow::keyReleased)
    EVT_MOUSEWHEEL(SAFplus7ScrolledWindow::mouseWheelMoved)
END_EVENT_TABLE()


#include <wx/rawbmp.h>
typedef wxAlphaPixelData PixelData;
wxBitmap *RGBAtoBitmap(unsigned char *rgba, int w, int h)
{
   wxBitmap *bitmap=new wxBitmap(w, h, 32);
   if(!bitmap->Ok()) {
      delete bitmap;
      return NULL;
   }

   PixelData bmdata(*bitmap);
   if(bmdata==NULL) {
      wxLogDebug(wxT("getBitmap() failed"));
      delete bitmap;
      return NULL;
   }

   bmdata.UseAlpha();
   PixelData::Iterator dst(bmdata);

   for(int y=0; y<h; y++) {
      dst.MoveTo(bmdata, 0, y);
      for(int x=0; x<w; x++) {
         // wxBitmap contains rgb values pre-multiplied with alpha
         unsigned char a=rgba[3];
         dst.Red()=rgba[0]*a/255;
         dst.Green()=rgba[1]*a/255;
         dst.Blue()=rgba[2]*a/255;
         dst.Alpha()=a;
         dst++;
         rgba+=4;
      }
   }
   return bitmap;
}

wxBitmap* SvgToBitmap()
  {
    wxBitmap* bitmap = new wxBitmap(250, 250,wxBITMAP_SCREEN_DEPTH );
    cairo_t* cr =  bitmap->CairoCreate();
    cairo_set_source_rgba               (cr,0,0,0,1);
    cairo_paint(cr);
    cairoTestDraw(cr);
    return bitmap;
  }

cairo_surface_t* SvgToCairo(RsvgHandle* im)
  {

    //wxBitmap::wxBitmap 	(int  	width,int  	height,int  depth = 32) //wxBITMAP_SCREEN_DEPTH
    cairo_surface_t* bitmap = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,250, 250);
    cairo_t* cr = cairo_create(bitmap);
    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba               (cr,0,1,0,0);
    cairo_paint(cr);
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);  // blending
    cairoTestDraw(cr);
    //rsvg_handle_set_dpi(im,1000);  // no effect that I can determine

    rsvg_handle_render_cairo(im,cr);

    cairo_destroy(cr);
    return bitmap;
  }
//SAFplus7ScrolledWindow::SAFplus7ScrolledWindow(wxWindow* parent, wxWindowID id) : wxScrolledWindow(parent, id, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxSUNKEN_BORDER|wxVSCROLL )
SAFplus7ScrolledWindow::SAFplus7ScrolledWindow(wxWindow* parent, wxWindowID id) : wxScrolledWindow(parent, id, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxSUNKEN_BORDER|wxVSCROLL, wxString::FromUTF8("SAFplusModeller") )
{
    m_parent = parent;
    //ctor

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    m_statusText = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    m_statusText->Wrap( -1 );
    sizer->Add( m_statusText, 0, wxALL, 5 );
    //sizer->Add(&details,0,wxALL,1);
    this->SetSizer(sizer);

    m_isDirty = false;

    icon = NULL;
     // g_object_unref(icon); to free
    GError *error;
    FILE* fp = fopen(ASSETLOC "test.svg","rb");

    if (fp)
    {
        icon = rsvg_handle_new();
        if (1)  // Actually loop it
          {
          unsigned char buf[65535];
          int amtRead = fread(buf,1,65535,fp);
          if (!rsvg_handle_write(icon,buf,amtRead,&error))
            {
            // failed!
            rsvg_handle_close(icon,&error);
            icon = NULL;
            }
          else rsvg_handle_close(icon,&error);
          }
        fclose(fp);

        RsvgDimensionData dim;
        rsvg_handle_get_dimensions (icon, &dim);
        printf("SVG dimensions: %d, %d em: %f ex: %f\n", dim.width, dim.height, dim.em, dim.ex);

        //gboolean rsvg_handle_get_position_sub (RsvgHandle *handle, RsvgPositionData *position_data, const char *id);
    }
    //notify wxAUI which frame to use
    m_mgr.SetManagedWindow(this);

    // create several control for SG, SU, .....
    wxPanel *m_propertiesPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(120, 1000));

    wxBoxSizer* bSizerProperties = new wxBoxSizer( wxVERTICAL );

    wxTextCtrl *m_textCtrl2 = new wxTextCtrl( m_propertiesPanel, wxID_ANY, wxT("Properties ..."), wxDefaultPosition, wxDefaultSize, 0 );
    bSizerProperties->Add( m_textCtrl2, 0, wxALL | wxEXPAND, 5 );

    m_propertiesPanel->SetSizer( bSizerProperties );
    m_propertiesPanel->Layout();
    bSizerProperties->Fit( m_propertiesPanel );

    // add the panes to the manager
    m_mgr.AddPane(m_propertiesPanel, wxAuiPaneInfo().Name(wxT("Properties")).Caption(wxT("Properties")).Right().Layer(1).Position(1).CloseButton(true).MaximizeButton(false));
    m_mgr.GetPane(wxT("Properties")).Show().Right().Layer(0).Row(0).Position(0);

    m_mgr.GetPane(wxT("Properties")).Show().Right().Layer(0).Row(0).Position(0);
    // tell the manager to "commit" all the changes just made
    m_mgr.Update();

}

SAFplus7ScrolledWindow::~SAFplus7ScrolledWindow()
{
    //dtor
    m_mgr.UnInit();
}

void SAFplus7ScrolledWindow::paintEvent(wxPaintEvent & evt)
{
    if (m_isDirty)
    {
      //Restore
      return;
    }

    wxPaintDC dc(this);
    m_isDirty = true;
}


char mouseMovedText[80];
void SAFplus7ScrolledWindow::mouseMoved(wxMouseEvent &event)
{
// TODO (hoangle#1#):
    wxClientDC dc(this);
    wxPoint pos = event.GetPosition();
    long x = dc.DeviceToLogicalX( pos.x );
    long y = dc.DeviceToLogicalY( pos.y );
    //wxString str;
    //str.Printf( "Current mouse position: %d,%d", (int)x, (int)y );

    snprintf(mouseMovedText,80,"Current mouse position: %d,%d", (int)x, (int)y );
    m_statusText->SetLabel(wxString::FromUTF8(mouseMovedText));
}

double rotateAmt = 0;
double scaleAmt = .1;
void SAFplus7ScrolledWindow::mouseDown(wxMouseEvent &event)
{
// TODO (hoangle#1#):

    wxClientDC dc(this);
    //wxPaintDC dc(this);
    //dc.GetImpl()->GetCairoContext()
    //cairo_t* cairo_surface = gdk_cairo_create((GdkDrawable*)dc.GetImpl()->GetCairoContext());  //m_gdkwindow);
    cairo_t* cairo_surface = NULL; // (cairo_t*) dc.GetImpl()->GetCairoContext();

#ifdef __WXGTK3__
    cairo_surface = (cairo_t*) dc.GetImpl()->GetCairoContext();
#else // __WXGTK3__
    wxWindow* wxwin = dc.GetWindow();
    GtkWidget* gtkwidget = wxwin->m_wxwindow;
    GdkWindow* gdkWindow = NULL;
    if (gtkwidget)
    {
      gdkWindow = gtkwidget->window; //wx_gtk_widget_get_window(gtkwidget);
    }
    if (gdkWindow)
      {
        cairo_surface = gdk_cairo_create(gdkWindow);
      }
#endif // __WXGTK3__
    if (cairo_surface)
    {
      cairoTestDraw(cairo_surface);
      if (icon)
        {
        rsvg_handle_render_cairo(icon,cairo_surface);
        }
    wxPoint pos = event.GetPosition();
    cur_posx = dc.DeviceToLogicalX( pos.x );
    cur_posy = dc.DeviceToLogicalY( pos.y );

    rotateAmt+=.1;
    scaleAmt += .2;

    cairo_surface_t* blit = SvgToCairo(icon);
    cairo_translate(cairo_surface,cur_posx,cur_posy);
    cairo_rotate(cairo_surface, rotateAmt);
    cairo_scale(cairo_surface, scaleAmt, scaleAmt);
    //cairo_rotate(cairo_surface, 3.1459/2);
    cairo_set_source_surface (cairo_surface, blit, 0,0); //(1/.3)*cur_posx, cur_posy/0.3);
    cairo_paint(cairo_surface);
    cairo_identity_matrix(cairo_surface);
    //cairo_paint_with_alpha(cairo_surface);
#ifndef __WXGTK3__
      cairo_destroy(cairo_surface);
#endif
    }
#if 0
    wxPoint pos = event.GetPosition();
    wxBitmap* bmp = SvgToBitmap();
    dc.DrawBitmap (*bmp, pos.x, pos.y,false);
    delete bmp;
#endif

#if 0
    wxClientDC dc(this);
    wxPoint pos = event.GetPosition();
    cur_posx = dc.DeviceToLogicalX( pos.x );
    cur_posy = dc.DeviceToLogicalY( pos.y );

    double rect_width  = 200, rect_height = 200;

    unsigned char* image_buffer = (unsigned char*)malloc(rect_width * rect_height * 4);
    unsigned int image_buffer_len = rect_width * rect_height * 4;

    cairo_surface_t* cairo_surface = cairo_image_surface_create_for_data(
                                    image_buffer,
                                    CAIRO_FORMAT_RGB24,
                                    rect_width,
                                    rect_height,
                                    rect_width * 4);
    cairo_t* cr = cairo_create(cairo_surface);

    //Calling test
    cairoTestDraw(cr);

    //Filling to dc
    // Now translate the raw image data from the format stored
    // by cairo into a format understood by wxImage.
    unsigned char* output = (unsigned char*)malloc(image_buffer_len);
    int offset = 0;
    for(size_t count = 0; count < image_buffer_len; count+=4)
    {
        int r = *(image_buffer+count+2);
        *(output + offset) = r;
        offset++;
        int g = *(image_buffer+count+1);
        *(output + offset) = g;
        offset++;
        int b = *(image_buffer+count+0);
        *(output + offset) = b;
        offset++;
    }

    wxImage img(rect_width, rect_height, output, true);
    wxBitmap bmp(img);

    // Create a double buffer to draw the plot
    // on screen to prevent flicker from occuring.
    dc.DrawBitmap(bmp, cur_posx, cur_posy);

    cairo_destroy(cr);
    cairo_surface_destroy(cairo_surface);
    free(image_buffer);
    free(output);
#endif

}

void SAFplus7ScrolledWindow::mouseWheelMoved(wxMouseEvent& event)
{
// TODO (hoangle#1#):
}

void SAFplus7ScrolledWindow::mouseReleased(wxMouseEvent& event)
{
// TODO (hoangle#1#):
}

void SAFplus7ScrolledWindow::rightClick(wxMouseEvent& event)
{
// TODO (hoangle#1#):
}

void SAFplus7ScrolledWindow::mouseLeftWindow(wxMouseEvent& event)
{
// TODO (hoangle#1#):
}
void SAFplus7ScrolledWindow::keyPressed(wxKeyEvent& event)
{
// TODO (hoangle#1#):
}

void SAFplus7ScrolledWindow::keyReleased(wxKeyEvent& event)
{
// TODO (hoangle#1#):
}

void cairoTestDraw(cairo_t *cr)
{
  //http://cairographics.org/samples/
  cairo_text_extents_t extents;

  const char *utf8 = "cairo";
  double x,y;

  cairo_select_font_face (cr, "Sans",
      CAIRO_FONT_SLANT_NORMAL,
      CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_source_rgba (cr, 1, 0, 1.0, 1.0);
  cairo_set_font_size (cr, 52.0);
  cairo_text_extents (cr, utf8, &extents);
  x = 128.0-(extents.width/2 + extents.x_bearing);
  y = 128.0-(extents.height/2 + extents.y_bearing);

  cairo_move_to (cr, x, y);
  cairo_show_text (cr, utf8);

  /* draw helping lines */
  cairo_set_source_rgba (cr, 1, 0.2, 0.2, .6);
  cairo_set_line_width (cr, 6.0);
  cairo_arc (cr, 0, 0, 20.0, 0, 2*M_PI);
  cairo_fill (cr);
  cairo_move_to (cr, 128.0, 0);
  cairo_rel_line_to (cr, 0, 256);
  cairo_move_to (cr, 0, 128.0);
  cairo_rel_line_to (cr, 256, 0);
  cairo_stroke (cr);
}

#ifndef SAFPLUS7SCROLLEDWINDOW_H
#define SAFPLUS7SCROLLEDWINDOW_H

#include <wx/window.h>
#include <wx/aui/aui.h>
#include <wx/scrolwin.h>
#include <wx/laywin.h>
#include <cairo.h>
#include <librsvg/rsvg.h>

class SAFplusScrolledWindow : public wxScrolledWindow
{
    public:
        SAFplusScrolledWindow(wxWindow* parent, wxWindowID id);
        virtual ~SAFplusScrolledWindow();

        void paintEvent(wxPaintEvent &evt);
        void mouseMoved(wxMouseEvent &event);
        void mouseDown(wxMouseEvent &event);
        void mouseWheelMoved(wxMouseEvent& event);
        void mouseReleased(wxMouseEvent& event);
        void rightClick(wxMouseEvent& event);
        void mouseLeftWindow(wxMouseEvent& event);
        void keyPressed(wxKeyEvent& event);
        void keyReleased(wxKeyEvent& event);

        long cur_posx; //Get current posX
        long cur_posy; //Get current posY

        wxWindow* m_parent;
        wxStaticText* m_statusText;
        bool m_isDirty;
        //void cairoTestDraw(cairo_t *cr);
        wxAuiManager m_mgr;
    protected:
        RsvgHandle* icon;
    private:
        DECLARE_EVENT_TABLE()
};

#endif // SAFPLUS7SCROLLEDWINDOW_H

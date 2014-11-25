/***************************************************************
 * Name:      standaloneMain.cpp
 * Purpose:   Code for Application Frame
 * Author:    OpenClovis ()
 * Created:   2014-11-11
 * Copyright: OpenClovis (www.openclovis.com)
 * License:
 **************************************************************/

#ifdef WX_PRECOMP
#include "wx_pch.h"
#endif

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#include "standaloneMain.h"

//helper functions
enum wxbuildinfoformat {
    short_f, long_f };

wxString wxbuildinfo(wxbuildinfoformat format)
{
    wxString wxbuild(wxVERSION_STRING);

    if (format == long_f )
    {
#if defined(__WXMSW__)
        wxbuild << _T("-Windows");
#elif defined(__WXMAC__)
        wxbuild << _T("-Mac");
#elif defined(__UNIX__)
        wxbuild << _T("-Linux");
#endif

#if wxUSE_UNICODE
        wxbuild << _T("-Unicode build");
#else
        wxbuild << _T("-ANSI build");
#endif // wxUSE_UNICODE
    }

    return wxbuild;
}

wxString dataFolder = wxString::FromUTF8("../data");

BEGIN_EVENT_TABLE(standaloneFrame, wxFrame)
    EVT_CLOSE(standaloneFrame::OnClose)
    EVT_MENU(idMenuQuit, standaloneFrame::OnQuit)
    EVT_MENU(idMenuAbout, standaloneFrame::OnAbout)
END_EVENT_TABLE()

standaloneFrame::standaloneFrame(wxFrame *frame, const wxString& title)
    : wxFrame(frame, -1, title, wxPoint(100,100), wxSize(800,600))
{
#if wxUSE_MENUS
    // create a menu bar
    wxMenuBar* mbar = new wxMenuBar();
    wxMenu* fileMenu = new wxMenu(_T(""));
    fileMenu->Append(idMenuQuit, _("&Quit\tAlt-F4"), _("Quit the application"));
    mbar->Append(fileMenu, _("&File"));

    wxMenu* helpMenu = new wxMenu(_T(""));
    helpMenu->Append(idMenuAbout, _("&About\tF1"), _("Show info about this application"));
    mbar->Append(helpMenu, _("&Help"));

    SetMenuBar(mbar);
#endif // wxUSE_MENUS

#if wxUSE_STATUSBAR
    // create a status bar with some information about the used wxWidgets version
    CreateStatusBar(2);
    SetStatusText(_("Hello Code::Blocks user!"),0);
    SetStatusText(wxbuildinfo(short_f), 1);
#endif // wxUSE_STATUSBAR

}

BEGIN_EVENT_TABLE(EditorBase, wxPanel)
END_EVENT_TABLE()

EditorBase::EditorBase(wxWindow* parent, const wxString& filename):wxPanel(parent)
{

}

EditorBase::~EditorBase()
  {

  }

standaloneFrame::~standaloneFrame()
{
}

void standaloneFrame::OnClose(wxCloseEvent &event)
{
    Destroy();
}

void standaloneFrame::OnQuit(wxCommandEvent &event)
{
    Destroy();
}

void standaloneFrame::OnAbout(wxCommandEvent &event)
{
    wxString msg = wxbuildinfo(long_f);
    wxMessageBox(msg, _("Welcome to..."));
}


wxBitmap cbLoadBitmap(const wxString &fileName,  wxBitmapType bitmapType)
  {
   return wxBitmap(fileName,bitmapType);
  }

Manager::Manager(const wxString& resfile)
  {
    wxImage::AddHandler(new wxGIFHandler);
    wxImage::AddHandler(new wxPNGHandler);
    wxXmlResource* res = wxXmlResource::Get();
    assert(res);
    res->InitAllHandlers();
    res->Load(resfile);
  }

Manager* theManager=NULL;
LogManager the_logManager;
wxFrame* theFrame = NULL;
cbProject theProject;
ProjectManager theProjectManager;
wxApp* theApp = NULL;

Manager* Manager::Get() { return theManager; };

LogManager* Manager::GetLogManager() { return &the_logManager;}


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
#include "../SAFplus7EditorPanel.h"

#include <Python.h>
#include <boost/python.hpp>
#include <boost/python/dict.hpp>
#include <vector>
#include "../utils.h"
#include "../yangParser.h"

using namespace std;
namespace bpy = boost::python;

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

std::string parse_python_exception();

wxString dataFolder = wxString::FromUTF8("../data");

BEGIN_EVENT_TABLE(standaloneFrame, wxFrame)
    EVT_CLOSE(standaloneFrame::OnClose)
    EVT_MENU(idMenuQuit, standaloneFrame::OnQuit)
    EVT_MENU(idMenuAbout, standaloneFrame::OnAbout)
    EVT_MENU(idYangParse, standaloneFrame::OnYangParse)
END_EVENT_TABLE()

standaloneFrame::standaloneFrame(wxFrame *frame, const wxString& title)
    : wxFrame(frame, -1, title, wxPoint(100,100), wxSize(800,600))
{
    Py_Initialize();
    new SAFplus7EditorPanel(this, title);
#if wxUSE_MENUS
    // create a menu bar
    wxMenuBar* mbar = new wxMenuBar();
    wxMenu* fileMenu = new wxMenu(_T(""));
    fileMenu->Append(idMenuQuit, _("&Quit\tAlt-F4"), _("Quit the application"));
    fileMenu->Append(idYangParse, _T("Yang Parse"), _T("Test Yang Parse"));

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
  Py_Finalize();
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

void standaloneFrame::OnYangParse(wxCommandEvent &event)
{
  wxString yangFile = wxT("../resources/SAFplusAmf.yang");
  try
  {
    std::vector<std::string> yangfiles;
    yangfiles.push_back(Utils::toString(yangFile));

    YangParser yangParser;
    bpy::tuple values = bpy::extract<bpy::tuple>(yangParser.parseFile(".", yangfiles));

    boost::python::dict ytypes = bpy::extract<bpy::dict>(values[0]);
    boost::python::dict yobjects = bpy::extract<bpy::dict>(values[1]);

    std::string resultStr = boost::python::extract<std::string>(yobjects["Cluster"]["startupAssignmentDelay"]["help"]);
#ifdef wxUSE_STATUSBAR
    SetStatusText(wxString::FromUTF8(resultStr.c_str()));
#else
    printf("AA [%s]", resultStr.c_str());
#endif
  }
  catch(boost::python::error_already_set const &e)
  {
    // Parse and output the exception
    string perror_str = parse_python_exception();
    cout << "Error during configuration parsing: " << perror_str << endl;
  }
}


wxBitmap cbLoadBitmap(const wxString &fileName,  wxBitmapType bitmapType)
  {
   return wxBitmap(fileName,bitmapType);
  }



Manager the_manager;
LogManager the_logManager;

Manager* Manager::Get() { return &the_manager; };

LogManager* Manager::GetLogManager() { return &the_logManager;}


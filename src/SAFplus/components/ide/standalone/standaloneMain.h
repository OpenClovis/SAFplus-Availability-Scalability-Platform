/***************************************************************
 * Name:      standaloneMain.h
 * Purpose:   Defines Application Frame
 * Author:    OpenClovis ()
 * Created:   2014-11-11
 * Copyright: OpenClovis (www.openclovis.com)
 * License:
 **************************************************************/

#ifndef STANDALONEMAIN_H
#define STANDALONEMAIN_H

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "standaloneApp.h"

class standaloneFrame: public wxFrame
{
    public:
        standaloneFrame(wxFrame *frame, const wxString& title);
        ~standaloneFrame();
    private:
        enum
        {
            idMenuQuit = 1000,
            idMenuAbout
        };
        void OnClose(wxCloseEvent& event);
        void OnQuit(wxCommandEvent& event);
        void OnAbout(wxCommandEvent& event);
        DECLARE_EVENT_TABLE()
};

class EditorBase : public wxPanel
{
public:
    DECLARE_EVENT_TABLE()

    EditorBase(wxWindow* parent, const wxString& filename);
    virtual ~EditorBase();
    virtual void SetTitle(const wxString& newTitle) {};

    wxString m_Shortname;
};

extern wxString dataFolder;
class ConfigManager
{
public:
static wxString& GetDataFolder(bool dunno) { return dataFolder; }
};

class cbConfigurationPanel:public wxPanel
{
    public:
};

class cbPlugin:public wxApp
  {
      public:

  bool IsAttached() { return m_IsAttached; }

  bool    m_IsAttached;
  };

class cbProject
  {
      public:
  };

enum ModuleType
  {
      SOMETHING=0
  };

enum
  {
      cgUnknown = 0
  };

class FileTreeData
  {
      public:
  };

class LogManager
  {
      public:

  };

class Manager
  {
  public:
    wxMenu* LoadMenu(wxString s, bool dunno) { return NULL; }
    static Manager* Get();
    LogManager* GetLogManager();
  };

wxBitmap cbLoadBitmap(const wxString &,  wxBitmapType bitmapType);

#endif // STANDALONEMAIN_H

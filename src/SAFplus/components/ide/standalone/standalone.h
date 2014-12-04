#pragma once

#if 0
#undef linux
#undef unix
#include <cbplugin.h> // for "class cbPlugin"
#include <editorbase.h>
#include <configmanager.h>
#else
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/xrc/xmlres.h>

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
extern wxString folder;
class ConfigManager
{
public:
static wxString& GetDataFolder(bool dunno) { return dataFolder; }
static wxString& GetFolder(bool dunno) { return folder; }
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

  virtual void OnAttach() {}
  virtual void OnRelease(bool appShutDown) {}
  friend class standaloneApp;

  virtual void BuildMenu(wxMenuBar* menuBar) = 0;
  };

class cbProject
  {
      public:
  };

enum ModuleType
  {
      mtEditorManager=1
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


extern cbProject theProject;

class ProjectManager
  {
     public:
     cbProject* GetActiveProject() { return &theProject; }
  };

extern ProjectManager theProjectManager;
extern wxFrame* theFrame;
extern wxApp* theApp;

class Manager
  {
  public:
    Manager(const wxString& resfile);
    wxMenu* LoadMenu(const wxString& s, bool dunno) { return wxXmlResource::Get()->LoadMenu(s); }
    static Manager* Get();
    LogManager*     GetLogManager();
    ProjectManager* GetProjectManager() const { return &theProjectManager; }
    wxFrame*  GetAppFrame()  const { return theFrame; }
    wxWindow* GetAppWindow() const { return theApp->GetTopWindow(); }

  };

extern Manager* theManager;

wxBitmap cbLoadBitmap(const wxString &,  wxBitmapType bitmapType);
#endif

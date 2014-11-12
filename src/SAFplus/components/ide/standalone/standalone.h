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

  virtual void OnAttach() {}
  virtual void OnRelease(bool appShutDown) {}
  friend class standaloneApp;
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
#endif

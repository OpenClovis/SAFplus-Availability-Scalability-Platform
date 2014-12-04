#ifndef STANDALONE
#include <logmanager.h>
#endif
#include <wx/artprov.h>
#include <wx/settings.h>
#include "resources/images/Tool.xpm"
#include "SAFplus7EditorPanel.h"
#include "SAFplus7ScrolledWindow.h"
#include <wx/splitter.h>
extern wxWindow* createPythonControlledWindow(wxWindow* parent,const char* module);

int wxIDShowProperties = wxNewId();

//Declare set editors
std::set<EditorBase *> SAFplus7EditorPanel::m_editors;
const wxString g_EditorModified = _T("*");

BEGIN_EVENT_TABLE(SAFplus7EditorPanel, EditorBase)
  EVT_IDLE(SAFplus7EditorPanel::OnIdle)
  EVT_MENU(wxID_NEW, SAFplus7EditorPanel::OnNew)
  EVT_MENU(wxIDShowProperties, SAFplus7EditorPanel::ShowProperties)
#if 0
  EVT_SASH_DRAGGED(ID_WINDOW_DETAILS, SAFplus7EditorPanel::OnSashDrag)
#endif
END_EVENT_TABLE()

SAFplus7EditorPanel::SAFplus7EditorPanel(wxWindow* parent, const wxString &editorTitle) : EditorBase(parent, editorTitle)
{
  m_title = editorTitle;
#ifndef STANDALONE
  SetTitle(editorTitle);
#endif // STANDALONE

  m_editors.insert( this );

    wxBoxSizer* bSizer = new wxBoxSizer( wxVERTICAL );

#if 0
    //Using Dockable
    details.Create(this,ID_WINDOW_DETAILS,wxDefaultPosition,wxSize(200,30),wxCLIP_CHILDREN | wxSW_3D,wxString::FromUTF8("SAFplusDetails"));
    details.SetDefaultSize(wxSize(120, 1000));
    details.SetOrientation(wxLAYOUT_VERTICAL);
    details.SetAlignment(wxLAYOUT_RIGHT);
    details.SetBackgroundColour(wxColour(0, 255, 0));
    details.SetSashVisible(wxSASH_RIGHT, true);
#endif

  // Create toolbar SAFplus entity
  m_designToolBar = new wxToolBar( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL );
  m_designToolBar->SetToolBitmapSize(wxSize(16, 16));

#ifndef STANDALONE
  const wxString &baseImagePath = ConfigManager::GetDataFolder(false);
  wxBitmap sgBitmap = cbLoadBitmap(baseImagePath + _T("/comp16.gif"), wxBITMAP_TYPE_ANY);
#else
  const wxString &baseImagePath = wxGetCwd() + wxT("/resources/images");
  wxBitmap sgBitmap = cbLoadBitmap(baseImagePath + _T("/comp16.gif"), wxBITMAP_TYPE_ANY);
  // wxBitmap(Tool_xpm);
#endif

  m_designToolBar->AddTool(wxID_NEW, wxT("New"), wxArtProvider::GetBitmap(wxART_NEW, wxART_MENU), wxT("New diagram"));
  m_designToolBar->AddTool(wxID_OPEN, wxT("Load"), wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_MENU), wxT("Open file..."));
  m_designToolBar->AddTool(wxID_SAVE, wxT("Save"), wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_MENU), wxT("Save file..."));
  m_designToolBar->AddSeparator();
  m_designToolBar->AddTool(wxID_PRINT, wxT("Print"), wxArtProvider::GetBitmap(wxART_PRINT, wxART_MENU), wxT("Print..."));
  m_designToolBar->AddTool(wxID_PREVIEW, wxT("Preview"), wxArtProvider::GetBitmap(wxART_FIND, wxART_MENU), wxT("Print preview..."));
  m_designToolBar->AddSeparator();
  m_designToolBar->AddTool(wxID_COPY, wxT("Copy"), wxArtProvider::GetBitmap(wxART_COPY, wxART_MENU), wxT("Copy to clipboard"));
  m_designToolBar->AddTool(wxID_CUT, wxT("Cut"), wxArtProvider::GetBitmap(wxART_CUT, wxART_MENU), wxT("Cut to clipboard"));
  m_designToolBar->AddTool(wxID_PASTE, wxT("Paste"), wxArtProvider::GetBitmap(wxART_PASTE, wxART_MENU), wxT("Paste from clipboard"));
  m_designToolBar->AddSeparator();
  m_designToolBar->AddTool(wxID_UNDO, wxT("Undo"), wxArtProvider::GetBitmap(wxART_UNDO, wxART_MENU), wxT("Undo"));
  m_designToolBar->AddTool(wxID_REDO, wxT("Redo"), wxArtProvider::GetBitmap(wxART_REDO, wxART_MENU), wxT("Redo"));
  m_designToolBar->AddSeparator();

  m_designToolBar->AddRadioTool(wxID_ANY, wxT("Tool"), wxBitmap(Tool_xpm), wxNullBitmap, wxT("Design tool"));
  //Get from SAFplusEntityDef.xml
  //Service

  m_designToolBar->AddRadioTool(wxID_ANY, wxT("Service Group"), sgBitmap, sgBitmap, wxT("Service Group [G]"));
  m_designToolBar->AddRadioTool(wxID_ANY, wxT("Service Instance"), sgBitmap, sgBitmap, wxT("Service Instance [I]"));
  m_designToolBar->AddRadioTool(wxID_ANY, wxT("Component SI"), sgBitmap, sgBitmap, wxT("Component SI [E]"));

  //Node
  m_designToolBar->AddRadioTool(wxID_ANY, wxT("Node"), sgBitmap, sgBitmap, wxT("Node [N]"));
  m_designToolBar->AddRadioTool(wxID_ANY, wxT("Service Unit"), sgBitmap, sgBitmap, wxT("Service Unit [U]"));
  m_designToolBar->AddRadioTool(wxID_ANY, wxT("SAF Component"), sgBitmap, sgBitmap, wxT("SAF Component [C]"));
  m_designToolBar->AddRadioTool(wxID_ANY, wxT("Non SAF Component"), sgBitmap, sgBitmap, wxT("Non SAF Component [W]"));

  m_designToolBar->AddSeparator();
  m_designToolBar->AddTool(wxIDShowProperties, wxT("Show Properties"), wxArtProvider::GetBitmap(wxART_REDO), wxT("Show Properties"));

  m_designToolBar->Realize();

  bSizer->Add( m_designToolBar, 0, wxEXPAND, 5 );

  wxSplitterWindow* sp = new wxSplitterWindow(this, -1);
  //m_paintArea = new SAFplus7ScrolledWindow(sp, wxID_ANY);
  m_paintArea = NULL;
  wxWindow* details = createPythonControlledWindow(sp,"entityDetailsDialog");  // new wxPanel(sp, wxID_ANY); // PythonWinTest(sp);
  wxWindow* uml = createPythonControlledWindow(sp,"umlEditor");  // new wxPanel(sp, wxID_ANY); // PythonWinTest(sp);

  //sp->SplitVertically(details, m_paintArea, GetClientSize().x/4);
  sp->SplitVertically(details, uml, GetClientSize().x/4);
  bSizer->Add(sp, 1, wxEXPAND, 5 );

#if 0
  wxFlexGridSizer* mainSizer = new wxFlexGridSizer( 2, 1 , 0, 0 );
  mainSizer->AddGrowableCol( 0 );
  mainSizer->AddGrowableRow( 0 );
  mainSizer->SetFlexibleDirection( wxBOTH );
  mainSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

  //mainSizer->Add( m_paintArea, 1, wxEXPAND | wxALL, 5);
  mainSizer->Add( PythonWinTest(this), 1, wxEXPAND | wxALL, 5);

  bSizer->Add( mainSizer, 1, wxEXPAND, 5 );

#if 0
  bSizer->Add( &details, 1, wxEXPAND| wxALL, 5);
#endif
#endif
  SetSizer( bSizer );


  Layout();
}


SAFplus7EditorPanel::~SAFplus7EditorPanel()
{
    //dtor
    m_editors.erase(this);
}

void SAFplus7EditorPanel::SetEditorTitle(const wxString& newTitle)
{
    if (m_isModified)
        SetTitle(g_EditorModified + newTitle);
    else
        SetTitle(newTitle);
}

void SAFplus7EditorPanel::closeAllEditors()
{
    for ( std::set<EditorBase*>::iterator i = m_editors.begin(); i != m_editors.end(); ++i )
    {
#ifndef STANDALONE
      Manager::Get()->GetEditorManager()->QueryClose(*i);
#endif
      (*i)->Close();
    }
}

bool SAFplus7EditorPanel::GetModified() const
{
  return m_isModified;
}

void SAFplus7EditorPanel::OnIdle(wxIdleEvent& event)
{
  if (m_paintArea) SetModified(m_paintArea->m_isDirty);
}

void SAFplus7EditorPanel::SetModified(bool modified)
{
    if (modified != m_isModified)
    {
      m_isModified = modified;
#ifndef STANDALONE
      SetEditorTitle(m_Shortname);
#endif // STANDALONE
    }
}

void SAFplus7EditorPanel::OnNew(wxCommandEvent &event)
{
  if(wxMessageBox(wxT("Current change will be lost. Do you want to proceed?"), wxT("SAFplus7 Design"), wxYES_NO | wxICON_QUESTION) == wxYES)
  {
    SetModified(false);
  }
}

void SAFplus7EditorPanel::ShowProperties(wxCommandEvent &event)
{
  static bool isShow = true;
  if (m_paintArea)
    {
    m_paintArea->m_mgr.GetPane(wxT("Properties")).Show(isShow = !isShow).Right().Layer(0).Row(0).Position(0);
    m_paintArea->m_mgr.Update();
    }
}

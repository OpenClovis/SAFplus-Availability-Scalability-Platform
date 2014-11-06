#include <logmanager.h>
#include <wx/artprov.h>
#include <wx/settings.h>
#include "resources/images/Tool.xpm"
#include "SAFplus7EditorPanel.h"

//Declare set editors
std::set<EditorBase *> SAFplus7EditorPanel::m_editors;
const wxString g_EditorModified = _T("*");

BEGIN_EVENT_TABLE(SAFplus7EditorPanel, EditorBase)
  //EVT_CLOSE(SAFplus7EditorPanel::OnClose)
  EVT_IDLE(SAFplus7EditorPanel::OnIdle)
  EVT_MENU(wxID_NEW, SAFplus7EditorPanel::OnNew)
END_EVENT_TABLE()

SAFplus7EditorPanel::SAFplus7EditorPanel(wxWindow* parent, const wxString &editorTitle) : EditorBase(parent, editorTitle)
{
  m_title = editorTitle;
  SetTitle(editorTitle);

  m_editors.insert( this );

  wxFlexGridSizer* mainSizer = new wxFlexGridSizer( 2, 1 , 0, 0 );
  mainSizer->AddGrowableCol( 0 );
  mainSizer->AddGrowableRow( 1 );
  mainSizer->SetFlexibleDirection( wxBOTH );
  mainSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

  // set some diagram manager properties if necessary...
  // set accepted shapes (accept only wxSFRectShape)
  m_diagramManager.ClearAcceptedShapes();
  m_diagramManager.AcceptShape(wxT("All"));

  // create shape canvas and associate it with shape manager
  m_pCanvas = new wxSFShapeCanvas(&m_diagramManager, this);
  // set some shape canvas properties if necessary...
  m_pCanvas->AddStyle(wxSFShapeCanvas::sfsGRID_SHOW);
  m_pCanvas->AddStyle(wxSFShapeCanvas::sfsGRID_USE);
  m_pCanvas->SetGridStyle(wxSHORT_DASH);

  // connect (some) shape canvas events
  m_pCanvas->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(SAFplus7EditorPanel::OnLeftClickCanvas), NULL, this);
  m_pCanvas->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(SAFplus7EditorPanel::OnRightClickCanvas), NULL, this);

  // Create toolbar SAFplus entity
  m_designToolBar = new wxToolBar( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL );
  m_designToolBar->SetToolBitmapSize(wxSize(16, 16));

  const wxString &baseImagePath = ConfigManager::GetDataFolder(false);

  wxBitmap sgBitmap = cbLoadBitmap(baseImagePath + _T("/comp16.gif"), wxBITMAP_TYPE_ANY);

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


  m_designToolBar->Realize();

  mainSizer->Add( m_designToolBar, 0, wxEXPAND, 5 );

  mainSizer->Add( m_pCanvas, 1, wxEXPAND, 0);

  SetSizer( mainSizer );
  Layout();
}

void SAFplus7EditorPanel::OnLeftClickCanvas(wxMouseEvent& event)
{
    // add new rectangular shape to the diagram ...
    wxSFShapeBase* pShape = m_diagramManager.AddShape(CLASSINFO(wxSFRectShape), event.GetPosition());
    // set some shape's properties...
    if(pShape)
    {
        // set accepted child shapes for the new shape
        pShape->AcceptChild(wxT("All"));
        // enable emmiting of shape events
        pShape->AddStyle( wxSFShapeBase::sfsEMIT_EVENTS );
    }
    event.Skip();
}

void SAFplus7EditorPanel::OnRightClickCanvas(wxMouseEvent& event)
{
    // ... and process standard canvas operations
    /* Load XRC */
    wxMenu *m_menu = Manager::Get()->LoadMenu(_T("safplus_design"),true);

    wxPoint point = event.GetPosition();
    point.y+=16;

    PopupMenu(m_menu, point);
    event.Skip();
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
      Manager::Get()->GetEditorManager()->QueryClose(*i);
      (*i)->Close();
    }
}

bool SAFplus7EditorPanel::GetModified() const
{
  return m_isModified;
}

void SAFplus7EditorPanel::OnIdle(wxIdleEvent& event)
{
  SetModified(m_diagramManager.IsModified());
}

void SAFplus7EditorPanel::SetModified(bool modified)
{
    if (modified != m_isModified)
    {
      m_isModified = modified;
      SetEditorTitle(m_Shortname);
    }
}

void SAFplus7EditorPanel::OnNew(wxCommandEvent &event)
{
  if(wxMessageBox(wxT("Current change will be lost. Do you want to proceed?"), wxT("SAFplus7 Design"), wxYES_NO | wxICON_QUESTION) == wxYES)
  {
    m_diagramManager.Clear();
    m_pCanvas->ClearCanvasHistory();
    m_pCanvas->SaveCanvasState();
    m_pCanvas->Refresh();
    m_diagramManager.SetModified(false);
    SetModified(false);
  }
}

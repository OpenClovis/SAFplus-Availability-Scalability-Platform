#ifndef STANDALONE
#include <logmanager.h>
#endif
#include <wx/artprov.h>
#include <wx/settings.h>
#include "resources/images/Tool.xpm"
#include "SAFplus7EditorPanel.h"
#include "SAFplus7ScrolledWindow.h"
#include <wx/splitter.h>
#include <boost/python.hpp>
#include "utils.h"

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

  Manager* mgr = Manager::Get();
  wxFrame* frm = mgr->GetAppFrame();
  wxMenuBar* mb = frm->GetMenuBar();
  wxStatusBar* sb = frm->GetStatusBar();
  boost::python::object model = loadModel("testModel.xml");

  ntbIdeEditor = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxSize(-1,-1), wxBK_DEFAULT);

  bSizer->Add(ntbIdeEditor, 1, wxALL|wxEXPAND, 5);

  pageUML = createChildPage(mb, sb, model, false);
  ntbIdeEditor->AddPage(pageUML, _("UML Editor"), false);

  pageInstance = createChildPage(mb, sb, model, true);
  ntbIdeEditor->AddPage(pageInstance, _("Instances Editor"), false);

  SetSizer( bSizer );
  Layout();
}

wxPanel *SAFplus7EditorPanel::createChildPage(wxMenuBar *mb, wxStatusBar *sb, boost::python::object& model, bool isinstance)
  {

    wxPanel *panel = new wxPanel(ntbIdeEditor, wxID_ANY, wxDefaultPosition, wxSize(-1,-1), wxTAB_TRAVERSAL);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    wxToolBar *tb = new wxToolBar(panel, wxID_ANY, wxDefaultPosition, wxSize(-1,-1), wxTB_FLAT);
    tb->SetToolBitmapSize(wxSize(16,16));
    sizer->Add(tb, 0, wxALL|wxEXPAND, 5);

    wxSplitterWindow* sp = new wxSplitterWindow(panel, -1);

    wxWindow* details = createPythonControlledWindow("entityDetailsDialog",sp,mb,tb,sb,model,isinstance);

    wxWindow* editor = NULL;
    if (isinstance)
      {
        editor = createPythonControlledWindow("instanceEditor",sp,mb,tb,sb,model,false);
      }
    else
      {
        editor = createPythonControlledWindow("umlEditor",sp,mb,tb,sb,model,false);
      }
    sp->SplitVertically(details, editor, 10);
    sizer->Add(sp, 1, wxEXPAND, 5 );
    tb->Realize();

    panel->SetSizer(sizer);
    return panel;
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
}

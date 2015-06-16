#ifndef SAFPLUS7EDITORPANEL_H
#define SAFPLUS7EDITORPANEL_H

#include <set>
#ifndef STANDALONE
#include <configmanager.h>
#include <editormanager.h>
#include <editorbase.h>
#else
#include "standalone/standaloneMain.h"
#endif // STANDALONE
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/toolbar.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <boost/python.hpp>


#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/hashmap.h>

#include "SAFplusScrolledWindow.h"

class SAFplusEditorPanel : public EditorBase
{
  public:
    SAFplusEditorPanel(wxWindow* parent, const wxString &dlgtitle = wxEmptyString, cbProject *prj = nullptr);
    virtual ~SAFplusEditorPanel();

    /** @brief Set the editor's title.
      *
      * @param newTitle The new title to set.
      */
    void SetEditorTitle(const wxString& newTitle);
    bool GetModified() const;
    void SetModified(bool modified = true);

    static std::set<EditorBase *> m_editors;
    static void closeAllEditors();

    void OnIdle(wxIdleEvent& event);
    void OnNew(wxCommandEvent &event);
    void OnSashDrag(wxSashEvent& event);

    void ShowProperties(wxCommandEvent &event);

    wxPanel *createChildPage(wxMenuBar *mb, wxStatusBar *sb, boost::python::object& model, bool isinstance);

    bool m_isModified;
    wxString m_title;
    wxWindow* m_parent;

    wxNotebook* ntbIdeEditor;
    wxPanel* pageUML;
    wxPanel* pageInstance;
    cbProject *project;

  protected:

  private:
    DECLARE_EVENT_TABLE();



};

#define ID_WINDOW_DETAILS       100

#endif // SAFPLUS7EDITORPANEL_H

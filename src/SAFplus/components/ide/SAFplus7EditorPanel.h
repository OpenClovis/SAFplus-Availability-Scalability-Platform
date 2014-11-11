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


#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/hashmap.h>

#include "SAFplus7ScrolledWindow.h"

class SAFplus7EditorPanel : public EditorBase
{
  public:
    SAFplus7EditorPanel(wxWindow* parent, const wxString &dlgtitle = wxEmptyString);
    virtual ~SAFplus7EditorPanel();

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

    wxToolBar* m_designToolBar;
    SAFplus7ScrolledWindow* m_paintArea;

    bool m_isModified;
    wxString m_title;
    wxWindow* m_parent;

  protected:

  private:
    DECLARE_EVENT_TABLE();



};

#endif // SAFPLUS7EDITORPANEL_H

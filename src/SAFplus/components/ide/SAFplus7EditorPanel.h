#ifndef SAFPLUS7EDITORPANEL_H
#define SAFPLUS7EDITORPANEL_H

#include <set>
#include <configmanager.h>
#include <editormanager.h>
#include <editorbase.h>
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

// add wxShapeFramework include file
#include "wx/wxsf/wxShapeFramework.h"

WX_DECLARE_HASH_MAP(wxEventType, wxString, wxIntegerHash, wxIntegerEqual, EventTypeMap);

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

    // declare event handlers for shape canvas
    void OnLeftClickCanvas(wxMouseEvent& event);
    void OnRightClickCanvas(wxMouseEvent& event);
    //void OnClose(wxCloseEvent& event);
    void OnIdle(wxIdleEvent& event);
    void OnNew(wxCommandEvent &event);

    wxToolBar* m_designToolBar;

    // create wxSF diagram manager
    wxSFDiagramManager m_diagramManager;
    // create pointer to wxSF shape
    // canvas
    wxSFShapeCanvas *m_pCanvas;
    // declare event handler for
    // wxSFShapeCanvas

    bool m_isModified;
    wxString m_title;

  protected:

  private:
    DECLARE_EVENT_TABLE();



};

#endif // SAFPLUS7EDITORPANEL_H

/***************************************************************
 * Name:      standaloneApp.cpp
 * Purpose:   Code for Application Class
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

#include "standaloneApp.h"
#include "standaloneMain.h"
#include "../SAFplus7IDE.h"
#include "../SAFplus7EditorPanel.h"
#include "../SAFplus7ScrolledWindow.h"


IMPLEMENT_APP(standaloneApp);

wxString rcfile(_T("SAFplus7IDE.xrc"));

bool standaloneApp::OnInit()
{
    theApp   = this;
    theManager = new Manager(rcfile);
    theFrame = new standaloneFrame(0L, _("wxWidgets Application Template"));
    cbPlugin* plugin = new SAFplus7IDE();
    //The callback calling by C::B -- NOT when there is not C::B (standalone)
    wxMenuBar* mb = theFrame->GetMenuBar();

    plugin->OnAttach();
    plugin->BuildMenu(mb);
    new SAFplus7EditorPanel(theFrame, _("title") );
    theFrame->Show();
    return true;
}


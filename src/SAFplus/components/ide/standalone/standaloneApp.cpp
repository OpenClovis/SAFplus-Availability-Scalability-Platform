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


IMPLEMENT_APP(standaloneApp);

bool standaloneApp::OnInit()
{
    wxXmlResource::Get()->InitAllHandlers();
    wxXmlResource::Get()->Load(_T("../resources/SAFplus7IDE.xrc"));

    standaloneFrame* frame = new standaloneFrame(0L, _("wxWidgets Application Template"));
    cbPlugin* plugin = new SAFplus7IDE();
    //The callback calling by C::B -- NOT when there is not C::B (standalone)
    plugin->OnAttach();
    plugin->BuildMenu(frame->m_menubar);
    frame->Show();
    return true;
}

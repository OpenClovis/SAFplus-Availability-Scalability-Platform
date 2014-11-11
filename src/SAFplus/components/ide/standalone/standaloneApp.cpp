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
    standaloneFrame* frame = new standaloneFrame(0L, _("wxWidgets Application Template"));
    cbPlugin* plugin = new SAFplus7IDE();
    plugin->OnAttach();
    frame->Show();
    return true;
}

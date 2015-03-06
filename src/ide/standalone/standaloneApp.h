/***************************************************************
 * Name:      standaloneApp.h
 * Purpose:   Defines Application Class
 * Author:    OpenClovis ()
 * Created:   2014-11-11
 * Copyright: OpenClovis (www.openclovis.com)
 * License:
 **************************************************************/

#ifndef STANDALONEAPP_H
#define STANDALONEAPP_H

#include <wx/app.h>
#include "standalone.h"

class standaloneApp : public wxApp
{
    public:
        virtual bool OnInit();
        cbPlugin* plugin;

        virtual void OnAttach() {}
        virtual void OnRelease(bool appShutDown) {}
};
#endif // STANDALONEAPP_H

#ifndef STANDALONE
#include <sdk.h> // Code::Blocks SDK
//cb header
#include <configurationpanel.h>
#include <logmanager.h>
#include <cbproject.h>
#include <manager.h>
#include <editormanager.h>
#include <configmanager.h>
#else // STANDALONE
#include "standalone/standaloneMain.h"
#endif

//wx header
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/ffile.h>
#include <wx/toolbar.h>
#include <wx/treectrl.h>
#include <wx/menu.h>

#include "SAFplus7IDE.h"
#include "SAFplus7EditorPanel.h"
#include <Python.h>
#include <boost/python.hpp>
namespace bpy = boost::python;

//work-around with python's bug LD_PRELOAD
#include <dlfcn.h>

#ifndef STANDALONE
// Register the plugin with Code::Blocks.
// We are using an anonymous namespace so we don't litter the global one.
namespace
{
PluginRegistrant<SAFplus7IDE> reg(_T("SAFplus7IDE"));
}
#endif

/*
Global define manager
*/
Manager    *m_manager = Manager::Get();
LogManager *m_log = Manager::Get()->GetLogManager();


/*
Binding wxWidget resource
*/
// Menu
int idMenuClusterDesignGUI = XRCID("idMenuClusterDesignGUI");
int idModuleYangParse = XRCID("idModuleYangParse");
int idModuleClusterDesignGUI = XRCID("idModuleClusterDesignGUI");

// Toolbar
int idToolbarClusterDesignGUI = XRCID("idToolbarClusterDesignGUI");

// events handling
BEGIN_EVENT_TABLE(SAFplus7IDE, cbPlugin)
    // add any events you want to handle here
    EVT_UPDATE_UI(idModuleYangParse, SAFplus7IDE::UpdateUI)

    EVT_MENU(idToolbarClusterDesignGUI, SAFplus7IDE::Action)
    EVT_MENU(idModuleClusterDesignGUI, SAFplus7IDE::Action)
    EVT_MENU(idMenuClusterDesignGUI, SAFplus7IDE::Action)
    EVT_MENU(idModuleYangParse, SAFplus7IDE::Action)

END_EVENT_TABLE()

const wxString g_editorTitle = _T("SAFplus Cluster Design GUI");

// constructor
SAFplus7IDE::SAFplus7IDE()
{
#ifndef STANDALONE
    // Make sure our resources are available.
    // In the generated boilerplate code we have no resources but when
    // we add some, it will be nice that this code is in place already ;)
    if(!Manager::LoadResource(_T("SAFplus7IDE.zip")))
    {
        NotifyMissingFile(_T("SAFplus7IDE.zip"));
    }
#endif
}

// destructor
SAFplus7IDE::~SAFplus7IDE()
{
}

#if 0
boost::python::dict globals()
   {
   boost::python::handle<> mainH(boost::python::borrowed(PyImport_GetModuleDict()));
   return boost::python::extract<boost::python::dict>(boost::python::object(mainH));
   }
#endif

char programName[80] = "SAFplusIDE";
void SAFplus7IDE::OnAttach()
{
    m_IsAttached = true;

    //work-around with python's bug LD_PRELOAD
    dlopen("libpython2.7.so", RTLD_LAZY | RTLD_GLOBAL);

    Py_SetProgramName(programName);
    Py_Initialize();
    //PyRun_SimpleString("print 'Embedded Python initialized'\n");
    //PyObject* pyobj = PyRun_String("({'foo':{}},{'bar':{}})",0,globals(),boost::python::dict());
    //boost::python::object o(boost::python::handle<>(pyobj));

    // http://www.boost.org/doc/libs/1_57_0/libs/python/doc/tutorial/doc/html/python/object.html
    boost::python::object obj = boost::python::eval("({'foo':1},{'bar':{}})");
    boost::python::object zero = obj[0];
    boost::python::object one = obj[1];
    int val = bpy::extract<int>(zero["foo"]);
    printf("test python %d\n", val);
}

void SAFplus7IDE::OnRelease(bool appShutDown)
{
    // do de-initialization for your plugin
    // if appShutDown is true, the plugin is unloaded because Code::Blocks is being shut down,
    // which means you must not use any of the SDK Managers
    // NOTE: after this function, the inherited member variable
    // m_IsAttached will be FALSE...
    //m_IsAttached = false;
    if (!appShutDown)
    {
      SAFplus7EditorPanel::closeAllEditors();
    }
    Py_Finalize();
}

int SAFplus7IDE::Configure()
{
#ifndef STANDALONE
    //create and display the configuration dialog for your plugin
    cbConfigurationDialog dlg(Manager::Get()->GetAppWindow(), wxID_ANY, _("Your dialog title"));
    cbConfigurationPanel* panel = GetConfigurationPanel(&dlg);
    if (panel)
    {
        dlg.AttachConfigurationPanel(panel);
        PlaceWindow(&dlg);
        return dlg.ShowModal() == wxID_OK ? 0 : -1;
    }
#endif
    return -1;
}

void SAFplus7IDE::BuildMenu(wxMenuBar* menuBar)
{
    if (!IsAttached())
        return;

    /* Load XRC */
    m_menu = m_manager->LoadMenu(_T("safplus_menu"),true);

    /* */
    int posInsert = 7;
    int posPluginMenu = menuBar->FindMenu(_("P&lugins"));
    if (posPluginMenu != wxNOT_FOUND)
    {
      posInsert = posPluginMenu;
    }
    menuBar->Insert(posInsert, m_menu, _("SAFpl&us"));
}

void SAFplus7IDE::BuildModuleMenu(const ModuleType type, wxMenu* menu, const FileTreeData* data)
{
    if (!IsAttached())
        return;
#ifndef STANDALONE
    if (type == mtEditorManager || (data && ((data->GetKind() == FileTreeData::ftdkProject) || (data->GetKind() == FileTreeData::ftdkFile))))
    {
      m_module_menu = m_manager->LoadMenu(_T("safplus_module_menu"),true);

      /* Attach to cb menu */
      menu->AppendSeparator();
      menu->Append(wxNewId(), _T("SAFplus"), m_module_menu);
    }
#endif // STANDALONE
}

bool SAFplus7IDE::BuildToolBar(wxToolBar* toolBar)
{
    if ( !IsAttached() || !toolBar )
        return false;
#ifndef STANDALONE
    m_toolbar = toolBar;

    /* Load XRC */
    m_manager->AddonToolBar(toolBar,_T("safplus_toolbar"));

    /* Refresh toolbar */
    toolBar->Realize();
    toolBar->SetInitialSize();
#endif
    // return true if you add toolbar items
    return true;
}

void SAFplus7IDE::UpdateUI(wxUpdateUIEvent& event)
{
#ifndef STANDALONE
    cbProject *prjActive = m_manager->GetProjectManager()->GetActiveProject();

    //Check to enable/disable yang parse menu
    wxTreeCtrl* tree = m_manager->GetProjectManager()->GetUI().GetTree();
    wxTreeItemId sel = m_manager->GetProjectManager()->GetUI().GetTreeSelection();
    FileTreeData* ftd = sel.IsOk() ? (FileTreeData*)tree->GetItemData(sel) : 0;
    wxMenu *module_menu = m_module_menu;
    if (module_menu)
    {
      if (ftd && ftd->GetKind() == FileTreeData::ftdkFile)
      {
        module_menu->Enable(idModuleYangParse, ftd->GetProjectFile()->file.GetExt().Matches(wxT("yang")));
      }
      else
      {
        module_menu->Enable(idModuleYangParse, false);
      }
    }
    wxToolBar* tbar = m_toolbar;
    if (tbar)
    {
      tbar->EnableTool(idToolbarClusterDesignGUI, prjActive);
    }
#endif
}

void SAFplus7IDE::Action(wxCommandEvent& event)
{
    #if 0 // Please use wxwindows 2.8 APIs or lower.  This is what can be installed automatically from Ubuntu.  If there is a very compelling reason to go higher let's talk about it.
    // load SAFplusEntityDef.xml and SAFplusAmf.yang
    wxString entity_contents;
    wxString safplus_amf_contents;

    wxFile entityDefFile(ConfigManager::GetDataFolder(false) + wxT("/SAFplusEntityDef.xml"));
    entityDefFile.ReadAll(&entity_contents);

    wxFile safplusAmfFile(ConfigManager::GetDataFolder(false) + wxT("/SAFplusAmf.yang"));
    safplusAmfFile.ReadAll(&safplus_amf_contents);
    #endif

    // Get selection project (singleton editor project)
    wxString projectName;

#ifndef STANDALONE
    wxTreeCtrl *tree = m_manager->GetProjectManager()->GetUI().GetTree();
    wxTreeItemId sel = m_manager->GetProjectManager()->GetUI().GetTreeSelection();
    FileTreeData *ftd = sel.IsOk() ? (FileTreeData*)tree->GetItemData(sel) : 0;

    if (event.GetId() == idModuleYangParse)
    {
      if (!ftd) return;

      /* TODO:
      Run yang parse on selection file and return PyObject -> extract
      */

      wxString yangFile = ftd->GetProjectFile()->file.GetFullPath();
      m_log->Log(yangFile + wxT("::") + ConfigManager::GetDataFolder(false));
      //setenv("PYTHONPATH", ConfigManager::GetDataFolder(false).mb_str(), 1);
      PyObject *pModule = PyImport_Import(PyString_FromString("yang"));
      if (pModule != NULL)
      {
        PyObject *pFunc = PyObject_GetAttrString(pModule, "go");
        Py_DECREF(pModule);

        if (pFunc != NULL)
        {
          //Setup arguments
          PyObject *args = PyTuple_New(2); //One is Respo dir, another is tuple yang files
          PyObject *tupleFiles = PyTuple_New(1); //Multi-selection on tree objects???
          PyTuple_SetItem(tupleFiles, 0, PyString_FromString(yangFile.mb_str()));

          PyTuple_SetItem(args, 0, PyString_FromString(ftd->GetProjectFile()->file.GetPath().mb_str()));
          PyTuple_SetItem(args, 1, tupleFiles);

          PyObject *returnValues = PyObject_CallObject(pFunc, args);
          Py_DECREF(pFunc);
          if (returnValues != NULL)
          {
            //Creating boost::python::object from PyObject*
            boost::python::object obj0(boost::python::handle<>(PyTuple_GetItem(returnValues, 0)));
            boost::python::object obj1(boost::python::handle<>(PyTuple_GetItem(returnValues, 1)));

            boost::python::dict d0 = bpy::extract<bpy::dict>(obj0);
            boost::python::dict d1 = bpy::extract<bpy::dict>(obj1);

            //Unit test
            std::string resultStr = boost::python::extract<std::string>(d1["Cluster"]["startupAssignmentDelay"]["help"]);
            m_log->Log(wxString::FromUTF8(resultStr.c_str()));
            Py_DECREF(returnValues);
          }
          else
          {
            m_log->Log(_T("Values empty!"));
          }
        }
        else
        {
          m_log->Log(_T("Could not get function!"));
        }
      }
      else
      {
        m_log->Log(_T("Could not load module! Please manual export PYTHONPATH environment."));
      }
    }
    else
    {
      if (ftd)
      {
        projectName = ftd->GetProject()->GetTitle();
      }
      else
      {
        projectName = m_manager->GetProjectManager()->GetActiveProject()->GetTitle();
      }
      wxString title(projectName + _T("::") + g_editorTitle);

      if (!m_manager->GetEditorManager()->IsOpen(title))
      {
        new SAFplus7EditorPanel((wxWindow*)m_manager->GetEditorManager()->GetNotebook(), title);
      }
    }
#endif
}

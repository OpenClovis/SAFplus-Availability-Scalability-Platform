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
#include "yangParser.h"
#include "utils.h"

extern wxWindow* PythonWinTest(wxWindow* parent);

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
int idMenuSAFplusPythonWinTest = XRCID("idMenuSAFplusPythonWinTest");

// Toolbar
int idToolbarClusterDesignGUI = XRCID("idToolbarClusterDesignGUI");

// events handling
BEGIN_EVENT_TABLE(SAFplus7IDE, cbPlugin)
    // add any events you want to handle here
    EVT_UPDATE_UI(idModuleYangParse, SAFplus7IDE::UpdateUI)
    EVT_UPDATE_UI(idMenuSAFplusPythonWinTest, SAFplus7IDE::UpdateUI)

    EVT_MENU(idToolbarClusterDesignGUI, SAFplus7IDE::Action)
    EVT_MENU(idModuleClusterDesignGUI, SAFplus7IDE::Action)
    EVT_MENU(idMenuClusterDesignGUI, SAFplus7IDE::Action)
    EVT_MENU(idModuleYangParse, SAFplus7IDE::Action)
    EVT_MENU(idMenuSAFplusPythonWinTest, SAFplus7IDE::PythonWinTest)

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

#ifndef STANDALONE
    //work-around with python's bug LD_PRELOAD
    dlopen("libpython2.7.so", RTLD_LAZY | RTLD_GLOBAL);
#endif

    Py_SetProgramName(programName);

#ifdef STANDALONE
    std::string pythonPathExt = "";
    char *curPythonPath = getenv("PYTHONPATH");
    char cwd[512] = {0};
    if (curPythonPath != NULL)
    {
      pythonPathExt.append(curPythonPath).append(":");
    }

    if (getcwd(cwd, 512) != NULL)
    {
      pythonPathExt.append(cwd);
    }
    setenv("PYTHONPATH", pythonPathExt.c_str(), 1);
#endif

    Py_Initialize();
    PyEval_InitThreads();
#if 0
    if ( ! wxPyCoreAPI_IMPORT() ) {
        wxLogError(wxT("***** Error importing the wxPython API! *****"));
        PyErr_Print();
        //Py_Finalize();
    }

    // Save the current Python thread state and release the
    // Global Interpreter Lock.
    m_mainTState = wxPyBeginAllowThreads();
#endif
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

#ifndef STANDALONE  // Inserting something into the plugin menu which does not exist in standalone
    /* */
    int posInsert = 7;
    int posPluginMenu = menuBar->FindMenu(_("P&lugins"));
    if (posPluginMenu != wxNOT_FOUND)
    {
      posInsert = posPluginMenu;
    }
    menuBar->Insert(posInsert, m_menu, _("SAFpl&us"));
#else
    if (!menuBar->Append(m_menu, _("SAFpl&us")))
    {
        printf("menu insert error!\n");
        assert(0);

    }
#endif
}

void SAFplus7IDE::BuildModuleMenu(const ModuleType type, wxMenu* menu, const FileTreeData* data)
{
    if (!IsAttached())
        return;
#ifndef STANDALONE
    if (type == mtEditorManager || (data && ((data->GetKind() == FileTreeData::ftdkProject) || (data->GetKind() == FileTreeData::ftdkFile))))
#endif
    {
      m_module_menu = m_manager->LoadMenu(_T("safplus_module_menu"),true);

      /* Attach to cb menu */
      menu->AppendSeparator();
      menu->Append(wxNewId(), _T("SAFplus"), m_module_menu);
    }

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
    cbProject *prjActive = m_manager->GetProjectManager()->GetActiveProject();

    //Check to enable/disable yang parse menu
    wxTreeCtrl* tree = m_manager->GetProjectManager()->GetUI().GetTree();
    wxTreeItemId sel = m_manager->GetProjectManager()->GetUI().GetTreeSelection();
    FileTreeData* ftd = sel.IsOk() ? (FileTreeData*)tree->GetItemData(sel) : 0;
    wxMenu *module_menu = m_module_menu;
    if (module_menu)
    {
    #if 0
      if (ftd && ftd->GetKind() == FileTreeData::ftdkFile)
      {
        module_menu->Enable(idModuleYangParse, ftd->GetProjectFile()->file.GetExt().Matches(wxT("yang")));
      }
      else
      {
        module_menu->Enable(idModuleYangParse, false);
      }
    #endif
    }
    wxToolBar* tbar = m_toolbar;
    if (tbar)
    {
     // tbar->EnableTool(idToolbarSAFplus7ClusterDesignGUI, prjActive);  // GAS freezes: illegal ID? the item is in the menubar not the toolbar
    }

}

void SAFplus7IDE::PythonWinTest(wxCommandEvent& event)
{
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

      try
      {
        std::vector<std::string> yangfiles;
        yangfiles.push_back(Utils::toString(ftd->GetProjectFile()->file.GetFullPath()));

        YangParser yangParser;
        bpy::tuple values = yangParser.parseFile(Utils::toString(ftd->GetProjectFile()->file.GetPath()), yangfiles);

        boost::python::dict ytypes = bpy::extract<bpy::dict>(values[0]);
        boost::python::dict yobjects = bpy::extract<bpy::dict>(values[1]);

        std::string resultStr = boost::python::extract<std::string>(yobjects["Cluster"]["startupAssignmentDelay"]["help"]);
        m_log->Log(wxString::FromUTF8(resultStr.c_str()));
      }
      catch(boost::python::error_already_set const &e)
      {
        //TODO: throw
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

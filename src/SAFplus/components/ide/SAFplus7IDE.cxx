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
#include <wx/fs_mem.h>
#include <wx/fs_arc.h>

#include "SAFplus7IDE.h"
#include "SAFplus7EditorPanel.h"
#include <Python.h>
#include <boost/python.hpp>
#include <boost/python/dict.hpp>
#include <vector>

using namespace std;
namespace bpy = boost::python;

//extern wxWindow* PythonWinTest(wxWindow* parent);

//work-around with python's bug LD_PRELOAD
#include <dlfcn.h>
#include "yangParser.h"
#include "utils.h"
#include "svgIcon.h"


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
int idModuleYangParse = XRCID("idModuleYangParse");
// Menu
int idMenuSAFplus7ClusterDesignGUI = XRCID("idMenuSAFplus7ClusterDesignGUI");
int idMenuSAFplusPythonWinTest = XRCID("idMenuSAFplusPythonWinTest");
int idMenuSAFplusYangTest = XRCID("idMenuSAFplusYangTest");
// Toolbar
int idToolbarSAFplus7ClusterDesignGUI = XRCID("idToolbarSAFplus7ClusterDesignGUI");

// events handling
BEGIN_EVENT_TABLE(SAFplus7IDE, cbPlugin)
    // add any events you want to handle here
    EVT_UPDATE_UI(idMenuSAFplus7ClusterDesignGUI, SAFplus7IDE::UpdateUI)
    EVT_UPDATE_UI(idMenuSAFplusPythonWinTest, SAFplus7IDE::UpdateUI)
    EVT_UPDATE_UI(idToolbarSAFplus7ClusterDesignGUI, SAFplus7IDE::UpdateUI)
    EVT_UPDATE_UI(idMenuSAFplusYangTest, SAFplus7IDE::UpdateUI)

    EVT_MENU(idMenuSAFplusPythonWinTest, SAFplus7IDE::PythonWinTest)
    EVT_MENU(idToolbarSAFplus7ClusterDesignGUI, SAFplus7IDE::Action)
    EVT_MENU(idMenuSAFplus7ClusterDesignGUI, SAFplus7IDE::Action)
    EVT_MENU(idModuleYangParse, SAFplus7IDE::Action)
    EVT_MENU(idMenuSAFplusYangTest, SAFplus7IDE::OnYangParse)

END_EVENT_TABLE()

const wxString g_editorTitle = _T("SAFplus Cluster Design GUI");

std::vector<wxString> SAFplus7IDE::extraFiles;

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

    std::string pythonPathExt = "";
    char *curPythonPath = getenv("PYTHONPATH");
    if (curPythonPath != NULL)
    {
      pythonPathExt.append(curPythonPath).append(":");
    }

#ifdef STANDALONE
    char cwd[512] = {0};
    if (getcwd(cwd, 512) != NULL)
    {
      pythonPathExt.append(cwd);
      pythonPathExt.append(":").append(cwd).append("/../");
    }
#else
    pythonPathExt.append(Utils::toString(ConfigManager::GetDataFolder(false)));

    //work-around with python's bug LD_PRELOAD
    dlopen("libpython2.7.so", RTLD_LAZY | RTLD_GLOBAL);
#endif
    setenv("PYTHONPATH", pythonPathExt.c_str(), 1);

    Py_SetProgramName(programName);
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

    //Extract some images/python script from resources (Extra file in manifest.xml is not enough)
    extractExtraFiles();
}

void SAFplus7IDE::extractExtraFiles()
{
#ifndef STANDALONE
    wxString resourceFilename = _T("memory:SAFplus7IDE.zip");
    wxString resourceImagesDir = ConfigManager::GetFolder(sdDataUser) + _T("/images/");

    wxFileSystem fsys;
    wxString fnd = fsys.FindFirst(resourceFilename + _T("#zip:images/*.svg"), wxFILE);
    while (!fnd.empty())
    {
      wxFileName fn(fnd);
      wxString outputFileName = resourceImagesDir + fn.GetFullName();

      // check if the destination file already exists
      if (wxFileExists(outputFileName) && !wxFile::Access(outputFileName, wxFile::write))
      {
          continue;
      }

      // make sure destination dir exists
      CreateDirRecursively(wxFileName(outputFileName).GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR));

      wxFile output(outputFileName, wxFile::write);

      wxFSFile* f = fsys.OpenFile(fnd);
      if (f)
      {
          // copy file
          wxInputStream* is = f->GetStream();
          char tmp[1025] = {};
          while (!is->Eof() && is->CanRead())
          {
              memset(tmp, 0, sizeof(tmp));
              is->Read(tmp, sizeof(tmp) - 1);
              output.Write(tmp, is->LastRead());
          }
          delete f;
          extraFiles.push_back(fn.GetFullName());
      }
      fnd = fsys.FindNext();
    }
#endif // STANDALONE
}

void SAFplus7IDE::cleanExtraFiles()
{
#ifndef STANDALONE
    wxString resourceImagesDir = ConfigManager::GetFolder(sdDataUser) + _T("/images/");
    for (std::vector<wxString>::iterator it = SAFplus7IDE::extraFiles.begin(); it != SAFplus7IDE::extraFiles.end(); ++it)
    {
      m_log->DebugLog(_T("Removed:") + resourceImagesDir + *it);
      wxRemoveFile(resourceImagesDir + *it);
    }
#endif
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
    //Cleanup extra files (images/python script)
    cleanExtraFiles();
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
#ifndef STANDALONE  // Inserting something into the plugin menu which does not exist in standalone
    int posPluginMenu = menuBar->FindMenu(_("P&lugins"));
    if (posPluginMenu != wxNOT_FOUND)
    {
      posInsert = posPluginMenu;
    }
#endif

    if (!menuBar->Append(m_menu, _("SAFpl&us")))
    {
        printf("menu insert error!\n");
        assert(0);

    }
}

void SAFplus7IDE::BuildModuleMenu(const ModuleType type, wxMenu* menu, const FileTreeData* data)
{
    if (!IsAttached())
        return;
#ifndef STANDALONE
    if (type == mtEditorManager || (data && data->GetKind() == FileTreeData::ftdkProject))
#endif
    {
      m_menu = m_manager->LoadMenu(_T("safplus_menu"),true);

      /* Attach to cb menu */
      menu->AppendSeparator();
      menu->Append(wxNewId(), _T("SAFplus7") ,m_menu);
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
    wxMenuBar* mbar = Manager::Get()->GetAppFrame()->GetMenuBar();
    if (mbar)
    {
      mbar->Enable(idMenuSAFplus7ClusterDesignGUI, prjActive);
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

void SAFplus7IDE::OnYangParse(wxCommandEvent &event)
{
  Manager* mgr = Manager::Get();
  wxFrame* frm = mgr->GetAppFrame();

  wxString yangFile = wxT("resources/SAFplusAmf.yang");
  try
  {
    std::vector<std::string> yangfiles;
    yangfiles.push_back(Utils::toString(yangFile));

    YangParser yangParser;
    bpy::tuple values = bpy::extract<bpy::tuple>(yangParser.parseFile(".", yangfiles));

    boost::python::dict ytypes = bpy::extract<bpy::dict>(values[0]);
    boost::python::dict yobjects = bpy::extract<bpy::dict>(values[1]);

    std::string resultStr = boost::python::extract<std::string>(yobjects["Cluster"]["startupAssignmentDelay"]["help"]);
#ifdef wxUSE_STATUSBAR
    frm->SetStatusText(wxString::FromUTF8(resultStr.c_str()));
#else
    printf("AA [%s]", resultStr.c_str());
#endif
  }
  catch(boost::python::error_already_set const &e)
  {
    // Parse and output the exception
    std::string perror_str = parse_python_exception();
    std::cout << "Traceback: "<< std::endl << perror_str << std::endl;
  }

  try
  {
    SvgIcon iconGen;
    RsvgHandle *icon_handle = rsvg_handle_new();

    /* build example entity configuration */
    bpy::dict compConfig;
    compConfig["name"] = "myName";
    compConfig["commandLine"] = "myCommandLine";

    /* Draw entity to screen */
    //iconGen.genSvgIcon(SVG_ICON_COMP, compConfig, &icon_handle);
    //m_paintPanel->m_paintArea->drawIcon(icon_handle, NULL);
  }
  catch(boost::python::error_already_set const &e)
  {
    // Parse and output the exception
    string perror_str = parse_python_exception();
    std::cout << "Traceback: "<< std::endl << perror_str << std::endl;
  }
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
    wxTreeCtrl* tree = m_manager->GetProjectManager()->GetUI().GetTree();
    wxTreeItemId sel = m_manager->GetProjectManager()->GetUI().GetTreeSelection();
    FileTreeData* ftd = sel.IsOk() ? (FileTreeData*)tree->GetItemData(sel) : 0;
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
#endif
}

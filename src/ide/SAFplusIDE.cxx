
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
#include <wx/fs_arc.h>
#include <wx/zipstrm.h>
#include <wx/wfstream.h>

#include "SAFplusIDE.h"
#include "SAFplusEditorPanel.h"
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
PluginRegistrant<SAFplusIDE> reg(_T("SAFplusIDE"));
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
int idMenuSAFplusClusterDesignGUI = XRCID("idMenuSAFplusClusterDesignGUI");
int idMenuSAFplusPythonWinTest = XRCID("idMenuSAFplusPythonWinTest");
int idMenuSAFplusYangTest = XRCID("idMenuSAFplusYangTest");
// Toolbar
int idToolbarSAFplusClusterDesignGUI = XRCID("idToolbarSAFplusClusterDesignGUI");

// events handling
BEGIN_EVENT_TABLE(SAFplusIDE, cbPlugin)
    // add any events you want to handle here
    EVT_UPDATE_UI(idMenuSAFplusClusterDesignGUI, SAFplusIDE::UpdateUI)
    EVT_UPDATE_UI(idMenuSAFplusPythonWinTest, SAFplusIDE::UpdateUI)
    EVT_UPDATE_UI(idToolbarSAFplusClusterDesignGUI, SAFplusIDE::UpdateUI)
    EVT_UPDATE_UI(idMenuSAFplusYangTest, SAFplusIDE::UpdateUI)

    EVT_MENU(idMenuSAFplusPythonWinTest, SAFplusIDE::PythonWinTest)
    EVT_MENU(idToolbarSAFplusClusterDesignGUI, SAFplusIDE::Action)
    EVT_MENU(idMenuSAFplusClusterDesignGUI, SAFplusIDE::Action)
    EVT_MENU(idModuleYangParse, SAFplusIDE::Action)
    EVT_MENU(idMenuSAFplusYangTest, SAFplusIDE::OnYangParse)

END_EVENT_TABLE()

const wxString g_editorTitle = _T("SAFplus Cluster Design GUI");

// constructor
SAFplusIDE::SAFplusIDE()
{
#ifndef STANDALONE
    // Make sure our resources are available.
    // In the generated boilerplate code we have no resources but when
    // we add some, it will be nice that this code is in place already ;)
    if(!Manager::LoadResource(_T("SAFplusIDE.zip")))
    {
        NotifyMissingFile(_T("SAFplusIDE.zip"));
    }
#endif
}

// destructor
SAFplusIDE::~SAFplusIDE()
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
void SAFplusIDE::OnAttach()
{
    m_IsAttached = true;

    std::string pythonPathExt = Utils::toString(ConfigManager::GetDataFolder(false) + _T("/helpers"));
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

    //work-around with python's bug LD_PRELOAD
    dlopen("libpython2.7.so", RTLD_LAZY | RTLD_GLOBAL);
#endif
    setenv("PYTHONPATH", pythonPathExt.c_str(), 1);
    wxLogDebug(_T("PYTHONPATH:") + pythonPathExt);

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

    //PyRun_SimpleString("print 'Embedded Python initialized'\n");
    //PyObject* pyobj = PyRun_String("({'foo':{}},{'bar':{}})",0,globals(),boost::python::dict());
    //boost::python::object o(boost::python::handle<>(pyobj));

    // http://www.boost.org/doc/libs/1_57_0/libs/python/doc/tutorial/doc/html/python/object.html
    boost::python::object obj = boost::python::eval("({'foo':1},{'bar':{}})");
    boost::python::object zero = obj[0];
    boost::python::object one = obj[1];
    int val = bpy::extract<int>(zero["foo"]);
    printf("test python %d\n", val);
#endif

    //Extract some images/python script from resources (Extra file in manifest.xml is not enough)
    extractExtraFiles();
}

bool SAFplusIDE::extractZipFile(const wxString& zipFile, const wxString& dstDir)
{
  bool ret = true;
  wxFileSystem fsys;

  wxString fnd = fsys.FindFirst(_T("memory:SAFplusIDE.zip#zip:") + zipFile, wxFILE);
  if (fnd.empty())
  {
    wxLogDebug(_T("Can not open file '") + zipFile + _T("'."));
    return false;
  }

  wxFSFile* f = fsys.OpenFile(fnd);
  std::auto_ptr<wxZipEntry> entry(new wxZipEntry());

  wxLogDebug(_T("SAFplusIDE::extractZipFile:") + zipFile + _T(" to ") + dstDir);
  do
  {
    wxZipInputStream zip(f->GetStream());
    while (entry.reset(zip.GetNextEntry()), entry.get() != NULL)
    {
      // access meta-data
      wxString name = entry->GetName();
      name = dstDir + wxFileName::GetPathSeparator() + name;

      // read 'zip' to access the entry's data
      if (entry->IsDir())
      {
        int perm = entry->GetMode();
        wxFileName::Mkdir(name, perm, wxPATH_MKDIR_FULL);
      }
      else
      {
        zip.OpenEntry(*entry.get());
        if (!zip.CanRead())
        {
          wxLogDebug(_T("Can not read zip entry '") + entry->GetName() + _T("'."));
          ret = false;
          break;
        }

        wxFileName fileName(name);

        // Check and create directory if not exists
        if (!fileName.DirExists())
        {
          wxFileName::Mkdir(fileName.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
        }

        // Write to file
        wxFileOutputStream file(name);

        if (!file)
        {
          wxLogDebug(_T("Can not create file '") + name + _T("'."));
          ret = false;
          break;
        }
        zip.Read(file);
      }
    }
  }
  while (false);

  return ret;
}

void SAFplusIDE::extractExtraFiles()
{
#ifndef STANDALONE
  wxString helperDir = ConfigManager::GetFolder(sdDataUser) + _T("/helpers");
  wxString wizardDir = ConfigManager::GetFolder(sdDataUser) + _T("/templates/wizard/SAFplusIDE");

  wxFileName::Mkdir(helperDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
  wxFileName::Mkdir(wizardDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

  extractZipFile(_T("helpers.zip"), helperDir);
  extractZipFile(_T("wizard.zip"), wizardDir);
#endif // STANDALONE
}

void SAFplusIDE::cleanExtraFiles()
{
#ifndef STANDALONE
  wxString helperDir = ConfigManager::GetFolder(sdDataUser) + _T("/helpers");
  wxString wizardDir = ConfigManager::GetFolder(sdDataUser) + _T("/templates/wizard/SAFplusIDE");

  wxLogDebug(_T("SAFplusIDE::Removed:") + helperDir);
  wxFileName::Rmdir(helperDir, wxPATH_MKDIR_FULL|wxPATH_RMDIR_RECURSIVE);
  wxLogDebug(_T("SAFplusIDE::Removed:") + wizardDir);
  wxFileName::Rmdir(wizardDir, wxPATH_MKDIR_FULL|wxPATH_RMDIR_RECURSIVE);
#endif
}

void SAFplusIDE::OnRelease(bool appShutDown)
{
    // do de-initialization for your plugin
    // if appShutDown is true, the plugin is unloaded because Code::Blocks is being shut down,
    // which means you must not use any of the SDK Managers
    // NOTE: after this function, the inherited member variable
    // m_IsAttached will be FALSE...
    //m_IsAttached = false;
    if (!appShutDown)
    {
      SAFplusEditorPanel::closeAllEditors();
    }
    //Cleanup extra files (images/python script)
    cleanExtraFiles();
    Py_Finalize();
}

int SAFplusIDE::Configure()
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

void SAFplusIDE::BuildMenu(wxMenuBar* menuBar)
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

void SAFplusIDE::BuildModuleMenu(const ModuleType type, wxMenu* menu, const FileTreeData* data)
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
      menu->Append(wxNewId(), _T("SAFplus") ,m_menu);
    }

}

bool SAFplusIDE::BuildToolBar(wxToolBar* toolBar)
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

void SAFplusIDE::UpdateUI(wxUpdateUIEvent& event)
{
    cbProject *prjActive = m_manager->GetProjectManager()->GetActiveProject();
    wxMenuBar* mbar = Manager::Get()->GetAppFrame()->GetMenuBar();
    if (mbar)
    {
      mbar->Enable(idMenuSAFplusClusterDesignGUI, prjActive);
    }
    wxToolBar* tbar = m_toolbar;
    if (tbar)
    {
     // tbar->EnableTool(idToolbarSAFplusClusterDesignGUI, prjActive);  // GAS freezes: illegal ID? the item is in the menubar not the toolbar
    }

}

void SAFplusIDE::PythonWinTest(wxCommandEvent& event)
{
}

void SAFplusIDE::OnYangParse(wxCommandEvent &event)
{
#ifdef UnitTest
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
#endif
  event.Skip();
}


void SAFplusIDE::Action(wxCommandEvent& event)
{
#ifdef UnitTest
    // Please use wxwindows 2.8 APIs or lower.  This is what can be installed automatically from Ubuntu.  If there is a very compelling reason to go higher let's talk about it.
    // load SAFplusEntityDef.xml and SAFplusAmf.yang
    wxString entity_contents;
    wxString safplus_amf_contents;

    wxFile entityDefFile(ConfigManager::GetDataFolder(false) + wxT("/SAFplusEntityDef.xml"));
    entityDefFile.ReadAll(&entity_contents);

    wxFile safplusAmfFile(ConfigManager::GetDataFolder(false) + wxT("/SAFplusAmf.yang"));
    safplusAmfFile.ReadAll(&safplus_amf_contents);
#endif

#ifndef STANDALONE
    wxTreeCtrl* tree = m_manager->GetProjectManager()->GetUI().GetTree();

    if (!tree)
      return;

    wxTreeItemId sel = m_manager->GetProjectManager()->GetUI().GetTreeSelection();

    if (!sel.IsOk())
      return;

    const FileTreeData *data = static_cast<FileTreeData*>( tree->GetItemData( sel ) );
    cbProject *prj;
    if (data )
    {
      prj = data->GetProject();
    }
    else
    {
      prj = m_manager->GetProjectManager()->GetActiveProject();
      if (!prj)
      {
        wxLogDebug(_T("There is no active project!"));
        return;
      }
    }
    wxString title(prj->GetTitle() + _T("::") + g_editorTitle);

    if (!m_manager->GetEditorManager()->IsOpen(title))
    {
      try
      {
        wxFileName modelFile(prj->GetCommonTopLevelPath() + _T("model.xml"));
        if (!modelFile.FileExists())
        {
          wxLogDebug(_T("There is no model.xml file existing at project!"));
          return;
        }
        new SAFplusEditorPanel((wxWindow*)m_manager->GetEditorManager()->GetNotebook(), title, prj);
      }
      catch(boost::python::error_already_set const &e)
      {
      }
    }

    event.Skip();
#endif
}

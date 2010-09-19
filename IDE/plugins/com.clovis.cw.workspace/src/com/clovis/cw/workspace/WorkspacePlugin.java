/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;
import java.util.MissingResourceException;
import java.util.ResourceBundle;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IWorkspace;
import org.eclipse.core.resources.IWorkspaceDescription;
import org.eclipse.core.resources.IWorkspaceRoot;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.FileLocator;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.internal.Workbench;
import org.eclipse.ui.plugin.AbstractUIPlugin;
import org.osgi.framework.BundleContext;

import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.snmp.MibFilesReader;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.natures.SystemProjectNature;

/**
 * The main plugin class to be used in the desktop.
 */
public class WorkspacePlugin extends AbstractUIPlugin
{
    //The shared instance.
    private static WorkspacePlugin plugin;
    //Resource bundle.
    private ResourceBundle _resourceBundle;

    public static Log LOG = null;
    private static String _mergeScript;
        
    static {
    	_mergeScript = "/usr/local/bin/clovismerge";
    }
    /**
     * The constructor.
     * @param descriptor - IPluginDescriptor
     */
    public WorkspacePlugin()
    {
        super();
        plugin = this;
        try {
        	_resourceBundle = ResourceBundle
                    .getBundle("com.clovis.cw.workspace.WorkspacePluginResources");
        	
        } catch (MissingResourceException x) {
            _resourceBundle = null;
        } 
        try {
            LOG = Log.getLog(WorkspacePlugin.getDefault());
            // Make Build automatically OFF
            IWorkspace ws = ResourcesPlugin.getWorkspace();
            IWorkspaceDescription desc = ws.getDescription();
            desc.setAutoBuilding(false);
            ws.setDescription(desc);
            if (Workbench.getInstance() != null) {
	            Shell shell = getDefault().getWorkbench().
	                getActiveWorkbenchWindow().getShell();
	            shell.addDisposeListener(new ShellDisposeListener());
            }
        } catch (CoreException e) {
            LOG.warn("Could not turn off build automatically", e);
        } catch (Exception e) {
        	LOG.error("Error in WorkspacePlugin constructor", e);
        }
    }

    /**
     * This method is called upon plug-in activation
     * @param context - BundleContext
     * @throws Exception -
     */
    public void start(BundleContext context) throws Exception
    {
        super.start(context);
    }

    /**
     * This method is called when the plug-in is stopped
     * @param context - BundleContext
     * @throws Exception -
     */
    public void stop(BundleContext context) throws Exception
    {
        super.stop(context);
    }

    /**
     * Returns the shared instance.
     * @return workspace plugin
     */
    public static WorkspacePlugin getDefault()
    {
        return plugin;
    }

    /**
     * @param key -String
     * @return the string from the plugin's resource bundle,
     * or 'key' if not found.
     */
    public static String getResourceString(String key)
    {
        ResourceBundle bundle = WorkspacePlugin.getDefault().
        getResourceBundle();
        try {
            return (bundle != null) ? bundle.getString(key) : key;
        } catch (MissingResourceException e) {
            return key;
        }
    }

    /**
     * @return the plugin's resource bundle,
     */
    public ResourceBundle getResourceBundle()
    {
        return _resourceBundle;
    }
    /**
     * @see java.lang.Object#toString()
     */
	public String toString()
	{
		return "com.clovis.cw.workspace";
	}
	
	public static String getMergeScriptFile() {
		return _mergeScript;
	}
     /**
    *
    * @author shubhada
    *Shell dispose listener class
    */
   private class ShellDisposeListener implements DisposeListener
   {
       /**
        * When widget disposed go thru all the PDM's and remove
        * dependency listener
        * @param event - DisposeEvent.
        */
       public void widgetDisposed(DisposeEvent event)
       {
           IWorkspaceRoot workspaceRoot = ResourcesPlugin.getWorkspace().getRoot();
           IProject [] projects = workspaceRoot.getProjects();
           for (int i = 0; i < projects.length; i++) {
				IProject project = projects[i];
				try {
					if (project
							.hasNature(SystemProjectNature.CLOVIS_SYSTEM_PROJECT_NATURE)) {
						ProjectDataModel pdm = ProjectDataModel
								.getProjectDataModel(project);
						pdm.removeDependencyListeners();
					}
				} catch (CoreException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
           
//         Write the present loaded mib files to the file in workspace dir.
           try {
               FileWriter mibfile  = new FileWriter(Platform.
                    getInstanceLocation().getURL().getPath().
                    concat("mibfilenames.txt"));
               ClovisNotifyingListImpl mibnames = MibFilesReader.
               getInstance().getFileNames();
               for (int i = 0; i < mibnames.size(); i++) {
                   mibfile.write((String) mibnames.get(i));
                   mibfile.write("\n");
               }
               mibfile.close();
               MibFilesReader.getInstance().removeListeners();
           } catch (IOException e) {
               LOG.error("MibFile cannot be written", e);
           }
           
       }
   }
   /**
    * Find and returns code generation options
    * @return String[]
    */
   public static String[] getCodeGenOptions() {
		URL codegenURL = FileLocator
				.find(getDefault().getBundle(), new Path(
						ICWProject.PROJECT_CODEGEN_FOLDER), null);
		File codegenFolder = null;
		try {
			codegenFolder = new Path(FileLocator.resolve(codegenURL).getPath())
					.toFile();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		File files[] = codegenFolder.listFiles();
		List cgOptions = new ArrayList();
		for (int i = 0; i < files.length; i++) {
			File file = files[i];
			if (file.isDirectory()
					&& !file.getName().startsWith(".")
					&& new File(file.getAbsolutePath() + File.separator
							+ "plugin.cfg").exists()) {
				cgOptions.add(file.getName());
			}
		}
		return (String[]) cgOptions.toArray(new String[0]);
	}
    /**
     * Check if the option is local codegen mode or remote mode
     * @param option codegen option
     * @return true if the codegen option is local or false
     */
    public static boolean isLocalCodeGenOption(String option) {
		URL codegenURL = FileLocator.find(getDefault().getBundle(), new Path(
				ICWProject.PROJECT_CODEGEN_FOLDER), null);
		File codegenFolder = null;
		try {
			codegenFolder = new Path(FileLocator.resolve(codegenURL).getPath())
					.toFile();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
    	return new File(codegenFolder.getAbsolutePath() + File.separator + option + File.separator
				+ "run.py").exists();
    }
}


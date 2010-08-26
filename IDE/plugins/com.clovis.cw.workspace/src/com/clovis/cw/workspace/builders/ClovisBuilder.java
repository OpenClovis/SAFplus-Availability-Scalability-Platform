/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/builders/ClovisBuilder.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.builders;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.net.URL;
import java.util.Map;

import org.eclipse.ant.core.AntCorePlugin;
import org.eclipse.ant.core.AntRunner;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IncrementalProjectBuilder;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;

import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.workspace.dialog.BuildConfigurationDialog;
import com.clovis.cw.workspace.project.CwProjectPropertyPage;

/**
 * @author Pushparaj
 * 
 * Clovis Builder.
 */
public class ClovisBuilder extends IncrementalProjectBuilder {
	
    /**
     * Get URL array of classpaths.
     * Takes classpath urls from Ant and adds current classpath to it
     * so that CodeGenBuilderLogger class can be loaded.
     * @return Array of Classpath URLs
     * @throws Exception If URL creation failes.
     */
    private URL[] getClassPathURL()
        throws Exception
    {
        URL[] urls = AntCorePlugin.getPlugin().getPreferences().getURLs();
        URL[] newUrls = new URL[urls.length + 1];
        System.arraycopy(urls, 0, newUrls, 1, urls.length);
        //Add the path to the Plugin classes
        String className = this.getClass().getName();
        if (!className.startsWith("/")) {
            className = "/" + className;
        }
        className = className.replace('.', '/');
        String classLoc = getClass().getClassLoader().
            getResource(className + ".class").toExternalForm();
        newUrls[0] = Platform.resolve(new URL(
                classLoc.substring(0, classLoc.indexOf(className))));
        return newUrls;
    }
	
	protected void clean(IProgressMonitor monitor) throws CoreException {
		IProject project = getProject();
		String projectAreaLocation = CwProjectPropertyPage.getProjectAreaLocation(project);
		if(projectAreaLocation.equals("")) {
			return;
		}
		AntRunner ant = new AntRunner();
		try {
			ant.setCustomClasspath(getClassPathURL());
			ant.addBuildLogger(ClovisCleanLogger.class.getName());
		} catch (Exception e) {
			ant.addBuildLogger("org.apache.tools.ant.DefaultLogger");
		}
		URL url = DataPlugin.getDefault().getBundle().getEntry("/");
		try {
			url = Platform.resolve(url);
		} catch (IOException e) {
			e.printStackTrace();
		}
		String codeGenMode = CwProjectPropertyPage.getCodeGenMode(project);
		String actionCommand = project.getLocation().append(codeGenMode + "_clean.sh").toOSString();
		String toolChainName = "local";
		if(codeGenMode.equals(ICWProject.PROJECT_DEFAULT_CODEGEN_OPTION)) {
		if(ClovisConfigurator.getCrossBuildMode(project).equals("true")) {
			toolChainName = ClovisConfigurator.getToolChainMode(project);
		}
		actionCommand = project.getLocation().append("clean.sh").toOSString();
		}
				
		StringBuffer buff = new StringBuffer("-Dprojectarea.loc=")
		.append(projectAreaLocation)
		.append(" -Dtoolchain.name=").append(toolChainName)
		.append(" -Dscript.file=").append(actionCommand)
		.append(" -Dproject.name=").append(project.getName());
		
		ant.setBuildFileLocation(project.getLocation().append("clean.xml")
				.toOSString());
		ant.setArguments(buff.toString());
		ant.setMessageOutputLevel(org.apache.tools.ant.Project.MSG_INFO);

		AntCleanThread thread = new AntCleanThread(ant, project, actionCommand);
		Display.getDefault().syncExec(thread);
	}

	/**
	 * Generate Code from Model
	 * 
	 * @param kind
	 *            Build Type
	 * @param args
	 *            Map
	 * @param monitor
	 *            Progress Monitor.
	 */
	protected IProject[] build(int kind, Map args, IProgressMonitor monitor)
			throws CoreException
	{
		IProject project = getProject();
		String projectAreaLocation = CwProjectPropertyPage.getProjectAreaLocation(project);
		String sdkLocation = CwProjectPropertyPage.getSDKLocation(project);
		String message = "";
		
		// check to see if the Project Area location is configured properly
		if (projectAreaLocation.equals("")) {
			message = message + "Project Area location is not set on Project [" + project.getName()
							+ "]. Use Project->Right Click->properties->Clovis System Project to set its value.\n\n";
		} else if (! isProjectAreaValid(projectAreaLocation)) {
			message = message + "The Project Area specified for this project [" + projectAreaLocation + "] has an"
						   + " invalid configuration. Please check the .config file in this project area. You may"
						   + " need to configure this project area again.\n\n";
		}
		
		// check to see if the SDK location is configured properly
		if(sdkLocation.equals("")) {
			message = message + "SDK location is not set on Project [" + project.getName()
			+ "]. Use Project->Right Click->properties->Clovis System Project to set its value.\n\n";
		} else if (!new File(sdkLocation + File.separator + "src"
				+ File.separator + "ASP" + File.separator + "configure")
				.exists()) {
			message = message + "The SDK location specified for this project [" + sdkLocation + "] is not valid."
			   + " Please check the ASP sources in this Location. You may need to configure this location again.";
		}
		if(!message.equals("")) {
			Display.getDefault().syncExec(new ErrorDialogThread(project.getName(), message));
			forgetLastBuiltState();
			return null;
		}
		
		// set up for the build
		String errMsg = null;
		AntRunner ant = new AntRunner();
		try {
			ant.setCustomClasspath(getClassPathURL());
			ant.addBuildLogger(ClovisBuilderLogger.class.getName());
		} catch (Exception e) {
			ant.addBuildLogger("org.apache.tools.ant.DefaultLogger");
		}
		URL url = DataPlugin.getDefault().getBundle().getEntry("/");
		try {
			url = Platform.resolve(url);
		} catch (IOException e) {
			e.printStackTrace();
		}
		String toolChainName = "local";
		String codeGenMode = CwProjectPropertyPage.getCodeGenMode(project);
		String actionCommand = project.getLocation().append(codeGenMode + "_build.sh").toOSString();
		if(codeGenMode.equals(ICWProject.PROJECT_DEFAULT_CODEGEN_OPTION)) {
		actionCommand = project.getLocation().append("build.sh").toOSString();	
		// Gather the build configuration settings
		ConfigDialogThread configThread = new ConfigDialogThread(project);
		Display.getDefault().syncExec(configThread);
		if (!configThread._status) {
			forgetLastBuiltState();
			return null;
		}
		
		// run the configuration
		ClovisConfigurator configurator = new ClovisConfigurator();
		if (!configurator.configureForBuild(project))
		{
			forgetLastBuiltState();
			return null;
		}
		
		if(ClovisConfigurator.getCrossBuildMode(project).equals("true")) {
			toolChainName = ClovisConfigurator.getToolChainMode(project);
		}
		}
				
		StringBuffer buff = new StringBuffer("-Dprojectarea.loc=").append(projectAreaLocation)
				.append(" -Dtoolchain.name=").append(toolChainName)
				.append(" -Dscript.file=").append(actionCommand)
				.append(" -Dproject.name=").append(project.getName());

		ant.setBuildFileLocation(project.getLocation().append("build.xml")
				.toOSString());
		ant.setArguments(buff.toString());
		ant.setMessageOutputLevel(org.apache.tools.ant.Project.MSG_INFO);

		AntBuildThread thread = new AntBuildThread(ant, project, actionCommand);
		Display.getDefault().syncExec(thread);

		if (errMsg != null) {
			Display.getDefault().syncExec(new ErrorDialogThread(project.getName(), errMsg));
			return null;
		}
		
		forgetLastBuiltState();
		return null;
	}
	
	/**
	 * Given a project area path this method interogates the .config file in that
	 *  project area and validates that the CL_SDKDIR and CL_BUILDTOOLS settings
	 *  point to valid locations.
	 * @param location
	 * @return
	 */
	private boolean isProjectAreaValid(String locationPath)
	{
		boolean isValid = true;
		
		try
		{
			FileReader input = new FileReader(locationPath + File.separator + ".config");
			BufferedReader bufRead = new BufferedReader(input);
			String line = bufRead.readLine();
			while (line != null)
			{
				if (line.trim().toUpperCase().startsWith("CL_SDKDIR"))
				{
					String rootPath = line.substring(line.indexOf("=") + 1);
					File file = new File(rootPath + File.separator + "src" + File.separator + "ASP" + File.separator + "configure");
					if (! file.exists()) isValid = false;
				} else if (line.trim().toUpperCase().startsWith("CL_BUILDTOOLS")) {
					String rootPath = line.substring(line.indexOf("=") + 1);
					File file = new File(rootPath + File.separator + "local");
					if (! file.exists()) isValid = false;
				}
				line = bufRead.readLine();
			}
			bufRead.close();
			
		} catch (FileNotFoundException e) {
			//file not found is ok...config file will be created correctly during configuration
		} catch (IOException e) {
			isValid = false;
		}
		
		return isValid;
	}

	class AntBuildThread implements Runnable {
		
		String _actionCommand;
		AntRunner ant = null;
		IProject project = null;

		public AntBuildThread(AntRunner ant, IProject project, String actionCommand)
		{
			_actionCommand = actionCommand;
			this.ant = ant;
			this.project = project;
		}

		public void run() {
			try {
				ClovisProgressMonitorDialog pmDialog = null;
				pmDialog = new ClovisProgressMonitorDialog(
						Display.getDefault().getActiveShell(), _actionCommand);
				pmDialog.run(true, true, new RunnableCode(ant, project, "Building source code for : "));
			} catch (InterruptedException ie) {
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
	}
	class RunnableCode implements IRunnableWithProgress, Runnable {

		IProject project = null;
		AntRunner ant = null;
		String _monitorCaption = "";

		RunnableCode(AntRunner ant, IProject project, String monitorCaption) {
			this.ant = ant;
			this.project = project;
			_monitorCaption = monitorCaption;
		}

		public void run(IProgressMonitor monitor)
				throws InvocationTargetException, InterruptedException {
			try {

				if (monitor.isCanceled()) {
					monitor.done();
					return;
				}
				monitor.beginTask(_monitorCaption + project.getName(), IProgressMonitor.UNKNOWN);
				ant.run(monitor);
			} catch (Exception e) {
				e.printStackTrace();
			}
		}

		public void run() {
			// TODO Auto-generated method stub
		}
		
	}

	/**
	 * Thread for running the configuration dialog.
	 * 
	 * @author matt
	 */
	class ConfigDialogThread implements Runnable {
		public boolean _status = false;

		public IProject _project = null;

		public ConfigDialogThread(IProject project) {
			_project = project;
		}

		public void run() {

			BuildConfigurationDialog dialog = new BuildConfigurationDialog(new Shell(), getProject());
			int returnCode = dialog.open();
			if (returnCode == Window.OK)
			{
				_status = true;
			}

		}
	}

	class AntCleanThread implements Runnable {

		String _actionCommand;
		AntRunner ant = null;
		IProject project = null;

		public AntCleanThread(AntRunner ant, IProject project, String actionCommand)
		{
			_actionCommand = actionCommand;
			this.ant = ant;
			this.project = project;
		}

		public void run()
		{
			try {
				ClovisProgressMonitorDialog pmDialog = null;
				pmDialog = new ClovisProgressMonitorDialog(
						Display.getDefault().getActiveShell(), _actionCommand);
				pmDialog.run(true, true, new RunnableCode(ant, project, "Cleaning source code for : "));
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
	}
	
	class ErrorDialogThread implements Runnable {
		
		String prjName, message;
		public ErrorDialogThread(String prjName, String message) {
			this.prjName = prjName;
			this.message = message;
		}

		public void run() {
			MessageDialog.openError(new Shell(), "Project settings errors for " + prjName, message);
		}
		
	}
}

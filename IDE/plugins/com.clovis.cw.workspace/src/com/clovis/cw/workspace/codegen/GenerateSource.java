/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/Ganga/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/codegen/GenerateSource.java $
 * $Author: pushparaj $
 * $Date: 2007/05/18 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.codegen;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FilenameFilter;
import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.net.URL;
import java.text.Collator;
import java.util.Arrays;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.StringTokenizer;

import org.eclipse.ant.core.AntCorePlugin;
import org.eclipse.ant.core.AntRunner;
import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IFolder;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.impl.EEnumLiteralImpl;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;

import com.clovis.common.utils.ClovisFileUtils;
import com.clovis.common.utils.UtilsPlugin;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.constants.SnmpConstants;
import com.clovis.cw.editor.ca.snmp.ClovisMibUtils;
import com.clovis.cw.licensing.ILicensingConstants;
import com.clovis.cw.licensing.LicenseInfo;
import com.clovis.cw.licensing.UserInfo;
import com.clovis.cw.licensing.dialog.LicenseWarningDialog;
import com.clovis.cw.licensing.dialog.LoginDialog;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.ProblemsView;
import com.clovis.cw.workspace.WorkspacePlugin;
import com.clovis.cw.workspace.builders.ClovisProgressMonitorDialog;
import com.clovis.cw.workspace.dialog.MergeDialog;
import com.clovis.cw.workspace.project.CwProjectPropertyPage;
import com.clovis.cw.workspace.project.ProjectValidatorThread;
import com.clovis.cw.workspace.utils.MergeUtils;

/**
 * @author Pushparaj
 * 
 * Class for generation of Code.
 */
public class GenerateSource {
	
	private Shell _shell;
	
	public GenerateSource(Shell shell) {
		_shell = shell;
	}

	public GenerateSource(){
		//Added for command line tool
	}
	/**
	 * Reads name Map File.
	 * @param project IProject
	 * @return Map which contains old and new file names 
	 * for user modifiable files
	 */
	private Map getFileNamesMap(IProject project) {
		Map fileNamesMap = new HashMap();
		/*NOTE : This method should be implemented for finding old new name mapping **/
		
		File mapFile = new File(project.getLocation().append(".nameMapFile")
				.toOSString());
		if (mapFile.exists()) {
			try {
				BufferedReader reader = new BufferedReader(new FileReader(
						mapFile));
				String line = null;
				while ((line = reader.readLine()) != null) {
					StringTokenizer tokenizer = new StringTokenizer(line, ",");
					String oldName = File.separator + tokenizer.nextToken();
					String newName = File.separator + tokenizer.nextToken();
					fileNamesMap.put(newName, oldName);
				}
			} catch (FileNotFoundException e) {
				e.printStackTrace();
				return fileNamesMap;
			} catch (IOException e) {
				e.printStackTrace();
				return fileNamesMap;
			}
		}
		return fileNamesMap;
	}
	/**
	 * Generate source for projects
	 * @param projects projects list
	 */
	public void generateSource(IProject[] projects)
	{
		IWorkbenchPage page = WorkspacePlugin.getDefault().getWorkbench()
    		.getActiveWorkbenchWindow().getActivePage();
		ProblemsView problemsView = null;
		if (page != null) {
			try {
				problemsView = (ProblemsView) page.showView("com.clovis.cw.workspace.problemsView");
			} catch (PartInitException e) {
				e.printStackTrace();
			}
		}
		for (int i = 0; i < projects.length; i++) {
			IProject project = projects[i];
			ProjectDataModel projectModel = ProjectDataModel
					.getProjectDataModel(project);
				
			String location = CwProjectPropertyPage.getSDKLocation(project);
			
			String pythonlocation = CwProjectPropertyPage.getPythonLocation(project);
			
			String sourceLocation = CwProjectPropertyPage.getSourceLocation(project);
			
			String codeGenMode = CwProjectPropertyPage.getCodeGenMode(project);
			
			String errMsg = validateProjectProperties(project, codeGenMode, location, sourceLocation, pythonlocation);
			if (errMsg != null && !errMsg.equals("")) {
				MessageDialog.openError(new Shell(), "Project settings errors for " + project.getName(), errMsg);
				continue;
			}
			if(!codeGenMode.equals(ICWProject.CLOVIS_CODEGEN_OPTION)) {
				LicenseInfo licInfo = com.clovis.cw.licensing.Activator.getLicenseInfo();
				UserInfo userInfo = com.clovis.cw.licensing.Activator.getUserInfo();
				String loginName = userInfo.getLoginName();
				String passwd = userInfo.getPassword();
				String licType = licInfo.getLicenseType();
				RemoteCodeGeneration codegen = new RemoteCodeGeneration();
				if(userInfo.isVerifiedUser() && !loginName.equals("") && !passwd.equals("")) {
					if(WorkspacePlugin.isLocalCodeGenOption(codeGenMode)) {
						//Normal code generation
					} else {
						if(LicenseWarningDialog.getRetVal() != LicenseWarningDialog.RET_ALWAYS_OK) {
							LicenseWarningDialog dialog = new LicenseWarningDialog(_shell);
							dialog.open();
							if(LicenseWarningDialog.getRetVal() != LicenseWarningDialog.RET_CANCEL) {
								codegen.generateSource(project, codeGenMode, userInfo.getLoginName(), userInfo.getPassword());
							}
						} else {
							codegen.generateSource(project, codeGenMode, userInfo.getLoginName(), userInfo.getPassword());
						}
						continue;
					}
				} else {
					if (WorkspacePlugin.isLocalCodeGenOption(codeGenMode)) {
						// Normal code generation
					} else {
						LoginDialog dialog = new LoginDialog(_shell);
						if (dialog.open() == 0
								&& LicenseWarningDialog.getRetVal() != LicenseWarningDialog.RET_CANCEL) {
							codegen.generateSource(project, codeGenMode,
									userInfo.getLoginName(), userInfo
											.getPassword());
						}
						continue;
					}
				}
				try {
					project.refreshLocal(IResource.DEPTH_INFINITE, null);
				} catch (CoreException e) {
					e.printStackTrace();
				}
			}

			URL url = DataPlugin.getDefault().getBundle().getEntry("/");
			try {
				url = Platform.resolve(url);
			} catch (IOException e) {
			}

			String dataTypeXMILocation = url.getFile()
					+ ICWProject.PLUGIN_XML_FOLDER + File.separator
					+ SnmpConstants.DATA_TYPES_XMI_FILE_NAME;
							
				File srcFolder = new File(sourceLocation);
				if (!srcFolder.exists()) {
					srcFolder.mkdirs();
				} 
				File linkFile = project.getLocation().append(
						ICWProject.CW_PROJECT_SRC_DIR_NAME).toFile();
				if (!linkFile.getAbsolutePath().equals(
						srcFolder.getAbsolutePath())) {
					if (linkFile.exists()) {

						try {
							String linkPath = linkFile.getCanonicalPath();
							String filePath = linkFile.getAbsolutePath();
							if (linkPath.equals(filePath)) {
								String message = "Your existing generated code is in workspace. It should be in project area location. If you wants to reuse the code, please move 'src' folder from  '"
										+ project.getLocation().toOSString()
										+ "'  to  '"
										+ CwProjectPropertyPage
												.getProjectAreaLocation(project)
										+ File.separator
										+ project.getName()
										+ "'  and try again. If you don't want to reuse the existing code just 'Continue'.";
								int result = new MessageDialog(new Shell(),
										"Conflicts in existing generated code",
										null, message, MessageDialog.QUESTION,
										new String[] { "Continue", "Cancel" },
										1).open();
								if (result == 1)
									return;
								project.getFolder(
										ICWProject.CW_PROJECT_SRC_DIR_NAME)
										.delete(true, null);
							} else if (!linkPath.equals(sourceLocation)) {
								String message = "Your existing generated code is in different project area location. If you wants to reuse the code, please change the project area location as   '"
										+ linkFile.getCanonicalFile()
												.getParentFile()
												.getParentFile()
												.getAbsolutePath()
										+ "'  using Project->Properties menu. If you don't want to use the existing code just 'Continue'.";
								int result = new MessageDialog(new Shell(),
										"Conflicts in project area location",
										null, message, MessageDialog.QUESTION,
										new String[] { "Continue", "Cancel" },
										1).open();
								if (result == 1)
									return;
								project.getFolder(
										ICWProject.CW_PROJECT_SRC_DIR_NAME)
										.delete(true, null);
							}
						} catch (IOException e2) {
							e2.printStackTrace();
						} catch (CoreException e) {
							e.printStackTrace();
						}
					}

					if (!linkFile.exists()) {
						ClovisFileUtils.createRelativeSourceLink(
								sourceLocation, linkFile.getAbsolutePath());
					}
				}
				
				IFolder idlFolder = project.getFolder(ICWProject.CW_PROJECT_IDL_DIR_NAME);
				if(!idlFolder.exists()) {
					try {
						idlFolder.create(true, true, null);
					} catch (CoreException e) {
						e.printStackTrace();
					}
				}
				IFolder nextGenFolder = project.getFolder(".ngc");
				if(nextGenFolder.exists()) {
					try {
						nextGenFolder.delete(true, false, null);
					} catch (CoreException e) {
						e.printStackTrace();
					}
				}
				try {
					nextGenFolder.create(true, true, null);
				} catch (CoreException e1) {
					e1.printStackTrace();
				}
				ProjectValidatorThread validatorThread = new ProjectValidatorThread(project);
				Display.getDefault().syncExec(validatorThread);
				List problems = validatorThread.getProblems();
				if(problemsView != null) {
					problemsView.addProblems(problems);
					problemsView.updateColorsForProblems();
				}

				if (hasProblems(problems)) {
					boolean status = MessageDialog.openConfirm(
							new Shell(),
							"Project Validation",
							"'" + project.getName() + "'"
							+ " model has errors/warnings. Do you want to continue?");
					if (!status) {
						return;
					}
				}
				
				String autoBackupMode = new String();

				// check if we need to backup existing source files as part of code generation
				int backupMode = backupSourceCodeFiles(project);
				if (backupMode == SourceBackupDialog.RET_CANCEL) return;
				if (backupMode == SourceBackupDialog.RET_YES) autoBackupMode = "true";
				if (backupMode == SourceBackupDialog.RET_NO) autoBackupMode = "false";

				try {
					runAntBuilder(project, location, pythonlocation, dataTypeXMILocation, autoBackupMode,
							codeGenMode, CwProjectPropertyPage.getAlwaysOverrideMode(project),
							false);
				} catch (Exception e) {
					e.printStackTrace();
				}

				// if src was backed up limit the number of backups to the number
				// of backups set for the project
				if (backupMode == SourceBackupDialog.RET_YES)
				{
					File backupDir = new File(project.getLocation().toOSString() + File.separator + "src.bak");
					if (backupDir.exists())
					{
						File[] backups = backupDir.listFiles(new TarGzFileFilter());
						int maxBackups = CwProjectPropertyPage.getNumBackups(project);
						if (backups != null && backups.length > maxBackups)
						{
							Arrays.sort(backups, new ReverseFilenameComparator());
							for (int j=maxBackups; j<backups.length; j++)
							{
								backups[j].delete();
							}
						}
					}
				}

				if(!CwProjectPropertyPage.getAlwaysOverrideMode(project)) {
					mergeGeneratedCode(project);
				}
				projectModel.getTrackingModel().clearAllLists();
				projectModel.getTrackingModel().save(false);
				try {
					// since we may have added files to the project through the file system we need to
					//  make sure that the project knows about them or actions like copy won't work
					project.setLocal(true, IResource.DEPTH_INFINITE, null);
					project.refreshLocal(IResource.DEPTH_INFINITE, null);
				} catch (CoreException e) {
					e.printStackTrace();
				}
		}
	}
	/**
	 * Validate Project Properties
	 * @param project IProject
	 * @param codeGenMode Code gen mode
	 * @param location SDK location
	 * @param sourceLocation ProjectArea Location
	 * @param pythonLocation Python Location
	 * @return Error message
	 */
	private String validateProjectProperties(IProject project, String codeGenMode, String location, String sourceLocation, String pythonLocation) {
		String errMsg = "";
		if (codeGenMode.equals(ICWProject.CLOVIS_CODEGEN_OPTION)) {
			// -------- error message if SDK location is null
			if (location.equals("")) {
				errMsg += "\n SDK location is not set on Project ["
						+ project.getName()
						+ "]. Use Project->Right Click->properties->Clovis System Project to set its value.\n";
			}
		}
		// -------- error message if Project Area location is null
		if (sourceLocation.equals("")) {
			errMsg += "\n Project Area location is not set on Project ["
					+ project.getName()
					+ "]. Use Project->Right Click->properties->Clovis System Project to set its value.\n";
		}
		// -------- error message if python location is null
		if (pythonLocation.equals("")) {
			errMsg += "\n Python location is not set on Project ["
					+ project.getName()
					+ "]. Use Project->Right Click->properties->Clovis System Project to set its value.";
		} 
		return errMsg;
	}
	/*
	 * Check whether or not the files in the ASP src directory should be backed up
	 *  before code generation.
	 */
	private int backupSourceCodeFiles(IProject project)
	{
		// if the project properties are set to always or never then return yes or no respectively
		String backupMode = CwProjectPropertyPage.getAutoBackupMode(project);
		if (backupMode.equals(CwProjectPropertyPage.AUTOBACKUP_VALUE_ALWAYS)) return SourceBackupDialog.RET_YES;
		if (backupMode.equals(CwProjectPropertyPage.AUTOBACKUP_VALUE_NEVER)) return SourceBackupDialog.RET_NO;
		
		File srcDir = new File(project.getLocation() + File.separator + "src");

		if (srcDir.exists())
		{
			// see if there are any src files or folders in the source directory
			String[] srcFiles = srcDir.list(new SrcFileFilter());
			if (srcFiles.length > 0)
			{
				SourceBackupDialog dialog = new SourceBackupDialog(_shell, project, srcDir.getAbsolutePath());
				dialog.open();
				
				return (dialog.getRetVal());
			}
		}

		return SourceBackupDialog.RET_NO;
	}
    
    /**
	 * Checks the problems array for errors and warnings. Informational
	 *  messages are not considered a problem.
	 * @param problems
	 * @return
	 */
	private boolean hasProblems(List problems)
	{
		boolean hasProblems = false;
		
		if (problems.size() > 0)
		{
			for (int i=0; i<problems.size(); i++)
			{
				EObject problem = (EObject)problems.get(i);
				EEnumLiteralImpl level = (EEnumLiteralImpl)EcoreUtils.getValue(problem, "level");
				if (level.getValue() < 2)
				{
					hasProblems = true;
					i = problems.size();
				}
			}
		}
		
		return hasProblems;
	}
	
	/**
	 * @param project - IProject
	 * @param location - ASP install location
	 * @param pythonlocation - python Location
	 * @param dataTypeXMILocation - DataTypeMapping xmi file location
	 * @param depLocation - Code generation location for the project
	 * @param autoBackupMode - True if backup of old code should be taken. False otherwise.
	 * @param codeGenMode - Indicates code generation mode. Either 'saf' or 'nonsaf'
	 * @param isOverride - True if new code should overwrite old code. False if it should be merged.
	 * @param isCommandLineMode - True if running from command line. False otherwise.
	 * @throws Exception
	 */
	public void runAntBuilder(IProject project, String location,
			String pythonlocation, String dataTypeXMILocation,
			String autoBackupMode, String codeGenMode, boolean isOverride,
			boolean isCommandLineMode) throws Exception {
		AntRunner ant = new AntRunner();

		setAntArgs(ant, project, location, pythonlocation, dataTypeXMILocation,
				autoBackupMode, codeGenMode, isOverride);
		// AntBuildThread thread = new AntBuildThread(ant, project);
		AntBuildThread thread = new AntBuildThread(ant, project,
				isCommandLineMode);
		Display.getDefault().syncExec(thread);
		if (thread.getException() != null) {
			throw thread.getException();
		}
		try {
			project.refreshLocal(IResource.DEPTH_INFINITE, null);
		} catch (CoreException e) {
			e.printStackTrace();
		}
	}

	/**
	 * @param ant - AntRunner
	 * @param project - IProject
	 * @param location - ASP install location
	 * @param pythonlocation - python Location
	 * @param dataTypeXMILocation - DataTypeMapping xmi file location
	 * @param depLocation - Code generation location for the project
	 * @param autoBackupMode - Backup mode to indicate whether backup of
	 * old code to be taken or not
	 * @param codeGenMode code generation mode
	 */
	private void setAntArgs(AntRunner ant, IProject project, String location,
			String pythonlocation, String dataTypeXMILocation,
			String autoBackupMode, String codeGenMode, boolean isOverride) 
	{
		try {
			ant.setCustomClasspath(getClassPathURL());
			ant.setCustomClasspath(getClassPathURL());
			if( UtilsPlugin.isCmdToolRunning() == false )
				ant.addBuildLogger(CodeGenLogger.class.getName());
			else
				ant.addBuildLogger(CodeGenStdLogger.class.getName());

		} catch (Exception e) {
			ant
					.addBuildLogger("org.apache.tools.ant.DefaultLogger");
		}
		String buildToolsLoc = new File(location).getParentFile().getAbsolutePath() + File.separator + "buildtools";
		String mib2cCompiler = buildToolsLoc + File.separator + "local" + File.separator + "bin" + File.separator + "mib2c";	
		if(! new File(mib2cCompiler).exists()) {
			mib2cCompiler = "/usr/bin/mib2c";
		}
		if(! new File(mib2cCompiler).exists()){
			mib2cCompiler = "mib2c";
		}
		StringBuffer buff = new StringBuffer("-Dproject.loc=")
				.append(project.getLocation().toOSString()).append(
						" -Dpkg.dir=").append(location).append(
						" -Dmib.compiler=").append(mib2cCompiler)
				.append(" -Dproject.name=").append(
						project.getName()).append(" -Dmapxmi.loc=")
				.append(dataTypeXMILocation).append(
						" -Dpython.loc=").append(pythonlocation)
				.append(" -Dbuildtools.loc=").append(buildToolsLoc);
		if(isOverride) {
			buff.append(" -Dsrc.dir=").append(project.getLocation() + File.separator + "src");
		} else {
			buff.append(" -Dsrc.dir=").append(project.getLocation() + File.separator + ".ngc");
		}

		if (autoBackupMode.equals("true")) {
			buff.append(" -Dautobackup.mode=").append(
					autoBackupMode);
		}
		
		buff.append(" -DcodeGenMode=").append(codeGenMode);
		buff.append(" -Dmib.path=").append(getMIBPath(location));
		
		try {
			project.refreshLocal(IResource.DEPTH_INFINITE, null);
		} catch (CoreException e) {
			e.printStackTrace();
		}
		ant.setBuildFileLocation(project.getLocation().append(
				"codegen.xml").toOSString());
		ant.setArguments(buff.toString());
		ant.setMessageOutputLevel(org.apache.tools.ant.Project.MSG_INFO);
	}
	

	/**
	 * Return the path to the common net-snmp and hpi mibs.
	 * @param location
	 * @return
	 */
	String getMIBPath(String location)
	{
		String mibPath = "";
		String buildToolsLoc = new File(location).getParentFile().getAbsolutePath() + File.separator + "buildtools";
		if(buildToolsLoc != null && !buildToolsLoc.equals("")) {
			String mibLoc = buildToolsLoc + File.separator + "local" + File.separator + "share" 
							+ File.separator + "snmp" + File.separator + "mibs";
			if(new File(mibLoc).exists()) {
				mibPath = mibLoc;
			}
			
			// check for an alternative mib path and add it to our path if different
			String altPath = ClovisMibUtils.checkForAlternateMibPath(location);
			if (altPath != null && altPath.length() > 0 && !altPath.equals(mibLoc))
			{
				mibPath = mibPath + ":" + altPath;
			}
		}
		return mibPath.equals("") ? "." : mibPath;
	}

	/**
	 * Merge generated code with existing code
	 * @param project Project
	 */
	public void mergeGeneratedCode(IProject project) {
		IFolder next = project.getFolder(".ngc");
		IFolder src = project.getFolder(ICWProject.CW_PROJECT_SRC_DIR_NAME)/*.getFolder("app")*/;
		IFolder last = src.getFolder(".lgc");
		Map namesMap = getFileNamesMap(project);
		Map modifiedFiles = MergeUtils.getModifiedFiles(src, next);
		//List changedList = (List) modifiedFiles.get("modify");
		List changedList = MergeUtils.filterUserModifiedFiles(
				(List) modifiedFiles.get("modify"), namesMap, src, last);
		List mergeImmuneList = MergeUtils.filterMergeImmuneFiles(project, changedList);
		List addedList = (List) modifiedFiles.get("add");
		List deletedList = (List) modifiedFiles.get("delete");
		MergeUtils.addNameChangedFiles(changedList, addedList, deletedList,
				namesMap, src, next, last);
		if (!changedList.isEmpty()) {
			if(CwProjectPropertyPage.getAlwaysMergeMode(project)) {
				MergeUtils.mergeFiles(changedList.toArray(), namesMap, src, next);
			} else {
				MergeDialog dialog = new MergeDialog(_shell, changedList,
					namesMap, next, src);
				if(dialog.open() == 1)
					return;
			}
		}
		changedList.addAll(mergeImmuneList);
		MergeUtils.overrideFiles(File.separator, changedList, next, src);
		MergeUtils.removeResources(deletedList, src, last);
		
		if (last.exists()) {
			try {
				last.delete(true, false, null);
			} catch (CoreException e) {
				e.printStackTrace();
			}
		}
		try {
			last.create(true, true, null);
		} catch (CoreException e1) {
			e1.printStackTrace();
		}
		try {
			project.refreshLocal(IResource.DEPTH_INFINITE, null);
		} catch (CoreException e) {
			e.printStackTrace();
		}
		
		MergeUtils.moveFiles(next, last);
		IFile nameMapFile = project.getFile(".nameMapFile");
		if(nameMapFile.exists()) {
			try {
				nameMapFile.delete(true, null);
			} catch (CoreException e) {
				e.printStackTrace();
			}
		}
;	}
	/**
	 * Get URL array of classpaths. Takes classpath urls from Ant and adds
	 * current classpath to it so that CodeGenLogger class can be loaded.
	 * 
	 * @return Array of Classpath URLs
	 * @throws Exception
	 *             If URL creation failes.
	 */
	private URL[] getClassPathURL() throws Exception {
		URL[] urls = AntCorePlugin.getPlugin().getPreferences().getURLs();
		URL[] newUrls = new URL[urls.length + 1];
		System.arraycopy(urls, 0, newUrls, 1, urls.length);
		// Add the path to the Plugin classes
		String className = this.getClass().getName();
		if (!className.startsWith("/")) {
			className = "/" + className;
		}
		className = className.replace('.', '/');
		String classLoc = getClass().getClassLoader().getResource(
				className + ".class").toExternalForm();
		newUrls[0] = Platform.resolve(new URL(classLoc.substring(0, classLoc
				.indexOf(className))));
		return newUrls;
	}
	
	/*
	 * Class to use for file filtering which ignores .saf and .nonsaf files
	 */
	class SrcFileFilter implements FilenameFilter {
	    public boolean accept(File dir, String name)
	    {
	    	//if (("." + ICWProject.PROJECT_DEFAULT_SAF_TEMPLATE_GROUP_FOLDER).equals(name)) return false;
	    	//if (("." + ICWProject.PROJECT_DEFAULT_NONSAF_TEMPLATE_GROUP_FOLDER).equals(name)) return false;
	    	return true;
	    }
	}
	
	/*
	 * Class used to filter out all non compressed tar files.
	 */
	class TarGzFileFilter implements FilenameFilter {
	    public boolean accept(File dir, String name)
	    {
	    	if (name.toLowerCase().endsWith(".tar.gz")) return true;
	    	return false;
	    }
	}

	/*
	 * Sorts file names in reverse order
	 */
	class ReverseFilenameComparator implements Comparator <File>
	{
		private Collator c = Collator.getInstance();

		public int compare(File f1, File f2)
		{
			if(f1 == f2) return 0;

			if(f1.isDirectory() && f2.isFile()) return -1;
			if(f1.isFile() && f2.isDirectory()) return 1;

			return c.compare(f1.getName(), f2.getName()) * -1;
		}
	}
	
	class AntBuildThread implements Runnable {

		AntRunner _ant = null;
		IProject _project = null;
		Exception _ex = null;
		boolean _isCommandLineMode = false;

		public AntBuildThread(AntRunner ant, IProject project, boolean isCommandLineMode)
		{
			_ant = ant;
			_project = project;
			_isCommandLineMode = isCommandLineMode;
		}

		public void run()
		{
			if (_isCommandLineMode)
			{
				try {
					_ant.run();
				} catch (CoreException e) {
					e.printStackTrace();
				}
			} else {
				// if running from within the IDE then use a progess monitor dialog
				ClovisProgressMonitorDialog pmDialog = null;
				pmDialog = new ClovisProgressMonitorDialog(Display.getDefault().getActiveShell());
				try {
					pmDialog.run(true, true, new RunnableCode(_ant, _project));
				} catch (InvocationTargetException e) {
					_ex = e;
				} catch (InterruptedException e) {
					_ex = e;
				}
			}

		}

		public Exception getException() {
			return _ex;
		}
	}
	
	class RunnableCode implements IRunnableWithProgress, Runnable {
		IProject project = null;
		AntRunner ant = null;
		Exception ex = null;
		RunnableCode(AntRunner ant, IProject project) {
			this.project = project;
			this.ant = ant;
		}
		public void run(IProgressMonitor monitor)
			throws InvocationTargetException, InterruptedException {
			try {
				
					if (project != null) {
						if (monitor.isCanceled()) {
							monitor.done();
							return;
						}
						monitor.beginTask(
							"Generating source code for : "
								+ project.getName(),
							IProgressMonitor.UNKNOWN);
						ant.run(monitor);
						try {
							project.refreshLocal(
								IResource.DEPTH_INFINITE,
								null);
							// donot need for monitor
						} catch (java.lang.Exception e) {
							ex = e;
						}
					}
				
			} catch (Exception e) {
				ex = e;
			}
		}
		public Exception getException() {
			return ex;
		}
		public void run() {
			
		}
	}
	/**
	 * Dialog used to report that files in the src directory will be overwritten
	 *  and allow the user to continue with backup, continue without backup, or cancel.
	 *  The user is also allowed to select an option to never show the dialog again for
	 *  the project.
	 *  
	 * @author matt
	 */
	class SourceBackupDialog extends Dialog
	{
		public static final int RET_YES    = 0;
		public static final int RET_NO     = 1;
		public static final int RET_CANCEL = 2;
		
		private Button _prompt;
		private IProject _project;
		private int _retVal = RET_CANCEL;
		
		private String _srcDir;

		public SourceBackupDialog(Shell parent, IProject project, String srcDir) {
			super(parent);
			_project = project;
			_srcDir = srcDir;
		}
		
		protected Control createDialogArea(Composite parent)
		{
			Composite composite = new Composite(parent, SWT.NONE);
			
			GridLayout layout = new GridLayout();
	        layout.marginHeight = 10;
	        layout.marginWidth = 10;
			
			composite.setLayout(layout);
			composite.setLayoutData(new GridData(GridData.FILL_BOTH));
			
			Label question = new Label(composite, SWT.WRAP);
			
			question.setText("Source and/or configuration files exist in the directory:\n\n    " + _srcDir
					+ "\n\nBefore generating code would you like to backup these files to the directory:\n\n"
					+ "    " + _srcDir + ".bak");
			GridData data = new GridData(GridData.FILL_HORIZONTAL);
			question.setLayoutData(data);
			
			GridData data2 = new GridData();
			data2.verticalIndent = 10;
			_prompt = new Button(composite, SWT.CHECK | SWT.WRAP);
			_prompt.setText("Never show this dialog again");
			_prompt.setLayoutData(data2);

			GridData data3 = new GridData();
			Label info = new Label(composite, SWT.WRAP);
			info.setText("(Dialog can be reenabled through Project Properties)");
			info.setLayoutData(data3);
			
			getShell().setText("Backup Configuration?");

			return composite;
		}
		
		
		protected void createButtonsForButtonBar(Composite parent)
		{
			Button yesButton = createButton(parent, IDialogConstants.YES_ID, IDialogConstants.YES_LABEL,
					true);
			yesButton.addSelectionListener(new SelectionListener() {
				public void widgetDefaultSelected(SelectionEvent e) {
				}
				public void widgetSelected(SelectionEvent e) {
					if (_prompt.getSelection())
					{
						CwProjectPropertyPage.setAutoBackupMode(_project, CwProjectPropertyPage.AUTOBACKUP_VALUE_ALWAYS);
					}
					_retVal = RET_YES;
					okPressed();
				}
	        });

			Button noButton = createButton(parent, IDialogConstants.NO_ID, IDialogConstants.NO_LABEL,
					false);
			noButton.addSelectionListener(new SelectionListener() {
				public void widgetDefaultSelected(SelectionEvent e) {
				}
				public void widgetSelected(SelectionEvent e) {
					if (_prompt.getSelection())
					{
						CwProjectPropertyPage.setAutoBackupMode(_project, CwProjectPropertyPage.AUTOBACKUP_VALUE_NEVER);
					}
					_retVal = RET_NO;
					okPressed();
				}
	        });

			Button cancelButton = createButton(parent, IDialogConstants.CANCEL_ID, IDialogConstants.CANCEL_LABEL,
					false);
			cancelButton.addSelectionListener(new SelectionListener() {
				public void widgetDefaultSelected(SelectionEvent e) {
				}
				public void widgetSelected(SelectionEvent e) {
					_retVal = RET_CANCEL;
					okPressed();
				}
	        });
		}

		/*
		 * Return a string representing the button which was clicked by the user.
		 */
		protected int getRetVal()
		{
			return _retVal;
		}

	    /*
	     * Set an initial size for the dialog that is reasonable.
	     * 
	     * @see org.eclipse.jface.dialogs.Dialog#getInitialSize()
	     */
		protected Point getInitialSize()
	    {
	        Point shellSize = super.getInitialSize();
	        return new Point(Math.max(convertHorizontalDLUsToPixels(400), shellSize.x),
	                Math.max(convertVerticalDLUsToPixels(160), shellSize.y));
	    }
	}
}

package com.clovis.cw.cmdtool;

import java.io.File;
import java.io.IOException;
import java.net.URL;

import org.eclipse.core.resources.ICommand;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IProjectDescription;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.resources.IWorkspace;
import org.eclipse.core.resources.IWorkspaceRoot;
import org.eclipse.core.resources.IncrementalProjectBuilder;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IPlatformRunnable;
import org.eclipse.core.runtime.Platform;

import com.clovis.common.utils.ClovisFileUtils;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.constants.SnmpConstants;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.codegen.GenerateSource;

/**
 * 
 * @author shubhada
 * Command line application for code generation
 *
 */
public class GenerateSourceCommand implements IPlatformRunnable
{
	/**
	 *  code generation application 
	 * @param args - arguments to the application
	 */
	public Object run(Object args) throws Exception {
		
		String [] cmdArgs = (String[]) args;
		if (cmdArgs.length == 4) {
			String projName = cmdArgs[0];
			String sdkLocation = cmdArgs[1];
			String modelSrcLocation = cmdArgs[2];
			String pythonLocation = cmdArgs[3];
			IWorkspace workspace = ResourcesPlugin.getWorkspace();
			IWorkspaceRoot root = workspace.getRoot();
			IProject project  = root.getProject(projName);
			boolean projectExist = project.exists();
			if (!projectExist) {
				IProjectDescription description = workspace
						.newProjectDescription(project.getName());
				project.create(description, null);
				project.open(null);
				description = project.getDescription();
				// add builder to project
				ICommand[] commands = description.getBuildSpec();
				ICommand command = description.newCommand();
				command
						.setBuilderName("com.clovis.cw.workspace.builders.ClovisBuilder");

				command
						.setBuilding(IncrementalProjectBuilder.AUTO_BUILD,
								false);
				command.setBuilding(IncrementalProjectBuilder.FULL_BUILD, true);
				ICommand[] newCommands = new ICommand[commands.length + 1];
				newCommands[0] = command;
				System.arraycopy(commands, 0, newCommands, 1, commands.length);
				description.setBuildSpec(newCommands);

				// adding system project nature
				String natures[] = description.getNatureIds();
				String newNatures[] = new String[natures.length + 1];
				System.arraycopy(natures, 0, newNatures, 0, natures.length);
				newNatures[natures.length] = "com.clovis.cw.workspace.natures.SystemProjectNature";
				description.setNatureIds(newNatures);

				project.setDescription(description, null);
			}
			System.out.println("Generating source for the project:" + project.getName());
			callCodeGeneration(project, sdkLocation, modelSrcLocation, pythonLocation);
			if(!projectExist) {
				project.delete(false, true, null);
			}
		} else if (cmdArgs.length == 3) {
			System.out.println("There is no input value provided for python location");
		} else {
			System.out.println("\n Usage : cl-generate-source -w <workspace-name> -p <project-name> -a <project_area-location> -y <python-location");
		}
			
			
		
		return EXIT_OK;
	}
	/**
	 * 
	 * @param project
	 * @param location
	 * @param pythonLocation
	 * @param modelSrcLocation
	 */
	private void callCodeGeneration(IProject project, String sdkLocation, String modelSrcLocation, String pythonLocation)
	{
		
		URL url = DataPlugin.getDefault().getBundle().getEntry("/");
		try {
			url = Platform.resolve(url);
		} catch (IOException e) {
			System.err.println(e.getMessage());
		}

		String dataTypeXMILocation = url.getFile()
				+ ICWProject.PLUGIN_XML_FOLDER + File.separator
				+ SnmpConstants.DATA_TYPES_XMI_FILE_NAME;
		
		if (sdkLocation == null || sdkLocation.equals("")) {
			System.out.println("SDK location is incorrect for project ["
					+ project.getName()
					+ "]. Please input a valid SDK location");
		}
		if (pythonLocation == null || pythonLocation.equals("")) {

			System.out.println("Python location is incorrect for project ["
					+ project.getName()
					+ "]. Please input a valid python location ");
		}

		if (sdkLocation != null && !sdkLocation.equals("")
				&& pythonLocation != null && !pythonLocation.equals("")) {
			
			File pythonFile = new File(pythonLocation + File.separator + "python");
			if (pythonFile.exists() && pythonFile.isFile()) {
				String autoBackupMode = "true";
				try {
					System.out.println("Generating code in location" + modelSrcLocation);
					GenerateSource gs = new GenerateSource(null);
					File linkFile = project.getLocation().append(
							ICWProject.CW_PROJECT_SRC_DIR_NAME).toFile();
					if (!modelSrcLocation.equals(linkFile.getAbsolutePath())) {
						try {
							Process proc = Runtime.getRuntime().exec(
									"rm " + linkFile);
							proc.waitFor();
							ClovisFileUtils.createRelativeSourceLink(
									modelSrcLocation, linkFile
											.getAbsolutePath());
							project
									.refreshLocal(IResource.DEPTH_INFINITE,
											null);

						} catch (IOException e) {
							e.printStackTrace();
						} catch (InterruptedException e) {
							e.printStackTrace();
						} catch (CoreException e) {
							e.printStackTrace();
						}
					}
					ProjectDataModel.copyConfigFilesToTempDir(project);
					gs.runAntBuilder(project, sdkLocation, pythonLocation,
							dataTypeXMILocation, autoBackupMode,
							ICWProject.PROJECT_DEFAULT_CODEGEN_OPTION,
							true, true);
					//gs.mergeGeneratedCode(project);
				} catch (Exception e) {
					System.err.println(e.getMessage());
				}
				
			} else {
				System.out
						.println("Please give a valid values\n.Usage : cl-generate-source -w <workspace-name> -p <project-name> -a <project_area-location> -y <python-location");
			}
		}
		
	}
	
}

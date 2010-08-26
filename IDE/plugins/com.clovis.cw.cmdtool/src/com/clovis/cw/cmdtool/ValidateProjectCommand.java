package com.clovis.cw.cmdtool;

import java.io.File;
import java.util.List;

import org.eclipse.core.resources.ICommand;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IProjectDescription;
import org.eclipse.core.resources.IWorkspace;
import org.eclipse.core.resources.IWorkspaceRoot;
import org.eclipse.core.resources.IncrementalProjectBuilder;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.IPlatformRunnable;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.project.ProjectValidator;
import com.clovis.cw.workspace.project.SubModelProblemReporter;
import com.clovis.cw.workspace.project.ValidationConstants;


public class ValidateProjectCommand implements IPlatformRunnable, ValidationConstants {

	public Object run(Object args) throws Exception {
		
		String [] cmdArgs = (String[]) args;
		if (cmdArgs.length > 0) {
			String projName = cmdArgs[0];
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
			System.out.println("-----------------------------------------------------------------------------");
			System.out.println("          Validating Project : " + project.getName());
			System.out.println("-----------------------------------------------------------------------------");
			reportProjectModelErrors(project);
			SubModelProblemReporter problemReporter = 
				new SubModelProblemReporter();
			problemReporter.reportSubModelProblems(project);
			if(!projectExist) {
				project.delete(false, true, null);
			}
		}
		//System.out.println("\n\n-----------------------------------------------------------------------------\n");
		return EXIT_OK;
	}

	public void reportProjectModelErrors(IProject project)
	{
		ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(project);
		ProjectValidator pv = new ProjectValidator();
		List problemsList = pv.validate(pdm);

		//System.out.println( problemsList.size() + " problems encountered in project [" + project.getName() + "]");
		//System.out.println("-----------------------------------------------------------------------------\n");
		System.out.println(" Source Severity  |  Object   |   Validation message  | File ");  
		System.out.println("-----------------------------------------------------------------------------\n");

		for (int i = 0; i < problemsList.size(); i++) {
            EObject problem = (EObject) problemsList.get(i);
            EObject sourceObj = (EObject) EcoreUtils.getValue(problem, PROBLEM_SOURCE);
            //int problemNumber = ((Integer) EcoreUtils.getValue(problem, PROBLEM_NUMBER)).intValue();
            String resourcePath = EcoreUtils.getValue(problem, PROBLEM_RESOURCE).toString();
            resourcePath = resourcePath.replace('/', File.separatorChar);
            
            String sourceObjName = "NONE";
            if (sourceObj != null) {
            	sourceObjName = EcoreUtils.getName(sourceObj);
            }
	    if( sourceObjName == null || sourceObjName.trim().equals("") )
                 sourceObjName = "NONE";

            String level = EcoreUtils.getValue(problem, PROBLEM_LEVEL).toString();
            String message = EcoreUtils.getValue(problem, PROBLEM_MESSAGE).toString();
            if (level.equals("ERROR")) {
                System.err.print("\n ERROR : " + "[" + sourceObjName + "] : " + message + " : " + resourcePath);
            } else if (level.equals("WARNING")) {
            	System.out.print("\n WARNING : " + "[" + sourceObjName + "] : " + message + " : " + resourcePath);
            } else {
            	System.out.print("\n INFO : " + "[" + sourceObjName + "] : " + message + " : " + resourcePath);
            }
        }
	}
	
}

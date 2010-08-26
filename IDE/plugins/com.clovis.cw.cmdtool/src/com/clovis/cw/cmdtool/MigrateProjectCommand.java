package com.clovis.cw.cmdtool;

import java.io.File;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IProjectDescription;
import org.eclipse.core.resources.IWorkspace;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.IPlatformRunnable;

import com.clovis.cw.workspace.migration.MigrationManager;
import com.clovis.cw.workspace.migration.MigrationUtils;

/**
 * Command line application for Migration.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class MigrateProjectCommand implements IPlatformRunnable {

	/**
	 * Migration application code
	 * 
	 * @param args -
	 *            arguments to the application
	 */
	public Object run(Object args) throws Exception {

		String[] cmdArgs = (String[]) args;
		if (cmdArgs.length == 1 || cmdArgs.length == 2) {

			String projectName = cmdArgs[0];
			IWorkspace workspace = ResourcesPlugin.getWorkspace();
			IProject project = workspace.getRoot()
					.getProject(projectName);

			if (!new File(workspace.getRoot().getLocation().append(projectName)
					.toOSString()).exists()) {
				System.out.println("Project does not exist.");
				return EXIT_OK;
			}

			boolean projectExist = project.exists();
			if (!projectExist) {
				IProjectDescription description = workspace
						.newProjectDescription(project.getName());
				project.create(description, null);
			}
			project.open(null);

			if (!MigrationUtils.isMigrationRequired(project)) {
				System.out.println("Project is already in the latest version.");
				return EXIT_OK;
			}

			if (cmdArgs.length == 2) {
				System.out.println("\n\nCreating the Backup of the project...");

				if (!MigrationManager.backupProjects(
						new IProject[] { project }, cmdArgs[1])) {
					return EXIT_OK;
				}
				System.out.println("[DONE]");
			}

			migrateProject(project);

			if(!projectExist) {
				project.delete(false, true, null);
			}

		} else {
			System.out
					.println("\n Usage : cl-migrate-project -w <workspace-name> -p <project-name> -b <backup-location>");
			System.out.println("-b <backup-location> is optional");
		}

		return EXIT_OK;
	}

	/**
	 * Performs the Migration.
	 * 
	 * @param project
	 */
	private void migrateProject(IProject project) {

		System.out.println("\n\nMigrating the project...");
		System.out
				.println("------------------------------------------------------------------------------");
		System.out
				.println("Severity     |               Problem               |               Description");
		System.out
				.println("------------------------------------------------------------------------------");

		MigrationManager manager = new MigrationManager(project);
		manager.migrateProject();
	}
}

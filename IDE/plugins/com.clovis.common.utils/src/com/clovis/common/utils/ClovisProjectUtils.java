/**
 * 
 */
package com.clovis.common.utils;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.Properties;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.views.navigator.IResourceNavigator;

/**
 * Utility class for clovis project.
 * 
 * @author Suraj Rajyaguru
 */
public class ClovisProjectUtils {

	/**
	 * Returns the currently selected project.
	 * 
	 * @return project
	 */
	public static IProject getSelectedProject() {
		IWorkbenchPage page = PlatformUI.getWorkbench()
				.getActiveWorkbenchWindow().getActivePage();
		IResourceNavigator navigator = (IResourceNavigator) page
				.findView("com.clovis.cw.workspace.clovisWorkspaceView");

		if (navigator == null) {
			try {
				navigator = (IResourceNavigator) page
						.showView("com.clovis.cw.workspace.clovisWorkspaceView");
			} catch (PartInitException e) {
				e.printStackTrace();
			}
		}

		ISelection selection = navigator.getViewer().getSelection();
		IProject project = null;

		if (selection instanceof IStructuredSelection) {
			IStructuredSelection sel = (IStructuredSelection) selection;
			Object res = sel.getFirstElement();

			if (res instanceof IResource) {
				project = ((IResource) res).getProject();
			}
		}

		return project;
	}

	/**
	 * Returns code-gen mode for the given project.
	 * 
	 * @param project
	 * @return code-gen mode
	 */
	public static String getCodeGenMode(IProject project) {
		Properties properties = new Properties();

		try {
			properties.load(new FileInputStream(project.getLocation().append(
					"project.properties").toOSString()));

		} catch (FileNotFoundException e) {
			return "openclovis";
		} catch (IOException e) {
			return "openclovis";
		}
		
		String value = properties.getProperty("codegenmode"); 
		return value != null ? value : "openclovis";
	}
}

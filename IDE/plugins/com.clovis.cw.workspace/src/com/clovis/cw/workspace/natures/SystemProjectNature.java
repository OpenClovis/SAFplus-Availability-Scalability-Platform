/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/natures/SystemProjectNature.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.natures;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IProjectNature;
import org.eclipse.core.runtime.CoreException;

/**
 * @author Pushparaj
 * Nature Class for Clovis System Project
 */
public class SystemProjectNature implements IProjectNature
{
	public static final String CLOVIS_SYSTEM_PROJECT_NATURE =
		"com.clovis.cw.workspace.natures.SystemProjectNature";
	public void configure() throws CoreException {
			
	}

	public void deconfigure() throws CoreException {
			
	}

	public IProject getProject() {
			return null;
	}

	public void setProject(IProject project) {
			
	}

}

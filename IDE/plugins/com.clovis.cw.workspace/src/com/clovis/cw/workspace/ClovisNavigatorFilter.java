/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/ClovisNavigatorFilter.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace;


import org.eclipse.core.resources.IFolder;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.ui.views.navigator.ResourcePatternFilter;

import com.clovis.cw.workspace.natures.SystemProjectNature;

/**
 * @author Pushparaj
 *
 * Filter for Clovis Workspace View
 */
public class ClovisNavigatorFilter extends ResourcePatternFilter {
	
	//*********************************************************************//
	// This variable is used to indicate that the project has been opened  //
	//  by the filter just so that we can check its nature. It will be     //
	//  closed again when we are finished. This is required to prevent a   //
	//  race condition with the validator which causes multiple threads to //
	//  attempt validation at the same time.                               //
	//*********************************************************************//
	public boolean _transientOpen = false;
	
	public boolean select(Viewer viewer, Object parent, Object element) {
		if (element instanceof IProject) {
			try {
				IProject proj = (IProject) element;
				boolean isOpen = true;
				if(! proj.isOpen()) {
					_transientOpen = true;
					proj.open(null);
					isOpen = false;
				}
				if (!proj.hasNature(SystemProjectNature.CLOVIS_SYSTEM_PROJECT_NATURE)) {
					if(!isOpen) {
						proj.close(null);
					}
					_transientOpen = false;
					return false;
				} else {
					if(!isOpen) {
						proj.close(null);
					}
					_transientOpen = false;
					return true;
				}
			} catch (Exception e) {
				_transientOpen = false;
				return false;
			}
		} else if (element instanceof IFolder) {
			IFolder folder = (IFolder) element;
			if(folder.getName().equals(".lgc") || folder.getName().equals(".ngc") || folder.getName().equals(".temp_dir")) {
					return false;
			}
		} else if (element instanceof IResource) {
			IResource res = (IResource) element;
			if(res.getName().endsWith(".tgz")) {
				return false;
			}
			if (res.getParent() instanceof IProject) {
				if (res.getName().endsWith(".sh")
						|| res.getName().endsWith(".xml")
						|| res.getName().equals("project.properties")) {
					return false;
				}
			}
		} 
		return super.select(viewer, parent, element);
	}
}

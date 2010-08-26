/**
 * 
 */
package com.clovis.cw.editor.ca.pm;

import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.IEditorInput;
import org.eclipse.ui.IPersistableElement;

import com.clovis.cw.project.data.ProjectDataModel;

/**
 * Editor input for PM Editor.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class PMEditorInput implements IEditorInput {

	private ProjectDataModel _pdm;

	public PMEditorInput(ProjectDataModel projectDataModel) {
		_pdm = projectDataModel;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.IEditorInput#exists()
	 */
	public boolean exists() {
		return false;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.IEditorInput#getImageDescriptor()
	 */
	public ImageDescriptor getImageDescriptor() {
		return null;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.IEditorInput#getName()
	 */
	public String getName() {
		return _pdm.getProject().getName() + " - Performance Monitoring Editor";
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.IEditorInput#getPersistable()
	 */
	public IPersistableElement getPersistable() {
		return null;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.IEditorInput#getToolTipText()
	 */
	public String getToolTipText() {
		return _pdm.getProject().getName() + " - Performance Monitoring Editor";
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.core.runtime.IAdaptable#getAdapter(java.lang.Class)
	 */
	public Object getAdapter(Class adapter) {
		return null;
	}

	/**
	 * Returns project data model.
	 * 
	 * @return
	 */
	public ProjectDataModel getProjectDataModel() {
		return _pdm;
	}
}

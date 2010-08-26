/**
 * 
 */
package com.clovis.cw.editor.ca.dialog;

import org.eclipse.core.resources.IProject;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.ecore.Model;

/**
 * Generic Dialog for the Preference View.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public abstract class GenericPreferenceDialog extends PreferenceDialog {

	protected PreferenceManager _preferenceManager;

	protected IProject _project;

	protected Model _viewModel;

	/**
	 * Constructor.
	 * 
	 * @param parentShell
	 * @param manager
	 * @param project
	 */
	public GenericPreferenceDialog(Shell parentShell,
			PreferenceManager manager, IProject project) {
		super(parentShell, manager);
		_preferenceManager = manager;
		_project = project;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.preference.PreferenceDialog#getTreeViewer()
	 */
	@Override
	public TreeViewer getTreeViewer() {
		return super.getTreeViewer();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.Dialog#getButton(int)
	 */
	@Override
	public Button getButton(int id) {
		return super.getButton(id);
	}

	/**
	 * @return the _project
	 */
	public IProject getProject() {
		return _project;
	}

	/**
	 * @return the _viewModel
	 */
	public Model getViewModel() {
		return _viewModel;
	}

	/**
	 * Adds the preference nodes.
	 */
	protected abstract void addPreferenceNodes();
}

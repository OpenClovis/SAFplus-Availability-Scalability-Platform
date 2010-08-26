/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/dialog/DeploymentDialog.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.dialog;

import java.util.List;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.editor.ca.constants.SafConstants;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * @author Pushparaj
 * 
 * Dialog for Deployment
 */
public class DeploymentDialog extends PreferenceDialog {
	private IProject _project;

	/**
	 * @param project IProject
	 * @param parentShell Shell
	 */
	public DeploymentDialog(IProject project, Shell parentShell) {
		this(parentShell, project, new PreferenceManager());
	}
	/**
	 * @param parentShell Shell
	 * @param manager PreferenceManager
	 */
	public DeploymentDialog(Shell parentShell, IProject project, PreferenceManager manager) {
		super(parentShell, manager);
		_project = project;
		addPreferenceNodes(manager);
	}
	/**
	 * Adds the nodes to tree on the left side of the dialog
	 */
	private void addPreferenceNodes(PreferenceManager manager) {
		ProjectDataModel pModel = ProjectDataModel
				.getProjectDataModel(_project);
		List amfList = pModel.getNodeProfiles().getEList();
		if (amfList.size() == 0)
			return;
		EObject amfObj = (EObject) amfList.get(0);
		EReference nodeInstsRef = (EReference) amfObj.eClass()
				.getEStructuralFeature(SafConstants.NODE_INSTANCES_NAME);
		EReference nodeInstanceRef = (EReference) nodeInstsRef
				.getEReferenceType().getEStructuralFeature(
						SafConstants.NODE_INSTANCELIST_NAME);
		EObject nodeInstancesObj = (EObject) amfObj.eGet(nodeInstsRef);
		NotifyingList nodeInstList = (NotifyingList) nodeInstancesObj
				.eGet(nodeInstanceRef);
		for (int i = 0; i < nodeInstList.size(); i++) {
				EObject nodeInstObj = (EObject) nodeInstList.get(i);
				String name = (String) EcoreUtils.getValue(nodeInstObj, "name");
				DeploymentPreferencePage deployPage = new DeploymentPreferencePage(
						_project, name);
				PreferenceNode node = new PreferenceNode(name, deployPage);
				manager.addToRoot(node);
		}
	}
	/**
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	protected void configureShell(Shell shell) {
		super.configureShell(shell);
		shell.setText(_project.getName() + " - " + "Deployment Details");
    }

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.preference.PreferenceDialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createButtonsForButtonBar(Composite parent) {
		// TODO Auto-generated method stub
		super.createButtonsForButtonBar(parent);
		getButton(IDialogConstants.OK_ID).setText("Deploy &All");
	}

	/**
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	protected void okPressed() {
		super.okPressed();
	}
}

/**
 * 
 */
package com.clovis.cw.workspace.modelTemplate;

import java.io.File;

import org.eclipse.jface.viewers.CheckStateChangedEvent;
import org.eclipse.jface.viewers.CheckboxTableViewer;
import org.eclipse.jface.viewers.ICheckStateListener;

/**
 * Listener to handle the change in the selection of the model template files
 * while import export.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class ModelTemplateImportExportCheckStateListener implements
		ICheckStateListener {

	private ModelTemplateImportExportDialog _modelTemplateImportExportDialog;

	/**
	 * Constructor.
	 * 
	 * @param modelTemplateImportExportDialog
	 */
	public ModelTemplateImportExportCheckStateListener(
			ModelTemplateImportExportDialog modelTemplateImportExportDialog) {
		_modelTemplateImportExportDialog = modelTemplateImportExportDialog;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.ICheckStateListener#checkStateChanged(org.eclipse.jface.viewers.CheckStateChangedEvent)
	 */
	public void checkStateChanged(CheckStateChangedEvent event) {

		if (ModelTemplateUtils
				.isModelTempalteFolder(_modelTemplateImportExportDialog
						.getImportExportLocation())) {
			return;
		}

		CheckboxTableViewer table = (CheckboxTableViewer) event.getSource();
		int count = table.getCheckedElements().length;

		if (count == 0) {
			_modelTemplateImportExportDialog.enableOKButton(false);

			if (_modelTemplateImportExportDialog.getDialogType() == ModelTemplateConstants.DIALOG_TYPE_IMPORT) {
				_modelTemplateImportExportDialog
						.setErrorMessage("There are no files currently selected for import.");

			} else if (_modelTemplateImportExportDialog.getDialogType() == ModelTemplateConstants.DIALOG_TYPE_EXPORT) {
				_modelTemplateImportExportDialog
						.setErrorMessage("There are no files currently selected for export.");
			}

		} else {
			String location = _modelTemplateImportExportDialog._importExportLocation.getText().trim();
			if(location.equals("")) {
				_modelTemplateImportExportDialog.setMessage("Selection should not be empty.");
				_modelTemplateImportExportDialog.enableOKButton(false);
			} else if(!new File(location).isDirectory()) {
				_modelTemplateImportExportDialog.setErrorMessage(location + "is not a valid location.");
				_modelTemplateImportExportDialog.enableOKButton(false);
			} else {
				_modelTemplateImportExportDialog.enableOKButton(true);
				_modelTemplateImportExportDialog.setErrorMessage(null);
				_modelTemplateImportExportDialog.setMessage(null);
			}
		}
	}
}

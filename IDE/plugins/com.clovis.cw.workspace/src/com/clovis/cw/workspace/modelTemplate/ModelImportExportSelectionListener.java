/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/modelTemplate/ModelImportExportSelectionListener.java $
 * $Author: srajyaguru $
 * $Date: 2007/04/30 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.workspace.modelTemplate;

import java.io.File;

import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;

/**
 * Listener to handle the change in the location field
 * while import/export.
 * @author pushparaj
 *
 */
public class ModelImportExportSelectionListener implements Listener{
	private ModelTemplateImportExportDialog _dialog;
	public ModelImportExportSelectionListener(ModelTemplateImportExportDialog dialog) {
		_dialog = dialog;
	}
	public void handleEvent(Event event) {
		String fileName = _dialog._importExportLocation.getText().trim();
		if(!fileName.equals("")) {
			if(!new File(fileName).isDirectory()) {
				_dialog.setErrorMessage(fileName + " is not a valid location.");
				_dialog.enableOKButton(false);
			} else {
				if(_dialog._modelTemplateFilesViewer.getCheckedElements().length == 0) {
					if(_dialog.getDialogType() == ModelTemplateConstants.DIALOG_TYPE_IMPORT) {
						_dialog.setErrorMessage("There are no files currently selected for import.");
					} else {
						_dialog.setErrorMessage("There are no files currently selected for export.");
					}
					_dialog.enableOKButton(false);
				} else {
					_dialog.setMessage(null);
					_dialog.setErrorMessage(null);
					_dialog.enableOKButton(true);
				}
			}
		} else {
			_dialog.setMessage("Selection should not be empty.");
			_dialog.setErrorMessage(null);
			_dialog.enableOKButton(false);
		}	
	}

}

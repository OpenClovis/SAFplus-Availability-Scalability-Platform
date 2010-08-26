/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/snmp/SNMPExportWizardDialog.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.editor.ca.snmp;

import org.eclipse.jface.wizard.IWizard;
import org.eclipse.jface.wizard.WizardDialog;
import org.eclipse.swt.widgets.Shell;

/**
 * @author pushparaj
 * Wizard Dialog for SNMP Export
 */
public class SNMPExportWizardDialog extends WizardDialog
{
	private String _projectName;
	private static final String DIALOG_TITLE = "SNMP Export";
	/**
	 * @param parentShell shell
	 * @param newWizard Wizard
	 * @param projectName project name
	 */
	public SNMPExportWizardDialog(Shell parentShell, IWizard newWizard, String projectName) {
		super(parentShell, newWizard);
		_projectName = projectName;
	}
	/**
	 * @see org.eclipse.jface.wizard.IWizardContainer#updateWindowTitle()
	 */
	public void updateWindowTitle() {
        if (getShell() == null)
            // Not created yet
            return;
        getShell().setText(_projectName + " - " + DIALOG_TITLE);
    }
}

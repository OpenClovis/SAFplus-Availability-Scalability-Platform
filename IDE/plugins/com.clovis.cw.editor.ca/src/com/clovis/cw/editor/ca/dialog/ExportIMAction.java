/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/ExportIMAction.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.dialog;

import org.eclipse.swt.widgets.Shell;
import org.eclipse.core.resources.IContainer;

import com.clovis.cw.editor.ca.snmp.SNMPExportWizardDialog;
import com.clovis.cw.editor.ca.snmp.SnmpExportWizard;
import com.clovis.common.utils.menu.IActionClassAdapter;
/**
 * @author shubhada
 * Export IM Action Class
 */
public class ExportIMAction extends IActionClassAdapter
{
    /**
     * Method to open Export wizard.
     * @param args Shell, project
     * @return whether action is successfull.
     */
    public boolean run(Object[] args) {
		Shell parentShell = (Shell) args[0];
		IContainer project = (IContainer) args[1];
		new SNMPExportWizardDialog(parentShell, new SnmpExportWizard(project),
				project.getName()).open();
		return true;
	}
}

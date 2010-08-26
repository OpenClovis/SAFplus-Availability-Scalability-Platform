/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/action/SNMPExportAction.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.action;

import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.ui.IViewActionDelegate;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;

import com.clovis.cw.editor.ca.snmp.SNMPExportWizardDialog;
import com.clovis.cw.editor.ca.snmp.SnmpExportWizard;
import com.clovis.cw.workspace.action.CommonMenuAction;

/**
 * @author pushparaj
 * Action Class for SNMP Export 
 */
public class SNMPExportAction extends CommonMenuAction implements
		IViewActionDelegate, IWorkbenchWindowActionDelegate {

	public void init(IViewPart view) {
		_shell = view.getViewSite().getShell();
	}

	public void dispose() {
			
	}

	public void init(IWorkbenchWindow window) {
		_shell = window.getShell();		
	}
	public void run(IAction action) {
		if (_project != null) {
        	int actionStatus = canUpdateIM();
        	if(actionStatus == ACTION_CANCEL) {
        		return;
            } else if(actionStatus == ACTION_SAVE_CONTINUE) {
            	updateIM();
            }
			new SNMPExportWizardDialog(_shell, new SnmpExportWizard(_project),
					_project.getName()).open();
		}
	}
	public void selectionChanged(IAction action, ISelection selection) {
		action.setEnabled(false);
	}
}

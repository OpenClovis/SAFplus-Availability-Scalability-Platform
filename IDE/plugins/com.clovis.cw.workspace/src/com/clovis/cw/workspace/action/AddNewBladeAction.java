/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/action/AddNewBladeAction.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.action;

import org.eclipse.core.resources.IProject;
import org.eclipse.jface.wizard.WizardDialog;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.menu.IActionClassAdapter;
import com.clovis.cw.workspace.dialog.AddResourceWizard;

public class AddNewBladeAction extends IActionClassAdapter
{
    /**
     * Adds a new Object in the list.
     * @param args 0 - Shell, 1 - project
     */
    
    public boolean run(Object[] args)
    {
        Shell shell = (Shell) args[0];
        IProject project = (IProject) args[1];
        WizardDialog dialog = new WizardDialog(shell, new AddResourceWizard(project));
        dialog.setPageSize(300, 80);
        dialog.open();
        return true;
    }
}

/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/action/BootPropertiesAction.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.action;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.IActionClassAdapter;
import com.clovis.cw.editor.ca.dialog.BootLevelDialog;

/**
 * @author shubhada
 *
 * EO Properties Action
 */
public class BootPropertiesAction extends IActionClassAdapter
{
    /**
     * Enabled only for single selection
     * @param environment Environment
     * @return true for single selection.
     */
    public boolean isEnabled(Environment environment)
    {
        StructuredSelection sel =
            (StructuredSelection) environment.getValue("selection");
        return (sel.size() == 1);
    }
    /**
     * Method to open properties dialog.
     * @param args 0 -
     * @return whether action is successfull.
     */
    public boolean run(Object[] args)
    {
        StructuredSelection sel = (StructuredSelection) args[1];
        EObject eobj = (EObject) sel.getFirstElement();
        PreferenceManager pmanager = new PreferenceManager();
        BootLevelDialog bDialog = new
                    BootLevelDialog((Shell) args[0], pmanager, eobj);
        bDialog.open();
        return true;
    }

}

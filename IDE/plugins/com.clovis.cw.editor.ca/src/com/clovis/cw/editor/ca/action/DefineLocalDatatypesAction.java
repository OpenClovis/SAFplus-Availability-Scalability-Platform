/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/action/DefineLocalDatatypesAction.java $
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
import com.clovis.cw.editor.ca.dialog.DataTypesDialog;

/**
 * @author shubhada
 *
 * Define User Defined Datatypes Action
 */
public class DefineLocalDatatypesAction extends IActionClassAdapter
{
    /**
     * makes the button disabled if the selection is empty
     * or if the selection is larger than 1.
     * @param Environment environment
     */
    public boolean isEnabled(Environment environment)
    {
        StructuredSelection sel =
            (StructuredSelection) environment.getValue("selection");
        return ((!sel.isEmpty()) && (sel.size() == 1));
    }
    /**
     * Method to open properties dialog.
     * @param args 0 -
     * @return whether action is successfull.
     */
    public boolean run(Object[] args)
    {
        PreferenceManager pmanager = new PreferenceManager();
        StructuredSelection sel = (StructuredSelection) args[1];
        EObject selObj = (EObject) sel.getFirstElement();
        DataTypesDialog dDialog = new
            DataTypesDialog((Shell) args[0], pmanager, false, selObj);
        dDialog.open();
        return true;
    }

}

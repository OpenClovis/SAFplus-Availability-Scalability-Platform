/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/OperationPropertiesAction.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.dialog;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.IActionClassAdapter;
/**
 * @author shubhada
 *
 * Opens method property dialog.
 */
public class OperationPropertiesAction extends IActionClassAdapter
{
    /**
     * Visible only for Method.
     * @param environment Environment
     * @return true for method, false otherwise.
     */
    public boolean isVisible(Environment environment)
    {
        EClass eclass = (EClass) environment.getValue("eclass");
        return eclass.getName().equals("Method");
    }
    /**
     * Action Class.
     * @param args 0 - EObject for Method from Selection
     * @return true if action is successfull else false.
     */
    public boolean run(Object[] args)
    {
        StructuredSelection sel = (StructuredSelection) args[1];
        EObject operObj = (EObject) sel.getFirstElement();
        if (!sel.isEmpty()) {

            try {
                 OperationPropertiesDialog operDialog =
                     new OperationPropertiesDialog((Shell) args[0], operObj);
                    operDialog.open();
                } catch (Exception e) {
                    e.printStackTrace();
                }

        }
        return true;


    }
}

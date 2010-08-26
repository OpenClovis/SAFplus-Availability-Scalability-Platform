/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/action/DefineDatatypesAction.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.action;

import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.IActionClassAdapter;
import com.clovis.cw.editor.ca.dialog.DataTypesDialog;

/**
 * @author shubhada
 *
 * Define User Defined Datatypes Action
 */
public class DefineDatatypesAction extends IActionClassAdapter
{
    /**
     * return false as of now as we are not supporting global 
     * data types
     */
    public boolean isVisible(Environment environment)
    {
        return false;
    }

    /**
     * Method to open properties dialog.
     * @param args 0 -
     * @return whether action is successfull.
     */
    public boolean run(Object[] args)
    {
        PreferenceManager pmanager = new PreferenceManager();
        DataTypesDialog dDialog = new
            DataTypesDialog((Shell) args[0], pmanager, true, null);
        dDialog.open();
        return true;
    }

}

/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/ImportMibAction.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.dialog;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.IActionClassAdapter;
import com.clovis.cw.editor.ca.snmp.TreeAttributesDialog;

/**
 * @author shubhada
 *
 * Action class for importing attributes from mib.
 */
public class ImportMibAction extends IActionClassAdapter
{
    /**
     * Visible only for Attributes.
     * @param environment Environment
     * @return true for method, false otherwise.
     */
    public boolean isVisible(Environment environment)
    {
        EClass eclass = (EClass) environment.getValue("eclass");
        return eclass.getName().equals("Attribute")
        || eclass.getName().equals("ProvAttribute");
    }
    /**
     * Action Class.
     *
     * @param args Action args.
     * @return true if action is successful else false
     */
    public boolean run(Object[] args)
    {
        EList eList = (EList) args[1];
        EClass eClass = (EClass) args[2];
        new TreeAttributesDialog((Shell) args[0], eList, eClass).open();
        return true;
    }
}

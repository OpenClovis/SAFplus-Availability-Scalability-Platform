/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/table/action/DeleteAction.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.table.action;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EAnnotation;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.viewers.IStructuredSelection;

import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.IActionClassAdapter;
/**
 * @author shubhada
 *
 * Deletes entry from EList.
 */
public class DeleteAction extends IActionClassAdapter
{
	/**
	 * makes the button disabled if the selection is empty
	 * @param Environment environment
	 */
	public boolean isEnabled(Environment environment) 
	{
		IStructuredSelection sel = 
            (IStructuredSelection) environment.getValue("selection");
		if(sel.isEmpty()) {
			return false;
		}
		Object model = environment.getValue("model");
		if (model instanceof EList) {
			EList list = (EList) model;
			if (list.size() > 0) {
				EObject obj = (EObject) list.get(0);
				return canDeleteObject(obj.eClass(), list);
			}
		}
		return true;
	}
	/**
	 * Adds a new Object in the list.
	 * @param args 0 - Tableviewer, 1 - Elist
	 */
	
	public boolean run(Object[] args)
	{
        IStructuredSelection selection = (IStructuredSelection) args[0];
		EList  eList  = (EList) args[1];
        if (selection != null) {
            eList.removeAll(selection.toList());            
        }
		return true;
	}
	/** Checks the lower bound */
    private boolean canDeleteObject(EClass eClass, EList list) {
		EAnnotation ann = eClass.getEAnnotation("CWAnnotation");
		if (ann != null) {
			if (ann.getDetails().get("LowerBound") != null) {
				String annValue = (String) ann.getDetails().get("LowerBound");
				if (list.size() <= Integer.parseInt(annValue)) {
					return false;
				}
			}
		}
		return true;
    }
}

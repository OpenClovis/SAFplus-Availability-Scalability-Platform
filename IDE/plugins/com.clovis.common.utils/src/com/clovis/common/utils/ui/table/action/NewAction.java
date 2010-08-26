/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/table/action/NewAction.java $
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
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.widgets.Table;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.IActionClassAdapter;
import com.clovis.common.utils.ui.table.TableUI;
/**
 * @author shubhada
 *
 * Action Class for Adding a new row in a table
 */
public class NewAction extends IActionClassAdapter
{
    /**
     * Adds a new Object in the list.
     * @param args 0 - List, 1 - EClass
     * @return true on success.
     */
    public boolean run(Object[] args)
    {
        EList  eList  = (EList)  args[0];
        EClass eClass = (EClass) args[1];
        Table table = (Table) args[2];
        EObject eObj  = EcoreUtils.createEObject(eClass, true);
        String nameField = EcoreUtils.getNameField(eObj.eClass());
        if (nameField != null) {
            String name =
                EcoreUtils.getNextValue(eClass.getName(), eList, nameField);
            EcoreUtils.setValue(eObj, nameField, name);
        }
        // handle specific field initialization in generic manner
        String initializationInfo = EcoreUtils.getAnnotationVal(eObj.eClass(),
                null, "initializationFields");
        if( initializationInfo != null ) {
            EcoreUtils.initializeFields(eObj, eList, initializationInfo);
        }
        eList.add(eObj);
        table.setSelection(eList.indexOf(eObj));
        TableViewer tableViewer = (TableViewer) args[3];
        ((TableUI)tableViewer).selectionChanged(null);
        return super.run(args);
    }
    /**
     * Visible status of Menu.
     * @param  environment Environment
     * @return Visible status of Menu.
     */
    public boolean isEnabled(Environment environment)
    {
		Object model = environment.getValue("model");
		if (model instanceof EList) {
			EList list = (EList) model;
			if (list.size() > 0) {
				EObject obj = (EObject) list.get(0);
				return canCreateObject(obj.eClass(), list);
			}
		}

		return true;
    }
    /** Checks the upper bound */
    private boolean canCreateObject(EClass eClass, EList list) {
		EAnnotation ann = eClass.getEAnnotation("CWAnnotation");
		if (ann != null) {
			if (ann.getDetails().get("UpperBound") != null) {
				String annValue = (String) ann.getDetails().get("UpperBound");
				if (list.size() >= Integer.parseInt(annValue)) {
					return false;
				}
			}
		}
		return true;
    }
}

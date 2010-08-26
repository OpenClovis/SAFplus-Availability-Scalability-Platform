/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/table/action/DuplicateAction.java $
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
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TableViewer;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreCloneUtils;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.IActionClassAdapter;
import com.clovis.common.utils.ui.table.TableUI;

/**
 * @author shubhada
 *
 * Action Class for duplicating rows in a table based on selected row
 */
public class DuplicateAction extends IActionClassAdapter
{
    /**
     * makes the button disabled if the selection is empty
     * @param environment Environment
     * @return true if to Enable (On Selection)
     */
    public boolean isEnabled(Environment environment)
    {
        StructuredSelection sel =
            (StructuredSelection) environment.getValue("selection");
        if(sel.isEmpty()) {
        	return false;
        }
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
    /**
     * Duplicates the object based on the selection.
     * @param args 0 - selection 1 - TableViewer 2-EList
     * @return true on success.
     */
    public boolean run(Object[] args)
    {
        IStructuredSelection selection = (IStructuredSelection) args[0];
        
        EList  eList = (EList)  args[2];
        EObject sel  = (EObject) selection.getFirstElement();
        EObject clonedObj = EcoreCloneUtils.cloneEObject(sel);
        String nameField = EcoreUtils.getNameField(clonedObj.eClass());
        if (nameField != null) {
           String name  =
            EcoreUtils.getNextValue(sel.eClass().getName(), eList, nameField);           
            EcoreUtils.setValue(clonedObj, nameField, name);
        }
        EcoreUtils.setValue(clonedObj, ModelConstants.RDN_FEATURE_NAME,
        		clonedObj.toString());
//      handle specific field initialization in generic manner
        String initializationInfo = EcoreUtils.getAnnotationVal(sel.eClass(),
                null, "initializationFields");
        if( initializationInfo != null ) {
            EcoreUtils.initializeFields(clonedObj, eList, initializationInfo);
        }
        eList.add(clonedObj);
        TableViewer tableViewer = (TableViewer) args[1];
        tableViewer.getTable().deselect(tableViewer.getTable().getSelectionIndex());
        tableViewer.getTable().select(eList.size() - 1);
        ((TableUI)tableViewer).selectionChanged(null);
        return super.run(args);
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

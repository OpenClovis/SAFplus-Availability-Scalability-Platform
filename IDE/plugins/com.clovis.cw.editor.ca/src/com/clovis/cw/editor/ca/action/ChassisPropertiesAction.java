/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/action/ChassisPropertiesAction.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.action;

import org.eclipse.emf.ecore.EObject;

import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.widgets.Shell;


import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.IActionClassAdapter;
import com.clovis.common.utils.ui.dialog.PushButtonDialog;
import com.clovis.cw.editor.ca.editpart.ClassEditPart;
import com.clovis.cw.genericeditor.editparts.BaseEditPart;
import com.clovis.cw.genericeditor.model.NodeModel;


public class ChassisPropertiesAction extends IActionClassAdapter
{
	/**
     * Visible only for single selection
     * @param environment Environment
     * @return true for single selection.
     */
    public boolean isVisible(Environment environment)
    {
    	boolean retValue = false;
        StructuredSelection selection =
            (StructuredSelection) environment.getValue("selection");
        
        if (selection.size() == 1 && selection.getFirstElement() instanceof ClassEditPart) {
        	NodeModel nodeModel = (NodeModel)((ClassEditPart)selection.getFirstElement()).getModel();
        	if ("ChassisResource".equals(nodeModel.getEObject().eClass().getName())) {
        		retValue = true;
        	}
        }         
        return retValue;        
    }
    /**
     * Method to open properties dialog.
     * @param args 0 - EObject for Method from Selection
     * @return whether action is successfull.
     */
    public boolean run(Object[] args)
    {
    	StructuredSelection selection = (StructuredSelection) args[1];
        BaseEditPart cep = (BaseEditPart) selection.getFirstElement();
        
        NodeModel model = (NodeModel) cep.getModel();
        EObject eObj = model.getEObject();
        new PushButtonDialog((Shell) args[0], eObj.eClass(), eObj).open();
        return true;
    }
}

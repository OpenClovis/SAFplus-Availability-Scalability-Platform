/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/action/AssociationPropertiesAction.java $
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
import com.clovis.cw.genericeditor.editparts.AbstractEdgeEditPart;
import com.clovis.cw.genericeditor.model.EdgeModel;
import com.clovis.cw.genericeditor.model.NodeModel;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.editpart.AggregationEditPart;
import com.clovis.cw.editor.ca.editpart.ClassEditPart;
import com.clovis.cw.editor.ca.editpart.CompositionEditPart;
/**
 * @author Pushparaj
 *
 * Action Class for Class Properties. Opens main propertyu
 * dialog for Class.
 */
public class AssociationPropertiesAction extends IActionClassAdapter
{
    /**
     * Visible only for single selection
     * @param environment Environment
     * @return true for single selection.
     */
    public boolean isVisible(Environment environment)
    {
    	StructuredSelection selection =
            (StructuredSelection) environment.getValue("selection");
    	return (selection.size() == 1)
        && ((selection.getFirstElement() instanceof AggregationEditPart 
        && ! (selection.getFirstElement() instanceof CompositionEditPart))
        || (selection.getFirstElement() instanceof CompositionEditPart
        && checkTargetValid(selection.getFirstElement())));
    }
    /**
     * Method to open properties dialog.
     * @param args 0 - EObject for Method from Selection
     * @return whether action is successfull.
     */
    public boolean run(Object[] args)
    {
        StructuredSelection selection = (StructuredSelection) args[1];
        Object part = selection.getFirstElement();
        EdgeModel edge = null;
        if (part instanceof AggregationEditPart) {
        	AggregationEditPart editPart = (AggregationEditPart) part;
        	edge = ((EdgeModel) editPart.getModel());
        } else if (part instanceof CompositionEditPart) {
        	CompositionEditPart editPart = (CompositionEditPart) part;
        	edge = ((EdgeModel) editPart.getModel());
        }
        EObject obj = edge.getEObject();
        PushButtonDialog dialog = new PushButtonDialog((Shell) args[0],
        		obj.eClass(), obj);
        dialog.open();
        return true;
    }
    /**
     * 
     * @param element ConnectionEditPart
     * @return boolean
     */
    private boolean checkTargetValid(Object element) {
    	ClassEditPart targetPart = null;
    	if (((AbstractEdgeEditPart) element).getTarget() instanceof ClassEditPart) {
			targetPart = (ClassEditPart) ((AbstractEdgeEditPart) element)
					.getTarget();
			EObject obj = ((NodeModel) targetPart.getModel()).getEObject();
			if (obj.eClass().getName().equals(
					ClassEditorConstants.DATA_STRUCTURE_NAME)) {
				return true;
			}
		}
    	return false;
    }
}

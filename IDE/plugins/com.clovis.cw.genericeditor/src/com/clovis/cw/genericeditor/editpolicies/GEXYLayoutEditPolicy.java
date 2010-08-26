/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor.editpolicies;

import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.gef.EditPart;
import org.eclipse.gef.Request;
import org.eclipse.gef.commands.Command;
import org.eclipse.gef.editpolicies.XYLayoutEditPolicy;
import org.eclipse.gef.requests.CreateRequest;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.genericeditor.Messages;
import com.clovis.cw.genericeditor.commands.AddCommand;
import com.clovis.cw.genericeditor.commands.CreateCommand;
import com.clovis.cw.genericeditor.commands.SetConstraintCommand;
import com.clovis.cw.genericeditor.model.ContainerNodeModel;
import com.clovis.cw.genericeditor.model.EditorModel;
import com.clovis.cw.genericeditor.model.NodeModel;

/**
 * @author pushparaj
 *
 * Layout Policy for Container EditParts.
 */
public class GEXYLayoutEditPolicy extends XYLayoutEditPolicy {

	/**
	 * @see org.eclipse.gef.editpolicies.ConstrainedLayoutEditPolicy#createAddCommand(org.eclipse.gef.EditPart,
	 *      java.lang.Object)
	 */
	protected Command createAddCommand(EditPart childEditPart, Object constraint) {
		NodeModel part = (NodeModel) childEditPart.getModel();
		ContainerNodeModel parent = (ContainerNodeModel) getHost().getModel();
		String value = EcoreUtils.getAnnotationVal(part.getEObject().eClass(), null, "scope"); 
		if(value != null && value.equals("object")) {
			return null;
		}
		Rectangle rect = (Rectangle) constraint;
		if (parent instanceof EditorModel
				&& !(part._validParentObjects.contains(NodeModel.DEFAULT_PARENT))) {
			return null;
		} else if (parent instanceof ContainerNodeModel && !(parent instanceof EditorModel)
				&& !(part._validParentObjects.contains(parent.getEObject()
						.eClass().getName()))) {
			return null;
		} 
		AddCommand add = new AddCommand();
		add.setParent(parent);
		add.setChild(part);
		add.setLabel(Messages.ADDCOMMAND_LABEL);
		add.setDebugLabel(Messages.XYPOLICY_ADDCOMMAND_DEBUG_LABEL);

		SetConstraintCommand setConstraint = new SetConstraintCommand();
		setConstraint.setLocation(rect);
		setConstraint.setPart(part);
		setConstraint.setLabel(Messages.ADDCOMMAND_LABEL);
		setConstraint.setDebugLabel(Messages.XYPOLICY_SETCONSTRAINTCOMMAND_DEBUG_LABEL);
		return add.chain(setConstraint);
		//return null;
	}

	/**
	 * @see org.eclipse.gef.editpolicies.ConstrainedLayoutEditPolicy#createChangeConstraintCommand(org.eclipse.gef.EditPart,
	 *      java.lang.Object)
	 */
	protected Command createChangeConstraintCommand(EditPart child,
			Object constraint) {
		SetConstraintCommand locationCommand = new SetConstraintCommand();
		locationCommand.setPart((NodeModel) child.getModel());
		locationCommand.setLocation((Rectangle) constraint);
		return locationCommand;
	}

	/**
	 * @see org.eclipse.gef.editpolicies.LayoutEditPolicy#getCreateCommand(org.eclipse.gef.requests.CreateRequest)
	 */
	protected Command getCreateCommand(CreateRequest request) {
		NodeModel part = (NodeModel) request.getNewObject(); 
		ContainerNodeModel parent = (ContainerNodeModel) getHost().getModel();
		if (parent instanceof EditorModel
				&& !(part._validParentObjects.contains(NodeModel.DEFAULT_PARENT))) {
			return null;
		} else if (parent instanceof ContainerNodeModel && !(parent instanceof EditorModel)
				&& !(part._validParentObjects.contains(parent.getEObject()
						.eClass().getName()))) {
			return null;
		} 
		CreateCommand create = new CreateCommand();
		create.setParent(parent);
		create.setChild(part);
		Rectangle constraint = (Rectangle) getConstraintFor(request);
		create.setLocation(constraint);
		create.setLabel(Messages.CREATECOMMAND_LABEL);
		return create;
	}

	/**
	 * @see org.eclipse.gef.editpolicies.LayoutEditPolicy#getDeleteDependantCommand(org.eclipse.gef.Request)
	 */
	protected Command getDeleteDependantCommand(Request request) {
		// TODO Auto-generated method stub
		return null;
	}
	
}

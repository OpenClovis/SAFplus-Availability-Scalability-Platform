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

import java.util.List;

import org.eclipse.gef.EditPart;
import org.eclipse.gef.commands.Command;
import org.eclipse.gef.commands.CompoundCommand;
import org.eclipse.gef.editpolicies.ContainerEditPolicy;
import org.eclipse.gef.requests.CreateRequest;
import org.eclipse.gef.requests.GroupRequest;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.genericeditor.Messages;
import com.clovis.cw.genericeditor.commands.OrphanChildCommand;
import com.clovis.cw.genericeditor.model.ContainerNodeModel;
import com.clovis.cw.genericeditor.model.EditorModel;
import com.clovis.cw.genericeditor.model.NodeModel;


/**
 * @author pushparaj
 *
 * This is used for container editparts. This policy
 * handles add, create and orphan requests.
 */
public class GEContainerEditPolicy extends ContainerEditPolicy {
	
	/**
	 * @see org.eclipse.gef.editpolicies.ContainerEditPolicy#getCreateCommand(org.eclipse.gef.requests.CreateRequest)
	 */
	protected Command getCreateCommand(CreateRequest request) {
		return null;
	}
	
	/**
	 * @see org.eclipse.gef.editpolicies.ContainerEditPolicy#getOrphanChildrenCommand(org.eclipse.gef.requests.GroupRequest)
	 */
	public Command getOrphanChildrenCommand(GroupRequest request) {
		List parts = request.getEditParts();
		CompoundCommand result =
			new CompoundCommand(Messages.CONTAINERPOLICY_ORPHANCOMMAND_LABEL);
		for (int i = 0; i < parts.size(); i++) {
			NodeModel child = (NodeModel) ((EditPart) parts.get(i)).getModel();
			ContainerNodeModel parent = (ContainerNodeModel) getHost().getModel();
			String value = EcoreUtils.getAnnotationVal(child.getEObject().eClass(), null, "scope"); 
			if(value != null && value.equals("object")) {
				continue;
			}
			if (parent instanceof EditorModel
					&& !(child._validParentObjects.contains(NodeModel.DEFAULT_PARENT))) {
				continue;
			} else if (parent instanceof ContainerNodeModel && !(parent instanceof EditorModel)
					&& !(child._validParentObjects.contains(parent.getEObject()
							.eClass().getName()))) {
				continue;
			}
			OrphanChildCommand orphan = new OrphanChildCommand();
			orphan.setChild(child);
			orphan.setParent(parent);
			orphan.setLabel(Messages.ELEMENTPOLICY_ORPHANCOMMAND_LABEL);
			result.add(orphan);
		}
		return result.unwrap();
		//return null;
	}
	
}

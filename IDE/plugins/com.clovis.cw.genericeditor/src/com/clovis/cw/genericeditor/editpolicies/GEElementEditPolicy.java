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

import org.eclipse.gef.commands.Command;
import org.eclipse.gef.commands.CompoundCommand;
import org.eclipse.gef.requests.GroupRequest;

import com.clovis.cw.genericeditor.commands.DeleteCommand;
import com.clovis.cw.genericeditor.model.ContainerNodeModel;
import com.clovis.cw.genericeditor.model.NodeModel;


/**
 * @author pushparaj
 *
 * Policy for components with in a container
 */
public class GEElementEditPolicy extends org.eclipse.gef.editpolicies.ComponentEditPolicy {
	
	/**
	 * @see org.eclipse.gef.editpolicies.ComponentEditPolicy#createDeleteCommand(org.eclipse.gef.requests.GroupRequest)
	 */
	protected Command createDeleteCommand(GroupRequest request) {
		Object parent = getHost().getParent().getModel();
		CompoundCommand compound = new CompoundCommand();
		ContainerNodeModel parentModel = (ContainerNodeModel) parent;
		NodeModel childModel = (NodeModel) getHost().getModel();
		if (childModel instanceof ContainerNodeModel) {
			createDeleteCommandsForDependentChilds(compound, (ContainerNodeModel)childModel);
		}
		DeleteCommand deleteCmd = new DeleteCommand();
		deleteCmd.setParent(parentModel);
		deleteCmd.setChild(childModel);
		compound.add(deleteCmd);
		return compound;
	}
	/**
	 * Create Delete Commands for Child Nodes
	 * @param compound CompoundCommand
	 * @param parentModel ContainerModel
	 */
	private void createDeleteCommandsForDependentChilds(CompoundCommand compound, ContainerNodeModel parentModel) {
		List childs = parentModel.getChildren();
		for(int i = 0; i < childs.size(); i++) {
			NodeModel childModel = (NodeModel) childs.get(i);
			if(childModel instanceof ContainerNodeModel) {
				createDeleteCommandsForDependentChilds(compound, (ContainerNodeModel)childModel);
			}
			DeleteCommand deleteCmd = new DeleteCommand();
			deleteCmd.setParent(parentModel);
			deleteCmd.setChild(childModel);
			compound.add(deleteCmd);
		}
	}
}

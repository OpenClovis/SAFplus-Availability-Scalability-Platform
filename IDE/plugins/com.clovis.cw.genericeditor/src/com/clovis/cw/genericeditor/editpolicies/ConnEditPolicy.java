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

import org.eclipse.gef.commands.Command;
import org.eclipse.gef.requests.GroupRequest;

import com.clovis.cw.genericeditor.commands.ConnectionCommand;
import com.clovis.cw.genericeditor.model.EdgeModel;

/***
 * 
 * @author pushparaj
 *
 * This will creates Connection Command. 
 */
public class ConnEditPolicy
	extends org.eclipse.gef.editpolicies.ConnectionEditPolicy {
	
	/**
	 * @see org.eclipse.gef.editpolicies.ConnectionEditPolicy#getDeleteCommand(org.eclipse.gef.requests.GroupRequest)
	 */
	protected Command getDeleteCommand(GroupRequest request) {
		ConnectionCommand c = new ConnectionCommand();
		c.setConnectionModel((EdgeModel) getHost().getModel());
		return c;
	}
}

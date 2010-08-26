/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor.commands;

import org.eclipse.draw2d.Bendpoint;


/**
 * 
 * @author pushparaj
 *
 * Command for delete Bendpoints
 */
public class DeleteBendpointCommand extends BendpointCommand {

	private Bendpoint _bendPoint;
	
	/**
	 * @see org.eclipse.gef.commands.Command#execute()
	 */
	public void execute() {
		_bendPoint =
			(Bendpoint) getConnectionModel().getBendpoints().get(getIndex());
		getConnectionModel().removeBendpoint(getIndex());
		super.execute();
	}

	/**
	 * @see org.eclipse.gef.commands.Command#undo()
	 */
	public void undo() {
		super.undo();
		getConnectionModel().insertBendpoint(getIndex(), _bendPoint);
	}

}

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

import com.clovis.cw.genericeditor.model.ConnectionBendpoint;


/**
 * 
 * @author pushparaj
 *
 * Command for creating Bendpoints
 */
public class CreateBendpointCommand extends BendpointCommand {

	/**
	 * @see org.eclipse.gef.commands.Command#execute()
	 */
	public void execute() {
		ConnectionBendpoint wbp = new ConnectionBendpoint();
		wbp.setRelativeDimensions(
			getFirstRelativeDimension(),
			getSecondRelativeDimension());
		getConnectionModel().insertBendpoint(getIndex(), wbp);
		super.execute();
	}
	
	/**
	 * @see org.eclipse.gef.commands.Command#undo()
	 */
	public void undo() {
		super.undo();
		getConnectionModel().removeBendpoint(getIndex());
	}

}

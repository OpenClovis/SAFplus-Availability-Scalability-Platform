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

import com.clovis.cw.genericeditor.model.ConnectionBendpoint;


/**
 * 
 * @author pushparaj
 *
 * Command for moving  Bendpoints
 */
public class MoveBendpointCommand extends BendpointCommand {

	private Bendpoint _oldBendpoint;

	/**
	 * @see org.eclipse.gef.commands.Command#execute()
	 */
	public void execute() {
		ConnectionBendpoint bp = new ConnectionBendpoint();
		bp.setRelativeDimensions(
			getFirstRelativeDimension(),
			getSecondRelativeDimension());
		setOldBendpoint(
			(Bendpoint) getConnectionModel().getBendpoints().get(getIndex()));
		getConnectionModel().setBendpoint(getIndex(), bp);
		super.execute();
	}

	/**
	 * returns old bendpoint.
	 * @return
	 */
	protected Bendpoint getOldBendpoint() {
		return _oldBendpoint;
	}

	/**
	 * sets old bendpoint.
	 * @param bp
	 */
	public void setOldBendpoint(Bendpoint bp) {
		_oldBendpoint = bp;
	}

	/**
	 * @see org.eclipse.gef.commands.Command#undo()
	 */
	public void undo() {
		super.undo();
		getConnectionModel().setBendpoint(getIndex(), getOldBendpoint());
	}

}

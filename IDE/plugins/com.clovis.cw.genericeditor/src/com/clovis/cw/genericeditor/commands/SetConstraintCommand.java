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

import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.gef.commands.Command;

import com.clovis.cw.genericeditor.model.NodeModel;


/**
 * @author pushparaj
 *
 * Command for handling resize, location requests. 
 */
public class SetConstraintCommand extends Command{

	private Point _newPos;

	private Dimension _newSize;

	private Point _oldPos;

	private Dimension _oldSize;

	private NodeModel _childModel;
	
	/**
	 * @see org.eclipse.gef.commands.Command#execute()
	 */
	public void execute() {
		_oldSize = _childModel.getSize();
		_oldPos = _childModel.getLocation();
		_childModel.setLocation(_newPos);
		_childModel.setSize(_newSize);
	}

	/**
	 * @see org.eclipse.gef.commands.Command#redo()
	 */
	public void redo() {
		_childModel.setSize(_newSize);
		_childModel.setLocation(_newPos);
	}

	/**
	 * set location.
	 * @param r
	 */	
	public void setLocation(Rectangle r) {
		setLocation(r.getLocation());
		setSize(r.getSize());
	}
	
	/**
	 * set location.
	 * @param p
	 */
	public void setLocation(Point p) {
		_newPos = p;
	}
	
	/**
	 * set node model.
	 * @param part
	 */
	public void setPart(NodeModel part) {
		this._childModel = part;
	}
	
	/**
	 * set size.
	 * @param p
	 */
	public void setSize(Dimension p) {
		_newSize = p;
	}

	public void undo() {
		_childModel.setSize(_oldSize);
		_childModel.setLocation(_oldPos);
	}

}

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

import java.util.List;

import org.eclipse.draw2d.geometry.Point;

import org.eclipse.gef.commands.Command;

import com.clovis.cw.genericeditor.Messages;
import com.clovis.cw.genericeditor.model.ContainerNodeModel;
import com.clovis.cw.genericeditor.model.NodeModel;


/**
 * 
 * @author pushparaj
 *
 * Command used when node is being removed from its current parent,
 * to be inserted into a new parent.
 */
public class OrphanChildCommand extends Command {

	private Point _oldLocation;
	private ContainerNodeModel _parentModel;
	private NodeModel _childModel;
	private int _index;
	
	/**
	 * constructor
	 *
	 */
	public OrphanChildCommand() {
		super(Messages.ORPHANCHILDCOMMAND_LABEL);
	}

	/**
	 * @see org.eclipse.gef.commands.Command#execute()
	 */
	public void execute() {
		List children = _parentModel.getChildren();
		_index = children.indexOf(_childModel);
		_oldLocation = _childModel.getLocation();
		_parentModel.removeChild(_childModel);
	}
	
	/**
	 * @see org.eclipse.gef.commands.Command#redo()
	 */
	public void redo() {
		_parentModel.removeChild(_childModel);
	}

	/**
	 * set child node.
	 * @param child
	 */
	public void setChild(NodeModel child) {
		this._childModel = child;
	}

	/**
	 * set parent model.
	 * @param parent
	 */
	public void setParent(ContainerNodeModel parent) {
		_parentModel = parent;
	}

	/**
	 * @see org.eclipse.gef.commands.Command#undo()
	 */
	public void undo() {
		_childModel.setLocation(_oldLocation);
		_parentModel.addChild(_childModel, _index);
	}

}

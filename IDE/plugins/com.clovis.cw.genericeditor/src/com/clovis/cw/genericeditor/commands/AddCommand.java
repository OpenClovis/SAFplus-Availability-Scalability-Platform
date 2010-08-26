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

import org.eclipse.gef.commands.Command;

import com.clovis.cw.genericeditor.Messages;
import com.clovis.cw.genericeditor.model.ContainerNodeModel;
import com.clovis.cw.genericeditor.model.NodeModel;



/**
 * @author pushparaj
 *
 * Command for adding NodeModel(child) into ContainerNodeModel(parent)
 */
public class AddCommand extends Command {

	private NodeModel _childModel;

	private ContainerNodeModel _parentModel;

	private int _index = -1;
	
	/**
	 * constructor
	 */
	public AddCommand() {
		super(Messages.ADDCOMMAND_LABEL);
	}

	/**
	 * @see org.eclipse.gef.commands.Command#execute()
	 */
	public void execute() {
		if (_index < 0)
			_parentModel.addChild(_childModel);
		else
			_parentModel.addChild(_childModel, _index);
	}
	
	/**
	 * returns container model.
	 * @return parent
	 */
	public ContainerNodeModel getParent() {
		return _parentModel;
	}
	
	/**
	 * @see org.eclipse.gef.commands.Command#redo()
	 */
	public void redo() {
		if (_index < 0)
			_parentModel.addChild(_childModel);
		else
			_parentModel.addChild(_childModel, _index);
	}

	/**
	 * set node model.
	 * @param subpart
	 */
	public void setChild(NodeModel subpart) {
		_childModel = subpart;
	}

	/**
	 * set index.
	 * @param i
	 */
	public void setIndex(int i) {
		_index = i;
	}
	
	/**
	 * set the parent model.
	 * @param newParent
	 */
	public void setParent(ContainerNodeModel newParent) {
		_parentModel = newParent;
	}

	/**
	 * @see org.eclipse.gef.commands.Command#undo()
	 */
	public void undo() {
		_parentModel.removeChild(_childModel);
	}

}

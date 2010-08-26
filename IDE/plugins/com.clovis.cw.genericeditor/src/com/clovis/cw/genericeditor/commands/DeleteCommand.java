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

import java.util.ArrayList;
import java.util.List;

import org.eclipse.gef.commands.Command;

import com.clovis.cw.genericeditor.Messages;
import com.clovis.cw.genericeditor.model.ContainerNodeModel;
import com.clovis.cw.genericeditor.model.EdgeModel;
import com.clovis.cw.genericeditor.model.NodeModel;


/***
 * 
 * @author pushparaj
 *
 * Command for deleting NodeModel(child) from ContainerNodeModel(parent)
 */
public class DeleteCommand extends Command {

	private NodeModel _childModel;
	private ContainerNodeModel _parentModel;
	private List _sourceConnections = new ArrayList();
	private List _targetConnections = new ArrayList();
	
	/**
	 * constructor
	 *
	 */
	public DeleteCommand() {
		super(Messages.DELETECOMMAND_LABEL);
	}
	
	/**
	 * delete all source and target connections for part(node). 
	 * @param part
	 */
	private void deleteConnections(NodeModel part) {
		if (part instanceof ContainerNodeModel) {
			List children = ((ContainerNodeModel) part).getChildren();
			for (int i = 0; i < children.size(); i++)
				deleteConnections((NodeModel)children.get(i));
		}
		_sourceConnections.addAll(part.getSourceConnections());
		for (int i = 0; i < _sourceConnections.size(); i++) {
			EdgeModel transition =
				(EdgeModel) _sourceConnections.get(i);
            part.getRootModel().removeEObject(transition.getEObject());
			//transition.detachSource();
			//transition.detachTarget();
		}
		_targetConnections.addAll(part.getTargetConnections());
		for (int i = 0; i < _targetConnections.size(); i++) {
			EdgeModel transition =
				(EdgeModel) _targetConnections.get(i);
            part.getRootModel().removeEObject(transition.getEObject());
			//transition.detachSource();
			//transition.detachTarget();
		}
	}

	/**
	 * @see org.eclipse.gef.commands.Command#undo()
	 */
	public void execute() {
		primExecute();
	}
	
	/**
	 * 
	 *
	 */
	protected void primExecute() {
		deleteConnections(_childModel);
		_parentModel.getChildren().indexOf(_childModel);
		//_parentModel.removeChild(_childModel);
        _childModel.getRootModel().removeEObject(_childModel.getEObject());
	}
	
	/**
	 * @see org.eclipse.gef.commands.Command#redo()
	 */
	public void redo() {
		primExecute();
	}
	
	/**
	 * restore all the connections.
	 */
	private void restoreConnections() {
		for (int i = 0; i < _sourceConnections.size(); i++) {
			EdgeModel transition =
				(EdgeModel) _sourceConnections.get(i);
            _childModel.getRootModel().addEObject(transition.getEObject());
			//transition.attachSource();
			//transition.attachTarget();
		}
		_sourceConnections.clear();
		for (int i = 0; i < _targetConnections.size(); i++) {
			EdgeModel transition =
				(EdgeModel) _targetConnections.get(i);
            _childModel.getRootModel().addEObject(transition.getEObject());
			//transition.attachSource();
			//transition.attachTarget();
		}
		_targetConnections.clear();
	}
	
	/**
	 * set child node.
	 * @param c
	 */
	public void setChild(NodeModel c) {
		_childModel = c;
	}
	
	/**
	 * set parent model.
	 * @param p
	 */
	public void setParent(ContainerNodeModel p) {
		_parentModel = p;
	}

	/**
	 * @see org.eclipse.gef.commands.Command#undo()
	 */
	public void undo() {
		//_parentModel.addChild(_childModel, _index);
        _childModel.getRootModel().addEObject(_childModel.getEObject());
		restoreConnections();
	}

}

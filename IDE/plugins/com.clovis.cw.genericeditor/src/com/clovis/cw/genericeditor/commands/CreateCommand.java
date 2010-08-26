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

import org.eclipse.draw2d.geometry.Insets;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.gef.commands.Command;

import com.clovis.cw.genericeditor.Messages;
import com.clovis.cw.genericeditor.model.ContainerNodeModel;
import com.clovis.cw.genericeditor.model.NodeModel;

/**
 * @author pushparaj
 *
 * Command for adding new NodeModel(child) into ContainerNodeModel(parent)
 */
public class CreateCommand extends Command {
	private NodeModel _childModel;

	private Rectangle _rect;

	private ContainerNodeModel _parentModel;

	/**
	 * constructor
	 *
	 */
	public CreateCommand() {
		super(Messages.CREATECOMMAND_LABEL);
	}
	
	/**
	 * @see org.eclipse.gef.commands.Command#execute()
	 */
	public void execute() {
		if (_rect != null) {
			Insets expansion = getInsets();
			if (!_rect.isEmpty())
				_rect.expand(expansion);
			else {
				_rect.x -= expansion.left;
				_rect.y -= expansion.top;
			}
			_childModel.setLocation(_rect.getLocation());
			if (!_rect.isEmpty())
				_childModel.setSize(_rect.getSize());
		}
        _childModel.setParent(_parentModel);
		redo();
	}

	/**
	 * 
	 * @return insets
	 */
	private Insets getInsets() {
		if (_childModel instanceof NodeModel)
			return new Insets(2, 0, 2, 0);
		return new Insets();
	}
    
    /**
     * 
     * @return _childModel
     */
    public NodeModel getChild()
    {
    	return _childModel;
    }
    /**
     * 
     * @return _rect
     */
    public Rectangle getLocation(){
        return _rect;
    }
        
	/**
	 * returns parent model.
	 * @return parent
	 */
	public ContainerNodeModel getParent() {
		return _parentModel;
	}

	/**
	 * @see org.eclipse.gef.commands.Command#redo()
	 */
	public void redo() {
        _childModel.getRootModel().addEObject(_childModel.getEObject());
	}
	
	/**
	 * set child node.
	 * @param subpart
	 */
	public void setChild(NodeModel subpart) {
		_childModel = subpart;
	}
	
	/**
	 * set location
	 * @param r
	 */
	public void setLocation(Rectangle r) {
		_rect = r;
	}
	
	/**
	 * set parent model.
	 * @param newParent
	 */
	public void setParent(ContainerNodeModel newParent) {
		_parentModel = newParent;
	}

	/**
	 * @see org.eclipse.gef.commands.Command#undo()
	 */
	public void undo() {
        _childModel.getRootModel().removeEObject(_childModel.getEObject());
	}
	
}

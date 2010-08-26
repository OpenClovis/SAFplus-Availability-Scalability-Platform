/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/commands/CollapseCommand.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor.commands;

import java.util.List;

import com.clovis.cw.genericeditor.editparts.BaseEditPart;
import com.clovis.cw.genericeditor.model.Base;

/**
 * @author pushparaj
 * Command for Collapse
 **/
public class CollapseCommand extends TreeCommand
{
	/*private BaseEditPart _collapsedParent;*/
	/**
	 * Constructor
	 */
	public CollapseCommand(BaseEditPart part)
	{
		super("Collapse", part);
	}
	/**
	 * @see org.eclipse.gef.commands.Command#execute()
	 */
	public void execute()
	{
		redo();
	}
	/**
	 * @see org.eclipse.gef.commands.Command#undo()
	 */
	public void undo()
	{
		Base model = (Base) _collapsedParent.getModel();
		model.setCollapsedParent(false);
		List cons = _collapsedParent.getSourceConnections();
		removeCollapsedNodes(cons);
	}
	/**
	 * @see org.eclipse.gef.commands.Command#redo()
	 */
	public void redo()
	{
		Base model = (Base) _collapsedParent.getModel();
		model.setCollapsedParent(true);
		List cons = _collapsedParent.getSourceConnections();
		setCollapsedNodes(cons);
	}
	/**
	 * Sets Colapse flag in Nodes
	 * @param cons Connections list
	 *//*
	private void setCollapsedNodes(List cons)
	{
		for (int i = 0; i < cons.size(); i++) {
			AbstractEdgeEditPart part = (AbstractEdgeEditPart) cons.get(i);
			EdgeModel edge = (EdgeModel) part.getModel();
			part.getFigure().setVisible(false);
			edge.setCollapsedElement(true);
			BaseEditPart targetPart = (BaseEditPart) part.getTarget();
			NodeModel target = (NodeModel) targetPart.getModel();
			if (target.getTargetConnections().size() == 1
					|| isAllOtherParentsCollapsed(target.getTargetConnections(), edge)) {
				targetPart.getFigure().setVisible(false);
				target.setCollapsedElement(true);
				if (!target.isCollapsedParent())
					setCollapsedNodes(targetPart.getSourceConnections());
			}
		}
	}
	*//**
	 * Sets Colapse flag in Nodes
	 * @param cons Connections list
	 *//*
	private void removeCollapsedNodes(List cons)
	{
		for (int i = 0; i < cons.size(); i++) {
			AbstractEdgeEditPart part = (AbstractEdgeEditPart) cons.get(i);
			EdgeModel edge = (EdgeModel) part.getModel();
			part.getFigure().setVisible(true);
			edge.setCollapsedElement(false);
			BaseEditPart targetPart = (BaseEditPart) part.getTarget();
			NodeModel target = (NodeModel) targetPart.getModel();
			//if (target.getTargetConnections().size() == 1
					//|| isAllParentsCollapsed(target.getTargetConnections())) {
				targetPart.getFigure().setVisible(true);
				target.setCollapsedElement(false);
				if (!target.isCollapsedParent())
					removeCollapsedNodes(targetPart.getSourceConnections());
			//}
		}
	}
	*//**
	 * Checks all parent nodes.
	 * @param cons Target connections for node.
	 * @return boolean
	 *//*
	private boolean isAllOtherParentsCollapsed(Vector cons, EdgeModel model) {
		for (int i = 0; i < cons.size(); i++) {
			EdgeModel edge = (EdgeModel) cons.get(i);
			if(edge == model)
				continue;
			NodeModel source = edge.getSource();
			if(!source.isCollapsedElement() && !source.isCollapsedParent()) {
				return false;
			}
		}
		return true;
	}*/
}

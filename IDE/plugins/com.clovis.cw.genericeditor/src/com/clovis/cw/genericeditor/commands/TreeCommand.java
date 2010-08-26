/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/commands/TreeCommand.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.genericeditor.commands;

import java.util.List;
import java.util.Vector;

import org.eclipse.gef.commands.Command;

import com.clovis.cw.genericeditor.editparts.AbstractEdgeEditPart;
import com.clovis.cw.genericeditor.editparts.BaseEditPart;
import com.clovis.cw.genericeditor.model.EdgeModel;
import com.clovis.cw.genericeditor.model.NodeModel;

/**
 * @author pushparaj
 * Common Command for Collapse/Expand
 **/
public class TreeCommand extends Command
{
	protected BaseEditPart _collapsedParent;
	/**
	 * NULL Constructor.
	 *
	 */
	public TreeCommand()
	{
		super("");
	}
	/**
	 * Constructor
	 * @param cmd Command name
	 */
	public TreeCommand(String cmd, BaseEditPart part)
	{
		super(cmd);
		_collapsedParent = part;
	}
	/**
	 * Sets Colapse flag in Nodes
	 * @param cons Connections list
	 */
	protected void setCollapsedNodes(List cons)
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
	/**
	 * Sets Colapse flag in Nodes
	 * @param cons Connections list
	 */
	protected void removeCollapsedNodes(List cons)
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
	/**
	 * Checks all parent Nodes.
	 * @param cons Target connection for Node
	 * @return boolean
	 */
	private boolean isAllOtherParentsCollapsed(Vector cons, EdgeModel model) {
		for (int i = 0; i < cons.size(); i++) {
			EdgeModel edge = (EdgeModel) cons.get(i);
			if (model == edge)
				continue;
			NodeModel source = edge.getSource();
			if(!source.isCollapsedElement() && !source.isCollapsedParent()) {
				return false;
			}
		}
		return true;
	}
}

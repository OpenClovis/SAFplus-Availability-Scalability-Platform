/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/commands/AutoArrangeCommand.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor.commands;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.eclipse.draw2d.graph.CompoundDirectedGraph;
import org.eclipse.draw2d.graph.CompoundDirectedGraphLayout;
import org.eclipse.gef.commands.Command;

import com.clovis.cw.genericeditor.editparts.AbstractEdgeEditPart;
import com.clovis.cw.genericeditor.editparts.BaseDiagramEditPart;
import com.clovis.cw.genericeditor.editparts.BaseEditPart;
import com.clovis.cw.genericeditor.model.EdgeModel;
import com.clovis.cw.genericeditor.model.NodeModel;

/**
 * Command for Auto Arranging nodes and edges.
 * @author pushparaj
 *
 */
public class AutoArrangeCommand extends Command
{
	private BaseDiagramEditPart _rootEditPart;
	
	/**
	 * Constructor 
	 * @param part Root EditPart
	 */
	public AutoArrangeCommand(BaseDiagramEditPart part)
	{
		super("Auto Arrange");
		_rootEditPart = part;
	}
	/**
	 * @see org.eclipse.gef.commands.Command#redo()
	 */
	public void redo()
	{
		_rootEditPart.arrangeNodesAndEdges();
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
		updateOldValues(_rootEditPart.getChildren());
	}
	/**
	 * Update Old Values for Nodes and Edges.
	 * @param parts Child Edit Parts.
	 */
	private void updateOldValues(List parts)
	{
		for (int i = 0; i < parts.size(); i++) {
			BaseEditPart part = (BaseEditPart) parts.get(i);
			NodeModel node = (NodeModel) part.getModel();
			node.setLocation(node.getOldLocation());
			List cons = part.getSourceConnections();
			for (int j = 0; j < cons.size(); j++) {
				AbstractEdgeEditPart edgePart = (AbstractEdgeEditPart) cons.get(j);
				EdgeModel edge = (EdgeModel) edgePart.getModel();
				edge.setSourceTerminal(edge.getOldSourceTerminal());
				edge.setTargetTerminal(edge.getOldTargetTerminal());
				edgePart.refresh();
				BaseEditPart nodePart = (BaseEditPart) edgePart.getTarget();
				updateOldValues(nodePart.getChildren());
			}
		}
	}
}

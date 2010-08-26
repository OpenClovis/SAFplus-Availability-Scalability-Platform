/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/editpart/MibEditPart.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.editor.ca.editpart;

import java.util.HashMap;
import java.util.Map;

import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.graph.CompoundDirectedGraph;
import org.eclipse.draw2d.graph.CompoundDirectedGraphLayout;
import org.eclipse.draw2d.graph.Subgraph;

import com.clovis.cw.editor.ca.figure.MibFigure;
import com.clovis.cw.genericeditor.editparts.AbstractContainerNodeEditPart;
import com.clovis.cw.genericeditor.editparts.BaseEditPart;
import com.clovis.cw.genericeditor.model.NodeModel;

/**
 * 
 * @author pushparaj
 * Mib EditPart
 */
public class MibEditPart extends AbstractContainerNodeEditPart {
	
	/**
	 * @see org.eclipse.gef.editparts.AbstractGraphicalEditPart#createFigure()
	 */
	protected IFigure createFigure() {
		return new MibFigure((NodeModel)getModel());
	}
	/**
	 * @see org.eclipse.gef.GraphicalEditPart#getContentPane()
	 */
	public IFigure getContentPane() {
		return ((MibFigure)getFigure()).getContentsPane();
	}
	public void applyGraphResults(CompoundDirectedGraph graph, Map map) {
		for (int i = 0; i < getChildren().size(); i++) {
			BaseEditPart part = (BaseEditPart)getChildren().get(i);
			if(!part.getBase().isCollapsedElement())
				part.applyGraphResult(graph, map);
		}
	}
	public void contributeEdgesToGraph(CompoundDirectedGraph graph, Map map) {
    	for (int i = 0; i < getChildren().size(); i++) {
    		BaseEditPart child = (BaseEditPart)children.get(i);
    		if(!child.getBase().isCollapsedElement())
    			child.contributeEdgeToGraph(graph, map);
    	}
    }
	public void contributedNodesToGraph(CompoundDirectedGraph graph, Subgraph s, Map map) {
    	Subgraph me = new Subgraph(this, s);
		me.outgoingOffset = 5;
		me.incomingOffset = 5;
		me.innerPadding = INNER_PADDING;
		me.setPadding(PADDING);
		map.put(this, me);
		graph.nodes.add(me);
		for (int i = 0; i < getChildren().size(); i++) {
			BaseEditPart activity = (BaseEditPart)getChildren().get(i);
			if(!activity.getBase().isCollapsedElement())
				activity.contributeNodeToGraph(graph, me, map);
		}
	}
	public void arrangeNodesAndEdges() {
    	CompoundDirectedGraph graph = new CompoundDirectedGraph();
		Map partsToNodes = new HashMap();
		contributedNodesToGraph(graph, null, partsToNodes);
		contributeEdgesToGraph(graph, partsToNodes);
		new CompoundDirectedGraphLayout().visit(graph);
		applyGraphResults(graph, partsToNodes);
		arrangeConnections();
    }
	/**
	 * @see com.clovis.cw.genericeditor.editparts.AbstractContainerNodeEditPart#removeWhenEmpty()
	 */
	public boolean removeWhenEmpty() {
		return true;
	}
}



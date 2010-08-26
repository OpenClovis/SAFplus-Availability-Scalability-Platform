/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor.editparts;

import java.beans.PropertyChangeEvent;
import java.util.HashMap;
import java.util.Map;

import org.eclipse.draw2d.AutomaticRouter;
import org.eclipse.draw2d.BendpointConnectionRouter;
import org.eclipse.draw2d.ConnectionLayer;
import org.eclipse.draw2d.FanRouter;
import org.eclipse.draw2d.Figure;
import org.eclipse.draw2d.FreeformLayer;
import org.eclipse.draw2d.FreeformLayout;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.ManhattanConnectionRouter;
import org.eclipse.draw2d.MarginBorder;
import org.eclipse.draw2d.graph.CompoundDirectedGraph;
import org.eclipse.draw2d.graph.CompoundDirectedGraphLayout;
import org.eclipse.draw2d.graph.Subgraph;
import org.eclipse.gef.AccessibleEditPart;
import org.eclipse.gef.DragTracker;
import org.eclipse.gef.EditPolicy;
import org.eclipse.gef.LayerConstants;
import org.eclipse.gef.Request;
import org.eclipse.gef.editpolicies.RootComponentEditPolicy;
import org.eclipse.gef.requests.SelectionRequest;
import org.eclipse.gef.tools.DeselectAllTracker;
import org.eclipse.gef.tools.MarqueeDragTracker;
import org.eclipse.swt.accessibility.AccessibleEvent;
import org.eclipse.swt.graphics.Color;

import com.clovis.cw.genericeditor.Messages;
import com.clovis.cw.genericeditor.model.EditorModel;


/**
 *
 * @author pushparaj
 *
 * This is the Root EditPart class for all Editors
 */
public class BaseDiagramEditPart extends AbstractContainerNodeEditPart
    implements LayerConstants
{
    /**
     * @see com.clovis.cw.editors.ge.editpart.BaseEditPart#createAccessible()
     */
    protected AccessibleEditPart createAccessible()
    {
        return new AccessibleGraphicalEditPart()
        {
            public void getName(AccessibleEvent e)
            {
                e.result = Messages.BASEDIAGRAM_LABELTEXT;
            }
        };
    }

    /**
     * @see org.eclipse.gef.editparts.AbstractEditPart#createEditPolicies()
     */
    protected void createEditPolicies()
    {
        super.createEditPolicies();
        installEditPolicy(EditPolicy.NODE_ROLE, null);
        installEditPolicy(EditPolicy.GRAPHICAL_NODE_ROLE, null);
        installEditPolicy(EditPolicy.SELECTION_FEEDBACK_ROLE, null);
        installEditPolicy(
            EditPolicy.COMPONENT_ROLE,
            new RootComponentEditPolicy());
    }

    /**
     * @see org.eclipse.gef.editparts.AbstractGraphicalEditPart#createFigure()
     */
    protected IFigure createFigure()
    {
        Figure f = new FreeformLayer();
        f.setLayoutManager(new FreeformLayout());
        f.setBorder(new MarginBorder(5));
        f.setBackgroundColor(new Color(null, 255, 250, 250));
        f.setOpaque(true);
        return f;
    }

    /**
     * Returns the model of this as a BaseDiagram.
     *
     * @return  BaseDiagram of this.
     */
    protected EditorModel getEditorModel()
    {
        return (EditorModel) getModel();
    }

    /**
     * @see org.eclipse.gef.EditPart#getDragTracker(org.eclipse.gef.Request)
     */
    public DragTracker getDragTracker(Request req)
    {
        if (req instanceof SelectionRequest
            && ((SelectionRequest) req).getLastButtonPressed() == 3) {
            return new DeselectAllTracker(this);
        }
        return new MarqueeDragTracker();
    }

    /**
     * @see java.beans.PropertyChangeListener#propertyChange
     * (java.beans.PropertyChangeEvent)
     */
    public void propertyChange(PropertyChangeEvent evt)
    {
        if (EditorModel.ID_ROUTER.equals(evt.getPropertyName())) {
            refreshVisuals();
        } else {
            super.propertyChange(evt);
        }
    }

    /**
     * @see org.eclipse.gef.editparts.AbstractEditPart#refreshVisuals()
     */
    public void refreshVisuals()
    {
        ConnectionLayer cLayer = (ConnectionLayer) getLayer(CONNECTION_LAYER);
        if (getEditorModel()
            .getConnectionRouter()
            .equals(EditorModel.ROUTER_MANUAL)) {
            AutomaticRouter router = new FanRouter();
            router.setNextRouter(new BendpointConnectionRouter());
            cLayer.setConnectionRouter(router);
        } else {
            cLayer.setConnectionRouter(new ManhattanConnectionRouter());
        }
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
    public void contributeNodesToGraph(CompoundDirectedGraph graph, Subgraph s, Map map) {
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
			if(activity instanceof AbstractContainerNodeEditPart) {
				((AbstractContainerNodeEditPart)activity).arrangeNodesAndEdges();
			}
		}
	}
    public void arrangeNodesAndEdges() {
    	CompoundDirectedGraph graph = new CompoundDirectedGraph();
		Map partsToNodes = new HashMap();
		contributeNodesToGraph(graph, null, partsToNodes);
		contributeEdgesToGraph(graph, partsToNodes);
		new CompoundDirectedGraphLayout().visit(graph);
		applyGraphResults(graph, partsToNodes);
		arrangeConnections();
    }
}

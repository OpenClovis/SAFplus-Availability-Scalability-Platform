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
import java.beans.PropertyChangeListener;
import java.util.List;
import java.util.Map;
import java.util.Vector;

import org.eclipse.draw2d.ConnectionAnchor;
import org.eclipse.draw2d.IFigure;

import org.eclipse.draw2d.geometry.Insets;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.draw2d.graph.CompoundDirectedGraph;
import org.eclipse.draw2d.graph.Node;
import org.eclipse.draw2d.graph.Subgraph;
import org.eclipse.gef.AccessibleEditPart;
import org.eclipse.gef.ConnectionEditPart;
import org.eclipse.gef.EditPolicy;
import org.eclipse.gef.GraphicalEditPart;
import org.eclipse.gef.NodeEditPart;
import org.eclipse.gef.Request;
import org.eclipse.gef.RequestConstants;
import org.eclipse.gef.requests.DropRequest;
import org.eclipse.gef.editparts.AbstractGraphicalEditPart;
import org.eclipse.jface.viewers.StructuredSelection;

import com.clovis.common.utils.menu.MenuAction;
import com.clovis.cw.genericeditor.GEContextMenuProvider;
import com.clovis.cw.genericeditor.figures.AbstractNodeFigure;
import com.clovis.cw.genericeditor.model.Base;
import com.clovis.cw.genericeditor.model.EdgeModel;
import com.clovis.cw.genericeditor.editpolicies.GENodeEditPolicy;
import com.clovis.cw.genericeditor.editpolicies.GEElementEditPolicy;
/**
 *
 * @author pushparaj
 * 
 * Base EditPart class for all Node EditParts
 * all EditPart's basic functionalities are implemented 
 * in this class.
 */
public abstract class BaseEditPart extends AbstractGraphicalEditPart
    implements PropertyChangeListener, NodeEditPart
{

    private AccessibleEditPart _accessibleEditPart;
    /**
     * @see org.eclipse.gef.EditPart#activate()
     */
    public void activate()
    {
        if (!isActive()) {
            super.activate();
            // This editpart is registered as a listener of its
            // model object to be informed about its changes, and
            // when these changes occur, it knows how to update
            // the view according to the new state of the model
            // object.
            getBase().addPropertyChangeListener(this);
        }
    }
    /**
     * Install edit policies.
     */
    protected void createEditPolicies()
    {
    	installEditPolicy(
                EditPolicy.COMPONENT_ROLE, new GEElementEditPolicy());
        installEditPolicy(
                EditPolicy.GRAPHICAL_NODE_ROLE, new GENodeEditPolicy());
    }
    /**
     * child class must implements this method.
     * @return AccessibleEditPart
     */
    protected abstract AccessibleEditPart createAccessible();
    /**
     * Deactivate the edit part, remove property change listener.
     */
    public void deactivate()
    {
        if (isActive()) {
            super.deactivate();
            // remove property change listener from model
            getBase().removePropertyChangeListener(this);
        }
    }
    /**
     * Get AccessibleEditPart.
     * @return AccessibleEditPart
     */
    protected AccessibleEditPart getAccessibleEditPart()
    {
        if (_accessibleEditPart == null) {
            _accessibleEditPart = createAccessible();
        }
        return _accessibleEditPart;
    }
    /**
     * Returns the model associated with this as a Base.
     * @return The model of this as a Base.
     */
    public Base getBase()
    {
        return (Base) getModel();
    }
    /**
     * Gets Model Source Connections.
     * @return list of source connections.
     */
    protected List getModelSourceConnections()
    {
        return getBase().getSourceConnections();
    }
    /**
     * Gets Model Target Connections.
     * @return list of target connections.
     */
    protected List getModelTargetConnections()
    {
        return getBase().getTargetConnections();
    }
    /**
     * Returns the Figure of this, as a node type figure.
     *
     * @return Figure as a NodeFigure.
     */
    public AbstractNodeFigure getNodeFigure()
    {
        return (AbstractNodeFigure) getFigure();
    }
    /**
     * Get Source connection anchor.
     * @param connEditPart ConnectionEditPart
     * @return Source Connection anchor
     */
    public ConnectionAnchor getSourceConnectionAnchor(
            ConnectionEditPart connEditPart)
    {
        EdgeModel transition = (EdgeModel) connEditPart.getModel();
        return getNodeFigure().getConnectionAnchor(
                transition.getSourceTerminal());
    }
    /**
     * Get Source connection anchor.
     * @param request Drop Request
     * @return Source Connection anchor
     */
    public ConnectionAnchor getSourceConnectionAnchor(Request request)
    {
        Point pt = new Point(((DropRequest) request).getLocation());
        return getNodeFigure().getSourceConnectionAnchorAt(pt);
    }
    /**
     * Get Target connection anchor.
     * @param connEditPart ConnectionEditPart
     * @return Target Connection anchor
     */
    public ConnectionAnchor getTargetConnectionAnchor(
            ConnectionEditPart connEditPart)
    {
        EdgeModel transition = (EdgeModel) connEditPart.getModel();
        return getNodeFigure().getConnectionAnchor(
                transition.getTargetTerminal());
    }
    /**
     * Get Target connection anchor.
     * @param request Drop Request
     * @return Target Connection anchor
     */
    public ConnectionAnchor getTargetConnectionAnchor(Request request)
    {
        Point pt = new Point(((DropRequest) request).getLocation());
        return getNodeFigure().getTargetConnectionAnchorAt(pt);
    }
    /**
     * Returns the name of the given connection anchor.
     * @param c Connection Anchor
     * @return The name of the ConnectionAnchor as a String.
     */
    public final String mapConnectionAnchorToTerminal(ConnectionAnchor c)
    {
        return getNodeFigure().getConnectionAnchorName(c);
    }
    /**
     * Property change callback
     * @param evt PropertyChangeEvent
     */
    public void propertyChange(PropertyChangeEvent evt)
    {
    	refreshChildren();
        refreshTargetConnections();
        refreshSourceConnections();
        refreshVisuals();
        String property = evt.getPropertyName();
        if(property.equals("selection")) {
        	refreshSelection();
        }
        if(property.equals("focus")) {
    		refreshFocusAndReveal();
    	}
    	if(property.equals("appendselection")) {
    		getViewer().appendSelection(this);
    	}
    }
    /**
     * Make selected edipart to visible
     *
     */
    private void refreshFocusAndReveal()
    {
    	getViewer().reveal(this);
    }
    /**
     * Create Figure
     * @return null
     */
    protected IFigure createFigure()
    {
        return null;
    }
    /**
     * Handle Request like double click Goes through list of attached Menu and
     * put default menu as doubleclick action.
     *
     * @param req Request
     */
    public void performRequest(Request req)
    {
        if (req.getType().equals(RequestConstants.REQ_OPEN)) {
            GEContextMenuProvider menu = (GEContextMenuProvider) this
                    .getViewer().getContextMenu();
            menu.buildContextMenu(menu);
            MenuAction action = menu.getDefaultAction();
            if (action != null) {
                action.run();
            }
        }
        super.performRequest(req);
    }
    /**
     * Refresh Visuals
     */
    public void refreshVisuals()
    {
        Base base = getBase();
          	((GraphicalEditPart) getParent()).setLayoutConstraint(this,
                    getFigure(), new Rectangle(base.getLocation(), base.getSize()));	
    }
    /**
     * Refreshes the selection
     *
     */
    public void refreshSelection()
    {
        Vector parts  =  new Vector();
        parts.add(this);
        StructuredSelection sel = new StructuredSelection(parts);
        this.getViewer().setSelection(sel);
    }
    /**
     * Creates Node for each EditPart and added into map
     * @param graph parent Graph
     * @param s sub Graph
     * @param map Map
     */
    public void contributeNodeToGraph(CompoundDirectedGraph graph, Subgraph s, Map map) {
    	Node n = new Node(this, s);
    	n.outgoingOffset = 5;
    	n.incomingOffset = 5;
    	//n.width = getFigure().getPreferredSize().width;
    	//n.height = getFigure().getPreferredSize().height;
    	n.width = ((Base)getModel()).getSize().width;
    	n.height = ((Base)getModel()).getSize().height;
    	n.setPadding(new Insets(20,40,20,40));
    	map.put(this, n);
    	graph.nodes.add(n);
    }
    public void applyGraphResult(CompoundDirectedGraph graph, Map map) {
    	Node n = (Node)map.get(this);
    	Base model = (Base)getModel();
    	model.setLocation(new Point(n.x, n.y));
    	//model.setSize(new Dimension(n.width, n.height));
    	getFigure().setBounds(new Rectangle(n.x, n.y, n.width, n.height));
    	for (int i = 0; i < getSourceConnections().size(); i++) {
    		AbstractEdgeEditPart trans = (AbstractEdgeEditPart) getSourceConnections().get(i);
    		if(!trans.getConnectionModel().isCollapsedElement())
    			trans.applyGraphResults(graph, map);
    	}
    	
    }
    public void contributeEdgeToGraph(CompoundDirectedGraph graph, Map map) {
    	List outgoing = getSourceConnections();
    	for (int i = 0; i < outgoing.size(); i++) {
    		AbstractEdgeEditPart part = (AbstractEdgeEditPart)getSourceConnections().get(i);
    		if(!part.getConnectionModel().isCollapsedElement())
    			part.contributeToGraph(graph, map);
    		
    	}
    }
    /**
     * Arrange all source and target connections
     * based on source and target locations
     */
    public void arrangeConnections() {
    	List outgoing = getSourceConnections();
    	Base model = (Base)getModel();
    	model.getLeftSourceConnectionsList().clear();
    	model.getRightSourceConnectionsList().clear();
    	model.getCentreSourceConnectionsList().clear();
    	for (int i = 0; i < outgoing.size(); i++) {
    		AbstractEdgeEditPart part = (AbstractEdgeEditPart)outgoing.get(i);
    		if(!part.getConnectionModel().isCollapsedElement())
    			model.addSourceConnections(part.getConnectionModel());
    	}
    	List incoming = getTargetConnections();
    	model.getLeftTargetConnectionsList().clear();
    	model.getRightTargetConnectionsList().clear();
    	model.getCentreTargetConnectionsList().clear();
    	for (int i = 0; i < incoming.size(); i++) {
    		AbstractEdgeEditPart part = (AbstractEdgeEditPart)incoming.get(i);
    		if(!part.getConnectionModel().isCollapsedElement())
    			model.addTargetConnections(part.getConnectionModel());
    	}
    	updateSourceAndTargetTerminals(model);
    	for (int i = 0; i < outgoing.size(); i++) {
    		AbstractEdgeEditPart part = (AbstractEdgeEditPart)outgoing.get(i);
    		part.refresh();
    	}
    	for (int i = 0; i < incoming.size(); i++) {
    		AbstractEdgeEditPart part = (AbstractEdgeEditPart)incoming.get(i);
    		part.refresh();
    	}
    	for (int i = 0; i < getChildren().size(); i++) {
    		BaseEditPart child = (BaseEditPart)children.get(i);
    		if(!child.getBase().isCollapsedElement())
    			child.arrangeConnections();
    	}
    }
    /**
     * Source and Target anchors are updated after
     * arranging the connections.
     * @param model EdgeModel
     */
    private void updateSourceAndTargetTerminals(Base model)
    {
    	List cons = model.getRightSourceConnectionsList();
    	int index = 34;
    	for (int i = 0; i < cons.size(); i++) {
    		EdgeModel edge = (EdgeModel) cons.get(i);
    		edge.setSourceTerminal(String.valueOf(index));
    		if(index == 52) {
    			index = 32;
    		} //else {
    			index = index + 2;
    		//}
    	}
    	cons = model.getLeftSourceConnectionsList();
    	index = 28;
    	for (int i = 0; i < cons.size(); i++) {
    		EdgeModel edge = (EdgeModel) cons.get(i);
    		edge.setSourceTerminal(String.valueOf(index));
    		if(index == 10) {
    			index = 30;
    		} //else {
    			index = index - 2;
    		//}
    	}
    	cons = model.getRightTargetConnectionsList();
    	if(cons.size() > 0) {
    		EdgeModel edge = (EdgeModel) cons.get(0);
    		edge.setTargetTerminal(String.valueOf((char)66));
    	}
    	index = 122;
    	for (int i = 1; i < cons.size(); i++) {
    		EdgeModel edge = (EdgeModel) cons.get(i);
    		edge.setTargetTerminal(String.valueOf((char)index));
    		if(index == 98) {
    			index = 124;
    			//System.out.println((char)124);
    		} //else {
    			index = index - 2;
    		//}
    	}
    	cons = model.getLeftTargetConnectionsList();
    	index = 72;
    	for (int i = 0; i < cons.size(); i++) {
    		EdgeModel edge = (EdgeModel) cons.get(i);
    		edge.setTargetTerminal(String.valueOf((char)index));
    		if(index == 90) {
    			index = 70;
    		} //else {
    			index = index + 2;
    		//}
    	}
    	cons = model.getCentreSourceConnectionsList();
    	index = 29;
    	for (int i = 0; i < cons.size(); i++) {
    		EdgeModel edge = (EdgeModel) cons.get(i);
    		edge.setSourceTerminal(String.valueOf(index));
    		if(index == 33) {
    			index = 32;
    		} //else {
    			index = index + 1;
    		//}
    	}
    	cons = model.getCentreTargetConnectionsList();
    	index = 71;
    	for (int i = 0; i < cons.size(); i++) {
    		EdgeModel edge = (EdgeModel) cons.get(i);
    		edge.setTargetTerminal(String.valueOf((char)index));
    		if(index == 66) {
    			index = 67;
    		} //else {
    			index = index - 1;
    		//}
    	}
    }
}

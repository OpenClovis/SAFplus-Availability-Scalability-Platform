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
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;

import org.eclipse.draw2d.AbsoluteBendpoint;
import org.eclipse.draw2d.Connection;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.ManhattanConnectionRouter;
import org.eclipse.draw2d.PolylineConnection;
import org.eclipse.draw2d.RelativeBendpoint;
import org.eclipse.draw2d.graph.CompoundDirectedGraph;
import org.eclipse.draw2d.graph.Edge;
import org.eclipse.draw2d.graph.Node;
import org.eclipse.draw2d.graph.NodeList;
import org.eclipse.gef.AccessibleEditPart;
import org.eclipse.gef.EditPolicy;
import org.eclipse.gef.Request;
import org.eclipse.gef.RequestConstants;
import org.eclipse.gef.editparts.AbstractConnectionEditPart;
import org.eclipse.swt.accessibility.AccessibleEvent;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Display;

import com.clovis.common.utils.menu.MenuAction;
import com.clovis.cw.genericeditor.GEContextMenuProvider;
import com.clovis.cw.genericeditor.Messages;
import com.clovis.cw.genericeditor.editpolicies.ConnBendpointEditPolicy;
import com.clovis.cw.genericeditor.editpolicies.ConnEditPolicy;
import com.clovis.cw.genericeditor.editpolicies.ConnEndpointEditPolicy;
import com.clovis.cw.genericeditor.figures.AbstractConnectionFigure;
import com.clovis.cw.genericeditor.model.ConnectionBendpoint;
import com.clovis.cw.genericeditor.model.EdgeModel;


/**
 *
 * @author pushparaj
 *
 * Base EditPart class for Connection
 */
public class AbstractEdgeEditPart extends AbstractConnectionEditPart
    implements PropertyChangeListener
{
    public static final Color DEAD = new Color(Display.getDefault(), 0, 0, 0);

    private AccessibleEditPart _acc;
    
    /**
     * @see org.eclipse.gef.editparts.AbstractGraphicalEditPart#createFigure()
     */
    protected IFigure createFigure()
    {
    	return new AbstractConnectionFigure(getConnectionModel());
    }
    /**
     * @see org.eclipse.gef.EditPart#activate()
     */
    public void activate()
    {
        super.activate();

        /* This editpart is registered as a listener of its model
         * object to be informed about its changes, and when these
         * changes occur, it knows how to update the view
         * according to the new state of the model object.
         */
        getConnectionModel().addPropertyChangeListener(this);
    }

    /**
     * @see org.eclipse.gef.editparts
     * .AbstractConnectionEditPart#activateFigure()
     */
    public void activateFigure()
    {
        super.activateFigure();
        /* Once the figure has been added to the ConnectionLayer,
         * start listening for its router to change.
         */
        getFigure().addPropertyChangeListener(
            Connection.PROPERTY_CONNECTION_ROUTER,
            this);
    }

    /**
     * @see org.eclipse.gef.editparts.AbstractEditPart#createEditPolicies()
     */
    protected void createEditPolicies()
    {
        installEditPolicy(
            EditPolicy.CONNECTION_ENDPOINTS_ROLE,
            new ConnEndpointEditPolicy());
        //Note that the Connection is already added to the diagram and knows
        //its Router.
        refreshBendpointEditPolicy();
        installEditPolicy(EditPolicy.CONNECTION_ROLE, new ConnEditPolicy());
    }

    /**
     * @see org.eclipse.gef.EditPart#deactivate()
     */
    public void deactivate()
    {
        // remove property change listener from model
        getConnectionModel().removePropertyChangeListener(this);
        super.deactivate();
    }

    /**
     * @see org.eclipse.gef.editparts.AbstractConnectionEditPart
     * #deactivateFigure()
     */
    public void deactivateFigure()
    {
        getFigure().removePropertyChangeListener(
            Connection.PROPERTY_CONNECTION_ROUTER,
            this);
        super.deactivateFigure();
    }

    /**
     * @return acc
     */
    public AccessibleEditPart getAccessibleEditPart()
    {
        if (_acc == null) {
            _acc = new AccessibleGraphicalEditPart() {
                public void getName(AccessibleEvent e)
                {
                    e.result = Messages.CONNECTION_LABELTEXT;
                }
            };
        }
        return _acc;
    }

    /**
     * Returns the model of this represented as a Transition.
     *
     * @return  Model of this as <code>Transition</code>
     */
    protected EdgeModel getConnectionModel()
    {
        return (EdgeModel) getModel();
    }

    /**
     * Returns the Figure associated with this, which draws the
     * Transition.
     *
     * @return  Figure of this.
     */
    protected IFigure getConnFigure()
    {
        return (PolylineConnection) getFigure();
    }

    /**
     * @see java.beans.PropertyChangeListener#propertyChange(
     * java.beans.PropertyChangeEvent)
     */
    public void propertyChange(PropertyChangeEvent event)
    {
        String property = event.getPropertyName();
        if (Connection.PROPERTY_CONNECTION_ROUTER.equals(property)) {
            refreshBendpoints();
            refreshBendpointEditPolicy();
        }
        if (Messages.CONNECTIONPROPERTY_VALUE.equals(property)) {
            refreshVisuals();
        }
        if (Messages.CONNECTIONPROPERTY_BENDPOINT.equals(property)) {
            refreshBendpoints();
        }
    }

    /**
     * Updates the bendpoints, based on the model.
     */
    protected void refreshBendpoints()
    {
        if (getConnectionFigure().getConnectionRouter()
            instanceof ManhattanConnectionRouter) {
            return;
        }
        List modelConstraint = getConnectionModel().getBendpoints();
        List figureConstraint = new ArrayList();
        for (int i = 0; i < modelConstraint.size(); i++) {
            ConnectionBendpoint wbp =
                (ConnectionBendpoint) modelConstraint.get(i);
            RelativeBendpoint rbp =
                new RelativeBendpoint(getConnectionFigure());
            rbp.setRelativeDimensions(
                wbp.getFirstRelativeDimension(),
                wbp.getSecondRelativeDimension());
            rbp.setWeight((i + 1) / ((float) modelConstraint.size() + 1));
            figureConstraint.add(rbp);
        }
        getConnectionFigure().setRoutingConstraint(figureConstraint);
    }

    /**
     * install policies based on connection type.
     *
     */
    private void refreshBendpointEditPolicy()
    {
        if (getConnectionFigure().getConnectionRouter()
            instanceof ManhattanConnectionRouter) {
            installEditPolicy(EditPolicy.CONNECTION_BENDPOINTS_ROLE, null);
        } else {
            installEditPolicy(
                EditPolicy.CONNECTION_BENDPOINTS_ROLE,
                new ConnBendpointEditPolicy());
        }
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
            if (action != null && action.isVisible()) {
                action.run();
            }
        }
        super.performRequest(req);
    }
    /**
     * @see org.eclipse.gef.editparts.AbstractEditPart#refreshVisuals()
     */
    public void refreshVisuals()
    {
        refreshBendpoints();
        getConnFigure().setForegroundColor(DEAD);
    }
    
    protected void applyGraphResults(CompoundDirectedGraph graph, Map map) {
		Edge e = (Edge) map.get(this);
		if (e != null) {
			NodeList nodes = e.vNodes;
			PolylineConnection conn = (PolylineConnection) getConnectionFigure();
			if (nodes != null) {
				List bends = new ArrayList();
				for (int i = 0; i < nodes.size(); i++) {
					Node vn = nodes.getNode(i);
					int x = vn.x;
					int y = vn.y;
					if (e.isFeedback) {
						bends.add(new AbsoluteBendpoint(x, y + vn.height));
						bends.add(new AbsoluteBendpoint(x, y));

					} else {
						bends.add(new AbsoluteBendpoint(x, y));
						bends.add(new AbsoluteBendpoint(x, y + vn.height));
					}
				}
				conn.setRoutingConstraint(bends);
			} else {
				conn.setRoutingConstraint(Collections.EMPTY_LIST);
			}
		}
	}
    
    public void contributeToGraph(CompoundDirectedGraph graph, Map map) {
		Node source = (Node) map.get(getSource());
		Node target = (Node) map.get(getTarget());
		if (source != null && target != null && source.getParent() == target.getParent()) {
			Edge e = new Edge(this, source, target);
			e.weight = 2;
			graph.edges.add(e);
			map.put(this, e);
		}
	}
}

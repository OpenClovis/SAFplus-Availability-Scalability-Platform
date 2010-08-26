/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/editpart/RelationEditPart.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.editpart;

import java.beans.PropertyChangeEvent;

import org.eclipse.draw2d.ColorConstants;
import org.eclipse.draw2d.ConnectionEndpointLocator;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.PolygonDecoration;
import org.eclipse.draw2d.PolylineConnection;
import org.eclipse.draw2d.PolylineDecoration;
import org.eclipse.draw2d.geometry.PointList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.genericeditor.editparts.AbstractEdgeEditPart;
import com.clovis.cw.genericeditor.model.EdgeModel;
import com.clovis.cw.genericeditor.model.NodeModel;

/**
 * @author pushparaj
 * EditPart class for Auto Relation
 */
public class RelationEditPart extends AbstractEdgeEditPart
{
	
	private IFigure _connectionFigure = null;
	static final PointList DEFAULT_CONTAINTMENT = new PointList();
    static {
        DEFAULT_CONTAINTMENT.addPoint(0, 0);
        DEFAULT_CONTAINTMENT.addPoint(-1, -1);
        DEFAULT_CONTAINTMENT.addPoint(-2, 0);
        DEFAULT_CONTAINTMENT.addPoint(-1, 1);
    }
	/**
     * @see org.eclipse.gef.editparts.AbstractGraphicalEditPart#createFigure()
     */
    protected IFigure createFigure()
    {
    	_connectionFigure = super.createFigure();
    	return _connectionFigure;
    }
    /**
     * Listens to changes in properties of the Transition (like the
     * contents being carried), and reflects is in the visuals.
     *
     * @param event  Event notifying the change.
     */
    public void propertyChange(PropertyChangeEvent event)
    {
       	EdgeModel edge = getConnectionModel();
        if (edge == null) {
            return;
        }
        
        if (event.getPropertyName().equals("attachsource")) {
            NodeModel sourcenode = edge.getSource();
            if (sourcenode != null) {
                EObject sourceobj = sourcenode.getEObject();
                edge.setEPropertyValue(ComponentEditorConstants.
                		CONNECTION_START, sourceobj.eGet(sourceobj
                        .eClass().getEStructuralFeature(
                        		ModelConstants.RDN_FEATURE_NAME)));
            }
        }
        if (event.getPropertyName().equals("attachtarget")) {
            NodeModel targetnode = edge.getTarget();
            if (targetnode != null) {
                EObject targetobj = targetnode.getEObject();
                edge.setEPropertyValue(ComponentEditorConstants.
                		CONNECTION_END, targetobj.eGet(targetobj
                        .eClass().getEStructuralFeature(
                        		ModelConstants.RDN_FEATURE_NAME)));
            }
        }
        //refreshVisuals();
        super.propertyChange(event);
    }
    /**
     * @see org.eclipse.gef.editparts.AbstractEditPart#refreshVisuals()
     */
    public void refreshVisuals()
    {
    	updateFigure();
    	super.refreshVisuals();
    }
    /**
     * Update Figure
     */
    private void updateFigure()
    {
    	EdgeModel edge = getConnectionModel();
    	if (edge != null)
    	{
    		NodeModel source = edge.getSource();
    		NodeModel target = edge.getTarget();
    		if(source != null && target != null)
    		{
    			EObject sourceObj = source.getEObject();
    			EObject targetObj = target.getEObject();
    			if(sourceObj != null && targetObj != null)
    			{
    				String sourceName = sourceObj.eClass().getName();
    				EObject conObj = edge.getEObject();
    				EStructuralFeature feature = conObj.eClass()
							.getEStructuralFeature(
									ComponentEditorConstants.CONNECTION_TYPE);
    				Object connType = "INVALID";
    				//String targetName = targetObj.eClass().getName();
    				if(sourceName.equals(ComponentEditorConstants.NODE_NAME)
    						|| sourceName.equals(ComponentEditorConstants.CLUSTER_NAME)
    						|| sourceName.equals(ComponentEditorConstants.SERVICEUNIT_NAME)
    						|| sourceName.equals(ComponentEditorConstants.SERVICEINSTANCE_NAME))
    				{
    					updateContainment();
    					connType = ComponentEditorConstants.CONTAINMENT_NAME;
    				} else if(sourceName.equals(ComponentEditorConstants.SERVICEGROUP_NAME)
    						|| sourceName.equals(ComponentEditorConstants.COMPONENTSERVICEINSTANCE_NAME))
    				{
    					updateAssociation();
    					connType = ComponentEditorConstants.ASSOCIATION_NAME;
    				} else if(sourceName.equals(ComponentEditorConstants.SAFCOMPONENT_NAME)) 
    				{
    					updateProxyProxied();
    					connType = ComponentEditorConstants.PROXY_PROXIED_NAME;
    				}
    				Object conName = conObj.eGet(feature);
    				if(!connType.equals(conName)) {
    					conObj.eSet(feature, connType);
    				}
    			}
    		}
		}
    }
    /**
     * Update Containment
     *
     */
    private void updateContainment()
    {
    	clearDecorations();
    	PolygonDecoration sourceDecoration = new PolygonDecoration();
        sourceDecoration.setTemplate(DEFAULT_CONTAINTMENT);
        sourceDecoration.setBackgroundColor(ColorConstants.white);
        ((PolylineConnection) _connectionFigure)
                .setSourceDecoration(sourceDecoration);

        PolylineDecoration targetDecoration = new PolylineDecoration();
        ((PolylineConnection) _connectionFigure).setTargetDecoration(
                targetDecoration);
    }
    /**
     * Update Association
     *
     */
    private void updateAssociation()
    {
    	clearDecorations();
    	PolylineDecoration targetDecoration = new PolylineDecoration();
        ((PolylineConnection) _connectionFigure).setTargetDecoration(
                targetDecoration);
    }
    /**
     * Update ProxyProxied
     *
     */
    private void updateProxyProxied()
    {
    	clearDecorations();
    	PolylineDecoration sourceDecoration = new PolylineDecoration();
        ((PolylineConnection) _connectionFigure)
                .setSourceDecoration(sourceDecoration);
        
        ConnectionEndpointLocator sourcelocator = 
            new ConnectionEndpointLocator(((PolylineConnection) _connectionFigure), false);
        sourcelocator.setVDistance(3);
        ((PolylineConnection) _connectionFigure).add(new Label("proxies"), sourcelocator);
        
        PolylineDecoration targetDecoration = new PolylineDecoration();
        ((PolylineConnection) _connectionFigure).setTargetDecoration(
                targetDecoration);
    }
    /**
     * Clear decorations
     *
     */
    private void clearDecorations()
    {
    	((PolylineConnection) _connectionFigure)
        .setSourceDecoration(null);
    	((PolylineConnection) _connectionFigure)
        .setTargetDecoration(null);
    	((PolylineConnection) _connectionFigure).removeAll();
    }
}

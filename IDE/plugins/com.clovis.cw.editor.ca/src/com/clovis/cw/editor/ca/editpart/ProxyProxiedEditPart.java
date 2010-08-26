/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/editpart/ProxyProxiedEditPart.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.editpart;

import java.beans.PropertyChangeEvent;

import org.eclipse.draw2d.ConnectionEndpointLocator;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.PolylineConnection;
import org.eclipse.draw2d.PolylineDecoration;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.genericeditor.editparts.AbstractEdgeEditPart;
import com.clovis.cw.genericeditor.model.EdgeModel;
import com.clovis.cw.genericeditor.model.NodeModel;

/**
 * @author pushparaj
 *
 * EditPart for Proxy-Proxied connection
 */
public class ProxyProxiedEditPart extends AbstractEdgeEditPart
{
    /**
     * @see org.eclipse.gef.editparts.AbstractGraphicalEditPart#createFigure()
     */
    protected IFigure createFigure()
    {
        IFigure connectionFigure = super.createFigure();
        PolylineDecoration sourceDecoration = new PolylineDecoration();
        ((PolylineConnection) connectionFigure)
                .setSourceDecoration(sourceDecoration);
        
        ConnectionEndpointLocator sourcelocator = 
            new ConnectionEndpointLocator(((PolylineConnection) connectionFigure), false);
        sourcelocator.setVDistance(3);
        ((PolylineConnection) connectionFigure).add(new Label("proxies"), sourcelocator);
        
        PolylineDecoration targetDecoration = new PolylineDecoration();
        ((PolylineConnection) connectionFigure).setTargetDecoration(
                targetDecoration);
        
        return connectionFigure;
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
        super.propertyChange(event);
    }
}

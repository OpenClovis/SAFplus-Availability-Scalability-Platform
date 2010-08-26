/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/editpart/ProtectsEditPart.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.editpart;

import org.eclipse.draw2d.ConnectionEndpointLocator;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.PolylineConnection;
import org.eclipse.draw2d.PolylineDecoration;

import com.clovis.cw.genericeditor.figures.AbstractConnectionFigure;

public class ProtectsEditPart extends ProxyProxiedEditPart {

	/**
     * @see org.eclipse.gef.editparts.AbstractGraphicalEditPart#createFigure()
     */
    protected IFigure createFigure()
    {
        IFigure connectionFigure = new AbstractConnectionFigure(getConnectionModel());
        ConnectionEndpointLocator sourcelocator = 
            new ConnectionEndpointLocator(
            		((PolylineConnection) connectionFigure), false);
        sourcelocator.setVDistance(3);
        ((PolylineConnection) connectionFigure).add(
        		new Label("protects"), sourcelocator);
        
        PolylineDecoration targetDecoration = new PolylineDecoration();
        ((PolylineConnection) connectionFigure).setTargetDecoration(
                targetDecoration);
        
        return connectionFigure;
    }
}

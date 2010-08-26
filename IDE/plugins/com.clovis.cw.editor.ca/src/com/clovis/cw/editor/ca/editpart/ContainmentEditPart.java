/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/editpart/ContainmentEditPart.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.editpart;

import org.eclipse.draw2d.ArrowLocator;
import org.eclipse.draw2d.ColorConstants;
import org.eclipse.draw2d.ConnectionLocator;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.PolygonDecoration;
import org.eclipse.draw2d.PolylineConnection;
import org.eclipse.draw2d.PolylineDecoration;
import org.eclipse.draw2d.geometry.PointList;

import com.clovis.cw.genericeditor.figures.AbstractConnectionFigure;


/**
 * @author pushparaj
 *
 * EditPart for Containment
 */
public class ContainmentEditPart extends ProxyProxiedEditPart
{
    
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
        IFigure connectionFigure = new AbstractConnectionFigure(getConnectionModel());
        PolygonDecoration sourceDecoration = new PolygonDecoration();
        sourceDecoration.setTemplate(DEFAULT_CONTAINTMENT);
        sourceDecoration.setBackgroundColor(ColorConstants.white);
        ((PolylineConnection) connectionFigure)
                .setSourceDecoration(sourceDecoration);

        PolylineDecoration targetNavigation = new PolylineDecoration();
        ((PolylineConnection) connectionFigure).add(targetNavigation,
                new ArrowLocator((PolylineConnection) connectionFigure,
                        ConnectionLocator.TARGET));
        return connectionFigure;
    }
}

/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


package com.clovis.cw.editor.ca.editpart;

import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;

import org.eclipse.draw2d.ColorConstants;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.PolygonDecoration;
import org.eclipse.draw2d.PolylineConnection;
import org.eclipse.draw2d.geometry.PointList;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.cw.genericeditor.editparts.AbstractEdgeEditPart;
import com.clovis.cw.genericeditor.model.EdgeModel;
import com.clovis.cw.genericeditor.model.NodeModel;


/**
 *
 * @author pushparaj
 *
 * EditPart for Inheritance
 */
public class InheritenceEditPart extends AbstractEdgeEditPart
    implements PropertyChangeListener
{

    /**
     * Returns a newly created Figure to represent the connection.
     *
     * @return  The created Figure.
     */
    protected IFigure createFigure()
    {
        if (getConnectionModel() == null) {
            return null;
        }

        IFigure connectionFigure =  super.createFigure();
        addInheritance((PolylineConnection) connectionFigure);

          return connectionFigure;
    }

    /**
     * Creates decoration an connection
     * @param connection polyline connection
     */
    private void addInheritance(PolylineConnection connection)
    {
        PolygonDecoration decoration = new PolygonDecoration();
        decoration.setBackgroundColor(ColorConstants.white);
        PointList list = new PointList();
        list.addPoint(0, 0);
        list.addPoint(-2, 1);
        list.addPoint(-2, -1);
        decoration.setTemplate(list);
        connection.setTargetDecoration(decoration);
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
                edge.setEPropertyValue("source", sourceobj.eGet(sourceobj
                        .eClass().getEStructuralFeature(
                        		ModelConstants.RDN_FEATURE_NAME)));
            }
        }
        if (event.getPropertyName().equals("attachtarget")) {
            NodeModel targetnode = edge.getTarget();
            if (targetnode != null) {
                EObject targetobj = targetnode.getEObject();
                edge.setEPropertyValue("target", targetobj.eGet(targetobj
                        .eClass().getEStructuralFeature(
                        		ModelConstants.RDN_FEATURE_NAME)));
            }
        }
        super.propertyChange(event);
    } 
}

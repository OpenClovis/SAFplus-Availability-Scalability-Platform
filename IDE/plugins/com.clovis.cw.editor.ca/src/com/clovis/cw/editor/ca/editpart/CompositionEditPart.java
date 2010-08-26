/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/editpart/CompositionEditPart.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.editpart;

import java.beans.PropertyChangeListener;

import org.eclipse.draw2d.ArrowLocator;
import org.eclipse.draw2d.ConnectionEndpointLocator;
import org.eclipse.draw2d.ConnectionLocator;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.PolygonDecoration;
import org.eclipse.draw2d.PolylineConnection;
import org.eclipse.draw2d.PolylineDecoration;
import org.eclipse.draw2d.geometry.PointList;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.genericeditor.figures.AbstractConnectionFigure;
import com.clovis.cw.genericeditor.model.EdgeModel;
import com.clovis.cw.genericeditor.model.NodeModel;


/**
 * @author ashish
 *
 * Class for Containtment EditPart
 */
public class CompositionEditPart extends AggregationEditPart
    implements PropertyChangeListener
{

    static final PointList DEFAULT_COMPOSITION = new PointList();
    static {
        DEFAULT_COMPOSITION.addPoint(0, 0);
        DEFAULT_COMPOSITION.addPoint(-1, -1);
        DEFAULT_COMPOSITION.addPoint(-2, 0);
        DEFAULT_COMPOSITION.addPoint(-1, 1);
    }

    /**
     * Returns a newly created Figure to represent the connection.
     *
     * @return  The created Figure.
     */
    protected IFigure createFigure() {
		IFigure connectionFigure;
		EdgeModel model = (EdgeModel) getModel();
		NodeModel targetNode = model.getTarget();
		connectionFigure = super.createFigure();
		if (targetNode != null) {
			EObject targetObj = targetNode.getEObject();
			if (targetObj.eClass().getName().equals(
					ClassEditorConstants.MIB_RESOURCE_NAME)) {
				connectionFigure = new InvisibleConnectionFigure();
			}
		} 
		PolygonDecoration sourceDecoration = new PolygonDecoration();
		sourceDecoration.setTemplate(DEFAULT_COMPOSITION);
		((PolylineConnection) connectionFigure)
				.setSourceDecoration(sourceDecoration);

		PolylineDecoration targetNavigation = new PolylineDecoration();
		((PolylineConnection) connectionFigure).add(targetNavigation,
				new ArrowLocator((PolylineConnection) connectionFigure,
						ConnectionLocator.TARGET));
		refreshMultiplicity();
		return connectionFigure;
	}
    /**
	 * Add Multiplicity in connection ends
	 */
    protected void refreshMultiplicity() {
    	EdgeModel model = (EdgeModel) getModel();
    	NodeModel targetNode = model.getTarget();
    	if (targetNode != null) {
    		EObject targetObj = targetNode.getEObject();
    		if (targetObj.eClass().getName().equals(
    			ClassEditorConstants.DATA_STRUCTURE_NAME)) {
    			EObject obj = model.getEObject();
				_sourceLabel.setText(EcoreUtils.getValue(obj,
						"sourceMultiplicity").toString());
				_targetLabel.setText(EcoreUtils.getValue(obj,
						"targetMultiplicity").toString());
			} else {
				_sourceLabel.setText("");
				_targetLabel.setText("");
			}
    	}
	}
    /**
     * @see org.eclipse.gef.editparts.AbstractEditPart#refreshVisuals()
     */
    public void refreshVisuals() {
    	EdgeModel model = (EdgeModel) getModel();
    	NodeModel targetNode = model.getTarget();
    	if (targetNode != null) {
    		EObject targetObj = targetNode.getEObject();
    		if(targetObj.eClass().getName().equals(ClassEditorConstants.MIB_RESOURCE_NAME)) {
    			getFigure().setVisible(false);
    		}
    	}
    	super.refreshVisuals();
    }
    class InvisibleConnectionFigure extends AbstractConnectionFigure {
		public InvisibleConnectionFigure() {
			super(getConnectionModel());
			ConnectionEndpointLocator sourcelocator = new ConnectionEndpointLocator(
					this, false);
			sourcelocator.setVDistance(3);
			add(_sourceLabel, sourcelocator);
			ConnectionEndpointLocator targetlocator = new ConnectionEndpointLocator(
					this, true);
			targetlocator.setVDistance(3);
			add(_targetLabel, targetlocator);

		}
		public void setVisible(boolean visible) {
	    	super.setVisible(false);
	    }
    }
    
}

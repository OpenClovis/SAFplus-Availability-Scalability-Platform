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

import org.eclipse.draw2d.ConnectionEndpointLocator;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.PolylineConnection;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.genericeditor.editparts.AbstractEdgeEditPart;
import com.clovis.cw.genericeditor.figures.AbstractConnectionFigure;
import com.clovis.cw.genericeditor.model.EdgeModel;
import com.clovis.cw.genericeditor.model.NodeModel;

/**
 * @author pushparaj
 *
 * Editpart class for Aggregation
 */
public class AggregationEditPart extends AbstractEdgeEditPart
implements PropertyChangeListener
{
	protected PolylineConnection _polyLine = null;
	protected Label _sourceLabel = new Label();
	protected Label _targetLabel = new Label();
	
	/**
	 * @see org.eclipse.gef.editparts.AbstractGraphicalEditPart#createFigure()
	 */
	protected IFigure createFigure() {
		 _polyLine = new AbstractConnectionFigure(getConnectionModel());
		 ConnectionEndpointLocator sourcelocator = 
	            new ConnectionEndpointLocator(_polyLine, false);
		 sourcelocator.setVDistance(3);
		 _polyLine.add(_sourceLabel, sourcelocator);
		 ConnectionEndpointLocator targetlocator = 
	           new ConnectionEndpointLocator(_polyLine, true);
		 targetlocator.setVDistance(3);
		 _polyLine.add(_targetLabel, targetlocator);
		 refreshMultiplicity();
		 return _polyLine;
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
        refreshMultiplicity();       
        super.propertyChange(event);
    }
    /**
     * Add Multiplicity in connection ends
     */
    protected void refreshMultiplicity() {
    	EdgeModel model = (EdgeModel) getModel();
    	EObject obj = model.getEObject();
    	_sourceLabel.setText(EcoreUtils.getValue(obj,
				"sourceMultiplicity").toString());
		_targetLabel.setText(EcoreUtils.getValue(obj,
				"targetMultiplicity").toString());
	}
}

/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor.editpolicies;
import org.eclipse.draw2d.PolylineConnection;
import org.eclipse.gef.GraphicalEditPart;

/**
 * 
 * @author pushparaj
 *
 * Policy which handles endpoints and selection
 */
public class ConnEndpointEditPolicy
	extends org.eclipse.gef.editpolicies.ConnectionEndpointEditPolicy {
	
	/**
	 * @see org.eclipse.gef.editpolicies.SelectionHandlesEditPolicy#addSelectionHandles()
	 */
	protected void addSelectionHandles() {
		super.addSelectionHandles();
		getConnectionFigure().setLineWidth(2);
	}

	/**
	 * returns connection figure.
	 * @return polyline connection
	 */	
	protected PolylineConnection getConnectionFigure() {
		return (PolylineConnection) ((GraphicalEditPart) getHost()).getFigure();
	}
	
	/**
	 * @see org.eclipse.gef.editpolicies.SelectionHandlesEditPolicy#removeSelectionHandles()
	 */
	protected void removeSelectionHandles() {
		super.removeSelectionHandles();
		getConnectionFigure().setLineWidth(1);
	}

}

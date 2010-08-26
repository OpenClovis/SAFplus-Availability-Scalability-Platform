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

import org.eclipse.draw2d.Connection;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.gef.commands.Command;
import org.eclipse.gef.requests.BendpointRequest;

import com.clovis.cw.genericeditor.commands.BendpointCommand;
import com.clovis.cw.genericeditor.commands.CreateBendpointCommand;
import com.clovis.cw.genericeditor.commands.DeleteBendpointCommand;
import com.clovis.cw.genericeditor.commands.MoveBendpointCommand;
import com.clovis.cw.genericeditor.model.EdgeModel;


/**
 * 
 * @author pushparaj
 *
 * Policy class for all bendpoint requests
 * This will creates create, move, delete bendpoint commands 
 */
public class ConnBendpointEditPolicy
	extends org.eclipse.gef.editpolicies.BendpointEditPolicy {
	
	/**
	 * @see org.eclipse.gef.editpolicies.BendpointEditPolicy#getCreateBendpointCommand(org.eclipse.gef.requests.BendpointRequest)
	 */
	protected Command getCreateBendpointCommand(BendpointRequest request) {
		CreateBendpointCommand com = new CreateBendpointCommand();
		Point p = request.getLocation();
		Connection conn = getConnection();

		conn.translateToRelative(p);

		com.setLocation(p);
		Point ref1 = getConnection().getSourceAnchor().getReferencePoint();
		Point ref2 = getConnection().getTargetAnchor().getReferencePoint();

		conn.translateToRelative(ref1);
		conn.translateToRelative(ref2);

		com.setRelativeDimensions(p.getDifference(ref1), p.getDifference(ref2));
		com.setConnectionModel((EdgeModel) request.getSource().getModel());
		com.setIndex(request.getIndex());
		return com;
	}

	/**
	 * @see org.eclipse.gef.editpolicies.BendpointEditPolicy#getMoveBendpointCommand(org.eclipse.gef.requests.BendpointRequest)
	 */
	protected Command getMoveBendpointCommand(BendpointRequest request) {
		MoveBendpointCommand com = new MoveBendpointCommand();
		Point p = request.getLocation();
		Connection conn = getConnection();

		conn.translateToRelative(p);

		com.setLocation(p);

		Point ref1 = getConnection().getSourceAnchor().getReferencePoint();
		Point ref2 = getConnection().getTargetAnchor().getReferencePoint();

		conn.translateToRelative(ref1);
		conn.translateToRelative(ref2);

		com.setRelativeDimensions(p.getDifference(ref1), p.getDifference(ref2));
		com.setConnectionModel((EdgeModel) request.getSource().getModel());
		com.setIndex(request.getIndex());
		return com;
	}
	
	/**
	 * @see org.eclipse.gef.editpolicies.BendpointEditPolicy#getDeleteBendpointCommand(org.eclipse.gef.requests.BendpointRequest)
	 */
	protected Command getDeleteBendpointCommand(BendpointRequest request) {
		BendpointCommand com = new DeleteBendpointCommand();
		Point p = request.getLocation();
		com.setLocation(p);
		com.setConnectionModel((EdgeModel) request.getSource().getModel());
		com.setIndex(request.getIndex());
		return com;
	}
}

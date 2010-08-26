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

import java.util.List;

import org.eclipse.draw2d.geometry.Insets;
import org.eclipse.gef.AccessibleEditPart;
import org.eclipse.gef.EditPolicy;
import org.eclipse.swt.accessibility.AccessibleEvent;

import com.clovis.cw.genericeditor.editpolicies.GEContainerEditPolicy;
import com.clovis.cw.genericeditor.editpolicies.GEXYLayoutEditPolicy;
import com.clovis.cw.genericeditor.model.ContainerNodeModel;

/**
 * @author 	Pushparaj
 *
 * Abstract class for Parent Node EditParts
 */
abstract public class AbstractContainerNodeEditPart extends AbstractComponentNodeEditPart {

	public static final Insets PADDING = new Insets(8, 6, 8, 6);
	public static final Insets INNER_PADDING = new Insets(0);
	
	
	/**
	 * Returns the <code>AccessibleEditPart</code> adapter for this EditPart. The <B>same</B>
	 * adapter instance must be used throughout the editpart's existance.  Each adapter has
	 * a unique ID.  Accessibility clients can only refer to this editpart via that ID
	 *  @return <code>null</code> or an AccessibleEditPart adapter
     */
	protected AccessibleEditPart createAccessible() {
		return new AccessibleGraphicalEditPart() {
			public void getName(AccessibleEvent e) {
				e.result = getBaseDiagram().toString();
			}
		};
	}

	/**
	 * @see org.eclipse.gef.editparts.AbstractEditPart#createEditPolicies()
	 */
	protected void createEditPolicies() {
		super.createEditPolicies();
		installEditPolicy(
			EditPolicy.CONTAINER_ROLE,
			new GEContainerEditPolicy());
		installEditPolicy(
			EditPolicy.LAYOUT_ROLE,
			new GEXYLayoutEditPolicy());
	}

	/**
	 * Returns the model of this as a BaseDiagram.
	 *
	 * @return  BaseDiagram of this.
	 */
	protected ContainerNodeModel getBaseDiagram() {
		return (ContainerNodeModel) getModel();
	}

	/**
	 * Returns the children of this through the model.
	 *
	 * @return  Children of this as a List.
	 */
	protected List getModelChildren() {
		return getBaseDiagram().getChildren();
	}
	/**
	 * Will return true, If Editpart is required bsed on child editparts.
	 * @return true or false
	 */
	public boolean removeWhenEmpty() {
		return false;
	}
	public abstract void arrangeNodesAndEdges();
}

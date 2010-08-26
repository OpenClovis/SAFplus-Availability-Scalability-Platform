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


import org.eclipse.gef.AccessibleEditPart;
import org.eclipse.swt.accessibility.AccessibleEvent;

/**
 * @author pushparaj
 *
 * Abstract class for Child Node EditParts.
 */
public abstract class AbstractComponentNodeEditPart extends BaseEditPart{
	
	/**
	 * Returns the <code>AccessibleEditPart</code> adapter for this EditPart. The <B>same</B>
	 * adapter instance must be used throughout the editpart's existance.  Each adapter has
	 * a unique ID.  Accessibility clients can only refer to this editpart via that ID
	 *  @return <code>null</code> or an AccessibleEditPart adapter
     */
    protected AccessibleEditPart createAccessible() {
        return new AccessibleGraphicalEditPart() {
        public void getName(AccessibleEvent e) {
            e.result = getModel().toString();
            }
        };
    }
}

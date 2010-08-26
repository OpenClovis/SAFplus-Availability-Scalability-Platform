/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor.figures;

import org.eclipse.draw2d.AbstractConnectionAnchor;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.ScalableFigure;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.PrecisionPoint;
import org.eclipse.draw2d.geometry.Rectangle;

/**
 * 
 * @author pushparaj
 *
 * TODO To change the template for this generated type comment go to
 * Window - Preferences - Java - Code Style - Code Templates
 */
public class FixedConnectionAnchor extends AbstractConnectionAnchor {

	public int offsetH, offsetV;
	
	/**
	 * constructor
	 * @param owner
	 */
	public FixedConnectionAnchor(IFigure owner) {
		super(owner);
	}

	/**
	 * @see org.eclipse.draw2d.AbstractConnectionAnchor#ancestorMoved(IFigure)
	 */
	public void ancestorMoved(IFigure figure) {
		if (figure instanceof ScalableFigure)
			return;
		super.ancestorMoved(figure);
	}
	
	/**
	 * @see org.eclipse.draw2d.ConnectionAnchor#getLocation(org.eclipse.draw2d.geometry.Point)
	 */
	public Point getLocation(Point reference) {
		Rectangle r = getOwner().getBounds();
		int x, y;

		x = r.x + offsetH;
		y = r.y + offsetV;

		Point p = new PrecisionPoint(x, y);
		getOwner().translateToAbsolute(p);
		return p;
	}
	
	/**
	 * @see org.eclipse.draw2d.ConnectionAnchor#getReferencePoint()
	 */
	public Point getReferencePoint() {
		return getLocation(null);
	}

}

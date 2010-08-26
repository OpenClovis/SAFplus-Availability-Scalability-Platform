/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor.model;

import org.eclipse.draw2d.Bendpoint;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Point;

/**
 * @version	1.0	18 Dec 2003
 * @author 	Pushparaj
 */
public class ConnectionBendpoint implements java.io.Serializable, Bendpoint {

	private float weight = 0.5f;
	private Dimension d1, d2;
	
	/**
	 * constructor
	 */
	public ConnectionBendpoint() {
	}
	
	/**
	 * 
	 * @return d1
	 */
	public Dimension getFirstRelativeDimension() {
		return d1;
	}
	
	/**
	 * @see org.eclipse.draw2d.Bendpoint#getLocation()
	 */
	public Point getLocation() {
		return null;
	}
	
	/**
	 * 
	 * @return d2
	 */
	public Dimension getSecondRelativeDimension() {
		return d2;
	}
	
	/**
	 * 
	 * @return weight
	 */
	public float getWeight() {
		return weight;
	}
	
	/**
	 * 
	 * @param dim1
	 * @param dim2
	 */
	public void setRelativeDimensions(Dimension dim1, Dimension dim2) {
		d1 = dim1;
		d2 = dim2;
	}
	
	/**
	 * 
	 * @param w
	 */
	public void setWeight(float w) {
		weight = w;
	}
}

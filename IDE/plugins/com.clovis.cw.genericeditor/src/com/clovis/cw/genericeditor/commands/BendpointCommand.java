/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor.commands;

import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.gef.commands.Command;

import com.clovis.cw.genericeditor.model.EdgeModel;


/**
 * 
 * @author pushparaj
 *
 * Command for Bendpoints 
 */
public class BendpointCommand extends Command {

	protected int _index;
	protected Point _bendLocation;
	protected EdgeModel _edgeModel;
	private Dimension _firstRelativeDimension, _secondRelativeDimension;
	
	/**
	 * returns 
	 * @return firstRelativeDimension
	 */
	protected Dimension getFirstRelativeDimension() {
		return _firstRelativeDimension;
	}

	/**
	 * returns
	 * @return secondRelativeDimension
	 */
	protected Dimension getSecondRelativeDimension() {
		return _secondRelativeDimension;
	}

	/**
	 * returns index.
	 * @return index
	 */
	protected int getIndex() {
		return _index;
	}
	
	/**
	 * returns Location for bendpoint.
	 * @return bendLocation
	 */
	protected Point getLocation() {
		return _bendLocation;
	}

	/**
	 * returns edge model, which have this bendpoint. 
	 * @return transitionModel
	 */
	protected EdgeModel getConnectionModel() {
		return _edgeModel;
	}
	
	/**
	 * @see org.eclipse.gef.commands.Command#redo()
	 */
	public void redo() {
		execute();
	}

	/**
	 * set first relative dimension for bendpoint.
	 * @param dim1
	 * @param dim2
	 */
	public void setRelativeDimensions(Dimension dim1, Dimension dim2) {
		_firstRelativeDimension = dim1;
		_secondRelativeDimension = dim2;
	}

	/**
	 * set index.
	 * @param i
	 */
	public void setIndex(int i) {
		_index = i;
	}

	/**
	 * set bendpoint location.
	 * @param p
	 */
	public void setLocation(Point p) {
		_bendLocation = p;
	}

	/**
	 * set edgemodel, which have this bendpoint.
	 * @param w
	 */
	public void setConnectionModel(EdgeModel w) {
		_edgeModel = w;
	}

}

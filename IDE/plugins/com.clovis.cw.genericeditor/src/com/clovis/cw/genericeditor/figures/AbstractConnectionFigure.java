/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/figures/AbstractConnectionFigure.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


package com.clovis.cw.genericeditor.figures;


import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.PolylineConnection;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.genericeditor.model.EdgeModel;

/**
 *
 * @author pushparaj
 *
 * Figure class for connection
 */
public class AbstractConnectionFigure extends PolylineConnection {
	
	private EdgeModel _model;
	public AbstractConnectionFigure(EdgeModel model) {
		_model = model;
	}
	/**
     * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
     */
    public void paintFigure(Graphics graphics) {
       	super.paintFigure(graphics);
    	if(_model.isCollapsedElement())
       		setVisible(false);
    	String sourceName = EcoreUtils.getName(_model.getSource().getEObject());
    	String targetName = EcoreUtils.getName(_model.getTarget().getEObject()); 
    	setToolTip(new Label(sourceName + " ----- " + targetName));
    }
}

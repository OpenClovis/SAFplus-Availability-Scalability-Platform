/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/editpart/ServiceInstanceEditPart.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.editpart;

import org.eclipse.draw2d.IFigure;


import com.clovis.cw.editor.ca.figure.ServiceInstanceFigure;
import com.clovis.cw.genericeditor.editparts.AbstractComponentNodeEditPart;
import com.clovis.cw.genericeditor.model.NodeModel;

public class ServiceInstanceEditPart extends AbstractComponentNodeEditPart {
	private ServiceInstanceFigure _nodeFigure;
    /**
     * Creates and Returns Component Figure
     * @return Figure Component Figure
     */
    protected IFigure createFigure()
    {
        _nodeFigure = new ServiceInstanceFigure((NodeModel) getModel());
        return _nodeFigure;
    }
    /**
     * @see org.eclipse.gef.editparts.AbstractEditPart#refreshVisuals()
     */
    public void refreshVisuals()
    {
        _nodeFigure.initNodeDisplay();
        super.refreshVisuals();
    }

}

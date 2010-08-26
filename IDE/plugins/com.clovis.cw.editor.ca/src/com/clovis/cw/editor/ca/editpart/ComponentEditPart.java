/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/editpart/ComponentEditPart.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.editpart;

import org.eclipse.draw2d.IFigure;

import com.clovis.cw.editor.ca.figure.ComponentFigure;
import com.clovis.cw.genericeditor.editparts.AbstractComponentNodeEditPart;
import com.clovis.cw.genericeditor.model.NodeModel;

/**
 * @author pushparaj
 *
 * EditPart for Component
 */
public class ComponentEditPart extends AbstractComponentNodeEditPart
{
    private ComponentFigure _componentFigure;
    /**
     * Creates and Returns Component Figure
     * @return Figure Component Figure
     */
    protected IFigure createFigure()
    {
        _componentFigure = new ComponentFigure((NodeModel) getModel());
        return _componentFigure;
    }
    /**
     * @see org.eclipse.gef.editparts.AbstractEditPart#refreshVisuals()
     */
    public void refreshVisuals()
    {
        _componentFigure.initComponentProperties();
        super.refreshVisuals();
    }
}

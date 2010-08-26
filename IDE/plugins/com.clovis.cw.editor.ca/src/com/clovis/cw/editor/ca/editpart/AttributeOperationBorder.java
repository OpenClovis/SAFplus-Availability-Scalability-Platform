/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/editpart/AttributeOperationBorder.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.editpart;

import org.eclipse.draw2d.AbstractBorder;
import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.geometry.Insets;
/**
 * @author nadeem
 *
 * Figure border for Attributes and Method.
 */
public class AttributeOperationBorder extends AbstractBorder
{
    /**
     * Inset.
     */
    public Insets getInsets(IFigure figure) 
    {    
        return new Insets(1, 0, 0, 0);
    }
    /**
     * Paint of figure.
     */
    public void paint(IFigure f, Graphics g, Insets i) 
    {
        g.drawLine(getPaintRectangle(f, i).getTopLeft(), tempRect.getTopRight());
    }
}

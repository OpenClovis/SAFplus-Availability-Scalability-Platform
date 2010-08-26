/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/figure/ShadowRoundedRectangle.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


package com.clovis.cw.editor.ca.figure;

import org.eclipse.draw2d.ColorConstants;
import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.geometry.Insets;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.swt.graphics.Color;

import com.clovis.cw.genericeditor.figures.AbstractNodeFigure;

/**
 *
 * @author pushparaj
 *
 * Figure with Shadow
 */
public abstract class ShadowRoundedRectangle extends AbstractNodeFigure
{
    public static final int SHADOW_VALUE = 2;
    /**
     * @see org.eclipse.draw2d.Shape#fillShape(org.eclipse.draw2d.Graphics)
     */
    protected void fillShape(Graphics graphics)
    {
        Rectangle rect = Rectangle.SINGLETON.setBounds(getBounds());
        Insets shadowInset = new Insets(0, 0, SHADOW_VALUE, SHADOW_VALUE);
        rect.crop(shadowInset);
        drawShadow(rect, graphics);
        graphics.fillRectangle(rect);
    }
    /**
     * @see Shape#outlineShape(Graphics)
     */
    protected void outlineShape(Graphics graphics)
    {
        Rectangle rect = Rectangle.SINGLETON.setBounds(getBounds());
        Insets shadowInset = new Insets(lineWidth / 2, lineWidth / 2,
        lineWidth + SHADOW_VALUE, lineWidth + SHADOW_VALUE);
        rect.crop(shadowInset);
        graphics.drawRectangle(rect);
    }
    /**
     * Draws Shadow
     * @param rectangle Rectangle
     * @param graphics Graphics
     */
    private void drawShadow(Rectangle rectangle, Graphics graphics)
    {
        //drawShadowLine(rectangle, graphics, 3, ColorConstants.darkGray);
        //drawShadowLine(rectangle, graphics, 2, ColorConstants.darkGray);
        drawShadowLine(rectangle, graphics, 1, ColorConstants.darkGray);
    }
    /**
     * Draws ShadowLayer
     * @param rectangle Rectangle
     * @param graphics Graphics
     * @param offset Offset
     * @param color Color
     */
    private void drawShadowLine(Rectangle rectangle, Graphics graphics,
            int offset, Color color)
    {
        // Saves graphics object's state
        graphics.pushState();
        graphics.setLineWidth(0);
        graphics.setBackgroundColor(color);
        Rectangle shadowLayer = new Rectangle(rectangle);
        shadowLayer.x += offset;
        shadowLayer.y += offset;
        graphics.fillRectangle(shadowLayer);
        // Restore graphics object's state
        graphics.popState();
    }
}

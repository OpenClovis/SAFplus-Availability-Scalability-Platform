package com.clovis.cw.editor.ca.figure;
/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/figure/MibFigure.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
import org.eclipse.draw2d.ColorConstants;
import org.eclipse.draw2d.FreeformLayer;
import org.eclipse.draw2d.FreeformLayout;
import org.eclipse.draw2d.FreeformViewport;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.LineBorder;
import org.eclipse.draw2d.ScrollPane;
import org.eclipse.draw2d.StackLayout;

import com.clovis.cw.genericeditor.figures.AbstractNodeFigure;
import com.clovis.cw.genericeditor.model.Base;
import com.clovis.cw.genericeditor.model.NodeModel;

/**
 * 
 * @author pushparaj
 * Mib Figure
 */
public class MibFigure extends AbstractNodeFigure {
	IFigure pane;
	NodeModel model;
	public MibFigure(NodeModel model) {
		this.model = model;
		pane = new FreeformLayer();
        pane.setLayoutManager(new FreeformLayout());
        ScrollPane scroll = new ScrollPane();
        setLayoutManager(new StackLayout());
        pane.setBorder(new LineBorder(ColorConstants.gray, 1));
        scroll.setViewport(new FreeformViewport());
        add(scroll);
        scroll.setContents(pane);
        createConnectionAnchors(this);
		setBackgroundColor(ColorConstants.white);
		setOpaque(true);
	}
	/**
	 * @see com.clovis.cw.genericeditor.figures.AbstractNodeFigure#getModel()
	 */
	public Base getModel() {
		return model;
	}
	public IFigure getContentsPane() {
	    return pane;
	}
}

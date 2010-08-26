/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/figure/ChassisFigure.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.figure;

import org.eclipse.draw2d.ColorConstants;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.ToolbarLayout;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.genericeditor.model.Base;
import com.clovis.cw.genericeditor.model.NodeModel;

public class ChassisFigure extends ShadowRoundedRectangle implements
ClassEditorConstants 
{
	private Label     _classNameLabel     = new Label();

    private Label     _classModifierLabel = new Label();
    
    private NodeModel _model;
    
    /**
     * Constructor
     * Creates Chassis Resource Figure
     * @param model Node Model
     */
    public ChassisFigure(NodeModel model) 
    {
    	 super();
         _model = model;
         setLayoutManager(new ToolbarLayout());
         setOpaque(true);
         setBackgroundColor(ColorConstants.white);
         setCornerDimensions(new Dimension(0, 0));

         _classNameLabel.setFont(CLASS_NAME_FONT);
         add(_classNameLabel);

         _classModifierLabel.setFont(CLASS_NAME_FONT);
         add(_classModifierLabel);
         
         createConnectionAnchors(this);
         initClassDisplay();
    }
    /**
     * Sets Chassis Name in Figure
     */
    private void setClassName()
    {
        EObject obj = _model.getEObject();
        String text = (String) EcoreUtils.getValue(obj, CLASS_NAME);
        _classNameLabel.setText(text);
        _classNameLabel.setFont(CLASS_NAME_FONT);
    }
    /**
     * Initialize the Display. This method is also called for refresh.
     *
     * @param model
     */
    public void initClassDisplay()
    {
        setClassName();
    }
    /**
     * @see com.clovis.cw.genericeditor.figures.AbstractNodeFigure#getModel()
     */
	public Base getModel() {
		return _model;
	}
}

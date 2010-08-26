/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/figure/ClusterFigure.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.figure;

import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.ToolbarLayout;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.genericeditor.model.Base;
import com.clovis.cw.genericeditor.model.NodeModel;

/**
 * @author pushparaj
 *
 * Figure for Cluster
 */
public class ClusterFigure extends ShadowRoundedRectangle implements
ClassEditorConstants, ComponentEditorConstants
{
    private Label     _classNameLabel     = new Label();

    private Label     _classModifierLabel = new Label();
    
    private NodeModel _model;
    /**
     * Constructor
     * @param model NodeModel
     */
    public ClusterFigure(NodeModel model)
    {
        super();
        _model = model;
        setLayoutManager(new ToolbarLayout());
        setOpaque(true);
        setCornerDimensions(new Dimension(0, 0));
        setBackgroundColor(CLUSTER_COLOR);
        
        _classNameLabel.setFont(CLASS_NAME_FONT);
        add(_classNameLabel);

        _classModifierLabel.setFont(CLASS_NAME_FONT);
        add(_classModifierLabel);
        
        createConnectionAnchors(this);
        initClusterDisplay();
    }
    /**
     * Sets Cluster Name in Figure
     */
    private void setClusterName()
    {
        EObject obj = _model.getEObject();
        String text = EcoreUtils.getName(obj);
        _classNameLabel.setText(text);
        _classNameLabel.setFont(CLASS_NAME_FONT);
    }
    /**
     * Initialize the Display. This method is also called for refresh.
     *
     */
    public void initClusterDisplay()
    {
        setClusterName();
    }
    /**
     * @see com.clovis.cw.genericeditor.figures.AbstractNodeFigure#getModel()
     */
	public Base getModel() {
		return _model;
	}
}

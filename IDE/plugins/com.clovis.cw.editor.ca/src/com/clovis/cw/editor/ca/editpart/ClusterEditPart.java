/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/editpart/ClusterEditPart.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.editpart;

import org.eclipse.draw2d.IFigure;

import com.clovis.cw.editor.ca.figure.ClusterFigure;
import com.clovis.cw.genericeditor.editparts.AbstractComponentNodeEditPart;
import com.clovis.cw.genericeditor.model.NodeModel;

/**
 * @author pushparaj
 *
 * EditPart for Cluster
 */
public class ClusterEditPart extends AbstractComponentNodeEditPart
{
    private ClusterFigure _clusterFigure;
    /**
     * Creates and Returns Component Figure
     * @return Figure Component Figure
     */
    protected IFigure createFigure()
    {
        _clusterFigure = new ClusterFigure((NodeModel) getModel());
        return _clusterFigure;
    }
    /**
     * @see org.eclipse.gef.editparts.AbstractEditPart#refreshVisuals()
     */
    public void refreshVisuals()
    {
        _clusterFigure.initClusterDisplay();
        super.refreshVisuals();
    }
}

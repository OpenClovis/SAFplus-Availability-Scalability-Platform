/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.editpart;

import org.eclipse.draw2d.IFigure;

import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.figure.ChassisFigure;
import com.clovis.cw.editor.ca.figure.DataStructureFigure;
import com.clovis.cw.editor.ca.figure.ResourceFigure;
import com.clovis.cw.genericeditor.model.NodeModel;
import com.clovis.cw.genericeditor.editparts.AbstractComponentNodeEditPart;

/**
 * @author ashish
 *
 * EditPart for Class.
 */
public class ClassEditPart extends AbstractComponentNodeEditPart
{
    /**
     * Refresh Visual.
     */
    public void refreshVisuals()
    {
        NodeModel model = (NodeModel) getModel();
        if (model.getName().equals(ClassEditorConstants.DATA_STRUCTURE_NAME)) {
            ((DataStructureFigure) getFigure()).initClassDisplay();
        } else if (model.getName().equals(ClassEditorConstants.CHASSIS_RESOURCE_NAME)) {
        	((ChassisFigure) getFigure()).initClassDisplay();
        } else {
            ((ResourceFigure) getFigure()).initClassDisplay();
        }
        super.refreshVisuals();
    }
    /**
     * Creates Figure for Class.
     *
     * @return Figure for Class (Data/Service).
     */
    protected IFigure createFigure()
    {
        NodeModel model = (NodeModel) getModel();
        if (model.getName().equals(ClassEditorConstants.DATA_STRUCTURE_NAME)) {
            return new DataStructureFigure(model);
        } else if (model.getName().equals(ClassEditorConstants.CHASSIS_RESOURCE_NAME)) {
            return new ChassisFigure(model);
        } else {
            return new ResourceFigure(model);
        }
    }
}

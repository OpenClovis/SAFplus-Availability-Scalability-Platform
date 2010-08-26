/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/figure/ComponentFigure.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.figure;


import java.util.List;

import org.eclipse.core.resources.IProject;
import org.eclipse.draw2d.ColorConstants;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.ToolbarLayout;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.editor.ca.ComponentDataUtils;
import com.clovis.cw.editor.ca.ComponentEditor;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.genericeditor.model.Base;
import com.clovis.cw.genericeditor.model.NodeModel;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.project.data.SubModelMapReader;

/**
 * @author pushparaj
 *
 * Figure class for component
 */
public class ComponentFigure extends ShadowRoundedRectangle
implements ComponentEditorConstants
{
    private NodeModel _nodeModel;
    private Label _componentNameLabel = new Label();
    private Label _componentPropertyLabel = new Label();

    /**
     * constructor
     * @param node NodeModel
     */
    public ComponentFigure(NodeModel node)
    {
        _nodeModel = node;
        setLayoutManager(new ToolbarLayout());
        setCornerDimensions(new Dimension(0, 0));
        setOpaque(true);

        _componentPropertyLabel.setFont(COMPONENT_PROPERTY_FONT);
        add(_componentPropertyLabel);

        _componentNameLabel.setFont(COMPONENT_NAME_FONT);
        add(_componentNameLabel);

        createConnectionAnchors(this);
        initComponentProperties();
    }
    /**
     * Sets all properties values
     *
     */
    public void initComponentProperties()
    {
        EObject obj = _nodeModel.getEObject();
        _componentNameLabel.setText((String) EcoreUtils.getValue
                (obj, NAME));
        String property = EcoreUtils.getValue(obj, COMPONENT_PROPERTY)
        .toString();
        _componentPropertyLabel.setText("< " + property + " >");
        setComponentColor();
    }
    /**
     * Sets the correct color for Component
     *
     */
    private void setComponentColor() {
		EObject obj = _nodeModel.getEObject();
		IProject project = _nodeModel.getRootModel().getProject();
		ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(project);
        GenericEditorInput geInput = (GenericEditorInput) pdm.getComponentEditorInput();
        ComponentEditor editor = (ComponentEditor) geInput.getEditor();
        String rdn = (String) EcoreUtils.getValue(obj,
        		ModelConstants.RDN_FEATURE_NAME);
        EObject mapObj = editor.getLinkViewModel().getEObject();
        EObject linkObj = SubModelMapReader.getLinkObject(mapObj,
        		ComponentEditorConstants.ASSOCIATE_RESOURCES_NAME);
		List realizes = SubModelMapReader.getLinkTargetObjects(linkObj, rdn);
		String type = EcoreUtils.getValue(obj, COMPONENT_PROPERTY).toString();
		if (type.equals(SAF_AWARE_COMPONENT_NAME)) {
			if (realizes != null && realizes.size() > 0) {
				setBackgroundColor(ColorConstants.lightBlue);
			} else {
				setBackgroundColor(SAFCOMPONENT_COLOR);
			}
		} else if (type.equals(PROXIED_PREINSTANTIABLE)) {
			setBackgroundColor(PROXIED_PREINSTANTIABLE_COLOR);
		} else if (type.equals(PROXIED_NON_PREINSTANTIABLE)) {
			setBackgroundColor(PROXIED_NON_PREINSTANTIABLE_COLOR);
		} else if (type.equals(NON_PROXIED_NON_PREINSTANTIABLE)) {
			setBackgroundColor(NON_PROXIED_NON_PREINSTANTIABLE_COLOR);
		}
	}
    /**
     * @see com.clovis.cw.genericeditor.figures.AbstractNodeFigure#getModel()
     */
	public Base getModel() {
		return _nodeModel;
	}
}

 
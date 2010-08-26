/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.figure;

import java.util.List;

import org.eclipse.draw2d.Figure;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.PositionConstants;
import org.eclipse.draw2d.ToolbarLayout;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.graphics.Image;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.editpart.AttributeOperationBorder;
import com.clovis.cw.genericeditor.model.Base;
import com.clovis.cw.genericeditor.model.NodeModel;

/**
 * @author pushparaj
 *
 * Figure for Data Structure
 */
public class DataStructureFigure extends ShadowRoundedRectangle
    implements ClassEditorConstants
{

    private Label     _classNameLabel     = new Label();
    //private Label     _stereoTypeLabel    = new Label();
    private Label     _classModifierLabel = new Label();
    private Figure    _attributeFigure    = new Figure();
    private NodeModel _model;

    /**
     * Create Figure
     * @param model NodeModel
     */
    public DataStructureFigure(NodeModel model)
    {
        super();
        _model = model;
        setLayoutManager(new ToolbarLayout());
        //setBorder(new LineBorder(ColorConstants.black, 1));
        setOpaque(true);
        setBackgroundColor(DATA_STRUCTURE_COLOR);
        setCornerDimensions(new Dimension(0, 0));

        _classNameLabel.setFont(CLASS_NAME_FONT);
        add(_classNameLabel);

        /*_stereoTypeLabel.setFont(EXTENSIONS_FONT);
        add(_stereoTypeLabel);*/

        _classModifierLabel.setFont(CLASS_NAME_FONT);
        add(_classModifierLabel);

         // Need to draw a line before starting painting attributes
        _attributeFigure.setBorder(new AttributeOperationBorder());
        _attributeFigure.setLayoutManager(new ToolbarLayout());
        add(_attributeFigure);


        createConnectionAnchors(this);
        initClassDisplay();
    }

    /**
     * Sets Class Name in Figure
     */
    private void setClassName()
    {
        EObject obj        = _model.getEObject();
        String text = (String) EcoreUtils.getValue(obj, CLASS_NAME);
        _classNameLabel.setText(text);
        _classNameLabel.setFont(CLASS_NAME_FONT);
        Image image = ImageDescriptor.createFromFile(
                NodeModel.class, "icons/class_public_obj.gif")
                .createImage();
        _classNameLabel.setIcon(image);
    }


    /**
     * Shows Attributes in the figure.
     */
    private void setClassAttributes()
    {
        _attributeFigure.removeAll();
        List listOfAttributes =
            (List) EcoreUtils.getValue(_model.getEObject(), CLASS_ATTRIBUTES);
        int size = listOfAttributes.size();
        for (int i = 0; i < size; i++) {
            Label attributeNameLabel = new Label();
            attributeNameLabel.setFont(ATTRIBUTE_NAME_FONT);
            attributeNameLabel.setLabelAlignment(PositionConstants.LEFT);
            EObject attr = (EObject) listOfAttributes.get(i);

            String name = (String) EcoreUtils.getValue(attr, ATTRIBUTE_NAME);
            String type = EcoreUtils.getValue(attr, ATTRIBUTE_TYPE).toString();
            boolean isImported = ((Boolean)EcoreUtils.getValue(attr,
            		ATTRIBUTE_IS_IMPORTED)).booleanValue();
            int multiplicity = ((Integer) EcoreUtils.getValue(attr,
            		ATTRIBUTE_MULTIPLICITY)).intValue();
            String visibility =
                EcoreUtils.getValue(attr, ATTRIBUTE_TYPE).toString();
            String icon = visibility.equalsIgnoreCase("private")
                ? "icons/field_private_obj.gif" : "icons/field_public_obj.gif";
            attributeNameLabel.setIcon(
               ImageDescriptor.createFromFile(getClass(), icon).createImage());
            if (multiplicity > 1) {
            	name = name + "[]";
            }
            if (isImported) {
            	attributeNameLabel.setText("*" + type + " " + name);
            } else {
            	attributeNameLabel.setText(type + " " + name);
            }
            _attributeFigure.add(attributeNameLabel);
        }
    }
    /**
     * Initialize the Display.
     * This method is also called for refresh.
     * @param model
     */
    public void initClassDisplay()
    {
        setClassName();
        setClassAttributes();
    }
    /**
     * @see com.clovis.cw.genericeditor.figures.AbstractNodeFigure#getModel()
     */
	public Base getModel() {
		return _model;
	}
}

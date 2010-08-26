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

import org.eclipse.draw2d.ColorConstants;
import org.eclipse.draw2d.Figure;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.PositionConstants;
import org.eclipse.draw2d.ToolbarLayout;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.jface.resource.ImageDescriptor;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.editpart.AttributeOperationBorder;
import com.clovis.cw.genericeditor.model.Base;
import com.clovis.cw.genericeditor.model.NodeModel;

/**
 * @author pushparaj
 *
 * Figure Class for Resource
 */
public class ResourceFigure extends ShadowRoundedRectangle implements
        ClassEditorConstants
{
    private Label     _classNameLabel     = new Label();

    //private Label     _stereoTypeLabel    = new Label();

    private Label     _classModifierLabel = new Label();

    private Figure    _attributeFigure    = new Figure();

   // private Figure    _operationFigure    = new Figure();

    private NodeModel _model;

    /**
     * Constructor
     * Creates Resource Figure
     * @param model Node Model
     */
    public ResourceFigure(NodeModel model)
    {
        super();
        _model = model;
        setLayoutManager(new ToolbarLayout());
        setOpaque(true);
        setNodeColor();
        setCornerDimensions(new Dimension(0, 0));
        if(model.getEObject().eClass().getName().equals(MIB_RESOURCE_NAME)) {
        	Label mibLabel = new Label();
        	mibLabel.setForegroundColor(ColorConstants.red);
        	mibLabel.setFont(NAME_ITALICS_FONT);
        	mibLabel.setText("< mib: " + (String) EcoreUtils.getValue(model.getEObject(), "mibName") + " >");
        	add(mibLabel);
        }
        _classNameLabel.setFont(CLASS_NAME_FONT);
        add(_classNameLabel);

        _classModifierLabel.setFont(CLASS_NAME_FONT);
        add(_classModifierLabel);

        // Need to draw a line before starting painting attributes
        _attributeFigure.setBorder(new AttributeOperationBorder());
        _attributeFigure.setLayoutManager(new ToolbarLayout());
        add(_attributeFigure);

        //Add Operation Figure.
        /*_operationFigure.setBorder(new AttributeOperationBorder());
        _operationFigure.setLayoutManager(new ToolbarLayout());
        add(_operationFigure);*/

        createConnectionAnchors(this);
        initClassDisplay();
    }
    /**
     * Sets Class Name in Figure
     */
    private void setClassName()
    {
        EObject obj = _model.getEObject();
        String text = "";
        /*String multiplicity = (String) EcoreUtils.getValue(obj,
                CLASS_MULTIPLICITY);*/
        text += (String) EcoreUtils.getValue(obj, CLASS_NAME);
        /*if (multiplicity != null && multiplicity.length() > 0) {
            text += "[" + multiplicity + "]";
        }*/
        _classNameLabel.setText(text);
        _classNameLabel.setFont(CLASS_NAME_FONT);
    }

    /**
     * Sets Class Modifier
     */
    /*private void setClassModifier()
    {
        String value = (String) EcoreUtils.getValue(_model.getEObject(),
                CLASS_MODIFIER);
        if (value.equalsIgnoreCase("public")) {
            Image publicImage = ImageDescriptor.createFromFile(NodeModel.class,
                    "icons/class_public_obj.gif").createImage();
            _classNameLabel.setIcon(publicImage);
        } else {
            Image privateImage = ImageDescriptor.createFromFile(
                    NodeModel.class, "icons/class_public_obj.gif")
                    .createImage();
            _classNameLabel.setIcon(privateImage);
        }
    	
    }*/
    /**
     * Shows Attributes in the figure.
     */
    private void setResourceProperties()
    {
        _attributeFigure.removeAll();
        //_operationFigure.removeAll();
        EObject eObject = _model.getEObject();
        List listOfAttributes = (List) EcoreUtils.getValue(eObject,
                CLASS_ATTRIBUTES);
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
            String visibility = EcoreUtils.getValue(attr, ATTRIBUTE_TYPE)
                    .toString();
            String icon = visibility.equalsIgnoreCase("private")
                ? "icons/field_private_obj.gif" : "icons/field_public_obj.gif";
            attributeNameLabel.setIcon(ImageDescriptor.createFromFile(
                    getClass(), icon).createImage());
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
        setNodeColor();
        setServiceAttributes();
    }

    /**
     * Parse Service EObjects and add attributes and methods in to Figure.
     *
     */
    private void setServiceAttributes()
    {
        /** this method should be change * */
        EObject eObject = _model.getEObject();
        EClass eclass = _model.getEObject().eClass();
        EReference serRef = (EReference) eclass
                .getEStructuralFeature(ClassEditorConstants.
                        RESOURCE_PROVISIONING);
        EObject serObj = (EObject) eObject.eGet(serRef);
        //EClass serClass = serRef.getEReferenceType();
        /*if (serObj == null) {
            serObj = serClass.getEPackage().getEFactoryInstance().create(
                    serClass);
            eObject.eSet(eObject.eClass().getEStructuralFeature(
                    ClassEditorConstants.RESOURCE_PROVISIONING), serObj);
        } else {
            setServiceAttributes(serObj);
            setServiceMethods(serObj);
        }*/
        if (serObj != null) {
            setServiceAttributes(serObj);
            setServiceMethods(serObj);
        }
        /*serRef = (EReference) eclass
                .getEStructuralFeature(ClassEditorConstants.RESOURCE_CHASSIS);
        serObj = (EObject) eObject.eGet(serRef);
        serClass = serRef.getEReferenceType();
        if (serObj == null) {
            serObj = serClass.getEPackage().getEFactoryInstance().create(
                    serClass);
            eObject.eSet(eObject.eClass().getEStructuralFeature(
                    ClassEditorConstants.RESOURCE_CHASSIS), serObj);
        } else {
            setServiceAttributes(serObj);
            setServiceMethods(serObj);
        }*/

        /*serRef = (EReference) eclass
                .getEStructuralFeature(ClassEditorConstants.RESOURCE_ALARM);
        serObj = (EObject) eObject.eGet(serRef);
        serClass = serRef.getEReferenceType();
        if (serObj == null) {
            serObj = serClass.getEPackage().getEFactoryInstance().create(
                    serClass);
            eObject.eSet(eObject.eClass().getEStructuralFeature(
                    ClassEditorConstants.RESOURCE_ALARM), serObj);
        } else {
            setServiceAttributes(serObj);
            setServiceMethods(serObj);
        }*/

        /*serRef = (EReference) eclass
                .getEStructuralFeature(ClassEditorConstants.RESOURCE_FAULT);
        serObj = (EObject) eObject.eGet(serRef);
        serClass = serRef.getEReferenceType();
        if (serObj == null) {
            serObj = serClass.getEPackage().getEFactoryInstance().create(
                    serClass);
            eObject.eSet(eObject.eClass().getEStructuralFeature(
                    ClassEditorConstants.RESOURCE_FAULT), serObj);
        } else {
            setServiceAttributes(serObj);
            setServiceMethods(serObj);
        }*/
    }

    /**
     * Add service's attributes in Figure
     * @param obj Service EObject
     */
    private void setServiceAttributes(EObject obj)
    {
        List listOfAttributes = (List) EcoreUtils.getValue(obj,
                CLASS_ATTRIBUTES);
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
            String visibility = EcoreUtils.getValue(attr, ATTRIBUTE_TYPE)
                    .toString();
            String icon = visibility.equalsIgnoreCase("private")
                ? "icons/field_private_obj.gif" : "icons/field_public_obj.gif";
            attributeNameLabel.setIcon(ImageDescriptor.createFromFile(
                    getClass(), icon).createImage());
            if (multiplicity > 1) {
            	name = name + "[]";
            }
            if (isImported) {
            	attributeNameLabel.setText("*" + type + " " + name + " <"
                        + getServiceName(obj) + ">");
            } else {
            	attributeNameLabel.setText(type + " " + name + " <"
                    + getServiceName(obj) + ">");
            }
            _attributeFigure.add(attributeNameLabel);
        }
    }

    /**
     *
     * @param obj
     *            Service EObject
     * @return service name.
     */
    private String getServiceName(EObject obj)
    {
        String name = obj.eContainingFeature().getName();
        if (name.equals(RESOURCE_PROVISIONING)) {
            return "prov";
        } else if (name.equals(RESOURCE_CHASSIS)) {
            return "chassis";
        } else if (name.equals(RESOURCE_ALARM)) {
            return "alarm";
        } else if (name.equals(RESOURCE_FAULT)) {
            return "fault";
        }
        return "";
    }

    /**
     * Initialize the Display. This method is also called for refresh.
     *
     * @param model
     */
    public void initClassDisplay()
    {
        setClassName();
        //setClassModifier();
        setResourceProperties();
    }

    /**
     * Sets the correct color for H/W and S/W resource, SC and abstract
     *
     */
    private void setNodeColor()
    {
        if (_model.getName().equals(HARDWARE_RESOURCE_NAME)) {
            setBackgroundColor(HARDWARE_RESOURCE_COLOR);
        } else if (_model.getName().equals(SOFTWARE_RESOURCE_NAME)) {
            setBackgroundColor(SOFTWARE_RESOURCE_COLOR);
        } else if (_model.getName().equals(NODE_HARDWARE_RESOURCE_NAME)) {
            setBackgroundColor(NODE_HARDWARE_RESOURCE_COLOR);
        } else if (_model.getName().equals(SYSTEM_CONTROLLER_NAME)) {
            setBackgroundColor(SYSTEM_CONROLLER_COLOR);
        } else if (_model.getName().equals(MIB_RESOURCE_NAME)) {
            setBackgroundColor(SYSTEM_CONROLLER_COLOR);
        } else {
        	setBackgroundColor(ColorConstants.white);
        }
    }

    /**
     * Add service's methods in to Figure.
     *
     * @param obj
     * Service EObject
     */
    private void setServiceMethods(EObject obj)
    {
        List methodList = (List) EcoreUtils.getValue(obj, CLASS_METHODS);
        for (int i = 0; methodList != null && i < methodList.size(); i++) {
            Label nameLabel = new Label();
            nameLabel.setFont(ATTRIBUTE_NAME_FONT);
            nameLabel.setLabelAlignment(PositionConstants.LEFT);

            EObject mObj = (EObject) methodList.get(i);
            String methodName = (String) EcoreUtils.getValue(mObj, METHOD_NAME);
            String methodType = EcoreUtils.getValue(mObj,
            		METHOD_RETURN_TYPE).toString();
            String visibility = EcoreUtils.getValue(mObj,
                    METHOD_MODIFIER).toString();
            String icon = visibility.equals("private") ?
            		"icons/methpri_obj.gif" : "icons/methpub_obj.gif";
            nameLabel.setIcon(ImageDescriptor.createFromFile(getClass(), icon)
                    .createImage());

            EReference paramReference = (EReference) mObj.eClass()
                    .getEStructuralFeature(METHOD_PARAMS);
            Object paramRefFeature = mObj.eGet(paramReference);
            EList paramList = (EList) paramRefFeature;

            String params = "";
            for (int j = 0; paramList != null && j < paramList.size(); j++) {
                EObject eObj = (EObject) paramList.get(j);
                String name = EcoreUtils.getValue(eObj, PARAM_NAME).toString();
                int multiplicity = ((Integer) EcoreUtils.getValue(eObj,
                		PARAM_MULTIPLICITY)).intValue();
                if (multiplicity > 1) {
                	name = name + "[]";
                }
                params += name + " : " + EcoreUtils.getValue(eObj, PARAM_TYPE);
                if (j != paramList.size() - 1) {
                    params += ", ";
                }
            }
            nameLabel.setText(methodType + " " + methodName + "("
                    + params + ")" + " <" + getServiceName(obj) + ">");
            nameLabel.setFont(METHOD_NAME_FONT);
            //_operationFigure.add(nameLabel);
        }
    }
    /**
     * @see com.clovis.cw.genericeditor.figures.AbstractNodeFigure#getModel()
     */
	public Base getModel() {
		return _model;
	}
}

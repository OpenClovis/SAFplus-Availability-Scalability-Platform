/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/model/GENodeElementFactory.java $
 * $Author: srajyaguru $
 * $Date: 2007/04/30 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor.model;

import java.util.List;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.gef.requests.CreationFactory;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreCloneUtils;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.editor.EditorUtils;
import com.clovis.cw.genericeditor.GEDataUtils;
import com.clovis.cw.genericeditor.GenericEditor;
/**
 * @author pushparaj
 * Factory for Nodes.
 */
public class GENodeElementFactory implements CreationFactory
{
    private GenericEditor _editor;
    private EObject       _template;
    /**
     *
     * @param editor Editor instance
     * @param obj    EObject instance
     */
    public GENodeElementFactory(GenericEditor editor, EObject obj)
    {
        _editor = editor;
        _template = obj;
    }
    /**
     * @return Object Type (_template)
     */
    public Object getObjectType()
    {
        return _template;
    }
    /**
     * @see org.eclipse.gef.requests.CreationFactory#getNewObject()
     */
    public Object getNewObject()
    {
        EClass eClass = _template.eClass();
		NodeModel node = null;
		String value = EcoreUtils.getAnnotationVal(eClass, null, "isContainer");
		if (value != null && value.equals("true")) {
			node = new ContainerNodeModel(eClass.getName());
		} else {
			node = new NodeModel(eClass.getName());
		}
		if (_template != null) {
            EStructuralFeature attr = eClass.getEStructuralFeature(
            		ModelConstants.RDN_FEATURE_NAME);
            EObject obj = EcoreCloneUtils.cloneEObject(_template);
            int code = obj.hashCode();
            obj.eSet(attr, String.valueOf(code));
            node.setEObject(obj);
            _editor.getEditorModel().putNodeModel(obj, node);
            String prefix  = EcoreUtils.getName(_template);
            if (prefix == null || prefix.equals("")) {
                prefix = _template.eClass().getName();
            }
            List currentObjs = _editor.getEditorModel().getEList();
            String fld = EcoreUtils.getNameField(_template.eClass());
            String newName = EditorUtils.getNextValue(prefix, currentObjs, fld);
            //obj.eSet(eClass.getEStructuralFeature(fld), newName);
            EcoreUtils.setValue(obj, fld, newName);

            // handle specific field initialization in generic manner
            String initializationInfo = EcoreUtils.getAnnotationVal(obj.eClass(), null, "initializationFields");
            if( initializationInfo != null ) {
            	EcoreUtils.initializeFields(obj, GEDataUtils
						.getNodeListFromType(currentObjs, obj.eClass()
								.getName()), initializationInfo);
            }
        }
        return node;
    }
    /**
     * 
     * @return
     */
    public EObject getNewEObject()
    {
        EClass eClass = _template.eClass();
        EObject obj = null;
        if (_template != null) {
            EStructuralFeature attr = eClass.getEStructuralFeature(
            		ModelConstants.RDN_FEATURE_NAME);
            obj = EcoreCloneUtils.cloneEObject(_template);
            int code = obj.hashCode();
            obj.eSet(attr, String.valueOf(code));
            String prefix  = EcoreUtils.getName(_template);
            if (prefix == null || prefix.equals("")) {
                prefix = _template.eClass().getName();
            }
            List currentObjs = _editor.getEditorModel().getEList();
            String fld = EcoreUtils.getNameField(_template.eClass());
            String newName = EditorUtils.getNextValue(prefix, currentObjs, fld);
            //obj.eSet(eClass.getEStructuralFeature(fld), newName);
            EcoreUtils.setValue(obj, fld, newName);

            // handle specific field initialization in generic manner
            String initializationInfo = EcoreUtils.getAnnotationVal(obj.eClass(), null, "initializationFields");
            if( initializationInfo != null )
            	EcoreUtils.initializeFields(obj, currentObjs, initializationInfo);
        }
        return obj;
    }
}

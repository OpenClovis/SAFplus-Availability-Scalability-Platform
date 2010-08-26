/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/model/GEEdgeElementFactory.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor.model;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.gef.requests.CreationFactory;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreCloneUtils;
import com.clovis.cw.genericeditor.GenericEditor;
/**
 * @author pushparaj
 *
 * Factory for Edge.
 */
public class GEEdgeElementFactory implements CreationFactory
{
    private EObject _template;
    private GenericEditor _editor;
    /**
     * constructor
     * @param editor GenericEditor
     * @param cl EObject
     */
    public GEEdgeElementFactory(GenericEditor editor, EObject obj)
    {
        _editor = editor;
        _template = obj;
    }
    /**
     * @return Object Type
     */
    public Object getObjectType()
    {
        return _template;
    }
    /**
     *
     * @see org.eclipse.gef.requests.CreationFactory#getNewObject()
     */
    public Object getNewObject()
    {
        EClass eClass = _template.eClass();
        EdgeModel edge = new EdgeModel(eClass.getName());
        if (_template != null) {
            EStructuralFeature attr = eClass.getEStructuralFeature(
            		ModelConstants.RDN_FEATURE_NAME);
            EObject obj = EcoreCloneUtils.cloneEObject(_template);
            int code = obj.hashCode();
            obj.eSet(attr, String.valueOf(code));
            edge.setEObject(obj);
            _editor.getEditorModel().putEdgeModel(obj, edge);
        }
        return edge;
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
        }
        return obj;
    }
}

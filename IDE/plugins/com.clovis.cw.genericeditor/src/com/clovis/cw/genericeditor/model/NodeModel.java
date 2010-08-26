/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


package com.clovis.cw.genericeditor.model;

import java.util.ArrayList;
import java.util.StringTokenizer;
import java.util.Vector;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;

import com.clovis.common.utils.ecore.EcoreUtils;

/**
 * @author pushparaj
 *
 * Model Class for Nodes
 */

public class NodeModel extends Base
{
    public static final String ID_SIZE     = "size";
    
    public static final String ID_LOCATION = "location";
    
    public static final String DEFAULT_PARENT = "default";
    
    private String             _className;

    private EditorModel        _rootModel;

    private EObject      _eObject;
    
    public ArrayList 		_validParentObjects = new ArrayList();
    
    /**
     * constructor
     *
     * @param value name for EClass
     */
    public NodeModel(String value)
    {
    	super();
        _className = value;
    }
    /**
     * set the name.
     *
     * @param value Name
     */
    public void setName(String value)
    {
        _className = value;
    }

    /**
     * returns name.
     *
     * @return Name
     */
    public String getName()
    {
        return _className;
    }

    /**
     * set the root model.
     *
     * @param model EditorModel
     */
    public void setRootModel(EditorModel model)
    {
        _rootModel = model;
    }

    /**
     *
     * @return root model
     */
    public EditorModel getRootModel()
    {
        if (_rootModel != null) {
            return _rootModel;
        } else {
            return findRootModel();
        }
    }

    /**
     * find and return root model
     *
     * @return editor model
     */
    private EditorModel findRootModel()
    {
        ContainerNodeModel parentModel = (ContainerNodeModel) getParent();
        while (parentModel.getParent() != null) {
            parentModel = (ContainerNodeModel) parentModel.getParent();
        }
        _rootModel = (EditorModel) parentModel;
        return _rootModel;
    }
    /**
     * returns list of source connections.
     * @return Vector
     */
    public Vector getSourceConnections()
    {
        return _sourceTerminal;
    }

    /**
     * returns list of target connections.
     * @return vector
     */
    public Vector getTargetConnections()
    {
        return _targetTerminal;
    }

    /**
     * List of source connections for current node.
     */
    private Vector _sourceTerminal = new Vector(2);

    /**
     * List of target connections for current node.
     */
    private Vector _targetTerminal = new Vector(2);

    /**
     *
     * @param iConnectionModel EdgeModel
     */
    public void connectInput(EdgeModel iConnectionModel)
    {
        _sourceTerminal.addElement(iConnectionModel);
        _listeners.firePropertyChange("inputs", null, iConnectionModel);
    }
    /**
     *
     * @param iConnectionModel EdgeModel
     */
    public void connectOutput(EdgeModel iConnectionModel)
    {
        _targetTerminal.addElement(iConnectionModel);
        _listeners.firePropertyChange("outputs", null, iConnectionModel);
    }

    /**
     *
     * @param iConnectionModel edgeModel
     */
    public void disconnectInput(EdgeModel iConnectionModel)
    {
        _sourceTerminal.remove(iConnectionModel);
        _listeners.firePropertyChange("inputs", null, iConnectionModel);
    }

    /**
     *
     * @param iConnectionModel edgeModel
     */
    public void disconnectOutput(EdgeModel iConnectionModel)
    {
        _targetTerminal.removeElement(iConnectionModel);
        _listeners.firePropertyChange("outputs", null, iConnectionModel);
    }

    /**
     * Sets EObject instance.
     * @param obj EObject
     */
    public void setEObject(EObject obj) {
		_eObject = obj;
		String value = EcoreUtils.getAnnotationVal(obj.eClass(), null,
				"possibleParentObjects");
		if (value == null || value.equals("")) {
			_validParentObjects.add(DEFAULT_PARENT);
		} else {
			StringTokenizer tokenizer = new StringTokenizer(value, ",");
			while (tokenizer.hasMoreTokens()) {
				String token = tokenizer.nextToken();
				_validParentObjects.add(token);
			}
		}
	}

    /**
	 * 
	 * @return Eobject
	 */
    public EObject getEObject()
    {
        return _eObject;
    }

    /**
     * Sets EObject property value
     * @param property Property
     * @param value Value
     */
    public void setEPropertyValue(Object property, Object value)
    {
        if (_eObject == null) {
            return;
        }
        EStructuralFeature attr = _eObject.eClass().getEStructuralFeature(
                String.valueOf(property));
        if (attr != null) {
            _eObject.eSet(attr, value);
        }
    }

    /**
     *
     * @param property Property
     * @return Value
     */
    public Object getEPropertyValue(Object property)
    {
        if (_eObject == null) {
            return null;
        }
        EStructuralFeature attr = _eObject.eClass().getEStructuralFeature(
                String.valueOf(property));
        if (attr != null) {
            return _eObject.eGet(attr);
        }
        return null;
    }


}

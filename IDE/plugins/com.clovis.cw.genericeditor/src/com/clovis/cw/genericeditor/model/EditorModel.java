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
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

import org.eclipse.core.resources.IProject;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.notify.impl.AdapterImpl;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.ui.views.properties.ComboBoxPropertyDescriptor;
import org.eclipse.ui.views.properties.IPropertyDescriptor;
import org.eclipse.ui.views.properties.IPropertySource;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.genericeditor.EditorModelValidator;
import com.clovis.cw.genericeditor.GEDataUtils;
import com.clovis.cw.genericeditor.Messages;
import com.clovis.cw.genericeditor.UIPropertiesCreator;

/**
 *
 * @author pushparaj
 *
 * Base Model class for Editor
 */
public class EditorModel extends ContainerNodeModel implements IPropertySource
{
    /**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	private double _zoom = 1.0;
    public static final String ID_ROUTER = "router";
    public static final Integer ROUTER_MANUAL = new Integer(1);
    public static final Integer ROUTER_MANHATTAN = new Integer(0);
    protected Integer _connectionRouter = null;

    private EList _eList;
    //hash map for EObject and NodeModel.
    private Map _nodeMap = new HashMap();
    // hash map for EObject and EdgeModel.
    private Map _edgeMap = new HashMap();
    private GEDataUtils _utils;
    private ArrayList _nodeNamesList;
    private ArrayList _edgeNamesList;
    private IProject _project;
    private boolean _isDirty;
    private EditorModelValidator _validator = null;
    private Map _referencesEListMap = new HashMap();
    
    /**
     * Constructor
     *
     */
    public EditorModel()
    {
        super("ROOT");
    }
    /**
     * Constructor
     * @param project Project
     */
    public EditorModel(IProject project)
    {
        this();
        _project = project;
    }
    /**
     * Sets Project
     * @param project IProject
     */
    public void setProject(IProject project)
    {
        _project = project;
    }
    /**
     * Returns IProject
     * @return IProject
     */
    public IProject getProject()
    {
        return _project;
    }
    /**
     * returns zoom value.
     *
     * @return zoom value
     */
    public double getZoom()
    {
        return _zoom;
    }
    /**
     * set zoom.
     *
     * @param zoom zoom value
     */
    public void setZoom(double zoom)
    {
        _zoom = zoom;
    }
    /**
     * Set Model state
     * @param dirty state
     */
    public void setDirty(boolean dirty)
    {
        _isDirty = dirty;
    }
    /**
     * @return model state
     */
    public boolean isDirty()
    {
        return _isDirty;
    }
    /**
     * 
     * @param obj
     */
    public void addEObject(EObject obj)
    {
    	if( _referencesEListMap.get(obj.eClass().getName()) == null) {
    		EObject rootObject = (EObject) _eList.get(0);
    		List refList = rootObject.eClass().getEAllReferences();
    		for (int i = 0; i < refList.size(); i++) {
    			EReference ref = (EReference) refList.get(i);
    			if(ref.getEType().getName().equals(obj.eClass().getName())) {
    				List list = (EList) rootObject.eGet(ref);
    				_referencesEListMap.put(obj.eClass().getName(), list);
    				list.add(obj);
    			}
    		}
    	} else {
    		EList list = (EList) _referencesEListMap.get(obj.eClass().getName());
    		list.add(obj);
    	}
    }
    /**
     * 
     * @param obj
     */
    public void removeEObject(EObject obj)
    {
    	EList list = (EList) _referencesEListMap.get(obj.eClass().getName());
    	list.remove(obj);
    }
    /**
     *
     * @param list
     *            EObjects
     * @param utils
     *            GEUtils
     * @param nodelist
     *            meta-class Info for nodes
     * @param edgelist
     *            met-class Info for edges
     */
    public void setModelInfo(EList list, GEDataUtils utils, ArrayList nodelist,
            ArrayList edgelist)
    {
        _eList = list;
        EAdapter eListener = new EAdapter(this);
        EcoreUtils.addListener((NotifyingList) _eList, eListener, -1);
        _utils = utils;
        _nodeNamesList = nodelist;
        _edgeNamesList = edgelist;
    }
    /**
     * Sets Validator
     * @param validator Editor Validator
     */
    public void setEditorModelValidator(EditorModelValidator validator)
    {
    	_validator = validator;
    }
    /**
     * Returns Validator
     * @return EditorModelValidator
     */
    public EditorModelValidator getEditorModelValidator()
    {
    	return _validator;
    }
    /**
     *
     * @return _eList list for EObjects
     */
    public EList getEList()
    {
        return _eList;
    }
    /**
     * Returns Map for Reference ELists
     * @return Map
     */
    public Map getEReferencesEListMap()
    {
    	return _referencesEListMap;
    }
    /**
     *
     * @param key
     *            EObject
     * @param value
     *            NodeModel
     */
    public void putNodeModel(Object key, Object value)
    {
        //if (!_eMap.containsKey(key))
        _nodeMap.put(key, value);
    }

    /**
     * Creates New NodeModel
     * @param obj EObject
     */
    private void createNewNodeModel(EObject obj)
    {
    	NodeModel node = null;
    	String value = EcoreUtils.getAnnotationVal(obj.eClass(), null, "isContainer");
    	if(value != null && value.equals("true")) {
    		node = new ContainerNodeModel(obj.eClass().getName());
    		node.setSize(new Dimension(260, 130));
    	} else {
    		node = new NodeModel(obj.eClass().getName());
    		node.setSize(new Dimension(130, 65));
    	}
        node.setEObject(obj);
        ContainerNodeModel parent = null;
        EObject parentObject = _utils.getParent(obj);
        if (parentObject == null) {
            parent = this;
            node.setLocation(UIPropertiesCreator.createNodeLocation(this));
        } else {
            parent = (ContainerNodeModel) _nodeMap.get(parentObject);
            node.setLocation(UIPropertiesCreator.createNodeLocation(parent));
        }
        putNodeModel(obj, node);
        parent.addChild(node);
    }
    /**
     *
     * @param key
     *            EObject
     * @param value
     *            EdgeModel
     */
    public void putEdgeModel(Object key, Object value)
    {
        _edgeMap.put(key, value);
    }

    /**
     * creates new EdgeModel
     *
     * @param obj
     *            EObject
     */
    private void createNewEdgeModel(EObject obj)
    {
        EdgeModel edge = new EdgeModel(obj.eClass().getName());
        edge.setEObject(obj);
        NodeModel source = (NodeModel) _nodeMap.get(_utils.getSource(obj));
        edge.setSource(source);
        NodeModel target = (NodeModel) _nodeMap.get(_utils.getTarget(obj));
        edge.setTarget(target);
        edge.setSourceTerminal("31");
        edge.setTargetTerminal("E");
        putEdgeModel(obj, edge);
        edge.attachSource();
        edge.attachTarget();
    }

    /**
     *
     * @return connectionRouter.
     */
    public Integer getConnectionRouter()
    {
        if (_connectionRouter == null) {
            _connectionRouter = ROUTER_MANHATTAN;
        }
        return _connectionRouter;
    }

    /**
     * Returns <code>null</code> for this model. Returns
     * normal descriptors for all subclasses.
     *
     * @return  Array of property descriptors.
     */
    public IPropertyDescriptor[] getPropertyDescriptors()
    {
         ComboBoxPropertyDescriptor cbd = new ComboBoxPropertyDescriptor(
                ID_ROUTER,
                Messages.PROPERTDESCRIPTOR_BASEDIAGRAM_CONNECTIONROUTER,
                new String[] { Messages.
                        PROPERTDESCRIPTOR_BASEDIAGRAM_MANHATTAN,
                        Messages.PROPERTDESCRIPTOR_BASEDIAGRAM_MANUAL });
        cbd.setLabelProvider(new ConnectionRouterLabelProvider());
        return new IPropertyDescriptor[] { cbd };
    }

    /**
     * 
     * @param router
     *            int value
     */
    public void setConnectionRouter(Integer router)
    {
        Integer oldConnectionRouter = _connectionRouter;
        _connectionRouter = router;
        _listeners.firePropertyChange(ID_ROUTER, oldConnectionRouter
                , _connectionRouter);
    }
    /**
     * @see org.eclipse.ui.views.properties.IPropertySource
     * #setPropertyValue(java.lang.Object, java.lang.Object)
     */
    public void setPropertyValue(Object id, Object value)
    {
        if (ID_ROUTER.equals(id)) {
            setConnectionRouter((Integer) value);
        }
    }

    /**
     * @see org.eclipse.ui.views.properties.IPropertySource
     * #getPropertyValue(java.lang.Object)
     */
    public Object getPropertyValue(Object propName)
    {
        if (propName.equals(ID_ROUTER)) {
            return _connectionRouter;
        }
        return null;
    }
    /**
     *
     * @author pushparaj
     *
     *  Label Provider for Connection router
     */
    private class ConnectionRouterLabelProvider
        extends org.eclipse.jface.viewers.LabelProvider
    {

        /**
         * constructor
         *
         */
        public ConnectionRouterLabelProvider()
        {
            super();
        }

        /**
         * @see org.eclipse.jface.viewers.ILabelProvider#getText
         * (java.lang.Object)
         */
        public String getText(Object element)
        {
            if (element instanceof Integer) {
                Integer integer = (Integer) element;
                if (ROUTER_MANUAL.intValue() == integer.intValue()) {
                    return Messages.PROPERTDESCRIPTOR_BASEDIAGRAM_MANUAL;
                }
                if (ROUTER_MANHATTAN.intValue() == integer.intValue()) {
                    return Messages.PROPERTDESCRIPTOR_BASEDIAGRAM_MANHATTAN;
                }
            }
            return super.getText(element);
        }
    }
    /**
     *
     * @author pushparaj
     *
     * This will listen all EList changes and update
     * visual parts.
     */
    public class EAdapter extends AdapterImpl
    {

        EditorModel _editorModel;

        /**
         *
         * @param model EditorModel
         */
        public EAdapter(EditorModel model)
        {
            _editorModel = model;
        }

        /**
         * Adds listenr to newObject. Install corresponding
         * visual part(node or edge). Then fire the
         * event to update the visuals.
         * @param newObject new EObject
         * @param parentObject Container for newObject
         */
        private void addEObject(EObject newObject, EObject parentObject)
        {
            EcoreUtils.addListener(newObject, this, -1);
            if (_nodeMap.containsKey(newObject)) {
            	NodeModel newNode = (NodeModel) _nodeMap.get(newObject);
                ((ContainerNodeModel) newNode.getParent())
                        .addChild(newNode);
            } else if (_edgeMap.containsKey(newObject)) {
            	EdgeModel newEdge = (EdgeModel) _edgeMap.get(newObject);
                newEdge.attachSource();
                newEdge.attachTarget();
            } else if (parentObject != newObject) {
                if (_nodeMap.containsKey(parentObject)) {
                    NodeModel newNode = (NodeModel) _nodeMap
                            .get(parentObject);
                    newNode._listeners.firePropertyChange("data", null, null);
                } else if (_edgeMap.containsKey(parentObject)) {
                    EdgeModel newEdge = (EdgeModel) _edgeMap
                            .get(parentObject);
                    newEdge._listeners.firePropertyChange("data", null, null);
                } else {
                    String name = newObject.eClass().getName();
                    if (_nodeNamesList.contains(name)) {
                        createNewNodeModel(newObject);
                    } else if (_edgeNamesList.contains(name)) {
                        createNewEdgeModel(newObject);
                    }
                }
                _editorModel._listeners.firePropertyChange("data", null, null);
            } else if (parentObject == newObject) {
                String name = newObject.eClass().getName();
                if (_nodeNamesList.contains(name)) {
                    createNewNodeModel(newObject);
                } else if (_edgeNamesList.contains(name)) {
                    createNewEdgeModel(newObject);
                }
            }
        }
        /**
         * Remove listenr from oldObject. remove corresponding visual
         * part from parent then fire the event to update the
         * visual parts.
         * @param oldObject EObject which is removed from List.
         * @param parentObject Container object for EObject.
         */
        private void removeEObject(EObject oldObject, EObject parentObject)
        {
            EcoreUtils.removeListener(oldObject, this, -1);
            if (_nodeMap.containsKey(oldObject)) {
                NodeModel oldNode = (NodeModel) _nodeMap.get(oldObject);
                ((ContainerNodeModel) oldNode.getParent())
                        .removeChild(oldNode);
                //_nodeMap.remove(oldObject);
            } else if (_edgeMap.containsKey(oldObject)) {
                EdgeModel newEdge = (EdgeModel) _edgeMap.get(oldObject);
                newEdge.detachSource();
                newEdge.detachTarget();
                setDirty(true);
                //_edgeMap.remove(oldObject);
            } else if (parentObject != oldObject) {
                if (_nodeMap.containsKey(parentObject)) {
                    NodeModel newNode = (NodeModel) _nodeMap
                            .get(parentObject);
                    newNode._listeners.firePropertyChange("data", null, null);
                } else if (_edgeMap.containsKey(parentObject)) {
                    EdgeModel newEdge = (EdgeModel) _edgeMap
                            .get(parentObject);
                    newEdge._listeners.firePropertyChange("data", null, null);
                }
                _editorModel._listeners.firePropertyChange("data", null, null);
            }
        }
  
        /**
         * Fire the event to update the visual part.
         * @param updateObject modified EObject
         * @param parentObject Container for updateObject
         * @param notification Notification Object
         */
        private void updateEObject(Notification notification,
                EObject updateObject, EObject parentObject)
        {
            if (_nodeMap.containsKey(updateObject)) {
                NodeModel updateNode = (NodeModel) _nodeMap
                        .get(updateObject);
                updateNode._listeners.firePropertyChange("data", null, null);
            } else if (_edgeMap.containsKey(updateObject)) {
                EdgeModel updateEdge = (EdgeModel) _edgeMap
                        .get(updateObject);
                updateEdge._listeners.firePropertyChange("data", null, null);
            } else if (parentObject != updateObject) {
                if (_nodeMap.containsKey(parentObject)) {
                    NodeModel newNode = (NodeModel) _nodeMap
                            .get(parentObject);
                    newNode._listeners.firePropertyChange("data", null, null);
                } else if (_edgeMap.containsKey(parentObject)) {
                    EdgeModel newEdge = (EdgeModel) _edgeMap
                            .get(parentObject);
                    newEdge._listeners.firePropertyChange("data", null, null);
                }
            }
            _editorModel._listeners.firePropertyChange("data", null, null);
        }


        /**
         * @param notification
         *            Notofication event
         */
        public void notifyChanged(Notification notification)
        {
        	// To accomodate schema changes Handler code needs to be modified/cleaned.
        	// This code is compatible with old/new schemas.
        	if (notification.isTouch()) {
        		return;
        	}
            switch (notification.getEventType()) {
            case Notification.SET:
                if (notification.getNotifier() instanceof EObject) {
                    if (!notification.wasSet()
                        && notification.getNewValue() instanceof EObject) {
                        EcoreUtils.addListener((EObject) notification
                                .getNewValue(), this, -1);
                    }
                    EObject updateObject = (EObject) notification.getNotifier();
                    EObject parentObject = getContainerObject(updateObject);
                    updateEObject(notification, updateObject, parentObject);
                }
                break;
            case Notification.ADD:
                if (notification.getNewValue() instanceof EObject) {
                    EObject newObject = (EObject) notification.getNewValue();
                    EObject parentObject1 = getContainerObject(newObject);
                    addEObject(newObject, parentObject1);
                }
                break;
            case Notification.REMOVE:
                if (notification.getOldValue() instanceof EObject) {
                    EObject oldObject = (EObject) notification.getOldValue();
                    EObject parentObject2 = oldObject;
                    if (notification.getNotifier() instanceof EObject) {
                        parentObject2 = getContainerObject
                        ((EObject) notification.getNotifier());
                    }
                    removeEObject(oldObject, parentObject2);
                }
                break;
            }
        }

        /**
         * This will return root container Object
         *
         * @param obj EObject
         * @return rootObject
         */
        private EObject getContainerObject(EObject obj)
        {
            EObject parentObject = obj;
			if (_nodeMap.containsKey(parentObject)
					|| _edgeMap.containsKey(parentObject))
				return parentObject;
			while (parentObject.eContainer() != null) {
				parentObject = parentObject.eContainer();
				if (_nodeMap.containsKey(parentObject)
						|| _edgeMap.containsKey(parentObject))
					return parentObject;
			}
			return parentObject;
        }
    }
    /**
	 * @see org.eclipse.ui.views.properties.IPropertySource#getEditableValue()
	 */
    public Object getEditableValue()
    {
        return null;
    }

    /**
     * @see org.eclipse.ui.views.properties.IPropertySource#isPropertySet
     * (java.lang.Object)
     */
    public boolean isPropertySet(Object id)
    {
        return false;
    }

    /**
     * @see org.eclipse.ui.views.properties.IPropertySource
     * #resetPropertyValue(java.lang.Object)
     */
    public void resetPropertyValue(Object id)
    {

    }
   /**
    * gets NodeModel from the map
    * @param eobjName - EObject Name
    * @return NodeModel corresponding to the EObject
    */
   public NodeModel getNodeModel(String eobjName)
   {
       Iterator iterator = _nodeMap.keySet().iterator();
       while (iterator.hasNext()) {
           EObject eobj = (EObject) iterator.next();
           String name = EcoreUtils.getName(eobj);
           if (name.equals(eobjName)) {
               return (NodeModel) _nodeMap.get(eobj);
           }
       }
       return null;
   }

   /**
    * gets EdgeModel from the map
    * @param eobj - Edge Object
    * @return EdgeModel corresponding to the EObject
    */
   public EdgeModel getEdgeModel(EObject eobj)
   {
       Iterator iterator = _edgeMap.keySet().iterator();
       while (iterator.hasNext()) {
           EObject edgeObj = (EObject) iterator.next();
           if (edgeObj.equals(eobj)) {
               return (EdgeModel) _edgeMap.get(edgeObj);
           }
       }
       return null;
   }
}

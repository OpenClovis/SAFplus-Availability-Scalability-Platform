/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor.actions;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.gef.commands.Command;
import org.eclipse.gef.commands.CompoundCommand;
import org.eclipse.gef.ui.actions.Clipboard;
import org.eclipse.gef.ui.actions.SelectionAction;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.IEditorPart;
import org.eclipse.ui.actions.ActionFactory;

import com.clovis.common.utils.constants.AnnotationConstants;
import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreCloneUtils;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.genericeditor.GenericEditor;
import com.clovis.cw.genericeditor.Messages;
import com.clovis.cw.genericeditor.commands.ConnectionCommand;
import com.clovis.cw.genericeditor.commands.CreateCommand;
import com.clovis.cw.genericeditor.model.ConnectionBendpoint;
import com.clovis.cw.genericeditor.model.ContainerNodeModel;
import com.clovis.cw.genericeditor.model.EdgeModel;
import com.clovis.cw.genericeditor.model.EditorModel;
import com.clovis.cw.genericeditor.model.NodeModel;

/**
 * @author pushparaj
 *
 * Class for Handling Paste action
 */
public class GEPasteAction extends SelectionAction
{

    private GenericEditor _editorPart;

    private Point         _pasteLocation;

    private Point         _mainLocation;

    private EditorModel   _editorModel;

    private Map           _copyMap = new HashMap();

    private List		  <Object>_selectedObjects;
    /**
     * constructor
     * @param editor Editor part
     */
    public GEPasteAction(IEditorPart editor)
    {
        super(editor);
        this._editorPart = (GenericEditor) editor;
    }

    /**
     * Calculates and returns the enabled state of this action.
     * @return <code>true</code> if the action is enabled
     */
    protected boolean calculateEnabled()
    {
        List cmd = (List) getClipboardContents();
         return (cmd != null && cmd.size() > 0
        		&& isValidSelection(cmd.get(0)));
    }
    /**
     * Check the selcted objects
     * @param obj Object
     * @return boolean
     */
    private boolean isValidSelection(Object obj)
    {
    	EObject eobj = null;
    	if (obj instanceof NodeModel) {
    		eobj = ((NodeModel) obj).getEObject();
    	} else if (obj instanceof EdgeModel) {
    		eobj = ((EdgeModel) obj).getEObject();
    	}
    	return (eobj != null && _editorPart.getCreationFactory(eobj.eClass()
				.getName()) != null);
    }
    /**
     * @see org.eclipse.gef.ui.actions.EditorPartAction#init()
     */
    protected void init()
    {
        setId(ActionFactory.PASTE.getId());
        setText(Messages.PASTEACTION_LABEL);
        setImageDescriptor(ImageDescriptor.createFromFile(getClass(),
		"icons/epaste_edit.gif"));
    }

    /**
     * Returns the contents of the default GEF {@link Clipboard}.
     *
     * @return the clipboard's contents
     */
    protected Object getClipboardContents()
    {
        return Clipboard.getDefault().getContents();
    }

    /**
     * Will return new location
     * @param obj Node Model
     * @return Point new Location
     */
    private Point getNewLocation(NodeModel obj)
    {
        if (obj == _selectedObjects.get(0)) {
            return _pasteLocation;
        } else if (obj.getParent() instanceof EditorModel) {
            return createNewLocation(obj);
        } else {
            return obj.getLocation();
        }
    }
    /**
     * Sets the first element's location. location
     * for other elements are calculated by using this
     * Location.
     */
    private void setMainLocation()
    {
    	NodeModel firstNode = null;
    	for (int i = 0; i < _selectedObjects.size(); i++) {
			if (_selectedObjects.get(i) instanceof NodeModel) {
				NodeModel node = (NodeModel) _selectedObjects.get(i);
				Point loc = node.getLocation();
				if(firstNode == null) {
					firstNode = node;
					_mainLocation = loc;
				}
				if (loc.y < _mainLocation.y) {
					_mainLocation = loc;
					firstNode = node;
				} else if (loc.y == _mainLocation.y) {
					if (loc.x < _mainLocation.x) {
						_mainLocation = loc;
						firstNode = node;
					}
				}
			}
		}
    	_selectedObjects.remove(firstNode);
    	_selectedObjects.add(0, firstNode);
    }
    /**
     * Creates and returns new location
     * @param obj NodeModel
     * @return new location for node
     */
    private Point createNewLocation(NodeModel obj)
    {
        Point point = obj.getLocation();
        int pasteX = _pasteLocation.x;
        int pasteY = _pasteLocation.y;
        int pointX = point.x;
        int pointY = point.y;
        int mainX = _mainLocation.x;
        int mainY = _mainLocation.y;
        if (pointX >= mainX) {
            pointX = pasteX + (pointX - mainX);
        } else if (pointX < mainX) {
            pointX = pasteX - (mainX - pointX);
        }

        if (pointY >= mainY) {
            pointY = pasteY + (pointY - mainY);
        } else if (pointY < mainY) {
            pointY = pasteY - (mainY - pointY);
        }
        return new Point(pointX, pointY);
    }

    /**
     * Sets the cursor location
     * @param loc cursor location
     */
    public void setPasteLocation(org.eclipse.swt.graphics.Point loc)
    {
        _pasteLocation = new Point(loc.x, loc.y);
    }
    
    /**
     * @see org.eclipse.jface.action.IAction#run()
     */
    public void run()
    {
    	EditorModel model = _editorPart.getEditorModel();
    	setEditorModel(model);
    	setPasteLocation(_editorPart.getCursorLocation());
        _selectedObjects = (List) getClipboardContents();
        setMainLocation();
        CompoundCommand compound = new CompoundCommand();
        List <NodeModel>nodesList = new ArrayList<NodeModel>();
        List <EdgeModel>edgesList = new ArrayList<EdgeModel>();
        for (int i = 0; i < _selectedObjects.size(); i++) {
            Object obj = _selectedObjects.get(i);
            if (obj instanceof NodeModel) {
                NodeModel childNode = createNode((NodeModel) obj);
                ContainerNodeModel parentNode = (ContainerNodeModel) _copyMap
                        .get(((NodeModel) obj).getParent());
                if (parentNode == null) {
                    parentNode = _editorModel;
                }
                compound.add(createNodeCommand(parentNode, childNode));
                nodesList.add(childNode);
            } else if (obj instanceof EdgeModel) {
                EdgeModel edge = (EdgeModel) obj;
                EdgeModel edgeModel = createEdge(edge);
                NodeModel source = (NodeModel) _copyMap.get(edge.getSource());
                NodeModel target = (NodeModel) _copyMap.get(edge.getTarget());
                compound.add(createEdgeCommand(edgeModel, source, target, edge
                        .getSourceTerminal(), edge.getTargetTerminal()));
                edgesList.add(edgeModel);
            }
        }
        execute(compound);
        for (int i = 0; i < nodesList.size(); i++) {
        	NodeModel node = nodesList.get(i);
        	node.appendSelection();
        }
    }

    /**
     * Set the EditorModel instance
     * @param model Editor Model
     */
    public void setEditorModel(Object model)
    {
        _editorModel = (EditorModel) model;
        _copyMap.put(model, model);
    }

    /**
     * Creates and returns new Node Model.
     * @param node NodeModel
     * @return new Node Model
     */
    private NodeModel createNode(NodeModel node)
    {
        NodeModel child = new NodeModel(node.getName());
        EObject eobj = EcoreCloneUtils.cloneEObject(node.getEObject());
        String nonClonnableFeatureNames = EcoreUtils.getAnnotationVal(eobj.eClass(),
                null, AnnotationConstants.NON_CLONEABLE_FEATURES);
        if (nonClonnableFeatureNames != null) {
            String [] nonClonnableFeatures = nonClonnableFeatureNames.split(",");
            for (int i = 0; i < nonClonnableFeatures.length; i++) {
                String nonClonnableFeature = nonClonnableFeatures[i];
                // using the nonClonnableFeature find the Structural Feature
                // which has the annotation to specify feature
                EStructuralFeature feature = eobj.eClass().
                    getEStructuralFeature(nonClonnableFeature);
                if (eobj.eIsSet(feature)) {
                    eobj.eUnset(feature);
                }
                  
            } 
         } 

        String nameField = EcoreUtils.getNameField(eobj.eClass());
		if (nameField != null) {
			eobj.eSet(eobj.eClass().getEStructuralFeature(nameField),
					getNextValue((String) EcoreUtils.getValue(eobj, nameField),
							_editorModel.getEList(), nameField));
		}

        String initializationInfo = EcoreUtils.getAnnotationVal(
        		eobj.eClass(), null, "initializationFields");
        List containerList = new ArrayList();

        List compList = ((EObject) _editorModel.getEList().get(0)).eContents();
        Iterator<EObject> itr = compList.iterator();
        while(itr.hasNext()) {
        	EObject obj = itr.next();
        	if(eobj.eClass().getName().equals(obj.eClass().getName())) {
        		containerList.add(obj);
        	}
        }

        if(initializationInfo != null) {
            EcoreUtils.initializeFields(eobj, containerList, initializationInfo);
        }

        child.setEObject(eobj);
        _editorModel.putNodeModel(eobj, child);
        child.setEPropertyValue(ModelConstants.RDN_FEATURE_NAME,
        		new Object().toString());
        child.setLocation(getNewLocation(node));
        child.setSize(node.getSize());
        _copyMap.put(node, child);
        return child;
    }
    /**
     * Creates and returns new EdgeModel
     * @param edge EdgeModel
     * @return new Edge Model
     */
    private EdgeModel createEdge(EdgeModel edge)
    {
        EdgeModel conn = new EdgeModel(edge.getName());
        EObject eobj = EcoreCloneUtils.cloneEObject(edge.getEObject());
        conn.setEObject(eobj);
        _editorModel.putEdgeModel(eobj, conn);
        conn.setEPropertyValue(ModelConstants.RDN_FEATURE_NAME,
        		new Object().toString());
        List bendpoints = edge.getBendpoints();
        for (int bp = 0; bp < bendpoints.size(); bp++) {
            ConnectionBendpoint bendpoint = new ConnectionBendpoint();
            ConnectionBendpoint connectionBendpoint
            = (ConnectionBendpoint) bendpoints.get(bp);
            bendpoint.setRelativeDimensions(connectionBendpoint
                    .getFirstRelativeDimension(), connectionBendpoint
                    .getSecondRelativeDimension());
            bendpoint.setWeight(connectionBendpoint.getWeight());
            conn.insertBendpoint(bp, bendpoint);
        }
        return conn;
    }

    /**
     * Creates and returns CreateCommand for Nodes
     * @param parentModel parent Node
     * @param childModel child Node
     * @return CreateCommand
     */
    private Command createNodeCommand(ContainerNodeModel parentModel,
            NodeModel childModel)
    {
        CreateCommand cmd = new CreateCommand();
        cmd.setParent(parentModel);
        cmd.setChild(childModel);
        cmd.setLocation(new Rectangle(childModel.getLocation(), childModel
                .getSize()));
        cmd.setLabel("Create");
        return cmd;
    }

    /**
     * Creates and return ConnectionCommand for Edges.
     * @param edge Connection Model
     * @param source source node
     * @param target target node
     * @param sourceterminal anchor point for source
     * @param targetterminal anchor  point for target
     * @return ConnectionCommand
     */
    private Command createEdgeCommand(EdgeModel edge, NodeModel source,
            NodeModel target, String sourceterminal, String targetterminal)
    {
        ConnectionCommand cmd = new ConnectionCommand();
        cmd.setConnectionModel(edge);
        cmd.setSource(source);
        cmd.setSourceTerminal(sourceterminal);
        cmd.setTarget(target);
        cmd.setTargetTerminal(targetterminal);
        cmd.setLabel("Create Connection");
        return cmd;
    }
    /**
     * Gets next value for the feature. The next value is always unique
     * @param prefix Prefix String before unique number
     * @param list   List of EObjects
     * @param featureName feature name
     * @return Unique value generated
     */
     public String getNextValue(String prefix, List editorList, String featureName) {
		HashSet set = new HashSet();
		String newVal = null;
		EObject rootObject = (EObject) editorList.get(0);
		List refList = rootObject.eClass().getEAllReferences();
		for (int j = 0; j < refList.size(); j++) {
			List list = (EList) rootObject.eGet((EReference) refList.get(j));
			for (int i = 0; i < list.size(); i++) {
				EObject eobj = (EObject) list.get(i);
				Object val = EcoreUtils.getValue(eobj, featureName);
				if (val instanceof Integer || val instanceof Short
						|| val instanceof Long) {
					set.add(String.valueOf(val));
				} else if(val != null){
					set.add((String) val);
				}
			}
		}
		int size = 1;
		for (int i = 0; i <= size; i++) {
			newVal = prefix + "_" + String.valueOf(i);
			if (!set.contains(newVal)) {
				break;
			} else {
				size = size + 1;
			}
		}
		return newVal;
	}
}

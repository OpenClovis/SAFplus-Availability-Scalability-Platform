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

import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.eclipse.emf.ecore.EAnnotation;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.gef.ConnectionEditPart;
import org.eclipse.gef.ui.actions.SelectionAction;
import org.eclipse.ui.IWorkbenchPart;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreCloneUtils;

import com.clovis.cw.genericeditor.GenericEditor;
import com.clovis.cw.genericeditor.model.ContainerNodeModel;
import com.clovis.cw.genericeditor.model.EdgeModel;
import com.clovis.cw.genericeditor.model.EditorModel;
import com.clovis.cw.genericeditor.model.NodeModel;
import com.clovis.cw.genericeditor.model.ConnectionBendpoint;
import com.clovis.cw.genericeditor.editparts.AbstractContainerNodeEditPart;
import com.clovis.cw.genericeditor.editparts.BaseEditPart;
import com.clovis.cw.genericeditor.editparts.BaseDiagramEditPart;
/**
 * @author pushparaj
 *
 * This class have common functionalities for both
 * Copy and Cut
 */
public abstract class AbstractCopyAction extends SelectionAction
{
    protected GenericEditor _editorPart;
    protected List _selectedObjects;
    protected EditorModel _editorModel;
    protected Map _copyMap = new HashMap();


    /**
     * @param part Editor instance.
     */
    public AbstractCopyAction(IWorkbenchPart part)
    {
        super(part);
        this._editorPart = (GenericEditor) part;
    }

    /**
     * Calculates and returns the enabled state of this action.
     * @return <code>true</code> if the action is enabled
     */
    protected boolean calculateEnabled()
    {
        _selectedObjects = getSelectedObjects();
        /*if(!isValidSelection()) {
        	return false;
        } else if (_selectedObjects.size() > 0
        	&& (_selectedObjects.get(0) instanceof BaseDiagramEditPart)) {
            _selectedObjects = null;
            return false;
        }*/
        if (_selectedObjects.size() == 0 || (_selectedObjects.get(0) instanceof BaseDiagramEditPart)) {
			_selectedObjects = null;
			return false;
		} else if (!isValidSelection()) {
			return false;
		}
        return true;
    }
    /**
	 * Checks whether the selections is valid.
	 * 
	 * @return boolean
	 */
    private boolean isValidSelection() {
		for (int i = 0; i < _selectedObjects.size(); i++) {
			if (_selectedObjects.get(i) instanceof BaseEditPart) {
				if (!canCopy((BaseEditPart) _selectedObjects.get(i))) {
					return false;
				}
			}
		}
		return true;
	}
    /**
	 * Checks whether EObject is deletable
	 * 
	 * @param editPart
	 *            EditPart which contains EObject
	 * @return boolean
	 */
    private boolean canCopy(BaseEditPart editPart) {
		NodeModel nodeModel = (NodeModel) editPart.getModel();
		EObject eobj = nodeModel.getEObject();
		EAnnotation ann = eobj.eClass().getEAnnotation("CWAnnotation");
		if (ann != null) {
			if (ann.getDetails().get("LowerBound") != null
					&& ann.getDetails().get("LowerBound").equals("1")
					&& ann.getDetails().get("UpperBound") != null
					&& ann.getDetails().get("UpperBound").equals("1")) {
				return false;
			}
		}
		return true;
	} 
    /**
	 * Set the EditorModel instance
	 * 
	 * @param model
	 *            EditorModel instance
	 */
    protected void setEditorModel(Object model)
    {
        _editorModel = (EditorModel) model;
        _copyMap.put(model, model);
    }

    /**
     * Create cloned connection objects and added into
     * List. This list will be maintained in Cipboard.
     * @param parts selected objects
     * @param copyList clonedList
     */
    protected void copyEdges(List parts, List copyList)
    {
        for (int i = 0; i < parts.size(); i++) {
        	if (parts.get(i) instanceof BaseEditPart) {
            BaseEditPart part = (BaseEditPart) parts.get(i);
            List cons = part.getTargetConnections();
            for (int j = 0; j < cons.size(); j++) {
                ConnectionEditPart connPart = (ConnectionEditPart) cons.get(j);
                if (_selectedObjects.contains(connPart.getSource())) {
                    EdgeModel edge = (EdgeModel) connPart.getModel();
                    EdgeModel edgeModel = createEdge(connPart);
                    edgeModel.setSource((NodeModel) _copyMap.get(edge.
                            getSource()));
                    edgeModel.setTarget((NodeModel) _copyMap.get(edge.
                            getTarget()));
                    edgeModel.setSourceTerminal(edge.getSourceTerminal());
                    edgeModel.setTargetTerminal(edge.getTargetTerminal());
                    copyList.add(edgeModel);
                }
            }
            copyEdges(part.getChildren(), copyList);
        	}
        }
    }

    /**
     * Creates and returns Cloned object for NodeModel.
     * @param parentPart parent EditPart
     * @param childPart child EditPart
     * @return NodeModel - cloned Object
     */
    protected NodeModel createNode(BaseEditPart parentPart,
            BaseEditPart childPart)
    {
        NodeModel node = (NodeModel) childPart.getModel();
        NodeModel child = null;
        if(childPart instanceof AbstractContainerNodeEditPart) {
        	child = new ContainerNodeModel(node.getName());
        } else {
        	child = new NodeModel(node.getName());
        }
        child.setEObject(EcoreCloneUtils.cloneEObject(node.getEObject()));
        child.setEPropertyValue(ModelConstants.RDN_FEATURE_NAME,
        		new Object().toString());
        child.setLocation(node.getLocation());
        child.setSize(node.getSize());
        _copyMap.put(childPart.getModel(), child);
        return child;
    }

    /**
     * Creates and returns Cloned object for Edge Model.
     * @param connPart connection EditPart
     * @return EdgeModel - cloned Object
     */
    protected EdgeModel createEdge(ConnectionEditPart connPart)
    {
        EdgeModel edge = (EdgeModel) connPart.getModel();
        EdgeModel conn = new EdgeModel(edge.getName());
        conn.setEObject(EcoreCloneUtils.cloneEObject(edge.getEObject()));
        conn.setEPropertyValue(ModelConstants.RDN_FEATURE_NAME,
        		new Object().toString());
        List bendpoints = edge.getBendpoints();
        for (int bp = 0; bp < bendpoints.size(); bp++) {
            ConnectionBendpoint bendpoint = new ConnectionBendpoint();
            ConnectionBendpoint connectionBendpoint =
                (ConnectionBendpoint) bendpoints.get(bp);
            bendpoint.setRelativeDimensions(connectionBendpoint.
                    getFirstRelativeDimension(), connectionBendpoint.
                    getSecondRelativeDimension());
            bendpoint.setWeight(connectionBendpoint.getWeight());
            conn.insertBendpoint(bp, bendpoint);
        }
        return conn;
    }
}

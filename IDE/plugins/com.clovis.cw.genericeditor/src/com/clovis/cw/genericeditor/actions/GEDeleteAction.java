/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/actions/GEDeleteAction.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor.actions;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.gef.EditPart;
import org.eclipse.gef.commands.Command;
import org.eclipse.gef.ui.actions.DeleteAction;
import org.eclipse.ui.IWorkbenchPart;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.genericeditor.editparts.AbstractEdgeEditPart;
import com.clovis.cw.genericeditor.editparts.BaseDiagramEditPart;
import com.clovis.cw.genericeditor.editparts.BaseEditPart;
import com.clovis.cw.genericeditor.model.ContainerNodeModel;
import com.clovis.cw.genericeditor.model.EdgeModel;
import com.clovis.cw.genericeditor.model.NodeModel;

/**
 * @author pushparaj
 * 
 * Class for handling Delete Action
 */
public class GEDeleteAction extends DeleteAction{

	public GEDeleteAction(IWorkbenchPart part) {
		super(part);
	}

	/**
	 * (non-Javadoc)
	 * @see org.eclipse.gef.ui.actions.WorkbenchPartAction#calculateEnabled()
	 */
	protected boolean calculateEnabled() {
		List objects = getSelectedObjects();
		if (objects.size() == 0
				|| objects.get(0) instanceof BaseDiagramEditPart
				|| !isValidSelection(objects)) {
			return false;
		}
		Command cmd = createDeleteCommand(objects);
		if (cmd == null)
			return false;
		return cmd.canExecute();
	}
	/**
     * Checks whether the selections is valid.
     * @return boolean
     */
    private boolean isValidSelection(List selectedObjects) {
		for (int i = 0; i < selectedObjects.size(); i++) {
			if (selectedObjects.get(i) instanceof BaseEditPart) {
				if (!canDelete((BaseEditPart) selectedObjects.get(i)))
					return false;
			} else if (selectedObjects.get(i) instanceof AbstractEdgeEditPart) {
				if (!canDelete((AbstractEdgeEditPart) selectedObjects.get(i)))
					return false;
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
    private boolean canDelete(BaseEditPart editPart)
    {
    	NodeModel nodeModel = (NodeModel) editPart.getModel();
    	EObject eobj = nodeModel.getEObject();
    	String delete = EcoreUtils.getAnnotationVal(eobj.eClass(), null, "canDelete");
    	if(delete != null && delete.equals("false")) {
    		return false;
    	}
    	String lower = EcoreUtils.getAnnotationVal(eobj.eClass(), null, "LowerBound");
    	String upper = EcoreUtils.getAnnotationVal(eobj.eClass(), null, "UpperBound");
    	if(lower != null && upper != null && lower.equals("1") && upper.equals("1"))
    		return false;
    	return true;
    }
    /**
     * Checks whether EObject is deletable
     * @param editPart EditPart which contains EObject
     * @return boolean
     */
    private boolean canDelete(AbstractEdgeEditPart editPart)
    {
    	EdgeModel edgeModel = (EdgeModel) editPart.getModel();
    	EObject eobj = edgeModel.getTarget().getEObject();
    	String delete = EcoreUtils.getAnnotationVal(eobj.eClass(), null, "canDelete");
    	if(delete != null && delete.equals("false")) {
    		return false;
    	}
    	
    	return true;
    }
	/**
	 * (non-Javadoc)
	 * @see org.eclipse.jface.action.IAction#run()
	 */
	public void run() {
		List nodesConnectionsList = new ArrayList();
		//List parentObjects = new ArrayList();
		List objs = new ArrayList();
		List selectedObjs = getSelectedObjects();
		List parentsToBeRemoved = new ArrayList();
		for (int i = 0; i < selectedObjs.size(); i++) {
			if((selectedObjs.get(i) instanceof BaseEditPart)) {
				BaseEditPart editpart = (BaseEditPart) selectedObjs.get(i);
				if(!(editpart.getParent() instanceof BaseDiagramEditPart)) {
					ContainerNodeModel nodeModel = (ContainerNodeModel) editpart.getParent().getModel();
					EObject nodeObj = nodeModel.getEObject();
					String value = EcoreUtils.getAnnotationVal(nodeObj.eClass(), null, "removeIfNoChild");
					if(value != null && value.equals("true")) {
						parentsToBeRemoved.add(editpart.getParent());
					}
				}
			}
		}
		objs.addAll(getSelectedObjects());
		for(int i = 0; i < parentsToBeRemoved.size(); i++) {
			EditPart parentPart = (EditPart) parentsToBeRemoved.get(i);
			if(objs.containsAll(parentPart.getChildren())) {
				objs.removeAll(parentPart.getChildren());
				objs.add(parentPart);
			}
		}
		for (int i = 0; i < objs.size(); i++) {
			if((objs.get(i) instanceof BaseEditPart)) {
				BaseEditPart editpart = (BaseEditPart) objs.get(i);
				/*AbstractContainerNodeEditPart parent = (AbstractContainerNodeEditPart) editpart.getParent();
				if(parent.removeWhenEmpty()) {
					parentObjects.add(parent);
				}*/
				nodesConnectionsList.addAll(editpart.getSourceConnections());
				nodesConnectionsList.addAll(editpart.getTargetConnections());
			}
		}
		
		for (int i = 0; i < nodesConnectionsList.size(); i++) {
			objs.remove(nodesConnectionsList.get(i));
		}
		/*for (int i = 0; i < parentObjects.size(); i++) {
			AbstractContainerNodeEditPart parent = (AbstractContainerNodeEditPart) parentObjects.get(i);
			if(objs.containsAll(parent.getChildren())) {
				objs.add(parent);
			}
		}*/
		execute(createDeleteCommand(objs));
	}
}

/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/AssociateResourceAction.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.dialog;


import java.util.List;

import org.eclipse.core.resources.IContainer;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.IActionClassAdapter;
import com.clovis.cw.editor.ca.ResourceDataUtils;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.editor.ca.editpart.ComponentEditPart;
import com.clovis.cw.genericeditor.model.NodeModel;
import com.clovis.cw.project.data.ProjectDataModel;
/**
 * @author pushparaj
 *
 * Action class for Associate Resources for Component
 */
public class AssociateResourceAction extends IActionClassAdapter
{
    IResource _resource;
    NodeModel _nodeModel;
    /**
     * Visible only for Component.
     * @param environment Environment
     * @return true for Component, false otherwise.
     */
    public boolean isVisible(Environment environment)
    {
        _resource = (IResource) environment.getValue("project");
        EObject selObj = null;
        StructuredSelection selection =
            (StructuredSelection) environment.getValue("selection");
        if (selection.size() == 1) {
                if((selection.getFirstElement() 
                        instanceof ComponentEditPart)) {
                    ComponentEditPart cep = (ComponentEditPart) selection.
                    getFirstElement();
                    _nodeModel = (NodeModel) cep.getModel(); 
                    selObj = _nodeModel.getEObject();
                    String property = EcoreUtils.getValue(selObj,ComponentEditorConstants.
                            COMPONENT_PROPERTY).toString();
                    boolean is3rdParty = false;
                    if(EcoreUtils.getValue(selObj,"is3rdpartyComponent") != null) {
                    	is3rdParty =  Boolean.parseBoolean(EcoreUtils.getValue(selObj,"is3rdpartyComponent").toString());
                    }
                    if (!property.equals(ComponentEditorConstants.PROXIED_PREINSTANTIABLE)
                            && !property.equals(ComponentEditorConstants.
                                    PROXIED_NON_PREINSTANTIABLE) && !is3rdParty) {
                        return true;
                    } else {
                        return false;
                    }
                } else {
                    return false;
                }
            
        }
        return false;
    }
    /**
     * Disable for SNMP Sub Agent.
     * @param environment Environment
     * @return false for Sub Agent, false otherwise.
     */
    public boolean isEnabled(Environment environment)
    {
        _resource = (IResource) environment.getValue("project");
        EObject selObj = null;
        StructuredSelection selection =
            (StructuredSelection) environment.getValue("selection");
        if (selection.size() == 1) {
                if((selection.getFirstElement() 
                        instanceof ComponentEditPart)) {
                    ComponentEditPart cep = (ComponentEditPart) selection.
                    getFirstElement();
                    _nodeModel = (NodeModel) cep.getModel(); 
                    selObj = _nodeModel.getEObject();
                    boolean isSubAgent = false;
                    if(EcoreUtils.getValue(selObj,"isSNMPSubAgent") != null) {
                    	isSubAgent =  Boolean.parseBoolean(EcoreUtils.getValue(selObj,"isSNMPSubAgent").toString());
                    }
                    if (isSubAgent) {
                        return false;
                    } else {
                        return true;
                    }
                } else {
                    return true;
                }
            
        }
        return true;
    }
    /**
     * Handling action
     * @param args 0 - EObject for Component from Selection
     * @return true if action is successfull else false.
     */
    public boolean run(Object[] args)
    {
        ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(
                (IContainer) _resource);
        List resList = pdm.getCAModel().getEList();
        StructuredSelection sel = (StructuredSelection) args[1];
        Object part = sel.getFirstElement();
        EObject eObj = null;
        if (part instanceof ComponentEditPart) {
            ComponentEditPart cep = (ComponentEditPart) sel.getFirstElement();
            eObj = ((NodeModel) cep.getModel()).getEObject();
            resList = (EList) ResourceDataUtils.getResourcesList(resList);
        } /*else {
            NodeEditPart nep = (NodeEditPart) sel.getFirstElement();
            eObj = ((NodeModel) nep.getModel()).getEObject();
            resList = getNodesList(resList);
        }*/
        /*EList feature = (EList) eObj.eGet(eObj
                .eClass().getEStructuralFeature("associatedResource"));*/
        AssociateResourcesDialog dialog = new AssociateResourcesDialog(
                (Shell) args[0], (IProject) _resource, (EList) resList, eObj);
        if(dialog.open() == Dialog.OK)
        	_nodeModel.refreshUI();
        return true;
    }
}

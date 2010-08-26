package com.clovis.cw.editor.ca.action;

import java.util.Iterator;
import java.util.List;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.events.SelectionEvent;

import com.clovis.common.utils.constants.AnnotationConstants;
import com.clovis.common.utils.ecore.EcoreCloneUtils;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.editor.ca.constants.SafConstants;
import com.clovis.cw.editor.ca.dialog.GenericFormPage;
import com.clovis.cw.editor.ca.dialog.NodeProfileDialog;
import com.clovis.cw.editor.ca.dialog.PreferenceUtils;
import com.clovis.cw.editor.ca.dialog.PreferenceUtils.PreferenceSelectionData;
import com.clovis.cw.editor.ca.validator.AMFListUniquenessValidator;

/**
 * 
 * @author shubhada
 * 
 * Default Action class for PreferenceTree actions 
 *
 */
public class AMFNodeTreeAction extends TreeAction
{
	/**
	 * Constructor
	 * 
	 * @param treeViewer  - TreeViewer with which action is associated
     * @param dialog - Preference Dialog of TreeViewer
     * @param ebj - PreferenceTree rootObj 
	 */
	public AMFNodeTreeAction(TreeViewer treeViewer, Dialog dialog, EObject eobj)
	{
		super(treeViewer, dialog, eobj);
		
	}
	/**
	 * 
	 * @param treeViewer  - TreeViewer with which action is associated
     * @param dialog - Preference Dialog of TreeViewer
     * @param ebj - PreferenceTree rootObj 
	 * @return the TreeAction instance
	 */
	public static TreeAction createAction(TreeViewer treeViewer, Dialog dialog, EObject eobj)
	{
		return new AMFNodeTreeAction(treeViewer, dialog, eobj);
	}
	/**
	 * Called when duplicate menu item is selected from the
	 * popup menu.
	 * @param e SelectionEvent
	 */
	public void duplicateSelected(SelectionEvent e) {
		if(!PreferenceUtils.isSelectionAvailable(_treeViewer)) {
			return;
		}
		PreferenceSelectionData psd = PreferenceUtils
			.getCurrentSelectionData(_treeViewer);
		EObject nodeObj = psd.getEObject();
		List nodeList = PreferenceUtils.getContainerEList(nodeObj);
		List amfList = NodeProfileDialog.getInstance().getViewModel().getEList();
		List amfConfigObjList = AMFListUniquenessValidator.getAllAMFObjects(amfList);

        EObject duplicateObj = EcoreCloneUtils.cloneEObject(nodeObj);
        nodeList.add(duplicateObj);
        if(! PreferenceUtils.initializeEObject(duplicateObj, amfConfigObjList, _dialog)) {
        	return;
        }
        amfConfigObjList.add(duplicateObj);
        if (duplicateObj.eClass().getName().equals(
        		SafConstants.NODE_INSTANCELIST_ECLASS)) {
        	initializeNode(duplicateObj, amfConfigObjList);
        } else if (duplicateObj.eClass().getName().equals(
        		SafConstants.SERVICEUNIT_INSTANCELIST_ECLASS)) {
        	initializeSU(duplicateObj, amfConfigObjList);
        } else if (duplicateObj.eClass().getName().equals(
        		SafConstants.SG_INSTANCELIST_ECLASS)) {
        	initializeSG(duplicateObj, amfConfigObjList);
        } else if (duplicateObj.eClass().getName().equals(
        		SafConstants.SI_INSTANCELIST_ECLASS)) {
        	initializeSI(duplicateObj, amfConfigObjList);
        }
        

		PreferenceNode parentPrefNode = psd.getParentPrefNode();
		PreferenceNode duplicatePrefNode = PreferenceUtils
			.createAndAddPerferenceNode(parentPrefNode, duplicateObj);
		PreferenceUtils.createChildTree(duplicatePrefNode, duplicateObj);
        PreferenceUtils.setTreeViewerSelection(_treeViewer, duplicatePrefNode);
		PreferenceUtils.performNotification(duplicateObj);
	}
	
	/**
	 * Called when menu item is selected from the new sub menu of
	 * popup menu.
	 * @param e SelectionEvent
	 */
	public void newSelected(SelectionEvent e) {
		if(!PreferenceUtils.isSelectionAvailable(_treeViewer)) {
			return;
		}
		PreferenceSelectionData psd = PreferenceUtils
			.getCurrentSelectionData(_treeViewer);
		PreferenceNode parentPrefNode;
		PreferencePage currentPrefPage = psd.getPrefPage();
		
		EClass nodeClass = null;
		EList nodeList = null;

		if(!(currentPrefPage instanceof GenericFormPage)) {
			String selectedItemText = psd.getTreeItem().getText();
        	EObject eObj = (psd.getEObject() != null) ? psd.getEObject() : _rootObjet;
        	EList list = eObj.eClass().getEAllStructuralFeatures();

        	Iterator itr = list.iterator();
			while(itr.hasNext()) {
				EStructuralFeature feature = (EStructuralFeature) itr.next();
				if(PreferenceUtils.isTreeListNode(feature)) {
            		Object obj = eObj.eGet(feature);
            		EStructuralFeature treeNodeFeature;

            		String label = EcoreUtils.getAnnotationVal(feature,
							null, AnnotationConstants.TREE_LABEL);

            		if(label.equalsIgnoreCase(selectedItemText)) {
						if(obj instanceof List) {
	            			treeNodeFeature = feature;
							nodeList = (EList) obj;
						} else {
	            			treeNodeFeature = (EStructuralFeature) 
            				((EObject)obj).eClass().getEAllReferences().get(0);
							nodeList = (EList)EcoreUtils.getValue((EObject) obj,
								treeNodeFeature.getName());
						}
						nodeClass = (EClass) treeNodeFeature.getEType();
						break;
					}
				}
			}
			parentPrefNode = psd.getPrefNode();
		} else {
			EObject eObj = psd.getEObject();
			nodeClass = eObj.eClass();
			nodeList = PreferenceUtils.getContainerEList(eObj);
			parentPrefNode = psd.getParentPrefNode();
		}
		
		if(nodeClass == null || nodeList == null) {
			return;
		}
		EObject nodeObj  = EcoreUtils.createEObject(nodeClass, true);
		List amfList = NodeProfileDialog.getInstance().getViewModel().getEList();
		List amfConfigObjList = AMFListUniquenessValidator.getAllAMFObjects(amfList);
		nodeList.add(nodeObj);
		if(! PreferenceUtils.initializeEObject(nodeObj, amfConfigObjList, _dialog)) {
			return;
		}
		PreferenceNode nodePrefNode = PreferenceUtils.
			createAndAddPerferenceNode(parentPrefNode, nodeObj);
		PreferenceUtils.createChildTree(nodePrefNode, nodeObj);
		PreferenceUtils.setTreeViewerSelection(_treeViewer,
			parentPrefNode, nodePrefNode);
		PreferenceUtils.performNotification(nodeObj);
	}
	
	private void initializeNode(EObject nodeObj, List amfConfigObjList)
	{
		EReference serviceUnitInstsRef = (EReference) nodeObj.eClass()
		.getEStructuralFeature(SafConstants.SERVICEUNIT_INSTANCES_NAME);
		EObject serviceUnitInstsObj = (EObject) nodeObj
			.eGet(serviceUnitInstsRef);
		if (serviceUnitInstsObj != null) {
			EReference serviceUnitInstRef = (EReference) serviceUnitInstsObj
				.eClass().getEStructuralFeature(
						SafConstants.SERVICEUNIT_INSTANCELIST_NAME);
			List suObjs = (List) serviceUnitInstsObj
				.eGet(serviceUnitInstRef);
	
			for (int j = 0; j < suObjs.size(); j++) {
				EObject su = (EObject) suObjs.get(j);
				if(! PreferenceUtils.initializeEObject(su, amfConfigObjList, _dialog)) {
		        	return;
		        }
				amfConfigObjList.add(su);
				EReference componentInstsRef = (EReference) su.eClass()
						.getEStructuralFeature(
								SafConstants.COMPONENT_INSTANCES_NAME);
				EObject componentInstsObj = (EObject) su
						.eGet(componentInstsRef);
				if (componentInstsObj != null) {
					EReference componentInstRef = (EReference) componentInstsObj
							.eClass()
							.getEStructuralFeature(
									SafConstants.COMPONENT_INSTANCELIST_NAME);
					List compObjs = (List) componentInstsObj
							.eGet(componentInstRef);
					for (int k = 0; k < compObjs.size(); k++) {
						EObject comp = (EObject) compObjs.get(k);
						if(! PreferenceUtils.initializeEObject(comp, amfConfigObjList, _dialog)) {
				        	return;
				        }
						amfConfigObjList.add(comp);
					}
				}
			}
		}
	}

	private void initializeSU(EObject su, List amfConfigObjList)
	{
		if(! PreferenceUtils.initializeEObject(su, amfConfigObjList, _dialog)) {
        	return;
        }
		EReference componentInstsRef = (EReference) su.eClass()
				.getEStructuralFeature(
						SafConstants.COMPONENT_INSTANCES_NAME);
		EObject componentInstsObj = (EObject) su
				.eGet(componentInstsRef);
		if (componentInstsObj != null) {
			EReference componentInstRef = (EReference) componentInstsObj
					.eClass()
					.getEStructuralFeature(
							SafConstants.COMPONENT_INSTANCELIST_NAME);
			List compObjs = (List) componentInstsObj
					.eGet(componentInstRef);
			for (int k = 0; k < compObjs.size(); k++) {
				EObject comp = (EObject) compObjs.get(k);
				if(! PreferenceUtils.initializeEObject(comp, amfConfigObjList, _dialog)) {
		        	return;
		        }
				amfConfigObjList.add(comp);
			}
		}
	}
	private void initializeSG(EObject sg, List amfConfigObjList)
	{
		EReference siInstsRef = (EReference) sg.eClass()
		.getEStructuralFeature(
				SafConstants.SERVICE_INSTANCES_NAME);
		EObject serviceInstanceInstsObj = (EObject) sg.eGet(siInstsRef);
		
		if (serviceInstanceInstsObj != null) {
			EReference siInstRef = (EReference) serviceInstanceInstsObj
					.eClass().getEStructuralFeature(
							SafConstants.SERVICE_INSTANCELIST_NAME);
			List siObjs = (List) serviceInstanceInstsObj
					.eGet(siInstRef);
		
			for (int j = 0; j < siObjs.size(); j++) {
				EObject si = (EObject) siObjs.get(j);
				if(! PreferenceUtils.initializeEObject(si, amfConfigObjList, _dialog)) {
		        	return;
		        }
				amfConfigObjList.add(si);
				EReference csiInstsRef = (EReference) si.eClass()
						.getEStructuralFeature(
								SafConstants.CSI_INSTANCES_NAME);
				EObject csiInstsObj = (EObject) si.eGet(csiInstsRef);
		
				if (csiInstsObj != null) {
		
					EReference csiInstRef = (EReference) csiInstsObj
							.eClass().getEStructuralFeature(
									SafConstants.CSI_INSTANCELIST_NAME);
					List csiObjs = (List) csiInstsObj.eGet(csiInstRef);
					for (int k = 0; k < csiObjs.size(); k++) {
						EObject csi = (EObject) csiObjs.get(k);
						if(! PreferenceUtils.initializeEObject(csi, amfConfigObjList, _dialog)) {
				        	return;
				        }
						amfConfigObjList.add(csi);
					}
				}
			}
		}
	}
	private void initializeSI(EObject si, List amfConfigObjList)
	{
		if(! PreferenceUtils.initializeEObject(si, amfConfigObjList, _dialog)) {
        	return;
        }
		EReference csiInstsRef = (EReference) si.eClass()
				.getEStructuralFeature(
						SafConstants.CSI_INSTANCES_NAME);
		EObject csiInstsObj = (EObject) si.eGet(csiInstsRef);

		if (csiInstsObj != null) {

			EReference csiInstRef = (EReference) csiInstsObj
					.eClass().getEStructuralFeature(
							SafConstants.CSI_INSTANCELIST_NAME);
			List csiObjs = (List) csiInstsObj.eGet(csiInstRef);
			for (int k = 0; k < csiObjs.size(); k++) {
				EObject csi = (EObject) csiObjs.get(k);
				if(! PreferenceUtils.initializeEObject(csi, amfConfigObjList, _dialog)) {
		        	return;
		        }
				amfConfigObjList.add(csi);
			}
		}
	}
}

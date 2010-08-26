package com.clovis.cw.editor.ca.action;

import java.util.Iterator;
import java.util.List;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.widgets.TreeItem;

import com.clovis.common.utils.constants.AnnotationConstants;
import com.clovis.common.utils.ecore.EcoreCloneUtils;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.editor.ca.dialog.GenericFormPage;
import com.clovis.cw.editor.ca.dialog.PreferenceUtils;
import com.clovis.cw.editor.ca.dialog.PreferenceUtils.PreferenceSelectionData;

/**
 * 
 * @author shubhada
 * 
 * Default Action class for PreferenceTree actions 
 *
 */
public class TreeAction
{
	protected Dialog _dialog = null;
	protected EObject _rootObjet = null;
	protected TreeViewer _treeViewer = null;
	
	/**
	 * 
	 * @param treeViewer - TreeViewer associated with the action
	 * @param dialog
	 * @param eobj
	 */
	public TreeAction(TreeViewer treeViewer, Dialog dialog, EObject eobj)
	{
		init(treeViewer, dialog, eobj);
	}

	/**
	 * Initializes the fields of the action.
	 * @param treeViewer
	 * @param dialog
	 * @param eobj
	 */
	public void init(TreeViewer treeViewer, Dialog dialog, EObject eobj) {
		_treeViewer = treeViewer;
		_dialog = dialog;
		_rootObjet = eobj;
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
		EList nodeList = PreferenceUtils.getContainerEList(nodeObj);

        EObject duplicateObj = EcoreCloneUtils.cloneEObject(nodeObj);
        if(! PreferenceUtils.initializeEObject(duplicateObj, nodeList, _dialog)) {
        	return;
        }
        nodeList.add(duplicateObj);

		PreferenceNode parentPrefNode = psd.getParentPrefNode();
		PreferenceNode duplicatePrefNode = PreferenceUtils
			.createAndAddPerferenceNode(parentPrefNode, duplicateObj);
		PreferenceUtils.createChildTree(duplicatePrefNode, duplicateObj);
        PreferenceUtils.setTreeViewerSelection(_treeViewer, duplicatePrefNode);
		PreferenceUtils.performNotification(duplicateObj);
	}

	/**
	 * Called when delete menu item is selected from the
	 * popup menu.
	 * @param e SelectionEvent
	 */
	public void deleteSelected(SelectionEvent e) {
		if(!PreferenceUtils.isSelectionAvailable(_treeViewer)) {
			return;
		}
		PreferenceSelectionData psd = PreferenceUtils
			.getCurrentSelectionData(_treeViewer);
		PreferenceNode node = psd.getPrefNode();
		TreeItem parentTreeItem = psd.getParentTreeItem();
		
		EObject nodeObj = psd.getEObject();
		EList nodeList = PreferenceUtils.getContainerEList(nodeObj);
		nodeList.remove(nodeObj);
		
		if(parentTreeItem != null) {
			PreferenceNode parentNode = psd.getParentPrefNode();
			parentNode.remove(node);
			if (((PreferencePage) ((PreferenceDialog) _dialog)
					.getSelectedPage()).isValid() == false) {
				((PreferencePage) ((PreferenceDialog) _dialog).getSelectedPage()).setValid(true);
			}
			PreferenceUtils.setTreeViewerSelection(_treeViewer, parentNode);
		}
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
		if(! PreferenceUtils.initializeEObject(nodeObj, nodeList, _dialog)) {
			return;
		}
		nodeList.add(nodeObj);
		PreferenceNode nodePrefNode = PreferenceUtils.
			createAndAddPerferenceNode(parentPrefNode, nodeObj);
		PreferenceUtils.createChildTree(nodePrefNode, nodeObj);
		PreferenceUtils.setTreeViewerSelection(_treeViewer,
			parentPrefNode, nodePrefNode);
		PreferenceUtils.performNotification(nodeObj);
	}

}

package com.clovis.cw.editor.ca.manageability.ui;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import org.eclipse.gef.EditPart;
import org.eclipse.gef.RequestConstants;
import org.eclipse.gef.commands.Command;
import org.eclipse.gef.commands.CompoundCommand;
import org.eclipse.gef.internal.GEFMessages;
import org.eclipse.gef.requests.GroupRequest;
import org.eclipse.swt.custom.BusyIndicator;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.TreeItem;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PlatformUI;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.editor.ca.editpart.MibEditPart;
import com.clovis.cw.editor.ca.manageability.common.LoadMibComposite;
import com.clovis.cw.editor.ca.manageability.common.ResourceTreeNode;
import com.clovis.cw.genericeditor.GenericEditor;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.genericeditor.model.ContainerNodeModel;
import com.clovis.cw.project.data.ProjectDataModel;
import com.ireasoning.util.MibTreeNode;

/**
 * Control which have Load Mib and Deassocite buttons
 * @author Pushparaj
 *
 */
public class ManageabilityLoadMibComposite extends LoadMibComposite {
	ManageabilityEditor _editor;
	public ManageabilityLoadMibComposite(Composite parent, int style,
			ArrayList<String> loadedMibs, ProjectDataModel pdm, ManageabilityEditor editor) {
		super(parent, style, loadedMibs, pdm);
		_editor = editor;
	}
	@Override
	protected void setControls() {
		super.setControls();
		Control controls[] = getChildren();
		for (int i = 0; i < controls.length; i++) {
			if(controls[i] instanceof Button) {
				Button btn = (Button)controls[i];
				if(btn.getText().contains("UnLoad")){
					btn.setText("Dissociate Mib(s)");
				}
			}
		}
	}
	@Override
	protected void unLoadMibs() {
		final List<String> selectedResNames = new ArrayList<String>();
		final List<String> selectedMibNames = new ArrayList<String>();
		TreeItem root = _treeViewer.getTree().getItem(0);
		TreeItem items[] = root.getItems();
		for (int i = 0; i < items.length; i++) {
			if(items[i].getChecked()) {
				ResourceTreeNode node = (ResourceTreeNode)items[i].getData();
				String mibFileName = node.getMibFileName();
				selectedMibNames.add(new File(mibFileName).getName());
				MibTreeNode mibNode = node.getChildNodes().get(0).getNode();
				addAllTables(mibNode, selectedResNames);
			}
		}
		if(_editor.getComponentsTreeComposite().deAssociateResources(selectedResNames)) {
		BusyIndicator.showWhile(Display.getDefault(),new Runnable(){
			public void run() {
				_editor.propertyChange(null);
				GenericEditorInput caInput = (GenericEditorInput)_pdm.getCAEditorInput();
				IWorkbenchPage page = PlatformUI.getWorkbench()
				.getActiveWorkbenchWindow().getActivePage();
				if(caInput != null && caInput.getEditor() != null && page.findEditor(caInput) != null) {
					GenericEditor caEditor = caInput.getEditor();
					List editparts = caEditor.getGraphicalViewer().getContents().getChildren();
					List<EditPart> objsNeedsToBeRemoved = new ArrayList<EditPart>();
					for (int i = 0 ; i < editparts.size(); i++) {
						if(editparts.get(i) instanceof MibEditPart) {
							MibEditPart mibEditPart = (MibEditPart)editparts.get(i);
							ContainerNodeModel nodeModel = (ContainerNodeModel)mibEditPart.getModel();
							String name = EcoreUtils.getName(nodeModel.getEObject());
							if(selectedMibNames.contains(name)) {
								objsNeedsToBeRemoved.add(mibEditPart);
							}
						}
					}
					if(objsNeedsToBeRemoved.size() > 0) {
						GroupRequest deleteReq =
							new GroupRequest(RequestConstants.REQ_DELETE);
						deleteReq.setEditParts(objsNeedsToBeRemoved);

						CompoundCommand compoundCmd = new CompoundCommand(
							GEFMessages.DeleteAction_ActionDeleteCommandName);
						for (int i = 0; i < objsNeedsToBeRemoved.size(); i++) {
							EditPart object = (EditPart) objsNeedsToBeRemoved.get(i);
							Command cmd = object.getCommand(deleteReq);
							if (cmd != null) compoundCmd.add(cmd);
						}
						compoundCmd.execute();
						try {
							caEditor.doSave(null);
						} catch (Exception e) {
						}
					}
				} else {
					    /** This code needs to be cleaned **/
						/*Model resourceViewModel = _editor.getResourceModel();
						EObject rootObject = (EObject) resourceViewModel.getEList()
								.get(0);
						List<EObject> chassisResList = (EList) rootObject
								.eGet(rootObject.eClass().getEStructuralFeature(
										ClassEditorConstants.CHASSIS_RESOURCE_REF_NAME));
						List<EObject> objsNeedsToBeRemoved = new ArrayList<EObject>();
						for (int i = 0; i < chassisResList.size(); i++) {
							EObject chassisObject = chassisResList.get(i);
							List<EObject> connectionList = (EList) chassisObject
									.eGet(chassisObject.eClass().getEStructuralFeature(
											"contains"));
							for(int j = 0; j < connectionList.size(); j++) {
								EObject conObj = connectionList.get(j);
								if(selectedResNames.contains(EcoreUtils.getValue(conObj, "target"))) {
									objsNeedsToBeRemoved.add(conObj);
								}
							}
							for (int j = 0; j < objsNeedsToBeRemoved.size(); j++){
								connectionList.remove(objsNeedsToBeRemoved.get(j));
							}
						}
						objsNeedsToBeRemoved.clear();
						List<EObject> mibResList = (EList) rootObject.eGet(rootObject
								.eClass().getEStructuralFeature(
										ClassEditorConstants.MIB_RESOURCE_REF_NAME));
						
						for (int i = 0; i < mibResList.size(); i++) {
							EObject mibObject = mibResList.get(i);
							String mibResName = EcoreUtils.getName(mibObject);
							if (selectedResNames.contains(mibResName)) {
								objsNeedsToBeRemoved.add(mibObject);
							}
						}
						for (int j = 0; j < objsNeedsToBeRemoved.size(); j++){
							mibResList.remove(objsNeedsToBeRemoved.get(j));
						}
						objsNeedsToBeRemoved.clear();
						List<EObject> mibList = (EList) rootObject.eGet(rootObject
								.eClass().getEStructuralFeature(
										ClassEditorConstants.MIB_REF_NAME));
						
						for (int i = 0; i < mibList.size(); i++) {
							EObject mibObject = mibList.get(i);
							String mibName = EcoreUtils.getName(mibObject);
							if (selectedMibNames.contains(mibName)) {
								objsNeedsToBeRemoved.add(mibObject);
							}
						}
						for (int j = 0; j < objsNeedsToBeRemoved.size(); j++){
							mibList.remove(objsNeedsToBeRemoved.get(j));
						}
						resourceViewModel.save(true);
						resourceViewModel.dispose();*/
					}
			}});
			super.unLoadMibs();
		}
	}
	/**
	 * Parse the Mib hierarchy and add the Mib resource's name in to list
	 * @param node
	 * @param resNames
	 */
	private void addAllTables(MibTreeNode node, List<String> resNames){
		if (node.isTableNode()) {
			resNames.add(node.getName().toString());
    	} else if (node.isGroupNode() && isGroupWithScalar(node)) {
    		resNames.add(node.getName().toString());
    	}
		List children = node.getChildNodes();
		for (int i = 0; i < children.size(); i++) {
			MibTreeNode child = (MibTreeNode) children.get(i);
			addAllTables(child, resNames);
		}
	}
	/**
	 * Check if the group node contains scalar node
	 * @param node Group Node
	 * @return boolean
	 */
	private boolean isGroupWithScalar(MibTreeNode node){
		List children = node.getChildNodes();
		for (int i = 0; i < children.size(); i++) {
			MibTreeNode child = (MibTreeNode) children.get(i);
			if (child.isScalarNode()) {
				return true;
			}
		}
		return false;
	}
}

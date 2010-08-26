/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/SelectionPushButtonCellEditor.java $
 * $Author: srajyaguru $
 * $Date: 2007/04/30 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca;

import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;

import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.ui.dialog.SelectionListDialog;
import com.clovis.common.utils.ui.factory.PushButtonCellEditor;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.editor.ca.constants.SafConstants;
import com.clovis.cw.editor.ca.dialog.CpmAspSUListDialog;
import com.clovis.cw.editor.ca.dialog.NodeProfileDialog;
/**
 *
 * @author shubhada
 * PushButtonCellEditor which displays the values to be chosen in a List format
 * from which user can choose the values, on clicking the push button. 
 */
public class SelectionPushButtonCellEditor extends PushButtonCellEditor
{
	private EReference _ref = null;
	private HashMap _nodeInstanceObjMap = null;
	private static final Log LOG = Log.getLog(CaPlugin.getDefault());
	/**
	 *
	 * @param parent Composite
	 * @param ref EReference
	 * @param parentEnv Environment
	 */
	public SelectionPushButtonCellEditor(Composite parent, EReference ref,
			Environment parentEnv)
	{
		super(parent, ref, parentEnv);
		_ref = ref;
	}
	/**
	 * @param cellEditorWindow - Control
	 * @return null
	 */
	protected Object openDialogBox(Control cellEditorWindow)
	{
		List selList = null;
		List modelList = null;
		ClovisNotifyingListImpl origList = new ClovisNotifyingListImpl();
		if (_parentEnv.getValue("model") instanceof List) {
			modelList = (List) _parentEnv.getValue("model");
			EObject selObj = (EObject) ((StructuredSelection) _parentEnv.
					getValue("selection")).getFirstElement();
			
			if (selObj.eClass().getName().equals("BootlevelType")) {
				//for boot level type display the associated service units.
				EObject susObj = (EObject) EcoreUtils.getValue(selObj, "serviceUnits");
				if (susObj == null) {
					susObj = EcoreUtils.createEObject(_ref.
							getEReferenceType(), true);
					selObj.eSet(_ref, susObj);
				}
				selList = (List) EcoreUtils.getValue(susObj, "serviceUnit");
				Environment parentEnv = _parentEnv.getParentEnv();
				Environment formEnv = parentEnv.getParentEnv();
				EObject cpmConfigObj = (EObject) formEnv.getValue("model");
				String nodeType = (String) EcoreUtils.
				getValue(cpmConfigObj, "nodeType");
				getSuInstances(nodeType, "Node", origList);
				new SelectionListDialog(getControl().
						getShell(), "Select Service Units", selList, origList).open();
			} 
		} else {
			EObject selObj = (EObject) _parentEnv.getValue("model");
			EStructuralFeature feature = (EStructuralFeature) selObj
			.eContainer().eClass().getEAllReferences().get(0);
			modelList = (List) EcoreUtils.getValue(selObj.eContainer(), feature.getName());
			
			if (selObj.eClass().getName().equals("cpmConfig")) {
				/*EReference aspSusRef = (EReference) selObj.eClass().
				getEStructuralFeature("aspServiceUnits");
				EReference aspSuRef = (EReference) aspSusRef.
				getEReferenceType().getEStructuralFeature("aspServiceUnit");
				EObject susObj = (EObject) selObj.eGet(aspSusRef);
				if (susObj == null) {
					susObj = EcoreUtils.createEObject(aspSusRef.
							getEReferenceType(), true);
					selObj.eSet(aspSusRef, susObj);
				}
				selList = (List) susObj.eGet(aspSuRef);
				String nodeType = EcoreUtils.getValue(selObj, "nodeType").
				toString();
				EObject nodeObj = NodeProfileDialog.getInstance().
				getObjectFrmName(nodeType);
				String nodeClassType = EcoreUtils.getValue(nodeObj,
						ComponentEditorConstants.NODE_CLASS_TYPE).toString();
				List suList = NodeProfileDialog.getAspSUs(nodeClassType);
				for (int i = 0; i < suList.size(); i++) {
					EObject eobj = (EObject) suList.get(i);
					EObject suObj = EcoreUtils.createEObject(aspSuRef.
							getEReferenceType(), true);
					origList.add(suObj);
					EcoreUtils.setValue(suObj, "name", (String) EcoreUtils.
							getValue(eobj, "name"));
				}*/
				
				new CpmAspSUListDialog(getControl().getShell(), selObj, modelList).open();
			} else if (selObj.eClass().getName().equals(SafConstants.NODE_INSTANCELIST_ECLASS)) {
				String name = EcoreUtils.getName(selObj);
				for (int i = 0; i < modelList.size(); i++) {
					EObject eobj = (EObject) modelList.get(i);
					String instName = EcoreUtils.getName(eobj);
					if (!name.equals(instName)) {
						origList.add(instName);
					}
				}
				EObject dependencyObj = (EObject) EcoreUtils.getValue(selObj, "dependencies");
				List dependencyList = (List) EcoreUtils.
				getValue(dependencyObj, "node");
				new SelectionListDialog(getControl().getShell(),
						"Select Node Dependencies", dependencyList, origList).open();
			} else if (selObj.eClass().getName().equals(SafConstants.SG_INSTANCELIST_ECLASS)) {
				EObject susObj = (EObject) EcoreUtils.
				getValue(selObj, "associatedServiceUnits");
				if (susObj == null) {
					susObj = EcoreUtils.createEObject(_ref.
							getEReferenceType(), true);
					selObj.eSet(_ref, susObj);
				}
				selList = (List) EcoreUtils.getValue(susObj,
				"associatedServiceUnit");
				String sgType = (String) EcoreUtils.getValue(selObj, "type");
				EObject sgObj = NodeProfileDialog.getInstance().
				getObjectFrmName(sgType);
				List suList = NodeProfileDialog.getInstance().getChildren(sgObj);
				if (!suList.isEmpty()) {
					// check for type to be of ServiceUnit since it can contain
					// ServiceInstance also
					for( int i=0; i<suList.size(); i++) {
						EObject obj = (EObject)suList.get(i);
						if( obj.eClass().getName().equals(ComponentEditorConstants.SERVICEUNIT_NAME))
						{
							String suType = EcoreUtils.getName(obj);
							getSuInstances(suType, "SG", origList);
						}
					}
				}
				new SelectionListDialog(getControl().
						getShell(), "Select Service Units", selList, origList).open();
			} else if (selObj.eClass().getName().equals(SafConstants.SI_INSTANCELIST_ECLASS)) {
				String name = EcoreUtils.getName(selObj);
				if (_ref.getName().equals("dependencies")) {
					for (int i = 0; i < modelList.size(); i++) {
						EObject eobj = (EObject) modelList.get(i);
						String instName = EcoreUtils.getName(eobj);
						if (!name.equals(instName) && !origList.contains(instName)) {
							origList.add(instName);
						} }
					EObject selectedServiceObj = selObj.eContainer().eContainer();
					EStructuralFeature serviceGrpFeature = (EStructuralFeature) selectedServiceObj
					.eContainer().eClass().getEAllReferences().get(0);
					List serviceGrpList = (List) EcoreUtils.getValue(
							selectedServiceObj.eContainer(), serviceGrpFeature.getName());
					
					//remove the selected service group as its instances added already
					for (int i = 0; i < serviceGrpList.size(); i++) {
						EObject serviceObj = (EObject) serviceGrpList.get(i);
						EObject serviceInstObj = (EObject) EcoreUtils.
						getValue(serviceObj, SafConstants.SERVICE_INSTANCES_NAME);
						List serviceInstList = (List) EcoreUtils.getValue(serviceInstObj,
								SafConstants.SERVICE_INSTANCELIST_NAME);
						for (int j = 0; (!serviceObj.equals(selectedServiceObj)
								&& j < serviceInstList.size()); j++) {
							EObject eobj = (EObject) serviceInstList.get(j);
							String instName = EcoreUtils.getName(eobj);
							if (!origList.contains(instName)) {
								origList.add(instName);
							}
						}
					}
					EObject dependencyObj = (EObject) EcoreUtils.getValue(selObj, "dependencies");
					selList = (List) EcoreUtils.
					getValue(dependencyObj, "serviceInstance");
					new SelectionListDialog(getControl().
							getShell(), "Select Service Instance Dependencies",
							selList, origList).open();
				} else if (_ref.getName().equals("prefferedServiceUnits")) {
					EObject susObj = (EObject) EcoreUtils.
					getValue(selObj, "prefferedServiceUnits");
					if (susObj == null) {
						susObj = EcoreUtils.createEObject(_ref.
								getEReferenceType(), true);
						selObj.eSet(_ref, susObj);
					}
					selList = (List) EcoreUtils.getValue(susObj,
					"prefferedServiceUnit");
					String siType = (String) EcoreUtils.getValue(selObj, "type");
					if (!siType.trim().equals("")) {
						EObject siObj = NodeProfileDialog.getInstance()
								.getObjectFrmName(siType);
						EObject parentSGObj = (EObject) NodeProfileDialog
								.getInstance().getParent(siObj).get(0);
						if (parentSGObj != null) {
							List suList = NodeProfileDialog.getInstance()
									.getChildren(parentSGObj);
							if (!suList.isEmpty()) {
								// check for type to be of ServiceUnit since it
								// can contain
								// ServiceInstance also
								for (int i = 0; i < suList.size(); i++) {
									EObject obj = (EObject) suList.get(i);
									if (obj
											.eClass()
											.getName()
											.equals(
													ComponentEditorConstants.SERVICEUNIT_NAME)) {
										String suType = EcoreUtils.getName(obj);
										getSuInstances(suType, "SG", origList);
									}
								}
							}
						}
					}
					new SelectionListDialog(getControl().
							getShell(), "Select Service Units", selList, origList).open();
				}

			} else if (selObj.eClass().getName().equals(SafConstants.CSI_INSTANCELIST_ECLASS)) {
				String name = EcoreUtils.getName(selObj);

				if (_ref.getName().equals("dependencies")) {
					for (int i = 0; i < modelList.size(); i++) {
						EObject eobj = (EObject) modelList.get(i);
						String instName = EcoreUtils.getName(eobj);
						if (!name.equals(instName)
								&& !origList.contains(instName)) {
							origList.add(instName);
						}
					}

					EObject dependencyObj = (EObject) EcoreUtils.getValue(
							selObj, "dependencies");
					selList = (List) EcoreUtils.getValue(dependencyObj,
							"componentServiceInstance");
					new SelectionListDialog(getControl().getShell(),
							"Select Component Service Instance Dependencies",
							selList, origList).open();
				}
			}
		}
		return null;
	}
	/**
	 *
	 * @param origList - Original List to be shown
	 */
	/*private void getAllSuInstances(List origList)
	 {
	 NodeProfileDialog.getInstance().initNodeInstanceMap();
	 _nodeInstanceObjMap = (HashMap) NodeProfileDialog.getInstance().
	 getNodeTypeInstancesMap();
	 origList.clear();
	 List suInstObjList = new Vector();
	 Iterator iterator = _nodeInstanceObjMap.keySet().iterator();
	 while (iterator.hasNext()) {
	 String nodetype = (String) iterator.next();
	 List instList = (List) _nodeInstanceObjMap.get(nodetype);
	 for (int i = 0; (instList != null && i < instList.size()); i++) {
	 EObject nodeInstObj = (EObject) instList.get(i);
	 EObject suInstsObj = (EObject) EcoreUtils.
	 getValue(nodeInstObj, "serviceUnitInstances");
	 if (suInstsObj != null) {
	 List suInstList = (List) EcoreUtils.getValue(
	 suInstsObj, "serviceUnitInstance");
	 suInstObjList.addAll(suInstList);
	 }
	 }
	 
	 }
	 for (int i = 0; i < suInstObjList.size(); i++) {
	 EObject suInstObj = (EObject) suInstObjList.get(i);
	 origList.add(EcoreUtils.getName(suInstObj));
	 }
	 }*/
	/**
	 *
	 * @param type - type of Editor Object for which SU instances
	 * to be filtered
	 * @param objTypeName - Object Type Name for which SU instances
	 * have to be fetched.For Now it takes the values "Node" or "SG"
	 *
	 * @param origList - Original List to be shown
	 */
	private void getSuInstances(String type, String objTypeName, List origList)
	{
		NodeProfileDialog.getInstance().initNodeInstanceMap();
		_nodeInstanceObjMap = (HashMap) NodeProfileDialog.getInstance().
		getNodeTypeInstancesMap();
		origList.clear();
		List suInstObjList = new Vector();
		if (objTypeName.equals("Node")) {
			List instList = (List) _nodeInstanceObjMap.get(type);
			if (instList == null) {
				return;
			}
			for (int i = 0; i < instList.size(); i++) {
				EObject nodeInstObj = (EObject) instList.get(i);
				EObject suInstsObj = (EObject) EcoreUtils.getValue(
						nodeInstObj, SafConstants.SERVICEUNIT_INSTANCES_NAME);
				if (suInstsObj != null) {
					List suInstList = (List) EcoreUtils.getValue(
							suInstsObj, SafConstants.
							SERVICEUNIT_INSTANCELIST_NAME);
					suInstObjList.addAll(suInstList);
				}
			}
			
			for (int i = 0; i < suInstObjList.size(); i++) {
				EObject suInstObj = (EObject) suInstObjList.get(i);
				origList.add(EcoreUtils.getName(suInstObj));
			}
		} else if (objTypeName.equals("SG")) {
			Iterator iterator = _nodeInstanceObjMap.keySet().iterator();
			while (iterator.hasNext()) {
				String nodetype = (String) iterator.next();
				List instList = (List) _nodeInstanceObjMap.get(nodetype);
				for (int i = 0; (instList != null && i < instList.size());
				i++) {
					EObject nodeInstObj = (EObject) instList.get(i);
					EObject suInstsObj = (EObject) EcoreUtils.getValue(
							nodeInstObj, SafConstants.
							SERVICEUNIT_INSTANCES_NAME);
					if (suInstsObj != null) {
						List suInstList = (List) EcoreUtils.getValue(
								suInstsObj, SafConstants.
								SERVICEUNIT_INSTANCELIST_NAME);
						suInstObjList.addAll(suInstList);
					}
				}
			}
			for (int i = 0; i < suInstObjList.size(); i++) {
				EObject suInstObj = (EObject) suInstObjList.get(i);
				String suInstType = (String) EcoreUtils.
				getValue(suInstObj, "type");
				if (suInstType.equals(type)) {
					origList.add(EcoreUtils.getName(suInstObj));
				}
			}
		}
	}
	/**
	 *
	 * @param parent  Composite
	 * @param feature EStructuralFeature
	 * @param env Environment
	 * @return cell Editor
	 */
	public static CellEditor createEditor(Composite parent,
			EStructuralFeature feature, Environment env)
	{
		return new
		SelectionPushButtonCellEditor(parent, (EReference) feature, env);
	}
}

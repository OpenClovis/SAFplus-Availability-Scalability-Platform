/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/handler/UpdateProject.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.handler;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.editor.ca.ComponentDataUtils;
import com.clovis.cw.editor.ca.ResourceDataUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.genericeditor.GenericEditor;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.project.data.ProjectDataModel;

public class UpdateProject {

	protected List _resEditorObjects = null;

	protected List _compEditorObjects = null;

	private List _compList = null;

	private ProjectDataModel _pdm;

	public UpdateProject(ProjectDataModel pdm) {
		_pdm = pdm;
		_resEditorObjects = pdm.getCAModel().getEList();
		_compEditorObjects = pdm.getComponentModel().getEList();
		_compList = getCompList(_compEditorObjects);

	}

	/**
	 * Selects libraries automatically
	 * for the Project.
	 */
	public void updateLibSelection() {
		GenericEditorInput editorinput = (GenericEditorInput) _pdm
				.getComponentEditorInput();
		boolean dirty = false, latestDirty = false;
		GenericEditor editor = null;
		if (editorinput != null && editorinput.getEditor() != null) {
			editor = editorinput.getEditor();
			dirty = editor.isDirty();
		}
		setLibs();
		if (editor != null) {
			latestDirty = editor.isDirty();
		}
		if (!dirty && latestDirty) {
			editor.doSave(null);
		}

	}

	/**
	 * 
	 * @Method to return Component list.
	 */
	private List getCompList(List list) {
		List compList = new Vector();
		EObject rootObject = (EObject) list.get(0);
		compList.addAll((EList) rootObject.eGet(rootObject.eClass().getEStructuralFeature(ComponentEditorConstants.SAFCOMPONENT_REF_NAME)));
		compList.addAll((EList) rootObject.eGet(rootObject.eClass().getEStructuralFeature(ComponentEditorConstants.NONSAFCOMPONENT_REF_NAME)));
		return compList;
	}

	/**
	 * Selects the libs.
	 *  
	 */
	private void setLibs() {
		for (int i = 0; i < _compList.size(); i++) {
			EObject compObj = (EObject) _compList.get(i);			
			setProvLib(compObj, _resEditorObjects, _pdm.getProject());
			setAlarmLib(compObj, _resEditorObjects, _pdm.getProject());
			setHalLib(compObj, _resEditorObjects, _pdm.getProject());
			setMandatoryLibs(compObj);
		}
	}
	
	/**
	 * Selects the Mandatory libs.
	 *  
	 */
	public static void setMandatoryLibs(EObject compObj) {		
			EObject eoPropEObj = (EObject) EcoreUtils.getValue(compObj,
					ComponentEditorConstants.EO_PROPERTIES_NAME);

			if (eoPropEObj != null) {

				EObject eoASPLib = (EObject) EcoreUtils.getValue(eoPropEObj,
						ComponentEditorConstants.EO_ASPLIB_NAME);
				if (eoASPLib != null) {

					String osalSelValue = EcoreUtils.getValue(eoASPLib,
							ComponentEditorConstants.EO_OSALLIB_NAME)
							.toString();
					if (osalSelValue.equals("CL_FALSE")) {
						EcoreUtils.setValue(eoASPLib,
								ComponentEditorConstants.EO_OSALLIB_NAME,
								"CL_TRUE");
					}
					String bufferSelValue = EcoreUtils.getValue(eoASPLib,
							ComponentEditorConstants.EO_BUFFERLIB_NAME)
							.toString();
					if (bufferSelValue.equals("CL_FALSE")) {
						EcoreUtils.setValue(eoASPLib,
								ComponentEditorConstants.EO_BUFFERLIB_NAME,
								"CL_TRUE");
					}
					String iocSelValue = EcoreUtils.getValue(eoASPLib,
							ComponentEditorConstants.EO_IOCLIB_NAME).toString();
					if (iocSelValue.equals("CL_FALSE")) {
						EcoreUtils.setValue(eoASPLib,
								ComponentEditorConstants.EO_IOCLIB_NAME,
								"CL_TRUE");
					}
					String rmdSelValue = EcoreUtils.getValue(eoASPLib,
							ComponentEditorConstants.EO_RMDLIB_NAME).toString();
					if (rmdSelValue.equals("CL_FALSE")) {
						EcoreUtils.setValue(eoASPLib,
								ComponentEditorConstants.EO_RMDLIB_NAME,
								"CL_TRUE");
					}
					String eoSelValue = EcoreUtils.getValue(eoASPLib,
							ComponentEditorConstants.EO_EOLIB_NAME).toString();
					if (eoSelValue.equals("CL_FALSE")) {
						EcoreUtils.setValue(eoASPLib,
								ComponentEditorConstants.EO_EOLIB_NAME,
								"CL_TRUE");
					}
					String omSelValue = EcoreUtils.getValue(eoASPLib,
							ComponentEditorConstants.EO_OMLIB_NAME).toString();
					

					String provSelValue = EcoreUtils.getValue(eoASPLib,
							ComponentEditorConstants.EO_PROVLIB_NAME)
							.toString();
					String alarmSelValue = EcoreUtils.getValue(eoASPLib,
							ComponentEditorConstants.EO_ALARMLIB_NAME)
							.toString();
					//check for the case where prov lib is selected but associated
					//resources does not have prov enabled
					if (omSelValue.equals("CL_FALSE")
							&& (provSelValue.equals("CL_TRUE") || alarmSelValue
									.equals("CL_TRUE"))) {
						EcoreUtils.setValue(eoASPLib,
								ComponentEditorConstants.EO_OMLIB_NAME,
								"CL_TRUE");
					}
					if (omSelValue.equals("CL_TRUE")
							&& provSelValue.equals("CL_FALSE") && alarmSelValue
									.equals("CL_FALSE")) {
						EcoreUtils.setValue(eoASPLib,
								ComponentEditorConstants.EO_OMLIB_NAME,
								"CL_FALSE");
					}
					
				}
			}
	}

	/**
	 *
	 * Selects the Prov lib.
	 * 
	 */
	public static void setProvLib(EObject compObj, List resObjs, IProject project) {		
		List associatedResList = ComponentDataUtils.getAssociatedResources(
				project, compObj);

		HashSet resObjList = new HashSet();
		if (associatedResList != null) {
			for (int j = 0; j < associatedResList.size(); j++) {

				String resName = (String) associatedResList.get(j);
				EObject resObj = ResourceDataUtils.getObjectFrmName(resObjs, resName);
				if (resObj != null) {
					resObjList.add(resObj);
				}
			}
		}

			EObject eoPropEObj = (EObject) EcoreUtils.getValue(compObj,
					ComponentEditorConstants.EO_PROPERTIES_NAME);

			if (eoPropEObj != null) {

				EObject eoASPLib = (EObject) EcoreUtils.getValue(eoPropEObj,
						ComponentEditorConstants.EO_ASPLIB_NAME);
				String provSelValue = EcoreUtils.getValue(eoASPLib,
						ComponentEditorConstants.EO_PROVLIB_NAME).toString();
				HashMap resInfoMap = new HashMap();
				Iterator iterator = resObjList.iterator();
				boolean associatedDOExists = false;

				while (iterator.hasNext()) {

					EObject resObj = (EObject) iterator.next();
					EObject provObj = (EObject) EcoreUtils.getValue(resObj,
							ClassEditorConstants.RESOURCE_PROVISIONING);

					if (provObj != null) {

						boolean enable = ((Boolean) EcoreUtils.getValue(
								provObj, "isEnabled")).booleanValue();
						if (enable) {

							List associatedDOList = (List) EcoreUtils
									.getValue(provObj,
											ClassEditorConstants.ASSOCIATED_DO);
							if (associatedDOList != null
									&& !associatedDOList.isEmpty())
								associatedDOExists = true;
							if (resObj.eClass().getName().equals(
									ClassEditorConstants.SOFTWARE_RESOURCE_NAME)
									|| resObj.eClass().getName().equals(
											ClassEditorConstants.MIB_RESOURCE_NAME)) {
								resInfoMap.put(resObj,
										"ProvEnabled,SoftwareResource");
							} else {
								resInfoMap.put(resObj,
										"ProvEnabled,HardwareResource");
							}
						}
					}
				}

				boolean provEnable = resInfoMap
						.containsValue("ProvEnabled,HardwareResource")
						|| resInfoMap
								.containsValue("ProvEnabled,SoftwareResource");
				boolean provEnabledHardware = resInfoMap
						.containsValue("ProvEnabled,HardwareResource");
				boolean provEnabledOnlySoftware = resInfoMap
						.containsValue("ProvEnabled,SoftwareResource")
						&& !resInfoMap
								.containsValue("ProvEnabled,HardwareResource");
				boolean provNotEnabled = !resInfoMap
						.containsValue("ProvEnabled,HardwareResource")
						&& !resInfoMap
								.containsValue("ProvEnabled,SoftwareResource");

				
				String halSelValue = EcoreUtils.getValue(eoASPLib,
						ComponentEditorConstants.EO_HALLIB_NAME).toString();
				String omSelValue = EcoreUtils.getValue(eoASPLib,
						ComponentEditorConstants.EO_OMLIB_NAME).toString();
				String txnSelValue = EcoreUtils.getValue(eoASPLib,
						ComponentEditorConstants.EO_TRANSACTIONLIB_NAME).toString();
				//check for the case where one of the associated resource has
				//prov enabled, but HAL/OM library is not selected.
				if (provEnabledHardware) {
					if (associatedDOExists && halSelValue.equals("CL_FALSE")) {
						EcoreUtils.setValue(eoASPLib,
								ComponentEditorConstants.EO_HALLIB_NAME,
								"CL_TRUE");
					}
					if (omSelValue.equals("CL_FALSE")) {
						EcoreUtils.setValue(eoASPLib,
								ComponentEditorConstants.EO_OMLIB_NAME,
								"CL_TRUE");
					}
				} else if (provEnabledOnlySoftware) {
					if (halSelValue.equals("CL_TRUE")) {
						EcoreUtils.setValue(eoASPLib,
								ComponentEditorConstants.EO_HALLIB_NAME,
								"CL_FALSE");
					}
					if (omSelValue.equals("CL_FALSE")) {
						EcoreUtils.setValue(eoASPLib,
								ComponentEditorConstants.EO_OMLIB_NAME,
								"CL_TRUE");
					}
				}

				//check for the case where prov lib is selected but associated
				//resources does not have prov enabled
				if (provSelValue.equals("CL_TRUE")) {
					if (provNotEnabled) {
						EcoreUtils.setValue(eoASPLib,
								ComponentEditorConstants.EO_PROVLIB_NAME,
								"CL_FALSE");
					}
					
				} else {
					if (provEnable) {
						EcoreUtils.setValue(eoASPLib,
								ComponentEditorConstants.EO_PROVLIB_NAME,
								"CL_TRUE");
						if (txnSelValue.equals("CL_FALSE")) {
							EcoreUtils.setValue(eoASPLib,
									ComponentEditorConstants.EO_TRANSACTIONLIB_NAME,
									"CL_TRUE");
						}
					}
				}

			}		
	}

	/**
	 *
	 * Selects the Hal lib.
	 */
	public static void setHalLib(EObject compObj, List resObjs, IProject project)
	{
		List associatedResList = ComponentDataUtils.getAssociatedResources(
				project, compObj);
		HashSet resObjList = new HashSet();
		if (associatedResList != null) {
			for (int j = 0; j < associatedResList.size(); j++) {
				String resName = (String) associatedResList.get(j);
				EObject resObj = ResourceDataUtils.getObjectFrmName(resObjs, resName);
				if (resObj != null) {
					resObjList.add(resObj);
				}
			}
		}
			EObject eoPropEObj = (EObject) EcoreUtils.getValue(compObj,
					ComponentEditorConstants.EO_PROPERTIES_NAME);
			if (eoPropEObj != null) {
				Iterator iterator = resObjList.iterator();
				boolean associatedDOExists = false;

				while (iterator.hasNext()) {

					EObject resObj = (EObject) iterator.next();
					EObject provObj = (EObject) EcoreUtils.getValue(resObj,
							ClassEditorConstants.RESOURCE_PROVISIONING);

					if (provObj != null) {

						boolean enable = ((Boolean) EcoreUtils.getValue(
								provObj, "isEnabled")).booleanValue();
						if (enable) {

							List associatedDOList = (List) EcoreUtils
									.getValue(provObj,
											ClassEditorConstants.ASSOCIATED_DO);
							if (associatedDOList != null
									&& !associatedDOList.isEmpty())
								associatedDOExists = true;

						}
					}

					EObject alarmObj = (EObject) EcoreUtils.getValue(resObj,
							ClassEditorConstants.RESOURCE_ALARM);
					if (alarmObj != null) {
						boolean enable = ((Boolean) EcoreUtils.getValue(
								alarmObj, "isEnabled")).booleanValue();
						if (enable) {
							List associatedDOList = (List) EcoreUtils.getValue(
									alarmObj,
									ClassEditorConstants.ASSOCIATED_DO);
							if (associatedDOList != null
									&& !associatedDOList.isEmpty())
								associatedDOExists = true;
						}
					}

				}
				EObject eoASPLib = (EObject) EcoreUtils.getValue(eoPropEObj,
						ComponentEditorConstants.EO_ASPLIB_NAME);
				String halSelValue = EcoreUtils.getValue(eoASPLib,
						ComponentEditorConstants.EO_HALLIB_NAME).toString();

				if (halSelValue.equals("CL_TRUE") && !associatedDOExists) {
					EcoreUtils
							.setValue(eoASPLib,
									ComponentEditorConstants.EO_HALLIB_NAME,
									"CL_FALSE");
				}

			}		
	}

	/**
	 *
	 * Selects the Alarm lib.
	 */
	public static void setAlarmLib(EObject compObj, List resObjs, IProject project) {
			List associatedResList = ComponentDataUtils.getAssociatedResources(
					project, compObj);
			List resObjList = new Vector();
			if (associatedResList != null) {
				for (int j = 0; j < associatedResList.size(); j++) {
					String resName = (String) associatedResList.get(j);
					EObject resObj = ResourceDataUtils.getObjectFrmName(resObjs, resName);
					if (resObj != null) {
						resObjList.add(resObj);
					}
				}
			}
			HashMap resInfoMap = new HashMap();
			EObject eoPropEObj = (EObject) EcoreUtils.getValue(compObj,
					ComponentEditorConstants.EO_PROPERTIES_NAME);
			if (eoPropEObj != null) {
				EObject eoASPLib = (EObject) EcoreUtils.getValue(eoPropEObj,
						ComponentEditorConstants.EO_ASPLIB_NAME);
				String alarmSelValue = EcoreUtils.getValue(eoASPLib,
						ComponentEditorConstants.EO_ALARMLIB_NAME).toString();

				boolean associatedDOExists = false;
				for (int j = 0; j < resObjList.size(); j++) {
					EObject resObj = (EObject) resObjList.get(j);
					EObject alarmObj = (EObject) EcoreUtils.getValue(resObj,
							ClassEditorConstants.RESOURCE_ALARM);
					if (alarmObj != null) {
						boolean enable = ((Boolean) EcoreUtils.getValue(
								alarmObj, "isEnabled")).booleanValue();
						if (enable) {
							List associatedDOList = (List) EcoreUtils.getValue(
									alarmObj,
									ClassEditorConstants.ASSOCIATED_DO);
							if (associatedDOList != null
									&& !associatedDOList.isEmpty())
								associatedDOExists = true;
							if (resObj.eClass().getName().equals(
											ClassEditorConstants.SOFTWARE_RESOURCE_NAME)
											|| resObj.eClass().getName().equals(
													ClassEditorConstants.MIB_RESOURCE_NAME)) {
								resInfoMap.put(resObj,
										"AlarmEnabled,SoftwareResource");
							} else {
								resInfoMap.put(resObj,
										"AlarmEnabled,HardwareResource");
							}

						}
					}
				}
				boolean alarmEnable = resInfoMap
						.containsValue("AlarmEnabled,HardwareResource")
						|| resInfoMap
								.containsValue("AlarmEnabled,SoftwareResource");
				boolean alarmEnabledHardware = resInfoMap
						.containsValue("AlarmEnabled,HardwareResource");
				boolean alarmEnabledOnlySoftware = resInfoMap
						.containsValue("AlarmEnabled,SoftwareResource")
						&& !resInfoMap
								.containsValue("AlarmEnabled,HardwareResource");
				boolean alarmNotEnabled = !resInfoMap
						.containsValue("AlarmEnabled,HardwareResource")
						&& !resInfoMap
								.containsValue("AlarmEnabled,SoftwareResource");
				String halSelValue = EcoreUtils.getValue(eoASPLib,
						ComponentEditorConstants.EO_HALLIB_NAME).toString();
				String omSelValue = EcoreUtils.getValue(eoASPLib,
						ComponentEditorConstants.EO_OMLIB_NAME).toString();
				//             check for the case where one of the associated resource has
				//prov enabled, but HAL/OM library is not selected.
				if (alarmEnabledHardware) {
					if (associatedDOExists && halSelValue.equals("CL_FALSE")) {
						EcoreUtils.setValue(eoASPLib,
								ComponentEditorConstants.EO_HALLIB_NAME,
								"CL_TRUE");
					}
					if (omSelValue.equals("CL_FALSE")) {
						EcoreUtils.setValue(eoASPLib,
								ComponentEditorConstants.EO_OMLIB_NAME,
								"CL_TRUE");
					}
				} else if (alarmEnabledOnlySoftware) {
					if (halSelValue.equals("CL_TRUE")) {
						EcoreUtils.setValue(eoASPLib,
								ComponentEditorConstants.EO_HALLIB_NAME,
								"CL_FALSE");
					}
					if (omSelValue.equals("CL_FALSE")) {
						EcoreUtils.setValue(eoASPLib,
								ComponentEditorConstants.EO_OMLIB_NAME,
								"CL_TRUE");
					}
				}
				//check for the case where alarm lib is selected but associated
				//resources does not have alarm enabled
				if (alarmSelValue.equals("CL_FALSE")) {
					if (alarmEnable) {
						EcoreUtils.setValue(eoASPLib,
								ComponentEditorConstants.EO_ALARMLIB_NAME,
								"CL_TRUE");
						return;
					}
				}

			}		
	}
}

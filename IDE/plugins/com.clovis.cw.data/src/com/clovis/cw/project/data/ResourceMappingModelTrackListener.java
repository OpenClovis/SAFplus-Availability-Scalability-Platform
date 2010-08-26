/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.data/src/com/clovis/cw/project/data/ResourceMappingModelTrackListener.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.project.data;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.ecore.resource.Resource;

import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;

/**
 * 
 * @author pushparaj
 * Listener for handle  component-resource mapping model
 * changes
 */
public class ResourceMappingModelTrackListener extends MappingModelTrackListener {

	private static final Log  LOG = Log.getLog(DataPlugin.getDefault());

    public ResourceMappingModelTrackListener(ProjectDataModel dataModel) {
		super(dataModel);
	}
	/**
	 * Creates Components List
	 * @param dataModel
	 */
	protected void createResourcesList() {
		List caList = _dataModel.getComponentModel().getEList();
		EObject rootObject = (EObject) caList.get(0);
		List resources = (List) rootObject.eGet(rootObject.eClass().getEStructuralFeature("safComponent"));
		_resourcesList.addAll(resources);
		resources = (List) rootObject.eGet(rootObject.eClass().getEStructuralFeature("nonSAFComponent"));
		_resourcesList.addAll(resources);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.cw.project.data.MappingModelTrackListener#notifyChanged(org.eclipse.emf.common.notify.Notification)
	 */
	@Override
	public void notifyChanged(Notification notification) {
		super.notifyChanged(notification);

		if (notification.getEventType() == Notification.ADD
				|| notification.getEventType() == Notification.ADD_MANY) {
            Object newVal = notification.getNewValue();
            if (newVal instanceof EObject) {
                EObject obj = (EObject) newVal;
                EcoreUtils.addListener(obj, this, -1);
            }
            return;
		}

		Object notifier = notification.getNotifier();
		if (notifier != null && notifier instanceof EObject) {

			if (((EObject) notifier).eClass().getName()
					.equals("linkObjectType")
					&& notification.getFeature() != null
					&& ((EStructuralFeature) notification.getFeature())
							.getName().equals("linkTarget")) {

				switch (notification.getEventType()) {

				case Notification.REMOVE:
					String resName = "\\" + notification.getOldStringValue() + ":";
					String compName = EcoreUtils.getValue((EObject) notifier, "linkSource").toString();

					Resource rtResource;
					EObject resObj;
					String moId, type;
					HashMap<String, List<String>> compTypeNameMap = new HashMap<String, List<String>>();
					List<String> nameList;

					EObject amfObj = _dataModel.getNodeProfiles().getEObject();
					EObject nodeInstancesObj = (EObject) EcoreUtils.getValue(
							amfObj, "nodeInstances");
					List<EObject> nodeList = (List<EObject>) EcoreUtils
							.getValue(nodeInstancesObj, "nodeInstance");

					if(nodeList == null)	return;

					for (int i = 0; i < nodeList.size(); i++) {
						EObject nodeInstObj = (EObject) nodeList.get(i);
						EObject serviceUnitInstsObj = (EObject) EcoreUtils.getValue(nodeInstObj,
								"serviceUnitInstances");

						if (serviceUnitInstsObj != null) {
							List suInstList = (List) EcoreUtils.getValue(serviceUnitInstsObj,
			                        "serviceUnitInstance");

							for (int j = 0; j < suInstList.size(); j++) {
								EObject suInstObj = (EObject) suInstList.get(j);
								EObject compInstsObj = (EObject) EcoreUtils.getValue(suInstObj,
			                            "componentInstances");

								if (compInstsObj != null) {
									List compInstList = (List) EcoreUtils.getValue(compInstsObj,
			                                "componentInstance");

									for (int k = 0; k < compInstList.size(); k++) {
										EObject compInstObj = (EObject) compInstList.get(k);
										type = EcoreUtils.getValue(compInstObj, "type").toString();
										nameList = compTypeNameMap.get(type);

										if(nameList ==  null) {
											nameList = new ArrayList<String>();
											compTypeNameMap.put(type,
													nameList);
										}
										nameList.add(EcoreUtils.getName(compInstObj));
									}
								}
							}
						}
					}

					Iterator<EObject> nodeItr = nodeList.iterator(), resItr;
					while (nodeItr.hasNext()) {
						rtResource = getCompResResource(
								EcoreUtils.getName(nodeItr.next()), false,
								_dataModel);

						resItr = getCompResListFromRTResForComps(
								rtResource, compTypeNameMap.get(compName)).iterator();
						while (resItr.hasNext()) {
							resObj = resItr.next();
							moId = EcoreUtils.getValue(resObj, "moID")
									.toString();

							if (moId.contains(resName)) {
								EObject container = resObj.eContainer();
								Object val = container.eGet(resObj
										.eContainingFeature());

								if (val instanceof List) {
									((List) val).remove(resObj);
								} else if (val instanceof EObject) {
									((EObject) val).eUnset(resObj
											.eContainingFeature());
								}
							}
						}

						EcoreModels.saveResource(rtResource);
					}

					break;
				}
			}
		}
	}

    /**
     *
     * @param nodeInstName NodeInstance Name
     * @param toBeCreated - to indicate if whether the file
     * has to be created or not if it does not exist.
     * @return the rt resource corresponding to Node Instance
     */
    private static Resource getCompResResource(String nodeInstName, boolean toBeCreated, ProjectDataModel pdm)
    {
        Resource nodeInstResource = null;
        try {
            // Now get the compRes xml file for each node Instance
            // defined from the config dir under project.
            String dataFilePath = pdm.getProject().getLocation().toOSString()
                + File.separator
                + ICWProject.CW_PROJECT_CONFIG_DIR_NAME
                + File.separator
                + nodeInstName + "_" + "rt.xml";
            URI uri = URI.createFileURI(dataFilePath);
            File xmlFile = new File(dataFilePath);
            if (toBeCreated) {
            nodeInstResource = xmlFile.exists()
                ? EcoreModels.getUpdatedResource(uri)
                : EcoreModels.create(uri);
            } else {
            	nodeInstResource = xmlFile.exists()
                ? EcoreModels.getUpdatedResource(uri)
                : null;
            }

        } catch (Exception exc) {
            LOG.error("Error Reading CompRes Resource.", exc);
        }
        return nodeInstResource;
    }

	/**
	 * Retruns the list of all the resources for the component.
	 * 
	 * @param rtResource
	 * @return
	 */
	private static List<EObject> getCompResListFromRTResForComps(Resource rtResource, List<String> comps) {
		List<EObject> resList = new ArrayList<EObject>();

		if (rtResource != null && rtResource.getContents().size() > 0) {
			EObject compInstancesObj = (EObject) rtResource.getContents()
					.get(0);
			List<EObject> compInstList = (List<EObject>) EcoreUtils.getValue(compInstancesObj,
					"compInst");

			for (int j = 0; compInstList != null && j < compInstList.size(); j++) {
				if(comps.contains(EcoreUtils.getValue(compInstList.get(j), "compName").toString())) {
					resList.addAll((List) EcoreUtils.getValue(
							compInstList.get(j),
							"resource"));
				}
			}
		}
		return resList;
	}
}

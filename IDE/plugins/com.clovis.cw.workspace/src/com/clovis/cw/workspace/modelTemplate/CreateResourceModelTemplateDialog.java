/**
 * 
 */
package com.clovis.cw.workspace.modelTemplate;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.ClovisUtils;
import com.clovis.common.utils.ecore.EcoreCloneUtils;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.genericeditor.editparts.BaseEditPart;
import com.clovis.cw.genericeditor.model.EdgeModel;
import com.clovis.cw.genericeditor.model.NodeModel;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.project.utils.FormatConversionUtils;

/**
 * Concrete Dialog implementation to capture the information required to create
 * the resource model template and creates the same.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class CreateResourceModelTemplateDialog extends
		CreateModelTemplateDialog {

	/**
	 * Constructor.
	 * 
	 * @param shell
	 * @param project
	 * @param selection
	 * @param modelType
	 */
	public CreateResourceModelTemplateDialog(Shell shell, IProject project,
			StructuredSelection selection, String modelType) {
		super(shell, project, selection, modelType);
		createModelTemplateObject();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.cw.workspace.modelTemplate.CreateModelTemplateDialog#createModelTemplateObject()
	 */
	@Override
	protected void createModelTemplateObject() {
		super.createModelTemplateObject();
		EcoreUtils.setValue(_modelTemplateObject, FEATURE_MODEL_TYPE,
				MODEL_TYPE_RESOURCE);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.cw.workspace.modelTemplate.CreateModelTemplateDialog#okPressed()
	 */
	@Override
	protected void okPressed() {
		populateModelTemplate();

		if (saveModelTemplate()) {
			updateModelTemplateView();
			super.okPressed();
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.cw.workspace.modelTemplate.CreateModelTemplateDialog#populateModelTemplate()
	 */
	@SuppressWarnings("unchecked")
	@Override
	protected void populateModelTemplate() {
		BaseEditPart bep = (BaseEditPart) _selection.getFirstElement();
		NodeModel nodeModel = (NodeModel) bep.getModel();

		List<EdgeModel> edgeList = new ArrayList<EdgeModel>();
		List<NodeModel> nodeList = new ArrayList<NodeModel>();

		ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);

		boolean includeChildHierarchy = Boolean
				.parseBoolean(EcoreUtils.getValue(_modelTemplateObject,
						FEATURE_INCLUDE_CHILD_HIERARCHY).toString());
		boolean includeRelatedEntities = Boolean.parseBoolean(EcoreUtils
				.getValue(_modelTemplateObject,
						FEATURE_INCLUDE_RELATED_ENTITIES).toString());
		ModelTemplateUtils.getChildHierarchy(nodeModel, edgeList, nodeList,
				includeChildHierarchy ? -1 : 1);

		EObject resAlarmMapInfo = pdm.getResourceAlarmMapModel().getEObject();
		EObject alarmRuleInfo = pdm.getAlarmRules().getEObject();
		EObject alarmInfo = pdm.getAlarmProfiles().getEObject();

		EObject resourceEntityObj = EcoreUtils.createEObject(ModelTemplateUtils
				.getClassFromSamePackage(nodeModel.getEObject(),
						FEATURE_RESOURCE_INFORMATION), true);

		EObject resAlarmMapEntityObj = EcoreUtils.createEObject(
				ModelTemplateUtils.getClassFromSamePackage(resAlarmMapInfo,
						"mapInformation"), true);
		EcoreUtils.setValue(resAlarmMapEntityObj, "sourceModel", EcoreUtils
				.getValue(resAlarmMapInfo, "sourceModel").toString());
		EcoreUtils.setValue(resAlarmMapEntityObj, "targetModel", EcoreUtils
				.getValue(resAlarmMapInfo, "targetModel").toString());
		EObject resAlarmMapLinkEntityObj = EcoreUtils.createEObject(
				ModelTemplateUtils.getClassFromSamePackage(resAlarmMapInfo,
						"link"), true);

		EObject alarmRuleEntityObj = EcoreUtils.createEObject(
				ModelTemplateUtils.getClassFromSamePackage(alarmRuleInfo,
						FEATURE_ALARM_RULE_INFORMATION), true);
		EObject alarmEntityObj = EcoreUtils.createEObject(ModelTemplateUtils
				.getClassFromSamePackage(alarmInfo, FEATURE_ALARM_INFORMATION),
				true);

		List alarmList = ModelTemplateUtils
				.getEObjListFromChildReferences(alarmInfo);
		List alarmRuleList = ModelTemplateUtils
				.getEObjListFromChildReferences(alarmRuleInfo);
		EObject resAlarmLinkObj = (EObject) ((List) EcoreUtils.getValue(
				resAlarmMapInfo, "link")).get(0);
		List resAlarmMapList = (List) EcoreUtils.getValue(resAlarmLinkObj,
				"linkDetail");
		EcoreUtils.setValue(resAlarmMapLinkEntityObj, "linkType", EcoreUtils
				.getValue(resAlarmLinkObj, "linkType").toString());

		List<EObject> includeResList = new ArrayList<EObject>();
		List<EObject> includeAlarmList = new ArrayList<EObject>();
		List<String> associatedAlarmIDList = new ArrayList<String>();
		List<EObject> includeAlarmRuleList = new ArrayList<EObject>();
		List<EObject> includeResAlarmMapList = new ArrayList<EObject>();

		Iterator<NodeModel> nodeItr = nodeList.iterator();
		while (nodeItr.hasNext()) {
			EObject resObj = nodeItr.next().getEObject();
			includeResList.add(EcoreCloneUtils.cloneEObject(resObj));

			if (includeRelatedEntities) {
				String resName = EcoreUtils.getName(resObj);
				EObject resAlarmMapObj = (EObject) ModelTemplateUtils
						.getObjectFrmFeature(resAlarmMapList, "linkSource",
								resName);
				if (resAlarmMapObj != null) {

					List<String> linkTargetList = (List<String>) EcoreUtils
							.getValue(resAlarmMapObj, "linkTarget");
					if (linkTargetList.size() == 0)
						continue;

					includeResAlarmMapList.add(EcoreCloneUtils
							.cloneEObject(resAlarmMapObj));
				}

				EObject alarmRuleObj = ClovisUtils.getObjectFrmName(
						alarmRuleList, resName);
				if (alarmRuleObj != null) {

					includeAlarmRuleList.add(EcoreCloneUtils
							.cloneEObject(alarmRuleObj));

					List<EObject> alarmRuleAlarmList = (List<EObject>) EcoreUtils
							.getValue(alarmRuleObj, "alarm");

					String alarmID;
					Iterator<EObject> alarmRuleAlarmIterator = alarmRuleAlarmList
							.iterator();
					while (alarmRuleAlarmIterator.hasNext()) {

						alarmID = EcoreUtils.getValue(
								alarmRuleAlarmIterator.next(), "alarmID")
								.toString();
						if (!associatedAlarmIDList.contains(alarmID)) {

							associatedAlarmIDList.add(alarmID);
							EObject alarmObj = ClovisUtils.getObjectFrmName(
									alarmList, alarmID);
							includeAlarmList.add(EcoreCloneUtils
									.cloneEObject(alarmObj));
						}
					}
				}
			}
		}

		List<EObject> connList = new ArrayList<EObject>();
		Iterator<EdgeModel> edgeItr = edgeList.iterator();
		while (edgeItr.hasNext()) {
			connList.add(EcoreCloneUtils.cloneEObject(edgeItr.next()
					.getEObject()));
		}

		ClovisUtils.addObjectsToModel(includeResList, resourceEntityObj);
		ClovisUtils.addObjectsToModel(connList, resourceEntityObj);

		FormatConversionUtils.convertToResourceFormat(resourceEntityObj,
				EDITOR_TYPE_RESOURCE);
		ModelTemplateUtils.setRDN(includeResList, true);
		addEntity(resourceEntityObj, FEATURE_RESOURCE_INFORMATION);

		if (includeRelatedEntities) {
			EStructuralFeature resAlarmMapLinkRef = resAlarmMapEntityObj
					.eClass().getEStructuralFeature("link");
			((List<EObject>) resAlarmMapEntityObj.eGet(resAlarmMapLinkRef))
					.add(resAlarmMapLinkEntityObj);

			ClovisUtils.addObjectsToModel(includeResAlarmMapList,
					resAlarmMapLinkEntityObj);
			addEntity(resAlarmMapEntityObj,
					FEATURE_RESOURCE_ALARM_MAP_INFORMATION);

			ClovisUtils.addObjectsToModel(includeAlarmRuleList,
					alarmRuleEntityObj);
			addEntity(alarmRuleEntityObj, FEATURE_ALARM_RULE_INFORMATION);

			ClovisUtils.addObjectsToModel(includeAlarmList, alarmEntityObj);
			ModelTemplateUtils.setRDN(includeAlarmList, true);
			addEntity(alarmEntityObj, FEATURE_ALARM_INFORMATION);
		}
	}
}

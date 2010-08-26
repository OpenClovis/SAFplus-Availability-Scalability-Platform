/**
 * 
 */
package com.clovis.cw.workspace.modelTemplate;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;

import com.clovis.common.utils.ClovisFileUtils;
import com.clovis.common.utils.ClovisUtils;
import com.clovis.common.utils.ecore.EcoreCloneUtils;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.editor.ca.ResourceDataUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.genericeditor.editparts.BaseEditPart;
import com.clovis.cw.genericeditor.model.EdgeModel;
import com.clovis.cw.genericeditor.model.NodeModel;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.project.utils.FormatConversionUtils;
import com.clovis.cw.workspace.project.CwProjectPropertyPage;

/**
 * Concrete Dialog implementation to capture the information required to create
 * the component model template and creates the same.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class CreateComponentModelTemplateDialog extends
		CreateModelTemplateDialog {

	private Text _nameText;
	private CCombo _includeSourceCombo, _includeRelatedEntitiesCombo;

	/**
	 * Constructor.
	 * 
	 * @param shell
	 * @param project
	 * @param selection
	 * @param modelType
	 */
	public CreateComponentModelTemplateDialog(Shell shell, IProject project,
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
				MODEL_TYPE_COMPONENT);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.cw.workspace.modelTemplate.CreateModelTemplateDialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent) {
		Composite contents = new Composite(parent, SWT.NONE);
		GridLayout layout = new GridLayout(2, false);
		contents.setLayout(layout);
		contents.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

		Label nameLabel = new Label(contents, SWT.NONE);
		nameLabel.setText("Name:");

		_nameText = new Text(contents, SWT.BORDER);
		_nameText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		BaseEditPart bep = (BaseEditPart) _selection.getFirstElement();
		NodeModel nodeModel = (NodeModel) bep.getModel();
		_nameText.setText(EcoreUtils.getName(nodeModel.getEObject()));

		Label sourceLabel = new Label(contents, SWT.NONE);
		sourceLabel.setText("Include Source:");

		_includeSourceCombo = new CCombo(contents, SWT.BORDER);
		_includeSourceCombo
				.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		_includeSourceCombo.setItems(new String[] { "true", "false" });
		_includeSourceCombo.select(0);
		_includeSourceCombo.setEditable(false);

		Label relatedEntitiesLabel = new Label(contents, SWT.NONE);
		relatedEntitiesLabel.setText("Include related entities:");

		_includeRelatedEntitiesCombo = new CCombo(contents, SWT.BORDER);
		_includeRelatedEntitiesCombo.setLayoutData(new GridData(
				GridData.FILL_HORIZONTAL));
		_includeRelatedEntitiesCombo.setItems(new String[] { "true", "false" });
		_includeRelatedEntitiesCombo.select(0);
		_includeRelatedEntitiesCombo.setEditable(false);

		setTitle("Create component model template");
		getShell().setText("Component Model Template Details");

		return contents;
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
		NodeModel nodeModel = null;
		HashMap<String, NodeModel> nodeMap = new HashMap<String, NodeModel>();

		Iterator<Object> itr = _selection.iterator();
		while (itr.hasNext()) {
			Object obj = itr.next();

			if (obj instanceof BaseEditPart) {
				nodeModel = (NodeModel) ((BaseEditPart) obj).getModel();
				nodeMap.put(EcoreUtils.getName(nodeModel.getEObject()),
						nodeModel);
			}
		}

		boolean includeRelatedEntities = Boolean.parseBoolean(_includeRelatedEntitiesCombo.getText());

		List<EObject> compEntityList = new ArrayList<EObject>();
		List<EObject> connectionList = new ArrayList<EObject>();
		List<EObject> compAppList =  new ArrayList<EObject>();

		for (NodeModel node : nodeMap.values()) {
			EObject compObj = node.getEObject();
			compEntityList.add(EcoreCloneUtils.cloneEObject(compObj));

			String compClassName = compObj.eClass().getName();
			if (compClassName
					.equals(ComponentEditorConstants.SAFCOMPONENT_NAME)
					|| compClassName
							.equals(ComponentEditorConstants.NONSAFCOMPONENT_NAME)) {
				compAppList.add(compObj);
			}

			List<EdgeModel> connections = node.getSourceConnections();
			for (EdgeModel edgeModel : connections) {
				if (nodeMap.containsKey(EcoreUtils.getName(edgeModel
						.getTarget().getEObject()))) {
					connectionList.add(EcoreCloneUtils.cloneEObject(edgeModel
							.getEObject()));
				}
			}

			if(includeRelatedEntities) {
				List<EObject> resList = populateResourceModel(compAppList);
				populateAlarmModel(resList);
			}
		}

		EObject componentEntityObj = EcoreUtils.createEObject(
				ModelTemplateUtils.getClassFromSamePackage(nodeModel
						.getEObject(), FEATURE_COMPONENT_INFORMATION), true);
		ClovisUtils.addObjectsToModel(compEntityList, componentEntityObj);
		ClovisUtils.addObjectsToModel(connectionList, componentEntityObj);

		FormatConversionUtils.convertToResourceFormat(componentEntityObj,
				EDITOR_TYPE_COMPONENT);
		ModelTemplateUtils.setRDN(compEntityList, true);
		addEntity(componentEntityObj, FEATURE_COMPONENT_INFORMATION);

	}

	/**
	 * Populates resource model for the model template object.
	 * 
	 * @param compAppList
	 *            apps for which resource model needs to be populated.
	 * @return resource list of the resources which are added
	 */
	private List<EObject> populateResourceModel(List<EObject> compAppList) {
		List<EObject> includeCompResMapList = new ArrayList<EObject>();
		List<EObject> includeResList = new ArrayList<EObject>();

		ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
		EObject compResMapInfo = pdm.getComponentResourceMapModel()
				.getEObject();
		EObject compResLinkObj = (EObject) ((List) EcoreUtils.getValue(
				compResMapInfo, "link")).get(0);
		List compResMapList = (List) EcoreUtils.getValue(compResLinkObj,
				"linkDetail");

		EObject resInfo = pdm.getResourceModel().getEObject();
		List<EObject> resList = ResourceDataUtils.getResourcesList(resInfo);

		for (EObject compApp : compAppList) {
			EObject compResMapObj = ClovisUtils.getEobjectWithFeatureVal(
					compResMapList, "linkSource", EcoreUtils.getName(compApp));
			if (compResMapObj != null) {
				List<String> linkTargetList = (List<String>) EcoreUtils
						.getValue(compResMapObj, "linkTarget");
				if (linkTargetList.size() == 0)
					continue;

				includeCompResMapList.add(EcoreCloneUtils
						.cloneEObject(compResMapObj));

				EClass resClass = ModelTemplateUtils.getClassFromSamePackage(
						resInfo, ClassEditorConstants.SOFTWARE_RESOURCE_NAME);
				EStructuralFeature containsFeature = resClass
						.getEStructuralFeature(ClassEditorConstants.CONTAINMENT_FEATURE);
				EStructuralFeature inheritsFeature = resClass
						.getEStructuralFeature(ClassEditorConstants.INHERITANCE_FEATURE);
				EObject resObjClone = null;
				Object containsObj, inheritsObj;

				for (String linkTarget : linkTargetList) {
					EObject resObj = ClovisUtils.getObjectFrmName(resList,
							linkTarget);

					if (resObj != null || !includeResList.contains(resObj)) {
						resObjClone = EcoreCloneUtils.cloneEObject(resObj);
						containsObj = EcoreUtils.getValue(resClass,
								ClassEditorConstants.CONTAINMENT_FEATURE);
						if (containsObj != null) {
							((List) containsObj).clear();
						}
						inheritsObj = EcoreUtils.getValue(resClass,
								ClassEditorConstants.INHERITANCE_FEATURE);
						if (inheritsObj != null) {
							((List) inheritsObj).clear();
						}
						includeResList.add(resObjClone);
					}
				}
			}
		}

		EObject compResMapEntityObj = EcoreUtils.createEObject(
				ModelTemplateUtils.getClassFromSamePackage(compResMapInfo,
						"mapInformation"), true);
		EcoreUtils.setValue(compResMapEntityObj, "sourceModel", EcoreUtils
				.getValue(compResMapInfo, "sourceModel").toString());
		EcoreUtils.setValue(compResMapEntityObj, "targetModel", EcoreUtils
				.getValue(compResMapInfo, "targetModel").toString());
		EObject compResMapLinkEntityObj = EcoreUtils.createEObject(
				ModelTemplateUtils.getClassFromSamePackage(compResMapInfo,
						"link"), true);
		EcoreUtils.setValue(compResMapLinkEntityObj, "linkType", EcoreUtils
				.getValue(compResLinkObj, "linkType").toString());

		EStructuralFeature compResMapLinkRef = compResMapEntityObj.eClass()
				.getEStructuralFeature("link");
		((List<EObject>) compResMapEntityObj.eGet(compResMapLinkRef))
				.add(compResMapLinkEntityObj);

		ClovisUtils.addObjectsToModel(includeCompResMapList,
				compResMapLinkEntityObj);
		addEntity(compResMapEntityObj,
				FEATURE_COMPONENT_RESOURCE_MAP_INFORMATION);

		EObject resourceEntityObj = EcoreUtils.createEObject(
				ModelTemplateUtils.getClassFromSamePackage(resInfo,
						FEATURE_RESOURCE_INFORMATION), true);
		ClovisUtils.addObjectsToModel(includeResList, resourceEntityObj);
		FormatConversionUtils.convertToResourceFormat(resourceEntityObj,
				EDITOR_TYPE_RESOURCE);
		ModelTemplateUtils.setRDN(includeResList, true);
		addEntity(resourceEntityObj, FEATURE_RESOURCE_INFORMATION);

		return includeResList;
	}

	/**
	 * Populates alarm model for the model template object.
	 * 
	 * @param resList
	 *            list of resources for which alarm model needs to be populated.
	 */
	private void populateAlarmModel(List<EObject> resList) {
		List<EObject> includeResAlarmMapList = new ArrayList<EObject>();
		List<EObject> includeAlarmList = new ArrayList<EObject>();
		List<EObject> includeAlarmRuleList = new ArrayList<EObject>();
		List<String> associatedAlarmIDList = new ArrayList<String>();

		ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
		EObject resAlarmMapInfo = pdm.getResourceAlarmMapModel().getEObject();
		EObject resAlarmLinkObj = (EObject) ((List) EcoreUtils.getValue(
				resAlarmMapInfo, "link")).get(0);
		List resAlarmMapList = (List) EcoreUtils.getValue(resAlarmLinkObj,
				"linkDetail");

		EObject alarmInfo = pdm.getAlarmProfiles().getEObject();
		List alarmList = ModelTemplateUtils
				.getEObjListFromChildReferences(alarmInfo);

		EObject alarmRuleInfo = pdm.getAlarmRules().getEObject();
		List alarmRuleList = ModelTemplateUtils
				.getEObjListFromChildReferences(alarmRuleInfo);

		for (EObject resObj : resList) {
			String resName = EcoreUtils.getName(resObj);
			EObject resAlarmMapObj = (EObject) ModelTemplateUtils
					.getObjectFrmFeature(resAlarmMapList, "linkSource", resName);
			if (resAlarmMapObj != null) {

				List<String> linkTargetList = (List<String>) EcoreUtils
						.getValue(resAlarmMapObj, "linkTarget");
				if (linkTargetList.size() == 0)
					continue;

				includeResAlarmMapList.add(EcoreCloneUtils
						.cloneEObject(resAlarmMapObj));
			}

			EObject alarmRuleObj = ClovisUtils.getObjectFrmName(alarmRuleList,
					resName);
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
		EcoreUtils.setValue(resAlarmMapLinkEntityObj, "linkType", EcoreUtils
				.getValue(resAlarmLinkObj, "linkType").toString());

		EStructuralFeature resAlarmMapLinkRef = resAlarmMapEntityObj.eClass()
				.getEStructuralFeature("link");
		((List<EObject>) resAlarmMapEntityObj.eGet(resAlarmMapLinkRef))
				.add(resAlarmMapLinkEntityObj);

		ClovisUtils.addObjectsToModel(includeResAlarmMapList,
				resAlarmMapLinkEntityObj);
		addEntity(resAlarmMapEntityObj, FEATURE_RESOURCE_ALARM_MAP_INFORMATION);

		EObject alarmRuleEntityObj = EcoreUtils.createEObject(
				ModelTemplateUtils.getClassFromSamePackage(alarmRuleInfo,
						FEATURE_ALARM_RULE_INFORMATION), true);

		ClovisUtils.addObjectsToModel(includeAlarmRuleList, alarmRuleEntityObj);
		addEntity(alarmRuleEntityObj, FEATURE_ALARM_RULE_INFORMATION);

		EObject alarmEntityObj = EcoreUtils.createEObject(ModelTemplateUtils
				.getClassFromSamePackage(alarmInfo, FEATURE_ALARM_INFORMATION),
				true);
		ClovisUtils.addObjectsToModel(includeAlarmList, alarmEntityObj);
		ModelTemplateUtils.setRDN(includeAlarmList, true);
		addEntity(alarmEntityObj, FEATURE_ALARM_INFORMATION);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.cw.workspace.modelTemplate.CreateModelTemplateDialog#saveModelTemplate()
	 */
	@Override
	protected boolean saveModelTemplate() {
		if (!super.saveModelTemplate()) {
			return false;
		}

		if (_includeSourceCombo.getText().equals("true")) {
			return includeSourceForModelTemplate();
		}

		return true;
	}

	/**
	 * Includes source code for the model template.
	 * 
	 * @return false if any error otherwise true
	 */
	private boolean includeSourceForModelTemplate() {

		String projectSourceLocation = CwProjectPropertyPage
				.getSourceLocation(_project);
		if (projectSourceLocation.equals("")
				|| !new File(projectSourceLocation).exists()) {
			return true;
		}

		NodeModel nodeModel;
		String compName, nodeClass;
		String modelTemplateSourceLocation = ModelTemplateConstants.MODEL_TEMPLATE_FOLDER_PATH
				+ File.separator + EcoreUtils.getName(_modelTemplateObject)
				+ File.separator + "src";

		for (Object entity : _selection.toArray()) {
			nodeModel = ((NodeModel) ((BaseEditPart) entity)
					.getModel());
			nodeClass = nodeModel.getName();
			if (nodeClass.equals(
					ComponentEditorConstants.SAFCOMPONENT_NAME)
					|| nodeClass.equals(
							ComponentEditorConstants.NONSAFCOMPONENT_NAME)) {

				compName = EcoreUtils.getName(nodeModel.getEObject());
				File sourceFolder = new File(projectSourceLocation
						+ File.separator + "app" + File.separator + compName);

				if (sourceFolder.exists()) {

					File appSrcFolder = new File(modelTemplateSourceLocation + File.separator + compName);
					if (appSrcFolder.exists()) {
						appSrcFolder.delete();
					}

					appSrcFolder.mkdirs();
					try {
						ClovisFileUtils
								.copyDirectory(sourceFolder, appSrcFolder, false);
					} catch (IOException e) {
						LOG.error("Error saving application source code for the Model Template.", e);
						return false;
					}
				}
			}
		}

		return true;
	}
}

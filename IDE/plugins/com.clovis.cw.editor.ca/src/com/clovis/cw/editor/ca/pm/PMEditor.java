/**
 * 
 */
package com.clovis.cw.editor.ca.pm;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.draw2d.ColorConstants;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.ExpandBar;
import org.eclipse.swt.widgets.ExpandItem;
import org.eclipse.ui.IEditorInput;
import org.eclipse.ui.IEditorSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.EditorPart;

import com.clovis.common.utils.ClovisUtils;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.cw.editor.ca.ResourceDataUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.manageability.common.ResourceTreeNode;
import com.clovis.cw.editor.ca.snmp.MibImportThread;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.project.utils.FormatConversionUtils;

/**
 * PM Editor.
 * 
 * @author Suraj Rajyaguru
 */
public class PMEditor extends EditorPart {

	public static final Color COLOR_PMEDITOR_BACKGROUND = ColorConstants.white;

	private PMEditorInput _editorInput;
	private ProjectDataModel _pdm;
	private Model _alarmAssociationModel;
	private Model _resourceModel;
	private Model _resAlarmMapModel;
	private Model _alarmRuleViewModel;
	private boolean _isDirty = false;

	private ResourceBrowserComposite _resourceBrowserComposite;
	private AttributeStatisticComposite _attributeStatisticComposite;
	private ThresholdCrossingComposite _thresholdCrossingComposite;
	private AlarmProfilesComposite _alarmProfilesComposite;
	private AlarmAssociationComposite _alarmAssociationComposite;
	private ExpandItem _thresholdCrossingItem;

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.part.EditorPart#doSave(org.eclipse.core.runtime.IProgressMonitor)
	 */
	@Override
	public void doSave(IProgressMonitor monitor) {
		_alarmAssociationModel.save(true);
		_resAlarmMapModel.save(true);
		_alarmRuleViewModel.save(true);
		GenericEditorInput caInput = (GenericEditorInput) _pdm
				.getCAEditorInput();

		if (caInput != null && caInput.getEditor() != null
				&& caInput.getEditor().isDirty()) {
			caInput.getEditor().doSave(null);

		} else {
    		FormatConversionUtils.convertToResourceFormat(
					(EObject) _resourceModel.getEList().get(0),
					"Resource Editor");
			_resourceModel.save(true);

			if (caInput != null && caInput.getEditor() != null
					&& caInput.getEditor().isDirty()) {
				caInput.getEditor().doSave(null);
			}
    	}

		setDirty(false);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.part.EditorPart#doSaveAs()
	 */
	@Override
	public void doSaveAs() {
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.part.EditorPart#init(org.eclipse.ui.IEditorSite,
	 *      org.eclipse.ui.IEditorInput)
	 */
	@Override
	public void init(IEditorSite site, IEditorInput input)
			throws PartInitException {
		setSite(site);
		setInput(input);
		setPartName(_pdm.getProject().getName() + " - " + getPartName());
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.part.EditorPart#isDirty()
	 */
	@Override
	public boolean isDirty() {
		return _isDirty;
	}

	/**
	 * Sets the dirty state of the editor.
	 * 
	 * @param value
	 */
	public void setDirty(boolean value) {
		_isDirty = value;
		firePropertyChange(PROP_DIRTY);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.part.EditorPart#isSaveAsAllowed()
	 */
	@Override
	public boolean isSaveAsAllowed() {
		return false;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent) {
		SashForm container = new SashForm(parent, SWT.HORIZONTAL);
		container.setLayout(new GridLayout(2, false));

		Composite leftComposite = new Composite(container, SWT.NONE);
		leftComposite.setBackground(COLOR_PMEDITOR_BACKGROUND);
		leftComposite.setLayout(new GridLayout());
		leftComposite
				.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

		_resourceBrowserComposite = new ResourceBrowserComposite(leftComposite,
				this);

		Composite rightComposite = new Composite(container, SWT.NONE);
		rightComposite.setBackground(COLOR_PMEDITOR_BACKGROUND);
		rightComposite.setLayout(new GridLayout());
		rightComposite.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true,
				true));

		ExpandBar expandBar = new ExpandBar(rightComposite, SWT.BORDER);
		expandBar.setBackground(COLOR_PMEDITOR_BACKGROUND);
		expandBar.setBackgroundMode(SWT.INHERIT_FORCE);
		expandBar.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		expandBar.addExpandListener(new PMExpandBarListener());

		_thresholdCrossingComposite = new ThresholdCrossingComposite(expandBar, this);
		_alarmProfilesComposite = _thresholdCrossingComposite.getAlarmProfilesComposite();
		_alarmAssociationComposite = _thresholdCrossingComposite.getAlarmAssociationComposite();

		_thresholdCrossingItem = new ExpandItem(expandBar, SWT.NONE);
		_thresholdCrossingItem.setHeight(_thresholdCrossingComposite.computeSize(
				SWT.DEFAULT, SWT.DEFAULT).y);
		_thresholdCrossingItem.setControl(_thresholdCrossingComposite);
		_thresholdCrossingItem.setText("Threshold Crossing Association");

		Composite attributeStatisticComposite = new Composite(expandBar,
				SWT.NONE);
		attributeStatisticComposite.setBackground(COLOR_PMEDITOR_BACKGROUND);
		GridLayout layout = new GridLayout();
		layout.marginWidth = layout.marginHeight = 0;
		attributeStatisticComposite.setLayout(layout);
		attributeStatisticComposite.setLayoutData(new GridData(SWT.FILL,
				SWT.FILL, true, true));

		_attributeStatisticComposite = new AttributeStatisticComposite(
				attributeStatisticComposite, this);

		ExpandItem attributeStatisticItem = new ExpandItem(expandBar, SWT.NONE);
		attributeStatisticItem.setHeight(attributeStatisticComposite
				.computeSize(SWT.DEFAULT, SWT.DEFAULT).y);
		attributeStatisticItem.setControl(attributeStatisticComposite);
		attributeStatisticItem.setText("Statistics Attributes");

		container.setWeights(new int[] { 60, 40 });
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus() {
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.part.EditorPart#setInput(org.eclipse.ui.IEditorInput)
	 */
	@Override
	protected void setInput(IEditorInput input) {
		super.setInput(input);
		_editorInput = (PMEditorInput) input;
		_pdm = _editorInput.getProjectDataModel();

		_alarmAssociationModel = _pdm.getAlarmAssociationModel().getViewModel();
		_resourceModel = _pdm.getCAModel().getViewModel();
		_resAlarmMapModel = _pdm.getResourceAlarmMapModel().getViewModel();
		_alarmRuleViewModel = _pdm.getAlarmRules().getViewModel();
	}

	/**
	 * Returns project data model.
	 * 
	 * @return
	 */
	public ProjectDataModel getProjectDataModel() {
		return _pdm;
	}

	/**
	 * Associates selected alarms to the selected resources/attributes.
	 * 
	 * @param lowerBound
	 * @param upperBound
	 * @param severity 
	 */
	public void associateAlarms(String thresholdValue, String severity) {
		Object[] checkedResources = _resourceBrowserComposite.getCheckedItems();
		Object[] checkedAlarms = _alarmProfilesComposite.getCheckedItems();

		List<String> editorResourceNames = ClovisUtils.getFeatureValueList(
				ResourceDataUtils.getMoList(_resourceModel.getEList()), "name");
		HashMap<String, List> mibFileResMap = new HashMap<String, List>();
		List resList;
		ResourceTreeNode resourceNode, attributeNode, node;

		for (int i = 0; i < checkedResources.length; i++) {
			node = (ResourceTreeNode) checkedResources[i];

			if (node.getNode().isScalarNode()
					|| node.getNode().isTableColumnNode()) {
				resourceNode = node.getParent();

			} else {
				resourceNode = node;
			}

			if (!editorResourceNames.contains(resourceNode.getName())) {
				resList = mibFileResMap.get(resourceNode
						.getMibFileName());

				if (resList == null) {
					resList = new ArrayList();
					mibFileResMap.put(resourceNode.getMibFileName(),
							resList);
				}
				resList.add(resourceNode.getNode());
			}
		}

		Iterator<String> mibFileIterator = mibFileResMap.keySet().iterator();
		String mibFile;

		while (mibFileIterator.hasNext()) {
			mibFile = mibFileIterator.next();
			Display.getDefault().syncExec(
					new MibImportThread(_pdm.getProject(), mibFile,
							mibFileResMap.get(mibFile)));
		}

		EClass resourceClass = (EClass) _alarmAssociationModel.getEPackage()
				.getEClassifier("Resource");
		EClass attributeClass = (EClass) _alarmAssociationModel.getEPackage()
				.getEClassifier("Attribute");
		EObject resourceObj, attributeObj;
		List<EObject> resourceList = (List<EObject>) EcoreUtils.getValue(
				(EObject) _alarmAssociationModel.getEList().get(0), "resource");
		List<EObject> attributeList;
		boolean isDirty = false;

		for (int i = 0; i < checkedResources.length; i++) {
			node = (ResourceTreeNode) checkedResources[i];

			if (node.getNode().isScalarNode()
					|| node.getNode().isTableColumnNode()) {
				resourceNode = node.getParent();
				attributeNode = node;
				if(!confirmAndMarkPM(resourceNode.getName(), attributeNode.getName())) {
					continue;
				}

			} else {
				resourceNode = node;
				attributeNode = null;
			}

			addResAlarmMapping(resourceNode.getName(), checkedAlarms);
			addAlarmGenRule(resourceNode.getName(), checkedAlarms);
			isDirty = true;
			
			resourceObj = ClovisUtils.getEobjectWithFeatureVal(resourceList, "name",
					resourceNode.getName());

			if (resourceObj == null) {
				resourceObj = EcoreUtils.createEObject(resourceClass, true);
				resourceList.add(resourceObj);
				EcoreUtils
						.setValue(resourceObj, "name", resourceNode.getName());

				if (attributeNode == null) {
					addAlarmAssociation(resourceObj, checkedAlarms, false,
							thresholdValue, severity);

				} else {
					attributeList = (List<EObject>) EcoreUtils.getValue(
							resourceObj, "attribute");
					attributeObj = EcoreUtils.createEObject(attributeClass,
							true);
					attributeList.add(attributeObj);

					EcoreUtils.setValue(attributeObj, "name", attributeNode
							.getName());
					addAlarmAssociation(attributeObj, checkedAlarms, false,
							thresholdValue, severity);
				}

			} else {
				if (attributeNode == null) {
					addAlarmAssociation(resourceObj, checkedAlarms, true,
							thresholdValue, severity);

				} else {
					attributeList = (List<EObject>) EcoreUtils.getValue(
							resourceObj, "attribute");
					attributeObj = ClovisUtils.getEobjectWithFeatureVal(attributeList,
							"name", attributeNode.getName());

					if (attributeObj == null) {
						attributeObj = EcoreUtils.createEObject(attributeClass,
								true);
						attributeList.add(attributeObj);

						EcoreUtils.setValue(attributeObj, "name", attributeNode
								.getName());
						addAlarmAssociation(attributeObj, checkedAlarms, false,
								thresholdValue, severity);

					} else {
						addAlarmAssociation(attributeObj, checkedAlarms, true,
								thresholdValue, severity);
					}
				}
			}
		}

		if (!_isDirty) {
			setDirty(isDirty);
		}
	}

	/**
	 * Checks if given attribute is PM attribute. If not prompts user to mark it
	 * as PM and updates accordingly.
	 * 
	 * @param resName
	 * @param attrName
	 * @return
	 */
	private boolean confirmAndMarkPM(String resName, String attrName) {
		EObject resObj = ClovisUtils.getEobjectWithFeatureVal(ResourceDataUtils
				.getMoList(_resourceModel.getEList()), "name", resName);
		EObject attrObj, pmAttrObj;

		EObject pmObj = (EObject) EcoreUtils.getValue(resObj,
				ClassEditorConstants.RESOURCE_PM);
		List pmAttrList = (List) EcoreUtils.getValue(pmObj,
				ClassEditorConstants.CLASS_ATTRIBUTES);
		attrObj = ClovisUtils.getEobjectWithFeatureVal(pmAttrList, "name", attrName);
		if (attrObj != null) {
			return true;
		}

		if (!MessageDialog.openQuestion(getSite().getShell(),
				"Mark Atrribute as PM",
				"Alarm can not be associated to a non PM attribute."
						+ "\nDo you want to mark '" + attrName + "' as PM?")) {
			return false;
		}

		EClass pmAttrClass = (EClass) _resourceModel.getEPackage()
				.getEClassifier("PMAttribute");

		EObject provObj = (EObject) EcoreUtils.getValue(resObj,
				ClassEditorConstants.RESOURCE_PROVISIONING);
		List provAttrList = (List) EcoreUtils.getValue(provObj,
				ClassEditorConstants.CLASS_ATTRIBUTES);
		attrObj = ClovisUtils.getEobjectWithFeatureVal(provAttrList, "name", attrName);
		if (attrObj != null) {
			provAttrList.remove(attrObj);
			if (provAttrList.size() == 0) {
				EcoreUtils.setValue(provObj, "isEnabled", "false");
			}

			pmAttrObj = EcoreUtils.createEObject(pmAttrClass, true);
			EcoreUtils.copyEObject(attrObj, pmAttrObj);
			pmAttrList.add(pmAttrObj);
			EcoreUtils.setValue(pmObj, "isEnabled", "true");
			return true;
		}

		List genAttrList = (List) EcoreUtils.getValue(resObj,
				ClassEditorConstants.CLASS_ATTRIBUTES);
		attrObj = ClovisUtils.getEobjectWithFeatureVal(genAttrList, "name", attrName);
		if (attrObj != null) {
			genAttrList.remove(attrObj);

			pmAttrObj = EcoreUtils.createEObject(pmAttrClass, true);
			EcoreUtils.copyEObject(attrObj, pmAttrObj);
			pmAttrList.add(pmAttrObj);
			EcoreUtils.setValue(pmObj, "isEnabled", "true");
			return true;
		}

		return false;
	}

	/**
	 * Adds resource alarm mapping.
	 * 
	 * @param resName
	 * @param checkedAlarms
	 */
	private void addResAlarmMapping(String resName, Object[] checkedAlarms) {
		EClass linkDetailClass = (EClass) _resAlarmMapModel.getEPackage()
				.getEClassifier("linkObjectType");

		Object links = EcoreUtils.getValue(_resAlarmMapModel.getEObject(), "link");
		if (links != null) {
			EObject linkObj = ((List<EObject>) links).get(0);

			Object linkDetails = EcoreUtils.getValue(linkObj, "linkDetail");
			if (linkDetails != null) {
				List<EObject> linkDetailList = (List<EObject>) linkDetails;

				EObject linkDetail = ClovisUtils.getEobjectWithFeatureVal(linkDetailList,
						"linkSource", resName);
				if (linkDetail == null) {
					linkDetail = EcoreUtils
							.createEObject(linkDetailClass, true);
					EcoreUtils.setValue(linkDetail, "linkSource", resName);
					linkDetailList.add(linkDetail);
				}

				Object linkTargets = EcoreUtils.getValue(linkDetail,
						"linkTarget");
				if (linkTargets != null) {
					List linkTargetList = (List) linkTargets;
					String alarmID;

					EObject resObj = ClovisUtils.getEobjectWithFeatureVal(ResourceDataUtils
							.getMoList(_resourceModel.getEList()), "name",
							resName);
					boolean alarmAdded = false;

					for (int i = 0; i < checkedAlarms.length; i++) {
						alarmID = EcoreUtils.getValue(
								(EObject) checkedAlarms[i], "alarmID")
								.toString();

						if (!linkTargetList.contains(alarmID)) {
							linkTargetList.add(alarmID);
							alarmAdded = true;
						}
					}

					if(alarmAdded) {
						EcoreUtils.setValue((EObject) EcoreUtils.getValue(
								resObj, "alarmManagement"), "isEnabled",
								"true");
					}
				}
			}
		}
	}
	
	/**
	 * Adds default generation rule for associated alarms.
	 * 
	 * @param resName
	 * @param checkedAlarms
	 */
	private void addAlarmGenRule(String resName, Object[] checkedAlarms) {
		EObject alarmRuleInfoObj = (EObject)_alarmRuleViewModel.getEList().get(0);
		List alarmRulesResList = (EList) EcoreUtils.getValue(alarmRuleInfoObj, 
        		ClassEditorConstants.ALARM_RESOURCE);
		
		EObject alarmRuleResObj = null;
		for(int i = 0; 	alarmRulesResList != null && i <  alarmRulesResList.size(); i++){
			EObject obj  = (EObject)alarmRulesResList.get(i);
        	String name = (String) EcoreUtils.getValue(obj,
        			ClassEditorConstants.ALARM_RESOURCE_NAME);
        	if(resName.equals(name)){
        		alarmRuleResObj = obj;
        		break;
        	}
		}
		if(alarmRuleResObj == null){
			List<EObject> ruleResObjList = (EList) EcoreUtils.getValue(alarmRuleInfoObj, ClassEditorConstants.ALARM_RESOURCE);
			alarmRuleResObj = EcoreUtils.createEObject( (EClass) _alarmRuleViewModel.getEPackage()
			.getEClassifier(ClassEditorConstants.ALARM_RESOURCE), true);
			EcoreUtils.setValue(alarmRuleResObj, ClassEditorConstants.ALARM_RESOURCE_NAME, resName);
			ruleResObjList.add(alarmRuleResObj);
		}
		EReference alarmRef = (EReference) alarmRuleResObj.eClass()
   		.getEStructuralFeature(ClassEditorConstants.ALARM_ALARMOBJ);
		EList alarmList = (EList) alarmRuleResObj.eGet(alarmRef);
		List<String> existingAlarms = new ArrayList<String>();
		for (int i = 0; i < alarmList.size(); i++) {
			EObject obj = (EObject)alarmList.get(i);
			String alarmID = EcoreUtils.getValue(obj,
					ClassEditorConstants.ALARM_ID).toString();
			existingAlarms.add(alarmID);
		}
		for (int i = 0; i < checkedAlarms.length; i++) {
			String alarmID = EcoreUtils.getValue(
					(EObject) checkedAlarms[i], ClassEditorConstants.ALARM_ID)
					.toString();
			if (!existingAlarms.contains(alarmID)) {
				EObject newObj = EcoreUtils.createEObject(alarmRef
						.getEReferenceType(), true);
				EObject genRule = (EObject) EcoreUtils.getValue(newObj,
						ClassEditorConstants.ALARM_GENERATIONRULE);
				EList genIds = (EList) EcoreUtils.getValue(genRule, "alarmIDs");
				genIds.add(alarmID);
				EcoreUtils.setValue(newObj, ClassEditorConstants.ALARM_ID,
						alarmID);
				alarmList.add(newObj);
			}
		}
	}
	
	/**
	 * Adds the alarm association for the given eObj.
	 * 
	 * @param eObj
	 * @param checkedAlarms
	 * @param checkExisting
	 * @param lowerBound
	 * @param upperBound
	 * @param severity 
	 */
	private void addAlarmAssociation(EObject eObj, Object[] checkedAlarms,
			boolean checkExisting, String thresholdValue, String severity) {
		List<EObject> alarmList = (List<EObject>) EcoreUtils.getValue(eObj,
				"alarm");

		EClass alarmClass = (EClass) _alarmAssociationModel.getEPackage()
				.getEClassifier("Alarm");
		EObject alarmObj;
		String alarmID;

		if (checkExisting) {
			for (int i = 0; i < alarmList.size(); i++) {
				alarmObj = alarmList.get(i);

				if (EcoreUtils.getValue(alarmObj, "thresholdValue").equals(
						thresholdValue)) {
					MessageDialog
							.openInformation(
									getSite().getShell(),
									"Alarm Association",
									"Alarm with these Threshold value is already configured for the selected entity.");
					return;
				}
			}

			for (int i = 0; i < checkedAlarms.length; i++) {
				alarmID = EcoreUtils.getValue((EObject) checkedAlarms[i],
						"alarmID").toString();
				alarmObj = ClovisUtils.getEobjectWithFeatureVal(alarmList, "alarmID",
						alarmID);

				if (alarmObj == null) {
					alarmObj = EcoreUtils.createEObject(alarmClass, true);
					alarmList.add(alarmObj);
					EcoreUtils.setValue(alarmObj, "alarmID", alarmID);
				}

				EcoreUtils.setValue(alarmObj, "thresholdValue", thresholdValue);
				EcoreUtils.setValue(alarmObj, "severity", severity);
			}

		} else {
			for (int i = 0; i < checkedAlarms.length; i++) {
				alarmID = EcoreUtils.getValue((EObject) checkedAlarms[i],
						"alarmID").toString();

				alarmObj = EcoreUtils.createEObject(alarmClass, true);
				alarmList.add(alarmObj);

				EcoreUtils.setValue(alarmObj, "alarmID", alarmID);
				EcoreUtils.setValue(alarmObj, "thresholdValue", thresholdValue);
				EcoreUtils.setValue(alarmObj, "severity", severity);
			}
		}
	}

	/**
	 * Dissociates the selected alarms from the selected resources/attributes.
	 */
	public void dissociateAlarms() {
		Object[] checkedResources = _resourceBrowserComposite.getCheckedItems();
		Object[] checkedAlarms = _alarmProfilesComposite.getCheckedItems();

		EObject resourceObj, attributeObj, alarmObj;

		List<EObject> alarmAssociationResList = (List<EObject>) EcoreUtils
				.getValue((EObject) _alarmAssociationModel.getEList().get(0),
						"resource");
		List<EObject> attributeList, alarmList;

		String alarmID;
		ResourceTreeNode resourceNode, attributeNode, node;

		for (int i = 0; i < checkedResources.length; i++) {
			node = (ResourceTreeNode) checkedResources[i];

			if (node.getNode().isScalarNode()
					|| node.getNode().isTableColumnNode()) {
				resourceNode = node.getParent();
				attributeNode = node;

			} else {
				resourceNode = node;
				attributeNode = null;
			}

			resourceObj = ClovisUtils.getEobjectWithFeatureVal(alarmAssociationResList,
					"name", resourceNode.getName());

			if (resourceObj != null) {
				if (attributeNode != null) {
					attributeList = (List<EObject>) EcoreUtils.getValue(
							resourceObj, "attribute");
					attributeObj = ClovisUtils.getEobjectWithFeatureVal(attributeList,
							"name", attributeNode.getName());

					if (attributeObj != null) {
						alarmList = (List<EObject>) EcoreUtils.getValue(
								attributeObj, "alarm");
					} else {
						continue;
					}

				} else {
					alarmList = (List<EObject>) EcoreUtils.getValue(
							resourceObj, "alarm");
				}

				for (int j = 0; j < checkedAlarms.length; j++) {
					alarmID = EcoreUtils.getValue((EObject) checkedAlarms[j],
							"alarmID").toString();
					alarmObj = ClovisUtils.getEobjectWithFeatureVal(alarmList, "alarmID",
							alarmID);

					if (alarmObj != null) {
						alarmList.remove(alarmObj);
					}
				}
			}
		}

		if (!_isDirty) {
			setDirty(true);
		}
	}

	/**
	 * Updates given statistic to the selected attributes.
	 * 
	 * @param mark
	 * @param cached
	 * @param persistent
	 * @param pm
	 */
	public void updateStatistic(boolean mark, boolean cached, boolean persistent, boolean pm) {
		Object[] checkedResources = _resourceBrowserComposite.getCheckedItems();
		ResourceTreeNode node, resourceNode;

		HashMap<String, List> mibFileResMap = new HashMap<String, List>();
		HashMap<String, List<String>> resAttributeMap = new HashMap<String, List<String>>();
		List resList;
		List<String> attrList;

		List<String> editorResourceNames = ClovisUtils.getFeatureValueList(
				ResourceDataUtils.getMoList(_resourceModel.getEList()), "name");

		for (int i = 0; i < checkedResources.length; i++) {
			node = (ResourceTreeNode) checkedResources[i];

			if (node.getNode().isScalarNode()
					|| node.getNode().isTableColumnNode()) {
				resourceNode = node.getParent();
				attrList = resAttributeMap.get(resourceNode.getName());

				if (attrList == null) {
					attrList = new ArrayList<String>();
					resAttributeMap.put(resourceNode.getName(), attrList);

					if (!editorResourceNames.contains(resourceNode.getName())) {
						resList = mibFileResMap.get(resourceNode
								.getMibFileName());

						if (resList == null) {
							resList = new ArrayList();
							mibFileResMap.put(resourceNode.getMibFileName(),
									resList);
						}
						resList.add(resourceNode.getNode());
					}
				}
				attrList.add(node.getName());
			}
		}

		Iterator<String> mibFileIterator = mibFileResMap.keySet().iterator();
		String mibFile;

		while (mibFileIterator.hasNext()) {
			mibFile = mibFileIterator.next();
			Display.getDefault().syncExec(
					new MibImportThread(_pdm.getProject(), mibFile,
							mibFileResMap.get(mibFile)));
		}

		HashMap<Object, EObject> resNameObjMap = ClovisUtils.getFeatureValueObjMap(
				ResourceDataUtils.getMoList(_resourceModel.getEList()), "name");
		String resName;
		EObject resObj, attrObj;
		EClass pmAttributeClass = (EClass) _resourceModel.getEPackage()
				.getEClassifier("PMAttribute");
		EClass provAttributeClass = (EClass) _resourceModel.getEPackage()
				.getEClassifier("ProvAttribute");
		EObject pmAttributeObj;

		Iterator<String> resAttributeIterator = resAttributeMap.keySet()
				.iterator();
		while (resAttributeIterator.hasNext()) {
			resName = resAttributeIterator.next();
			resObj = resNameObjMap.get(resName);

			List genAttrList = (List) EcoreUtils.getValue(resObj,
					ClassEditorConstants.CLASS_ATTRIBUTES);
			EObject provObj = (EObject) EcoreUtils.getValue(resObj,
					ClassEditorConstants.RESOURCE_PROVISIONING);
			List provAttrList = (List) EcoreUtils.getValue(provObj,
					ClassEditorConstants.CLASS_ATTRIBUTES);
			EObject pmObj = (EObject) EcoreUtils.getValue(resObj,
					ClassEditorConstants.RESOURCE_PM);
			List pmAttrList = (List) EcoreUtils.getValue(pmObj,
					ClassEditorConstants.CLASS_ATTRIBUTES);

			attrList = resAttributeMap.get(resName);
			for (int i = 0; i < attrList.size(); i++) {
				attrObj = ClovisUtils.getEobjectWithFeatureVal(genAttrList, "name",
						attrList.get(i));

				if (attrObj != null) {
					if (mark) {
						if (cached) {
							EcoreUtils.setValue(attrObj,
									ClassEditorConstants.ATTRIBUTE_CACHING,
									"true");
						}
						if (persistent) {
							EcoreUtils.setValue(attrObj,
									ClassEditorConstants.ATTRIBUTE_PERSISTENCY,
									"true");
						}

						if (pm) {
							genAttrList.remove(attrObj);

							pmAttributeObj = EcoreUtils.createEObject(
									pmAttributeClass, true);
							EcoreUtils.copyEObject(attrObj, pmAttributeObj);
							pmAttrList.add(pmAttributeObj);
							EcoreUtils.setValue(pmObj, "isEnabled", "true");
						}

					} else {
						if (cached) {
							EcoreUtils.setValue(attrObj,
									ClassEditorConstants.ATTRIBUTE_CACHING,
									"false");
						}
						if (persistent) {
							EcoreUtils.setValue(attrObj,
									ClassEditorConstants.ATTRIBUTE_PERSISTENCY,
									"false");
						}
					}

				} else {
					attrObj = ClovisUtils.getEobjectWithFeatureVal(provAttrList, "name",
							attrList.get(i));

					if (attrObj != null) {
						if (mark) {
							if (cached) {
								EcoreUtils.setValue(attrObj,
										ClassEditorConstants.ATTRIBUTE_CACHING,
										"true");
							}
							if (persistent) {
								EcoreUtils
										.setValue(
												attrObj,
												ClassEditorConstants.ATTRIBUTE_PERSISTENCY,
												"true");
							}

							if (pm) {
								provAttrList.remove(attrObj);
								if (provAttrList.size() == 0) {
									EcoreUtils.setValue(provObj, "isEnabled",
											"false");
								}

								pmAttributeObj = EcoreUtils.createEObject(
										pmAttributeClass, true);
								EcoreUtils.copyEObject(attrObj, pmAttributeObj);
								pmAttrList.add(pmAttributeObj);
								EcoreUtils.setValue(pmObj, "isEnabled", "true");
							}

						} else {
							if (cached) {
								EcoreUtils.setValue(attrObj,
										ClassEditorConstants.ATTRIBUTE_CACHING,
										"false");
							}
							if (persistent) {
								EcoreUtils
										.setValue(
												attrObj,
												ClassEditorConstants.ATTRIBUTE_PERSISTENCY,
												"false");
							}
						}

					} else {
						attrObj = ClovisUtils.getEobjectWithFeatureVal(pmAttrList, "name",
								attrList.get(i));
						if (attrObj != null) {
							if (mark) {
								if (cached) {
									EcoreUtils
											.setValue(
													attrObj,
													ClassEditorConstants.ATTRIBUTE_CACHING,
													"true");
								}
								if (persistent) {
									EcoreUtils
											.setValue(
													attrObj,
													ClassEditorConstants.ATTRIBUTE_PERSISTENCY,
													"true");
								}

							} else {
								if (cached) {
									EcoreUtils
											.setValue(
													attrObj,
													ClassEditorConstants.ATTRIBUTE_CACHING,
													"false");
								}
								if (persistent) {
									EcoreUtils
											.setValue(
													attrObj,
													ClassEditorConstants.ATTRIBUTE_PERSISTENCY,
													"false");
								}

								if (pm) {
									pmAttrList.remove(attrObj);
									if (pmAttrList.size() == 0) {
										EcoreUtils.setValue(pmObj, "isEnabled",
												"false");
									}

									EObject provAttributeObj = EcoreUtils
											.createEObject(provAttributeClass,
													true);
									EcoreUtils.copyEObject(attrObj,
											provAttributeObj);
									provAttrList.add(provAttributeObj);
									EcoreUtils.setValue(provObj, "isEnabled",
											"true");
								}
							}
						}
					}
				}
			}
		}

		if (!_isDirty) {
			setDirty(true);
		}
	}

	/**
	 * Returns resource browser composite.
	 * 
	 * @return the _resourceBrowserComposite
	 */
	public ResourceBrowserComposite getResourceBrowserComposite() {
		return _resourceBrowserComposite;
	}

	/**
	 * Returns alarm association composite.
	 * 
	 * @return the _alarmAssociationComposite
	 */
	public AlarmAssociationComposite getAlarmAssociationComposite() {
		return _alarmAssociationComposite;
	}

	/**
	 * Returns alarm profiles composite.
	 * 
	 * @return the _alarmProfilesComposite
	 */
	public AlarmProfilesComposite getAlarmProfilesComposite() {
		return _alarmProfilesComposite;
	}

	/**
	 * Returns Threshold crossing composite.
	 * 
	 * @return the _thresholdCrossingComposite
	 */
	public ThresholdCrossingComposite getThresholdCrossingComposite() {
		return _thresholdCrossingComposite;
	}

	/**
	 * Returns Threshold crossing item.
	 * 
	 * @return the _thresholdCrossingItem
	 */
	public ExpandItem getThresholdCrossingItem() {
		return _thresholdCrossingItem;
	}

	/**
	 * Returns resource model.
	 * 
	 * @return the _resourceModel
	 */
	public Model getResourceModel() {
		return _resourceModel;
	}

	/**
	 * Returns alarm association model.
	 * 
	 * @return the _alarmAssociationModel
	 */
	public Model getAlarmAssociationModel() {
		return _alarmAssociationModel;
	}

	/**
	 * Returns true if resource browser selection is available.
	 * 
	 * @return
	 */
	public boolean isResourceBrowserSelectionAvailable() {
		if (_resourceBrowserComposite.getCheckedItems().length == 0) {
			return false;
		}
		return true;
	}
}

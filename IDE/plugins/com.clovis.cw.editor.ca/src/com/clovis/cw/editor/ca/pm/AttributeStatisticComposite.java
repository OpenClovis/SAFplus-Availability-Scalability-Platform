/**
 * 
 */
package com.clovis.cw.editor.ca.pm;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.custom.StackLayout;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Widget;

import com.clovis.common.utils.ClovisUtils;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.cw.editor.ca.ResourceDataUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.manageability.common.ResourceTreeNode;

/**
 * Composite for attribute statistic configuration.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class AttributeStatisticComposite extends Composite {

	private PMEditor _editor;
	private Button _cachedButton, _persistentButton, _pmButton;
	private Composite _operationalAttributeComposite;
	private ArrayList<String> _selectedAttributes;
	private String _parentResourceName;

	/**
	 * Constructor.
	 * 
	 * @param parent
	 * @param editor
	 */
	public AttributeStatisticComposite(Composite parent, PMEditor editor) {
		super(parent, SWT.NONE);
		_editor = editor;
		createControls();
	}

	/**
	 * Creates the child controls.
	 */
	private void createControls() {
		GridLayout layout = new GridLayout();
		layout.marginWidth = layout.marginHeight = 0;
		setLayout(layout);
		setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

		Group pmGroup = new Group(this, SWT.NONE);
		pmGroup.setBackground(PMEditor.COLOR_PMEDITOR_BACKGROUND);
		pmGroup.setText("PM");

		pmGroup.setLayout(new GridLayout(2, false));
		pmGroup.setLayoutData(new GridData(SWT.FILL, 0, true, false));

		Composite detailsComposite = new Composite(pmGroup, SWT.NONE);
		detailsComposite.setBackground(PMEditor.COLOR_PMEDITOR_BACKGROUND);

		GridLayout detailsLayout = new GridLayout();
		detailsLayout.marginWidth = detailsLayout.marginHeight = 0;
		detailsComposite.setLayout(detailsLayout);
		GridData detailsData = new GridData(SWT.FILL, 0, true, false);
		detailsData.horizontalSpan = 2;
		detailsComposite.setLayoutData(detailsData);

		_cachedButton = new Button(detailsComposite, SWT.CHECK);
		_cachedButton.setBackground(PMEditor.COLOR_PMEDITOR_BACKGROUND);
		_cachedButton.setText("Cached");
		_cachedButton.addSelectionListener(new SelectionAdapter() {
			/*
			 * (non-Javadoc)
			 * 
			 * @see org.eclipse.swt.events.SelectionAdapter#widgetSelected(org.eclipse.swt.events.SelectionEvent)
			 */
			@Override
			public void widgetSelected(SelectionEvent e) {
				if (!_cachedButton.getSelection()) {
					_persistentButton.setSelection(false);
				}
			}
		});

		_persistentButton = new Button(detailsComposite, SWT.CHECK);
		_persistentButton.setBackground(PMEditor.COLOR_PMEDITOR_BACKGROUND);
		_persistentButton.setText("Persistent");
		_persistentButton.addSelectionListener(new SelectionAdapter() {
			/*
			 * (non-Javadoc)
			 * 
			 * @see org.eclipse.swt.events.SelectionAdapter#widgetSelected(org.eclipse.swt.events.SelectionEvent)
			 */
			@Override
			public void widgetSelected(SelectionEvent e) {
				if(_persistentButton.getSelection()) {
					_cachedButton.setSelection(true);
				}
			}
		});

		_pmButton = new Button(detailsComposite, SWT.CHECK);
		_pmButton.setBackground(PMEditor.COLOR_PMEDITOR_BACKGROUND);
		_pmButton.setText("PM");
		_pmButton.setSelection(true);

		Button markStatisticButton = new Button(pmGroup, SWT.PUSH);
		markStatisticButton.setText("Apply");

		markStatisticButton.setLayoutData(new GridData(GridData.END));
		markStatisticButton.addSelectionListener(new SelectionAdapter() {
			/*
			 * (non-Javadoc)
			 * 
			 * @see org.eclipse.swt.events.SelectionAdapter#widgetSelected(org.eclipse.swt.events.SelectionEvent)
			 */
			@Override
			public void widgetSelected(SelectionEvent e) {
				if (!_editor.isResourceBrowserSelectionAvailable()) {
					MessageDialog.openError(getShell(), "No Selection",
							"Make selection from Resource Browser.");
					return;
				}

				_editor.updateStatistic(true, _cachedButton.getSelection(),
						_persistentButton.getSelection(), _pmButton
								.getSelection());
			}
		});

		Button unmarkStatisticsButton = new Button(pmGroup, SWT.PUSH);
		unmarkStatisticsButton.setText("Remove");

		unmarkStatisticsButton.setLayoutData(new GridData(GridData.END));
		unmarkStatisticsButton.addSelectionListener(new SelectionAdapter() {
			/*
			 * (non-Javadoc)
			 * 
			 * @see org.eclipse.swt.events.SelectionAdapter#widgetSelected(org.eclipse.swt.events.SelectionEvent)
			 */
			@Override
			public void widgetSelected(SelectionEvent e) {
				if (!_editor.isResourceBrowserSelectionAvailable()) {
					MessageDialog.openError(getShell(), "No Selection",
							"Make selection from Resource Browser.");
					return;
				}

				_editor.updateStatistic(false, _cachedButton.getSelection(),
						_persistentButton.getSelection(), _pmButton
								.getSelection());
			}
		});

		Group operationalGroup = new Group(this, SWT.NONE);
		operationalGroup.setBackground(PMEditor.COLOR_PMEDITOR_BACKGROUND);
		operationalGroup.setText("PM Reset Value");

		operationalGroup.setLayout(new GridLayout());
		operationalGroup.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true,
				true));

		final Composite stackComposite = new Composite(operationalGroup,
				SWT.NONE);
		final StackLayout stackLayout = new StackLayout();
		stackComposite.setLayout(stackLayout);
		stackComposite.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true,
				true));

		final Composite buttonComposite = new Composite(stackComposite,
				SWT.NONE);
		buttonComposite.setLayout(new GridLayout());

		final Composite attributeComposite = new Composite(stackComposite,
				SWT.NONE);
		attributeComposite.setLayout(new GridLayout());
		attributeComposite.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true,
				true));

		Button operationalButton = new Button(buttonComposite, SWT.PUSH);
		operationalButton.setText("PM Reset Value");
		operationalButton.addSelectionListener(new SelectionAdapter() {
			/*
			 * (non-Javadoc)
			 * 
			 * @see org.eclipse.swt.events.SelectionAdapter#widgetSelected(org.eclipse.swt.events.SelectionEvent)
			 */
			@Override
			public void widgetSelected(SelectionEvent e) {
				String errorMessage = showOperationalAttributes();
				if (!errorMessage.equals("")) {
					MessageDialog.openError(getShell(), "PM Validations",
							errorMessage);
					return;
				}

				stackLayout.topControl = attributeComposite;
				stackComposite.layout();
			}
		});

		ScrolledComposite scroll = new ScrolledComposite(attributeComposite,
				SWT.V_SCROLL);
		scroll.setLayout(new GridLayout());
		scroll.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

		_operationalAttributeComposite = new Composite(scroll, SWT.NONE);
		scroll.setContent(_operationalAttributeComposite);
		_operationalAttributeComposite.setLayout(new GridLayout());
		_operationalAttributeComposite.setLayoutData(new GridData(SWT.FILL,
				SWT.FILL, true, true));

		Button markOperationalButton = new Button(attributeComposite, SWT.PUSH);
		markOperationalButton.setText("Apply");
		markOperationalButton.addSelectionListener(new SelectionAdapter() {
			/*
			 * (non-Javadoc)
			 * 
			 * @see org.eclipse.swt.events.SelectionAdapter#widgetSelected(org.eclipse.swt.events.SelectionEvent)
			 */
			@Override
			public void widgetSelected(SelectionEvent e) {
				String errorMessage = configureOperationalAttribute();
				if (!errorMessage.equals("")) {
					MessageDialog.openError(getShell(), "PM Validations",
							errorMessage);
					return;
				}
				MessageDialog.openInformation(getShell(),
						"PM Reset Value",
						"PM reset value configured.");

				stackLayout.topControl = buttonComposite;
				stackComposite.layout();
			}
		});

		stackLayout.topControl = buttonComposite;
	}

	/**
	 * Shows the potential operational attributes.
	 * @return
	 */
	private String showOperationalAttributes() {
		Object[] checkedResources = _editor.getResourceBrowserComposite()
				.getCheckedItems();

		_selectedAttributes = new ArrayList<String>();
		_parentResourceName = "";
		String errorMessage = "";
		ResourceTreeNode node;

		if (checkedResources.length == 0) {
			errorMessage = "Select PM attributes to configure PM reset value.";
		}

		for (int i = 0; i < checkedResources.length; i++) {
			node = (ResourceTreeNode) checkedResources[i];

			if (node.getNode().isScalarNode()
					|| node.getNode().isTableColumnNode()) {

				if (_parentResourceName.equals("")) {
					_parentResourceName = node.getParent().getName();
				} else if (!_parentResourceName.equals(node.getParent().getName())) {
					errorMessage = "All selected attributes should be from the same resource.";
					break;
				}

				_selectedAttributes.add(node.getName());

			} else {
				errorMessage = "Select attributes only.";
				break;
			}
		}

		if (!errorMessage.equals("")) {
			return errorMessage;
		}

		EObject resObj = ResourceDataUtils.getObjectFrmName(_editor
				.getResourceModel().getEList(), _parentResourceName);
		if (resObj == null) {
			errorMessage = "Before configuring PM reset value, mark selected attributes as PM.";
			return errorMessage;
		}

		EObject pmObj = (EObject) EcoreUtils.getValue(resObj,
				ClassEditorConstants.RESOURCE_PM);
		List pmAttrList = (List) EcoreUtils.getValue(pmObj,
				ClassEditorConstants.CLASS_ATTRIBUTES);
		HashMap<Object, EObject> attributeMap = ClovisUtils.getFeatureValueObjMap(
				pmAttrList, "name");
		EObject attributeObj;

		for (int i = 0; i < _selectedAttributes.size(); i++) {
			attributeObj = attributeMap.get(_selectedAttributes.get(i));

			if (attributeObj == null) {
				errorMessage = "Attribute '" + _selectedAttributes.get(i)
						+ "' is not marked as PM.";
				break;

			} else if (isReadWriteAttribute(attributeObj)
					&& ((Boolean) EcoreUtils.getValue(attributeObj,
							ClassEditorConstants.ATTRIBUTE_CACHING))
							.booleanValue()) {

				errorMessage = "Attribute '"
						+ _selectedAttributes.get(i)
						+ "' is a READWRITE, cached PM attribute.\nPM reset value configuration is not required.";
				break;
			}
		}

		if (!errorMessage.equals("")) {
			return errorMessage;
		}

		HashMap<String, EObject> operationalAttributeMap = new HashMap<String, EObject>();
		getOperationalAttributes(resObj, _selectedAttributes,
				operationalAttributeMap);

		if (operationalAttributeMap.size() == 0) {
			errorMessage = "No potential PM reset value availabel for this PM attribute selection.";
			return errorMessage;
		}

		Control[] children = _operationalAttributeComposite.getChildren();
		for (int i = 0; i < children.length; i++) {
			((Widget) children[i]).dispose();
		}

		Button button;
		Iterator<String> operationaAttributeItr = operationalAttributeMap
				.keySet().iterator();
		while (operationaAttributeItr.hasNext()) {
			button = new Button(_operationalAttributeComposite, SWT.RADIO);
			button.setText(operationaAttributeItr.next());
		}

		_operationalAttributeComposite.setSize(_operationalAttributeComposite
				.computeSize(SWT.DEFAULT, SWT.DEFAULT));
		_operationalAttributeComposite.layout();

		return errorMessage;
	}

	/**
	 * Finds the potential operational attribute for selected PM attributes.
	 * 
	 * @param resObj
	 * @param selectedAttributes
	 * @param operationalAttributeMap
	 */
	private void getOperationalAttributes(EObject resObj,
			ArrayList<String> selectedAttributes,
			HashMap<String, EObject> operationalAttributeMap) {

		EObject attrObj;
		List genAttrList = (List) EcoreUtils.getValue(resObj,
				ClassEditorConstants.CLASS_ATTRIBUTES);

		for (int i = 0; i < genAttrList.size(); i++) {
			attrObj = (EObject) genAttrList.get(i);

			if (isReadWriteAttribute(attrObj)) {
				operationalAttributeMap.put(EcoreUtils.getName(attrObj),
						attrObj);
			}
		}

		EObject provObj = (EObject) EcoreUtils.getValue(resObj,
				ClassEditorConstants.RESOURCE_PROVISIONING);
		List provAttrList = (List) EcoreUtils.getValue(provObj,
				ClassEditorConstants.CLASS_ATTRIBUTES);

		for (int i = 0; i < provAttrList.size(); i++) {
			attrObj = (EObject) provAttrList.get(i);

			if (isReadWriteAttribute(attrObj)) {
				operationalAttributeMap.put(EcoreUtils.getName(attrObj),
						attrObj);
			}
		}

		EObject pmObj = (EObject) EcoreUtils.getValue(resObj,
				ClassEditorConstants.RESOURCE_PM);
		List pmAttrList = (List) EcoreUtils.getValue(pmObj,
				ClassEditorConstants.CLASS_ATTRIBUTES);

		for (int i = 0; i < pmAttrList.size(); i++) {
			attrObj = (EObject) pmAttrList.get(i);

			if (isReadWriteAttribute(attrObj)
					&& !selectedAttributes
							.contains(EcoreUtils.getName(attrObj))) {
				operationalAttributeMap.put(EcoreUtils.getName(attrObj),
						attrObj);
			}
		}
	}

	/**
	 * Checks whether the given attribute is Read-write or not.
	 * 
	 * @param attributeObj
	 * @return
	 */
	private boolean isReadWriteAttribute(EObject attributeObj) {
		if (((Boolean) EcoreUtils.getValue(attributeObj,
				ClassEditorConstants.ATTRIBUTE_WRITABLE))
				.booleanValue()) {

			return true;
		}

		return false;
	}

	/**
	 * Configures the operational attribute for the selected PM attributes.
	 * 
	 * @return
	 */
	private String configureOperationalAttribute() {
		Control[] children = _operationalAttributeComposite.getChildren();
		Button button;
		String attributeName = null, errorMessage = "";

		for (int i = 0; i < children.length; i++) {
			button = (Button) children[i];
			if (button.getSelection()) {
				attributeName = button.getText();
				break;
			}
		}

		if (attributeName == null) {
			errorMessage = "Select PM reset value.";
			return errorMessage;
		}

		Model alarmAssociationModel = _editor.getAlarmAssociationModel();
		List<EObject> resourceList = (List<EObject>) EcoreUtils.getValue(
				(EObject) alarmAssociationModel.getEList().get(0), "resource");
		EObject resourceObj = ClovisUtils.getEobjectWithFeatureVal(resourceList,
				"name", _parentResourceName);

		if (resourceObj == null) {
			EClass resourceClass = (EClass) alarmAssociationModel.getEPackage()
					.getEClassifier("Resource");

			resourceObj = EcoreUtils.createEObject(resourceClass, true);
			EcoreUtils.setValue(resourceObj, "name", _parentResourceName);
			resourceList.add(resourceObj);
		}

		EObject operationalAttributeObj = (EObject) EcoreUtils.getValue(
				resourceObj, "operationalAttribute");
		if (operationalAttributeObj == null) {
			EClass operationaAttrClass = (EClass) alarmAssociationModel
					.getEPackage().getEClassifier("OperationalAttribute");
			operationalAttributeObj = EcoreUtils.createEObject(
					operationaAttrClass, true);
		}

		resourceObj.eSet(resourceObj.eClass().getEStructuralFeature(
				"operationalAttribute"), operationalAttributeObj);
		EcoreUtils.setValue(operationalAttributeObj, "name", attributeName);

		List pmAttributes = (List) EcoreUtils.getValue(operationalAttributeObj,
				"pmAttributes");
		pmAttributes.clear();
		pmAttributes.addAll(_selectedAttributes);

		if (!_editor.isDirty()) {
			_editor.setDirty(true);
		}

		moveOperationalAttrToPmAttrList(attributeName);
		return errorMessage;
	}

	/**
	 * Moves the attribute configured as operational attribute to the pm
	 * attribute list.
	 * 
	 * @param operationalAttr
	 */
	private void moveOperationalAttrToPmAttrList(String operationalAttr) {

		EObject resObj = ResourceDataUtils.getObjectFrmName(_editor
				.getResourceModel().getEList(), _parentResourceName);
		EClass pmAttributeClass = (EClass) _editor.getResourceModel()
				.getEPackage().getEClassifier("PMAttribute");

		EObject pmObj = (EObject) EcoreUtils.getValue(resObj,
				ClassEditorConstants.RESOURCE_PM);
		List pmAttrList = (List) EcoreUtils.getValue(pmObj,
				ClassEditorConstants.CLASS_ATTRIBUTES);

		EObject attrObj;
		List<EObject> genAttrList = (List<EObject>) EcoreUtils.getValue(resObj,
				ClassEditorConstants.CLASS_ATTRIBUTES);

		Iterator<EObject> genAttrItr = genAttrList.iterator();
		while (genAttrItr.hasNext()) {
			attrObj = genAttrItr.next();

			if (EcoreUtils.getName(attrObj).equals(operationalAttr)) {
				EObject pmAttributeObj = EcoreUtils.createEObject(
						pmAttributeClass, true);
				EcoreUtils.copyEObject(attrObj, pmAttributeObj);
				setOperationalAttrDefaultValue(pmAttributeObj);

				pmAttrList.add(pmAttributeObj);
				EcoreUtils.setValue(pmObj, "isEnabled", "true");
				genAttrItr.remove();
				return;
			}
		}

		EObject provObj = (EObject) EcoreUtils.getValue(resObj,
				ClassEditorConstants.RESOURCE_PROVISIONING);
		List<EObject> provAttrList = (List<EObject>) EcoreUtils.getValue(
				provObj, ClassEditorConstants.CLASS_ATTRIBUTES);

		Iterator<EObject> provAttrItr = provAttrList.iterator();
		while (provAttrItr.hasNext()) {
			attrObj = provAttrItr.next();

			if (EcoreUtils.getName(attrObj).equals(operationalAttr)) {
				EObject pmAttributeObj = EcoreUtils.createEObject(
						pmAttributeClass, true);
				EcoreUtils.copyEObject(attrObj, pmAttributeObj);
				setOperationalAttrDefaultValue(pmAttributeObj);

				pmAttrList.add(pmAttributeObj);
				EcoreUtils.setValue(pmObj, "isEnabled", "true");
				provAttrItr.remove();
				return;
			}
		}

		setOperationalAttrDefaultValue(ClovisUtils.getEobjectWithFeatureVal(
				pmAttrList, "name", operationalAttr));
	}

	/**
	 * Sets default for min, max and default value of the attribute.
	 * 
	 * @param attrObj
	 */
	private void setOperationalAttrDefaultValue(EObject attrObj) {
		EcoreUtils.setValue(attrObj, "minValue", "0");
		EcoreUtils.setValue(attrObj, "maxValue", "1");
		EcoreUtils.setValue(attrObj, "defaultValue", "0");
	}
}

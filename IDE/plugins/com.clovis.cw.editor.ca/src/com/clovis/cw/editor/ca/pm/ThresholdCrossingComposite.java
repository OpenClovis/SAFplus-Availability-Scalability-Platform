/**
 * 
 */
package com.clovis.cw.editor.ca.pm;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;

import com.clovis.common.utils.ClovisUtils;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.editor.ca.manageability.common.ResourceTreeNode;

/**
 * Threshold crossing composite.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class ThresholdCrossingComposite extends Composite {

	private PMEditor _editor;
	private AlarmProfilesComposite _alarmProfilesComposite;
	private AlarmAssociationComposite _alarmAssociationComposite;

	/**
	 * Constructor.
	 * 
	 * @param parent
	 * @param editor
	 */
	public ThresholdCrossingComposite(Composite parent, PMEditor editor) {
		super(parent, SWT.NONE);
		_editor = editor;
		createControls();
	}

	/**
	 * Creates the child controls.
	 */
	private void createControls() {
		setBackground(PMEditor.COLOR_PMEDITOR_BACKGROUND);
		GridLayout layout = new GridLayout();
		layout.marginWidth = layout.marginHeight = 0;
		setLayout(layout);
		setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

		_alarmProfilesComposite = new AlarmProfilesComposite(this, _editor);
		_alarmAssociationComposite = new AlarmAssociationComposite(this,
				_editor);
	}

	/**
	 * Returns Alarm profiles composite.
	 * 
	 * @return the _alarmProfilesComposite
	 */
	public AlarmProfilesComposite getAlarmProfilesComposite() {
		return _alarmProfilesComposite;
	}

	/**
	 * Returns Alarm association composite.
	 * 
	 * @return the _alarmAssociationComposite
	 */
	public AlarmAssociationComposite getAlarmAssociationComposite() {
		return _alarmAssociationComposite;
	}

	/**
	 * Shows the current association details.
	 * 
	 * @param node
	 */
	public void showCurrentAssociation(ResourceTreeNode node) {
		if (!_editor.getThresholdCrossingItem().getExpanded()
				|| node == null
				|| node.getNode() == null) {
			return;
		}

		List<EObject> resourceList = (List<EObject>) EcoreUtils.getValue(
				(EObject) _editor.getAlarmAssociationModel().getEList().get(0),
				"resource");
		List<EObject> alarmList = new ArrayList<EObject>();

		if (node.getNode().isScalarNode() || node.getNode().isTableColumnNode()) {
			String resName = node.getParent().getName();
			EObject resObj = ClovisUtils.getEobjectWithFeatureVal(resourceList,
					"name", resName);

			if (resObj != null) {
				List<EObject> attributeList = (List<EObject>) EcoreUtils
						.getValue(resObj, "attribute");
				EObject attrObj = ClovisUtils.getEobjectWithFeatureVal(
						attributeList, "name", node.getName());

				if (attrObj != null) {
					alarmList = (List<EObject>) EcoreUtils.getValue(attrObj,
							"alarm");
				}
			}

		} else {
			String resName = node.getName();
			EObject resObj = ClovisUtils.getEobjectWithFeatureVal(resourceList,
					"name", resName);

			if (resObj != null) {
				alarmList = (List<EObject>) EcoreUtils
						.getValue(resObj, "alarm");
			}
		}

		_editor.getAlarmProfilesComposite().setSelection(
				ClovisUtils.getFeatureValueList(alarmList, "alarmID"), alarmList);
	}
}

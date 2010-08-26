/**
 * 
 */
package com.clovis.cw.editor.ca.pm;

import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;

/**
 * Composite for Alarm Association.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class AlarmAssociationComposite extends Composite {

	private PMEditor _editor;

	private Text _thresholdText;

	private CCombo _severityCombo;

	private Button _associateButton, _dissociateButton;

	/**
	 * Constructor.
	 * 
	 * @param editor
	 * @param parent
	 */
	public AlarmAssociationComposite(Composite parent, PMEditor editor) {
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
		setLayoutData(new GridData(SWT.FILL, 0, true, false));

		Group alarmAssociationGroup = new Group(this, SWT.NONE);
		alarmAssociationGroup.setBackground(PMEditor.COLOR_PMEDITOR_BACKGROUND);
		alarmAssociationGroup.setText("Alarm Association");

		alarmAssociationGroup.setLayout(new GridLayout(2, false));
		alarmAssociationGroup.setLayoutData(new GridData(SWT.FILL, 0, true,
				false));

		Composite associationComposite = new Composite(alarmAssociationGroup,
				SWT.NONE);
		associationComposite.setBackground(PMEditor.COLOR_PMEDITOR_BACKGROUND);

		GridLayout associationLayout = new GridLayout(3, false);
		associationLayout.marginWidth = associationLayout.marginHeight = 0;
		associationComposite.setLayout(associationLayout);
		GridData associationData = new GridData(SWT.FILL, 0, true, false);
		associationData.horizontalSpan = 2;
		associationComposite.setLayoutData(associationData);

		Label thresholdLabel = new Label(associationComposite, SWT.NONE);
		thresholdLabel.setBackground(PMEditor.COLOR_PMEDITOR_BACKGROUND);
		thresholdLabel.setText("Threshold Value: ");

		_thresholdText = new Text(associationComposite, SWT.BORDER);
		_thresholdText.addVerifyListener(new NumericVerifyListener());
		_thresholdText.addModifyListener(new BoundModifyListener());
		new Label(associationComposite, SWT.NONE).setLayoutData(new GridData(
				SWT.FILL, 0, true, false));

		Label severityLabel = new Label(associationComposite, SWT.NONE);
		severityLabel.setBackground(PMEditor.COLOR_PMEDITOR_BACKGROUND);
		severityLabel.setText("Severity: ");

		_severityCombo = new CCombo(associationComposite, SWT.BORDER);
		_severityCombo.setItems(new String[] { "Critical", "Major", "Minor",
				"Warning" });
		_severityCombo.setText("Critical");

		_associateButton = new Button(associationComposite, SWT.PUSH);
		_associateButton.setText("Associate");
		_associateButton.setLayoutData(new GridData(
				GridData.HORIZONTAL_ALIGN_END));
		_associateButton.setEnabled(false);
		_associateButton.addSelectionListener(new SelectionAdapter() {
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

				_editor.associateAlarms(_thresholdText.getText(), _severityCombo.getText());
			}
		});

		Label dissociateLabel = new Label(alarmAssociationGroup, SWT.NONE);
		dissociateLabel.setBackground(PMEditor.COLOR_PMEDITOR_BACKGROUND);
		dissociateLabel.setLayoutData(new GridData(SWT.FILL, 0, true, false));

		_dissociateButton = new Button(alarmAssociationGroup, SWT.PUSH);
		_dissociateButton.setText("Remove Association");
		_dissociateButton.addSelectionListener(new SelectionAdapter() {
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

				_editor.dissociateAlarms();
			}
		});
	}

	/**
	 * Sets the given field values.
	 * 
	 * @param lowerBound
	 * @param upperBound
	 * @param severity
	 */
	public void setFieldValues(String thresholdValue, String severity) {
		_thresholdText.setText(thresholdValue);
		_severityCombo.setText(severity);
	}

	/**
	 * Sets controls in view mode if true, false otherwise.
	 * 
	 * @param viewMode
	 */
	public void setViewMode(boolean viewMode) {
		_associateButton.setVisible(!viewMode);
		_dissociateButton.setVisible(!viewMode);

		_thresholdText.setEnabled(!viewMode);
		_severityCombo.setEnabled(!viewMode);
	}

	/**
	 * Modify listener for bound text.
	 * 
	 * @author Suraj Rajyaguru
	 * 
	 */
	class BoundModifyListener implements ModifyListener {

		/*
		 * (non-Javadoc)
		 * 
		 * @see org.eclipse.swt.events.ModifyListener#modifyText(org.eclipse.swt.events.ModifyEvent)
		 */
		public void modifyText(ModifyEvent e) {

			if (_thresholdText.getText().equals("")) {
				_associateButton.setEnabled(false);
			} else {
				_associateButton.setEnabled(true);
			}
		}
	}
}

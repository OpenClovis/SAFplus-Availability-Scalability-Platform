/**
 * 
 */
package com.clovis.logtool.ui;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;

/**
 * Navigation Panel provides support for the navigation of records.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class NavigationPanel extends Composite {

	/**
	 * Next Button.
	 */
	private Button _nextButton;

	/**
	 * Previous Button.
	 */
	private Button _previousButton;

	/**
	 * First Button.
	 */
	private Button _firstButton;

	/**
	 * Last Button.
	 */
	private Button _lastButton;

	/**
	 * Constructs the Navigation Panel with the given parent.
	 * 
	 * @param parent
	 *            the Parent Composite
	 */
	NavigationPanel(Composite parent) {
		super(parent, SWT.FLAT);
		createControls();
	}

	/**
	 * Creates the Controls for this Navigation Panel.
	 */
	private void createControls() {
		RowLayout layout = new RowLayout();
		layout.pack = false;
		setLayout(layout);

		_firstButton = new Button(this, SWT.FLAT);
		_firstButton.setText("First");
		_firstButton.setEnabled(false);
		_firstButton.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				showBoundryRecordBatch(true);
			}
		});

		_previousButton = new Button(this, SWT.FLAT);
		_previousButton.setText("Previous");
		_previousButton.setEnabled(false);
		_previousButton.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				showRecordBatch(false);
			}
		});

		_nextButton = new Button(this, SWT.FLAT);
		_nextButton.setText("Next");
		_nextButton.setEnabled(false);
		_nextButton.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				showRecordBatch(true);
			}
		});

		_lastButton = new Button(this, SWT.FLAT);
		_lastButton.setText("Last");
		_lastButton.setEnabled(false);
		_lastButton.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				showBoundryRecordBatch(false);
			}
		});
	}

	/**
	 * Sets the status for the buttons of this panel.
	 * 
	 * @param nextStatus
	 *            the status of next button
	 * @param previousStatus
	 *            the status of previous button
	 * @param firstStatus
	 *            the status of first button
	 * @param lastStatus
	 *            the status of last button
	 */
	public void setButtonStatus(boolean nextStatus, boolean previousStatus,
			boolean firstStatus, boolean lastStatus) {
		_nextButton.setEnabled(nextStatus);
		_previousButton.setEnabled(previousStatus);
		_firstButton.setEnabled(firstStatus);
		_lastButton.setEnabled(lastStatus);
	}

	/**
	 * Shows the Record Batch to user based on the given flag.
	 * 
	 * @param nextFlag
	 *            true value indicates next batch and false means previous
	 */
	private void showRecordBatch(boolean nextFlag) {
		LogDisplay logDisplay = LogDisplay.getInstance();
		RecordPanel recordPanel = (RecordPanel) logDisplay
				.getRecordPanelFolder().getSelection().getControl();
		recordPanel.updateRecordViewerData(nextFlag);
	}

	/**
	 * Shows the Record Batch from boundry.
	 * 
	 * @param firstFlag
	 *            true for first batch and false for last batch
	 */
	private void showBoundryRecordBatch(boolean firstFlag) {
		LogDisplay logDisplay = LogDisplay.getInstance();
		RecordPanel recordPanel = (RecordPanel) logDisplay
				.getRecordPanelFolder().getSelection().getControl();
		recordPanel.getRecordManager().getRecordFetcher().setFetchBoundry(firstFlag);
		recordPanel.updateRecordViewerData(firstFlag);
	}
}

/**
 * 
 */
package com.clovis.logtool.ui.filter;

import java.util.ArrayList;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;

import com.clovis.logtool.record.filter.FilterObject;
import com.clovis.logtool.ui.LogDisplay;
import com.clovis.logtool.ui.RecordPanel;

/**
 * Filter Panel provides support for the filtering.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class FilterPanel extends Composite {

	/**
	 * Filter Object Panel instance.
	 */
	private FilterObjectPanel _filterObjectPanel;

	/**
	 * Filter Button instance.
	 */
	private Button _filterButton;

	/**
	 * Clear Button instance.
	 */
	private Button _clearButton;

	/**
	 * Constructs Filter Panel instance.
	 * 
	 * @param parent
	 *            the parent composite
	 */
	public FilterPanel(Composite parent) {
		super(parent, SWT.FLAT);
		createControls();
	}

	/**
	 * Creates the controls for this Filter Panel.
	 */
	private void createControls() {
		GridLayout layout = new GridLayout(4, false);
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		setLayout(layout);

		Label label = new Label(this, SWT.NONE);
		label.setText("Specify Filter :");

		_filterObjectPanel = new FilterObjectPanel(this);
		GridData filterObjectPanelData = new GridData(SWT.FILL, 0, true, false);
		_filterObjectPanel.setLayoutData(filterObjectPanelData);

		_filterButton = new Button(this, SWT.FLAT);
		_filterButton.setText("Filter");
		_filterButton.setEnabled(false);
		_filterButton.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				filterRecords();
			}
		});

		_clearButton = new Button(this, SWT.FLAT);
		_clearButton.setText("Clear");
		_clearButton.setEnabled(false);
		_clearButton.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				clearFilter();
			}
		});
	}

	/**
	 * Filters the records from the stream.
	 */
	private void filterRecords() {
		ArrayList<FilterObject> filterObjectList = new ArrayList<FilterObject>();
		filterObjectList.add(_filterObjectPanel.getFilterObject());

		LogDisplay logDisplay = LogDisplay.getInstance();
		RecordPanel recordPanel = (RecordPanel) logDisplay
				.getRecordPanelFolder().getSelection().getControl();

		recordPanel.getRecordManager().clearRecordBatch();
		recordPanel.getRecordManager().getRecordFetcher().setFilterData(filterObjectList, false);
		recordPanel.updateRecordViewerData(true);
	}

	/**
	 * Clears the filter applied to the stream.
	 */
	private void clearFilter() {
		LogDisplay logDisplay = LogDisplay.getInstance();
		RecordPanel recordPanel = (RecordPanel) logDisplay
				.getRecordPanelFolder().getSelection().getControl();

		recordPanel.getRecordManager().clearRecordBatch();
		recordPanel.updateRecordViewerData(true);
	}

	/**
	 * Sets the status for the buttons of this panel.
	 * 
	 * @param filterStatus
	 *            the status of filter button
	 * @param clearStatus
	 *            the status of clear button
	 */
	public void setButtonStatus(boolean filterStatus, boolean clearStatus) {
		_filterButton.setEnabled(filterStatus);
		_clearButton.setEnabled(clearStatus);
	}
}

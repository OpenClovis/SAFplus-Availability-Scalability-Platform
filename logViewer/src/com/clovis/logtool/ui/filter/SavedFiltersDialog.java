/**
 * 
 */
package com.clovis.logtool.ui.filter;

import java.util.HashMap;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.List;
import org.eclipse.swt.widgets.Shell;

import com.clovis.logtool.record.filter.RecordFilter;
import com.clovis.logtool.ui.LogDisplay;
import com.clovis.logtool.ui.RecordPanel;
import com.clovis.logtool.utils.LogConstants;
import com.clovis.logtool.utils.LogUtils;

/**
 * Dialog to show the saved filters and do the manipulations for filters.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class SavedFiltersDialog extends Dialog {

	/**
	 * View Model for the Filter Map.
	 */
	private HashMap<String, RecordFilter> _filterMap;

	/**
	 * Represents the Filter list.
	 */
	private List _filterList;

	/**
	 * Creates the instance of this dialog.
	 * 
	 * @param shell
	 *            the parent shell
	 */
	public SavedFiltersDialog(Shell shell) {
		super(shell);
		_filterMap = LogUtils.getFilterMap();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent) {
		Composite composite = new Group(parent, SWT.SHADOW_IN);
		composite.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		composite.setLayout(new GridLayout());

		Group group = new Group(composite, SWT.BORDER);
		group.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		GridLayout groupLayout = new GridLayout(2, false);
		groupLayout.marginWidth = 0;
		groupLayout.marginHeight = 0;
		group.setLayout(groupLayout);

		Group filterListGroup = new Group(group, SWT.BORDER);
		GridLayout filterListGroupLayout = new GridLayout();
		filterListGroupLayout.marginWidth = 0;
		filterListGroupLayout.marginHeight = 0;
		filterListGroup.setLayout(filterListGroupLayout);

		_filterList = new List(filterListGroup, SWT.V_SCROLL | SWT.H_SCROLL);
		loadFilterListItems();

		GridData listData = new GridData(SWT.FILL, SWT.FILL, true, true);
		Rectangle bounds = Display.getCurrent().getClientArea();
		listData.heightHint = (int) (1.5 * bounds.height / 5);
		listData.widthHint = bounds.width / 5;
		_filterList.setLayoutData(listData);

		Composite buttonPanel = new Composite(group, SWT.NONE);
		RowLayout buttonPanelLayout = new RowLayout(SWT.VERTICAL);
		buttonPanelLayout.marginLeft = 0;
		buttonPanelLayout.marginRight = 0;
		buttonPanelLayout.fill = true;
		buttonPanel.setLayout(buttonPanelLayout);

		Button applyButton = new Button(buttonPanel, SWT.PUSH);
		applyButton.setText("Apply");
		applyButton.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				applySelected();
			}
		});

		Button addButton = new Button(buttonPanel, SWT.PUSH);
		addButton.setText("Add...");
		addButton.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				addSelected();
			}
		});

		Button removeButton = new Button(buttonPanel, SWT.PUSH);
		removeButton.setText("Remove");
		removeButton.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				removeSelected();
			}
		});

		return composite;
	}

	/**
	 * Handler for the apply button. It applies the selected filter to the
	 * current record batch.
	 */
	private void applySelected() {
		LogDisplay logDisplay = LogDisplay.getInstance();

		if (logDisplay.getRecordPanelFolder().getItemCount() == 0) {
			LogUtils.displayMessage(LogConstants.DISPLAY_MESSAGE_WARNING,
					"No Stream", "Stream is not opened");
			return;
		}

		RecordPanel recordPanel = (RecordPanel) logDisplay
				.getRecordPanelFolder().getSelection().getControl();
		recordPanel.getRecordManager().clearRecordBatch();

		RecordFilter filter = _filterMap.get(_filterList
				.getSelection()[0]);

		recordPanel.getRecordManager().getRecordFetcher().setFilterData(
				filter.getFilterObjectList(), filter.isAndFlag());
		recordPanel.updateRecordViewerData(true);

		super.okPressed();
	}

	/**
	 * Handler for the add button. It allows to create a filter by opening the
	 * filter dialog.
	 */
	private void addSelected() {
		FilterDialog dialog = new FilterDialog(getShell(), true);
		dialog.setFilterMap(_filterMap);
		dialog.open();

		loadFilterListItems();
	}

	/**
	 * Handler for the remove button. It allows to remove the selected filter.
	 */
	private void removeSelected() {
		_filterMap.remove(_filterList.getSelection()[0]);
		loadFilterListItems();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed() {
		LogUtils.saveFilterMap(_filterMap);
		super.okPressed();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell shell) {
		shell.setText("Saved Filters");
		super.configureShell(shell);
	}

	/**
	 * Updates the filter list with current view model value.
	 */
	private void loadFilterListItems() {
		_filterList.setItems(_filterMap.keySet().toArray(
				new String[] {}));
	}
}

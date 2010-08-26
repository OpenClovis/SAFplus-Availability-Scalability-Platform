/**
 * 
 */
package com.clovis.logtool.ui.filter;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;

import com.clovis.logtool.record.filter.FilterObject;
import com.clovis.logtool.record.filter.RecordFilter;
import com.clovis.logtool.ui.LogDisplay;
import com.clovis.logtool.ui.RecordPanel;
import com.clovis.logtool.utils.LogConstants;
import com.clovis.logtool.utils.LogUtils;

/**
 * The Filter Dialog to create advanced Filter.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class FilterDialog extends Dialog {

	/**
	 * Specifies if the dialog is opened from some other dialog or not.
	 */
	private boolean _childFlag;

	/**
	 * Represents the Name for the filter.
	 */
	private LTTextCellEditor _nameText;

	/**
	 * Represents the Type for he filter.
	 */
	private CCombo _typeCombo;

	/**
	 * It holds the set of filter object composites.
	 */
	private Composite _filterGroup;

	/**
	 * The Filter Map.
	 */
	private HashMap<String, RecordFilter> _filterMap;

	/**
	 * Constructs the instance of Filter Dialog Class.
	 * 
	 * @param shell
	 *            the parent shell
	 */
	public FilterDialog(Shell shell, boolean childFlag) {
		super(shell);
		_childFlag = childFlag;
		if (!_childFlag) {
			_filterMap = LogUtils.getFilterMap();
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.TitleAreaDialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent) {
		Composite composite = new Group(parent, SWT.SHADOW_IN);
		composite.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		composite.setLayout(new GridLayout(3, false));

		Label nameLabel = new Label(composite, SWT.NONE);
		nameLabel.setText("Filter Name :");

		_nameText = new LTTextCellEditor(composite);
		_nameText.setValue("Filter");
		GridData nameTextData = (GridData) _nameText.getControl()
				.getLayoutData();
		nameTextData.horizontalSpan = 2;

		Label typeLabel = new Label(composite, SWT.NONE);
		typeLabel.setText("Filter Type :");

		_typeCombo = new CCombo(composite, SWT.BORDER | SWT.READ_ONLY);
		_typeCombo.setItems(new String[] { "AND", "OR" });
		_typeCombo.select(0);

		Button addButton = new Button(composite, SWT.FLAT);
		addButton.setText("Create Filter Criterion");
		addButton.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_END));
		addButton.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				addSelected();
			}
		});

		Group borderGroup = new Group(composite, SWT.SHADOW_NONE | SWT.V_SCROLL);
		GridData groupData = new GridData(SWT.FILL, SWT.FILL, true, true);
		groupData.horizontalSpan = 3;
		groupData.heightHint = 150;
		borderGroup.setLayoutData(groupData);

		GridLayout borderGroupLayout = new GridLayout();
		borderGroupLayout.marginWidth = 0;
		borderGroupLayout.marginHeight = 0;
		borderGroup.setLayout(borderGroupLayout);

		ScrolledComposite scroll = new ScrolledComposite(borderGroup,
				SWT.V_SCROLL);
		scroll.setAlwaysShowScrollBars(true);
		scroll.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		scroll.setLayout(new GridLayout());

		_filterGroup = new Composite(scroll, SWT.NONE);
		scroll.setContent(_filterGroup);
		_filterGroup
				.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

		GridLayout filterGroupLayout = new GridLayout();
		filterGroupLayout.marginWidth = 0;
		filterGroupLayout.marginHeight = 0;
		_filterGroup.setLayout(filterGroupLayout);

		addSelected();
		return composite;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell shell) {
		shell.setText("Advanced Filter");
		super.configureShell(shell);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createButtonsForButtonBar(Composite parent) {
		Button applyButton = createButton(parent, -1, "Apply", false);
		applyButton.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				applySelected();
			}
		});

		super.createButtonsForButtonBar(parent);
		getButton(OK).setText("Save");
	}

	/**
	 * Handler when the Add button is selected by the user. It creates one more
	 * composite to specify the filter.
	 */
	private void addSelected() {
		Composite filterObjectComposite = new Composite(_filterGroup, SWT.NONE);
		filterObjectComposite.setLayoutData(new GridData(SWT.FILL, 0, true,
				false));

		GridLayout layout = new GridLayout(2, false);
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		filterObjectComposite.setLayout(layout);

		new FilterObjectPanel(filterObjectComposite);
		final Button removeButton = new Button(filterObjectComposite, SWT.FLAT);
		removeButton.setText("Remove");
		removeButton.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_END));
		removeButton.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				removeSelected(removeButton);
			}
		});
		_filterGroup
				.setSize(_filterGroup.computeSize(SWT.DEFAULT, SWT.DEFAULT));
	}

	/**
	 * Handler when the Remove button is selected by the user. It removes the
	 * composite which is having the given button.
	 * 
	 * @param removeButton
	 *            the Remove button
	 */
	private void removeSelected(Button removeButton) {
		if (removeButton.getParent().getParent().getChildren().length == 1) {
			return;
		}
		removeButton.getParent().dispose();
		_filterGroup
				.setSize(_filterGroup.computeSize(SWT.DEFAULT, SWT.DEFAULT));
	}

	/**
	 * Handler when the Apply button is selected by the user. It applies the
	 * filter with the given information to the current record batch.
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

		List<FilterObject> filterObjectList = getFilterObjectList();
		boolean andFlag = _typeCombo.getText().equals("AND") ? true : false;

		recordPanel.getRecordManager().getRecordFetcher().setFilterData(
				filterObjectList, andFlag);
		recordPanel.updateRecordViewerData(true);

		super.okPressed();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed() {
		String filterName = getFilterName();
		boolean store = true;

		if (_filterMap.containsKey(filterName)) {
			store = LogUtils.displayMessage(
					LogConstants.DISPLAY_MESSAGE_QUESTION,
					"Filter Already Exsist",
					"Do you want to replace the existing filter?");
		}

		if (store) {
			boolean andFlag = _typeCombo.getText().equals("AND") ? true : false;
			_filterMap.put(filterName, new RecordFilter(
					getFilterObjectList(), andFlag));

			if(!_childFlag) {
				LogUtils.saveFilterMap(_filterMap);
			}
			super.okPressed();
		}
	}

	/**
	 * Returns the list of Filter Objects.
	 * 
	 * @return the filter object list
	 */
	private List<FilterObject> getFilterObjectList() {
		List<FilterObject> filterObjectList = new ArrayList<FilterObject>();
		Control[] filterObjs = _filterGroup.getChildren();

		for (int i = 0; i < filterObjs.length; i++) {
			FilterObjectPanel fop = (FilterObjectPanel) ((Composite) filterObjs[i])
					.getChildren()[0];
			filterObjectList.add(fop.getFilterObject());
		}

		return filterObjectList;
	}

	/**
	 * Returns the name of the Filter.
	 * 
	 * @return the filter name
	 */
	private String getFilterName() {
		return _nameText.getValue().toString();
	}

	/**
	 * Sets the filter map.
	 * 
	 * @param filterMap
	 *            the Filter Map
	 */
	public void setFilterMap(HashMap<String, RecordFilter> filterMap) {
		_filterMap = filterMap;
	}
}

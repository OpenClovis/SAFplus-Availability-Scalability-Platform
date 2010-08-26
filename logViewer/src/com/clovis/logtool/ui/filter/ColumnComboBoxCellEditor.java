/**
 * 
 */
package com.clovis.logtool.ui.filter;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.eclipse.jface.viewers.ComboBoxCellEditor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.widgets.Composite;

import com.clovis.logtool.utils.LogConstants;
import com.clovis.logtool.utils.LogUtils;
import com.clovis.logtool.utils.UIColumn;

/**
 * Column Combo cell editor for selecting the fields.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class ColumnComboBoxCellEditor extends ComboBoxCellEditor {

	/**
	 * Constructs the Column Combo cell editor.
	 * 
	 * @param parent
	 *            the parent composite
	 */
	public ColumnComboBoxCellEditor(final Composite parent) {
		super(parent, new String[0], SWT.BORDER | SWT.READ_ONLY | SWT.FLAT);
		setItems(getComboValues());

		CCombo combo = (CCombo) getControl();
		combo.setVisible(true);
		combo.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				selectionOccurred();
			}
		});

		setValue(new Integer(combo.getItemCount() - 1));
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.CellEditor#deactivate()
	 */
	public void deactivate() {
		fireCancelEditor();
	}

	/**
	 * Returns the set of items for the Combo.
	 * 
	 * @return the set of items
	 */
	private String[] getComboValues() {
		List<String> columns = new ArrayList<String>();
		List columnList = LogUtils.getUIColumns();

		Iterator itr = columnList.iterator();
		while (itr.hasNext()) {
			UIColumn column = (UIColumn) itr.next();
			if (column.isShowFlag()) {
				columns.add(column.getName());
			}
		}

		String items[] = new String[columns.size()];
		for (int i = 0; i < columns.size(); i++) {
			items[i] = columns.get(i);
		}
		return items;
	}

	/**
	 * Handles the selection for this combo.
	 */
	private void selectionOccurred() {
		FilterObjectPanel filterObjectPanel = (FilterObjectPanel) getControl()
				.getParent();
		int columnIndex = ((Integer) getValue()).intValue();

		String items[] = null;
		int type = ((UIColumn) LogUtils.getUIColumns().get(columnIndex))
				.getType();

		switch (type) {

		case LogConstants.TYPE_SHORT:
		case LogConstants.TYPE_INT:
		case LogConstants.TYPE_LONG:
		case LogConstants.TYPE_DATE:
			items = new String[] { "=", "<", ">", "<=", ">=", "Between", "Range" };
			break;

		case LogConstants.TYPE_STRING:
			items = new String[] { "Is", "Is Not", "Contains",
					"Does Not Contain", "Starts With", "Does Not Start With",
					"Ends With", "Does Not End With" };
			break;
		}

		TypeComboBoxCellEditor typeCombo = (TypeComboBoxCellEditor) filterObjectPanel
				.getTypeCombo();
		typeCombo.setItems(items);
		typeCombo.setValue(new Integer(0));
		typeCombo.selectionOccurred();
	}
}

/**
 * 
 */
package com.clovis.logtool.ui.filter;

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
 * Tpye Combo cell editor for selecting the type of filter.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class TypeComboBoxCellEditor extends ComboBoxCellEditor {

	/**
	 * Constructs type combo cell editor.
	 * 
	 * @param parent
	 *            the parent composite
	 */
	public TypeComboBoxCellEditor(final Composite parent) {
		super(parent, new String[0], SWT.BORDER | SWT.READ_ONLY | SWT.FLAT);
		setItems(getComboValues());
		setValue(new Integer(0));

		CCombo combo = (CCombo) getControl();
		combo.setVisible(true);
		combo.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				selectionOccurred();
			}
		});
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
		FilterObjectPanel filterObjectPanel = (FilterObjectPanel) getControl()
				.getParent();
		int columnIndex = ((Integer) filterObjectPanel.getColumnCombo()
				.getValue()).intValue();

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
		return items;
	}

	/**
	 * Handles the selection for this combo.
	 */
	void selectionOccurred() {
		FilterObjectPanel parent = (FilterObjectPanel) getControl().getParent();
		int columnIndex = ((Integer) parent.getColumnCombo().getValue())
				.intValue();

		CriterionComposite criterionComposite = (CriterionComposite) parent
				.getCriterionComposite();
		int type = ((UIColumn) LogUtils.getUIColumns().get(columnIndex))
				.getType();

		switch (type) {

		case LogConstants.TYPE_SHORT:
		case LogConstants.TYPE_INT:
		case LogConstants.TYPE_LONG:
		case LogConstants.TYPE_DATE:
			int itemIdex = ((Integer) getValue()).intValue();

			switch (itemIdex) {
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
				if(type == LogConstants.TYPE_DATE) {
					criterionComposite.select(CriterionComposite.SINGLE_TIMESTAMP);
				} else {
					criterionComposite.select(CriterionComposite.SINGLE_SPINNER);
				}
				break;
			case 5:
			case 6:
				if(type == LogConstants.TYPE_DATE) {
					criterionComposite.select(CriterionComposite.TWO_TIMESTAMP);
				} else {
					criterionComposite.select(CriterionComposite.TWO_SPINNER);
				}
				break;
			}
			break;

		case LogConstants.TYPE_STRING:
			criterionComposite.select(CriterionComposite.TEXT);
			break;
		}
	}
}

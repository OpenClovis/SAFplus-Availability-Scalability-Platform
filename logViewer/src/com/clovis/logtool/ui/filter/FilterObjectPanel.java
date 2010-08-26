/**
 * 
 */
package com.clovis.logtool.ui.filter;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;

import com.clovis.logtool.record.filter.FilterObject;
import com.clovis.logtool.utils.LogConstants;
import com.clovis.logtool.utils.LogUtils;
import com.clovis.logtool.utils.RecordField;
import com.clovis.logtool.utils.UIColumn;

/**
 * Filter Object Panel provides support to select filtering options.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class FilterObjectPanel extends Composite {

	/**
	 * For selecting the fields.
	 */
	private ColumnComboBoxCellEditor _columnCombo;

	/**
	 * For selecting the type of filter.
	 */
	private TypeComboBoxCellEditor _typeCombo;

	/**
	 * For specifying the filter criteras.
	 */
	private CriterionComposite _criterionComposite;

	/**
	 * Constructs the Filter Panel Object.
	 * 
	 * @param parent
	 *            the paren composite
	 */
	public FilterObjectPanel(Composite parent) {
		super(parent, SWT.NONE);
		createControls();
	}

	/**
	 * Creates the controls for this filter object panel.
	 */
	private void createControls() {
		GridLayout layout = new GridLayout(3, false);
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		setLayout(layout);

		_columnCombo = new ColumnComboBoxCellEditor(this);
		_typeCombo = new TypeComboBoxCellEditor(this);
		_criterionComposite = new CriterionComposite(this);
	}

	/**
	 * Returns the Tpye Combo cell editor.
	 * 
	 * @return the Tpye Combo cell editor
	 */
	public TypeComboBoxCellEditor getTypeCombo() {
		return _typeCombo;
	}

	/**
	 * Returns the Column Combo cell editor.
	 * 
	 * @return the Tpye Combo cell editor
	 */
	public ColumnComboBoxCellEditor getColumnCombo() {
		return _columnCombo;
	}

	/**
	 * Returns the Criterion Composite.
	 * 
	 * @return the Criterion Composite
	 */
	public CriterionComposite getCriterionComposite() {
		return _criterionComposite;
	}

	/**
	 * Returns the Filter Object created from the values of controls of this
	 * panel.
	 * 
	 * @return the filter object
	 */
	public FilterObject getFilterObject() {
		int columnIndex = ((Integer) _columnCombo.getValue()).intValue();
		int fieldIndex = ((UIColumn) LogUtils.getUIColumns().get(columnIndex))
				.getFieldIndex();

		int fieldType = fieldIndex != -1 ? ((RecordField) LogUtils
				.getRecordFields().get(fieldIndex)).getType()
				: ((UIColumn) LogUtils.getUIColumns().get(columnIndex))
						.getType();

		String criterion = _criterionComposite.getCriterionValue();
		boolean negateFlag = false;

		if (fieldType == LogConstants.TYPE_STRING) {
			int filterTypeIndex = ((Integer) getTypeCombo().getValue())
					.intValue();

			switch (filterTypeIndex) {

			case 0:
				criterion = "^" + criterion + "$";
				break;

			case 1:
				criterion = "^" + criterion + "$";
				negateFlag = true;
				break;

			case 2:
				break;

			case 3:
				negateFlag = true;
				break;

			case 4:
				criterion = "^" + criterion;
				break;

			case 5:
				criterion = "^" + criterion;
				negateFlag = true;
				break;

			case 6:
				criterion = criterion + "$";
				break;

			case 7:
				criterion = criterion + "$";
				negateFlag = true;
				break;
			}
		}
		return new FilterObject(fieldType, fieldIndex, criterion, negateFlag);
	}
}

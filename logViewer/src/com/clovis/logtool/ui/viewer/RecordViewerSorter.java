package com.clovis.logtool.ui.viewer;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerSorter;
import org.eclipse.swt.SWT;

import com.clovis.logtool.message.MessageFormatter;
import com.clovis.logtool.record.Record;
import com.clovis.logtool.ui.LogDisplay;
import com.clovis.logtool.utils.LogConstants;
import com.clovis.logtool.utils.LogUtils;
import com.clovis.logtool.utils.UIColumn;

/**
 * Sorter for the record viewer.
 * 
 * @author Suraj Rajyaguru
 */
public class RecordViewerSorter extends ViewerSorter {

	/**
	 * Column for which sort is to be applied.
	 */
	private int _column;

	/**
	 * Flag to decide whether to sort or not.
	 */
	private boolean _sort;

	/**
	 * Direction for the sort.
	 */
	private int _direction;

	/**
	 * Sets the direction of sort and the column for which sort is requested.
	 * 
	 * @param column
	 *            the column which is seleted for sorting
	 */
	public void doSort(int column) {
		if (_column == column) {
			_direction = _direction == SWT.UP ? SWT.DOWN : SWT.UP;
		} else {
			_column = column;
			_direction = SWT.DOWN;
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.ViewerSorter#compare(org.eclipse.jface.viewers.Viewer,
	 *      java.lang.Object, java.lang.Object)
	 */
	public int compare(Viewer viewer, Object e1, Object e2) {
		if(!_sort) 	return 0;

		Record record1 = (Record) e1;
		Record record2 = (Record) e2;

		UIColumn column = LogUtils.getUIColumns().get(_column);
		int fieldIndex = column.getFieldIndex();
		int result = 0;

		switch (column.getType()) {

		case LogConstants.TYPE_SHORT:
			result = ((Short) record1.getField(fieldIndex, false)).shortValue() > ((Short) record2
					.getField(fieldIndex, false)).shortValue() ? 1 : -1;
			break;

		case LogConstants.TYPE_INT:
			result = ((Integer) record1.getField(fieldIndex, false)).intValue() > ((Integer) record2
					.getField(fieldIndex, false)).intValue() ? 1 : -1;
			break;

		case LogConstants.TYPE_LONG:
		case LogConstants.TYPE_DATE:
			result = ((Long) record1.getField(fieldIndex, false)).longValue() > ((Long) record2
					.getField(fieldIndex, false)).longValue() ? 1 : -1;
			break;

		case LogConstants.TYPE_STRING:
			if (fieldIndex == -1) {
				MessageFormatter formatter = LogDisplay.getInstance()
						.getMessageFormatter();
				result = collator.compare(formatter.format(record1), formatter
						.format(record2));
			} else {
				result = collator.compare(record1.getField(fieldIndex, false)
						.toString(), record2.getField(fieldIndex, false).toString());
			}
			break;
		}

		if (_direction == SWT.UP) {
			result = -result;
		}
		return result;
	}

	/**
	 * Sets the value of sort flag.
	 * 
	 * @param sort
	 *            the sort Flag
	 */
	public void setSort(boolean sort) {
		_sort = sort;
	}

	/**
	 * Returns the Direction of sort.
	 * 
	 * @return the Sort Direction
	 */
	public int getDirection() {
		return _direction;
	}
}

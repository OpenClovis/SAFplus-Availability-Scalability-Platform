package com.clovis.logtool.ui.viewer;

import java.util.Date;

import org.eclipse.jface.viewers.IColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;

import com.clovis.logtool.record.Record;
import com.clovis.logtool.ui.LogDisplay;
import com.clovis.logtool.utils.LogConstants;
import com.clovis.logtool.utils.LogUtils;
import com.clovis.logtool.utils.UIColumn;

/**
 * LabelProvider for the log viewer.
 * 
 * @author Suraj Rajyaguru
 */
public class RecordViewerLabelProvider extends LabelProvider implements
		ITableLabelProvider, IColorProvider {

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object,
	 *      int)
	 */
	public Image getColumnImage(Object element, int columnIndex) {
		return null;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object,
	 *      int)
	 */
	public String getColumnText(Object element, int columnIndex) {
		Record record = (Record) element;
		UIColumn column = LogUtils.getUIColumns().get(columnIndex);

		int fieldIndex = column.getFieldIndex();
		String text = "";

		switch (column.getType()) {

		case LogConstants.TYPE_BYTE:
		case LogConstants.TYPE_SHORT:
		case LogConstants.TYPE_INT:
		case LogConstants.TYPE_LONG:
			text = record.getField(fieldIndex, true).toString();
			break;

		case LogConstants.TYPE_STRING:
			if (fieldIndex != -1) {
				text = record.getField(fieldIndex, true).toString();
			} else {
				text = LogDisplay.getInstance().getMessageFormatter().format(
						record);
			}
			break;

		case LogConstants.TYPE_DATE:
			text = new Date(((Long) record.getField(fieldIndex, true)).longValue() / 1000000)
					.toString();
			break;
		}
		return text;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.IColorProvider#getBackground(java.lang.Object)
	 */
	public Color getBackground(Object element) {
		return null;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.IColorProvider#getForeground(java.lang.Object)
	 */
	public Color getForeground(Object element) {
		Record record = (Record) element;

		if (((Long) record.getField(LogConstants.FIELD_INDEX_SEVERITY, false))
				.longValue() == 3) {
			return Display.getCurrent().getSystemColor(SWT.COLOR_DARK_RED);
		}

		return null;
	}
}

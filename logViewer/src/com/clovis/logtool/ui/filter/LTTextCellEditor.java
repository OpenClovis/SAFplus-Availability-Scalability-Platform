/**
 * 
 */
package com.clovis.logtool.ui.filter;

import org.eclipse.jface.viewers.TextCellEditor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Text;

/**
 * Text cell editor to specify the criterion string for filtering.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class LTTextCellEditor extends TextCellEditor {

	/**
	 * Constructs Text cell editor.
	 * 
	 * @param parent
	 *            the parent composite
	 */
	public LTTextCellEditor(Composite parent) {
		super(parent, SWT.BORDER | SWT.FLAT);
		Text text = (Text) getControl();

		GridData textData = new GridData(SWT.FILL, 0, true, false);
		text.setLayoutData(textData);

		text.setVisible(true);
		text.setBackground(Display.getCurrent().getSystemColor(
				SWT.COLOR_WHITE));
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.CellEditor#deactivate()
	 */
	public void deactivate() {
		fireCancelEditor();
	}
}

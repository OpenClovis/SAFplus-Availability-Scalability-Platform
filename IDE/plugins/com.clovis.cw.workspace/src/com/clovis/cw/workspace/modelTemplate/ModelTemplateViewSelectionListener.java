/**
 * 
 */
package com.clovis.cw.workspace.modelTemplate;

import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.widgets.List;

/**
 * Selection Listener for the model template view which loads the currently
 * selected model template.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class ModelTemplateViewSelectionListener extends SelectionAdapter {

	private ModelTemplateView _modelTemplateView;

	/**
	 * Constructor.
	 * 
	 * @param modelTemplateView
	 */
	public ModelTemplateViewSelectionListener(
			ModelTemplateView modelTemplateView) {
		_modelTemplateView = modelTemplateView;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.swt.events.SelectionAdapter#widgetSelected(org.eclipse.swt.events.SelectionEvent)
	 */
	@Override
	public void widgetSelected(SelectionEvent e) {
		List list = (List) e.widget;
		String modelTemplate = list.getSelection()[0];
		_modelTemplateView.selectModelTemplate(modelTemplate);
	}
}

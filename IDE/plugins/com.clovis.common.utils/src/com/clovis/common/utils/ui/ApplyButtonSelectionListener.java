package com.clovis.common.utils.ui;

import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;

import com.clovis.common.utils.ecore.Model;

/**
 * 
 * @author shubhada
 * 
 * Class to save the view model to model 
 * on selecting apply button
 *
 */
public class ApplyButtonSelectionListener
implements SelectionListener
{
	private Model _viewModel = null;
	private boolean _saveUp = false;
	
	/**
	 * Constructor
	 * @param viewModel - viewModel 
	 */
	public ApplyButtonSelectionListener(Model viewModel, boolean saveUp)
	{
		_viewModel = viewModel;
		_saveUp = saveUp;
		
	}

	/**
	 * @param e - SelectionEvent
	 */
	public void widgetDefaultSelected(SelectionEvent e)
	{
	}

	/**
	 * @param e - SelectionEvent
	 */
	public void widgetSelected(SelectionEvent e)
	{
		_viewModel.save(_saveUp);

	}

}

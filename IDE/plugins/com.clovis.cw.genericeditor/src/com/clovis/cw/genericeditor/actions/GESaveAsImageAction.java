/**
 * 
 */
package com.clovis.cw.genericeditor.actions;

import org.eclipse.gef.ui.actions.SelectionAction;

import com.clovis.cw.genericeditor.GenericEditor;
import com.clovis.cw.genericeditor.utils.EditorImageUtils;

/**
 * Action class for saving editor as Image.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class GESaveAsImageAction extends SelectionAction {

	private GenericEditor _editor;

	/**
	 * Constructor.
	 * 
	 * @param part
	 */
	public GESaveAsImageAction(GenericEditor part) {
		super(part);
		_editor = part;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.gef.ui.actions.WorkbenchPartAction#calculateEnabled()
	 */
	@Override
	protected boolean calculateEnabled() {
		return true;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.gef.ui.actions.WorkbenchPartAction#init()
	 */
	@Override
	protected void init() {
		setId("saveAsImage");
		setText("Save As Image");
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.action.Action#run()
	 */
	@Override
	public void run() {
		EditorImageUtils.save(_editor, _editor.getGraphicalViewer());
	}
}

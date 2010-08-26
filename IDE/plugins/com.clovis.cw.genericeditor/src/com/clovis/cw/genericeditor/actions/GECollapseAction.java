/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/actions/GECollapseAction.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor.actions;

import org.eclipse.gef.ui.actions.SelectionAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;

import com.clovis.cw.genericeditor.GenericEditor;
import com.clovis.cw.genericeditor.commands.CollapseCommand;
import com.clovis.cw.genericeditor.editparts.BaseEditPart;

public class GECollapseAction extends SelectionAction
{
	/**
     * Constructor
     * @param part Editor
     */
	public GECollapseAction(GenericEditor part)
	{
		super(part);
	}
	/**
	 * @see org.eclipse.gef.ui.actions.WorkbenchPartAction#calculateEnabled()
	 */
	protected boolean calculateEnabled()
	{
		return true;
	}
	/**
     * @see org.eclipse.gef.ui.actions.EditorPartAction#init()
     */
    protected void init()
    {
        setId("collapse");
        setText("Collapse");
    }
	/**
	 * @see org.eclipse.jface.action.IAction#run()
	 */
	public void run()
	{
		ISelection selection = getSelection();
		if (selection instanceof IStructuredSelection ) {
			BaseEditPart part = (BaseEditPart) ((IStructuredSelection) selection).getFirstElement();
			CollapseCommand cmd = new CollapseCommand(part);
			execute(cmd);
		}
	}
	
}

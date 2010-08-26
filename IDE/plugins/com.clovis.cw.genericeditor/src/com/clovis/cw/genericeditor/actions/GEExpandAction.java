/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/actions/GEExpandAction.java $
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
import com.clovis.cw.genericeditor.commands.ExpandCommand;
import com.clovis.cw.genericeditor.editparts.BaseEditPart;

public class GEExpandAction extends SelectionAction
{
	/**
	 * Constructor
	 * @param part Editor
	 */
	public GEExpandAction(GenericEditor part)
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
        setId("expand");
        setText("Expand");
    }
	/**
	 * @see org.eclipse.jface.action.IAction#run()
	 */
	public void run()
	{
		ISelection selection = getSelection();
		if (selection instanceof IStructuredSelection ) {
			BaseEditPart part = (BaseEditPart) ((IStructuredSelection) selection).getFirstElement();
			ExpandCommand cmd = new ExpandCommand(part);
			execute(cmd);
		}
	}
}

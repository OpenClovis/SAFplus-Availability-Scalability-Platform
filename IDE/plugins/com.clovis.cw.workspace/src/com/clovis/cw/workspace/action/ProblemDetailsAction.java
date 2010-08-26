/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/action/ProblemDetailsAction.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.action;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.workspace.project.ValidationConstants;

public class ProblemDetailsAction extends Action
//implements ISelectionChangedListener
{
	private TableViewer _tableViewer = null;
	/**
	 * Constructor
	 * @param viewer - Table Viewer for which auto correct
	 * option is provided 
	 *
	 */
	public ProblemDetailsAction(TableViewer viewer)
	{
		_tableViewer = viewer;
	}
	/**
	 * @param event - SelectionChangedEvent
	 */
	/*public void selectionChanged(SelectionChangedEvent event)
	{
		ISelection selection = event.getSelection();
		System.out.println(((IStructuredSelection) selection).getFirstElement());
	}*/
	/**
	 * Implementation of run method
	 */
	public void run()
	{
		IStructuredSelection sel = (IStructuredSelection) _tableViewer.getSelection();
		EObject selObj = (EObject) sel.getFirstElement();
		if (selObj != null)
		{
			String problemDesc = (String) EcoreUtils.getValue(selObj,
					ValidationConstants.PROBLEM_DESCRIPTION);
			MessageDialog.openInformation(_tableViewer.getTable().getShell(), "Problem Details", problemDesc);
		}
	}
}


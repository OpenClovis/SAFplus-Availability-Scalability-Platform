/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/actions/GEAutoArrangeAction.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor.actions;

import org.eclipse.gef.ui.actions.SelectionAction;

import com.clovis.cw.genericeditor.GenericEditor;
import com.clovis.cw.genericeditor.commands.AutoArrangeCommand;
import com.clovis.cw.genericeditor.editparts.BaseDiagramEditPart;

/**
 * @author pushparaj
 * Action class for Auto Arrange 
 */
public class GEAutoArrangeAction extends SelectionAction {
    private GenericEditor _editor;
	public GEAutoArrangeAction(GenericEditor part) {
		super(part);
		_editor = part;
	}

	protected boolean calculateEnabled() {
		return true;
	}
	
	protected void init()
    {
        setId("auto");
        setText("Auto Arrange");
    }
	
	public void run()
	{
		final BaseDiagramEditPart editPart = (BaseDiagramEditPart) _editor
				.getViewer().getRootEditPart().getContents();
		AutoArrangeCommand cmd = new AutoArrangeCommand(editPart);
		execute(cmd);
		_editor.propertyChange(null);
	}
}

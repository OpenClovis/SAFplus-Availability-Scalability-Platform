/*
 * @(#) $RCSfile: AlarmProfileDialog.java,v $
 * $Revision: #2 $ $Date: 2007/01/03 $
 *
 * Copyright (C) 2005 -- Clovis Solutions.
 * Proprietary and Confidential. All Rights Reserved.
 *
 * This software is the proprietary information of Clovis Solutions.
 * Use is subject to license terms.
 *
 */
/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/dialog/AlarmProfileDialog.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

/*
 * @(#) $RCSfile: AlarmProfileDialog.java,v $
 * $Revision: #2 $ $Date: 2007/01/03 $
 *
 * Copyright (C) 2005 -- Clovis Solutions.
 * Proprietary and Confidential. All Rights Reserved.
 *
 * This software is the proprietary information of Clovis Solutions.
 * Use is subject to license terms.
 *
 */
package com.clovis.cw.workspace.dialog;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.ui.dialog.PushButtonDialog;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * 
 * @author shubhada
 *
 * AlarmProfileDialog which will allow to add/edit alarms
 */
public class AlarmProfileDialog extends PushButtonDialog
{
    private IProject _project = null;
    private static final String DIALOG_TITLE = "Alarm Profile";
    private static AlarmProfileDialog instance    = null;
    /**
     * Gets current instance of Dialog
     * @return current instance of Dialog
     */
    public static AlarmProfileDialog getInstance()
    {
        return instance;
    }
    /**
     * Close the dialog.
     * Remove static instance
     * @return super.close()
     */
    public boolean close()
    {
        instance = null;
        return super.close();
    }
    /**
     * Open the dialog.
     * Set static instance to itself
     * @return super.open()
     */
    public int open()
    {
        instance = this;
        return super.open();
    }
    /**
     * @param shell parent Shell
     * @param eClass Eclass
     * @param value value
     */
    public AlarmProfileDialog(Shell shell, EClass eClass, Object value, IProject project) {
        super(shell, eClass, value);
        _project = project;
    }
    /**
     * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
     */
    protected Control createDialogArea(Composite parent)
    {
        ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
        Control control = super.createDialogArea(parent);
        getShell().setText(pdm.getProject().getName() + " - " + DIALOG_TITLE);
        return control;
    }
	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	protected void configureShell(Shell newShell) {
		super.configureShell(newShell);
		Rectangle bounds = Display.getCurrent().getClientArea();
		newShell.setBounds(bounds.width / 5, bounds.height / 5,
				3 * bounds.width / 5, 2 * bounds.height / 5);
	}
}

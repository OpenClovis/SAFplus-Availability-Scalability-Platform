/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/action/AddAlarmProfileAction.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.action;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.action.IAction;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IViewActionDelegate;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.ui.dialog.PushButtonDialog;
import com.clovis.cw.editor.ca.ResourceDataUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.project.utils.FormatConversionUtils;
import com.clovis.cw.workspace.WorkspacePlugin;
import com.clovis.cw.workspace.dialog.AlarmProfileAdapter;
import com.clovis.cw.workspace.dialog.AlarmProfileDialog;


/**
 * @author pushparaj
 *
 * Action class for adding alarm profile
 */
public class AddAlarmProfileAction extends CommonMenuAction implements
		IViewActionDelegate, IWorkbenchWindowActionDelegate
{
	
	/**
	 * Initialize Action.
	 * @param view View
	 */
	public void init(IViewPart view) {
		_shell = view.getViewSite().getShell();
	}

	public void dispose() {
		
	}

	public void init(IWorkbenchWindow window) {
		_shell = window.getShell();
	}
	
	public void run(IAction action)
	{
		
        if (_project != null) {
        	int actionStatus = canUpdateIM();
        	if(actionStatus == ACTION_CANCEL) {
        		return;
            } else if(actionStatus == ACTION_SAVE_CONTINUE) {
            	updateIM();
            }
            openAlarmDialog(_shell, _project);
        }
	}
    /**
     * 
     * @param shell
     * @param project
     */
    public static void openAlarmDialog(Shell shell, IProject project)
    {
        ArrayList deletedAlarmProfiles = new ArrayList();
        ProjectDataModel pdm = ProjectDataModel
        .getProjectDataModel(project);
        AlarmProfileAdapter adapter = new AlarmProfileAdapter(shell, deletedAlarmProfiles, pdm);
        Model model = pdm.getAlarmProfiles();
        EObject alarmInfoObj = (EObject) model.getEList().get(0);
        List alarmProfileList = (List) EcoreUtils.getValue(alarmInfoObj, "AlarmProfile");
        AlarmProfileDialog dialog = new AlarmProfileDialog(shell,
                (EClass) model.getEPackage().getEClassifier("AlarmProfile"),
                alarmProfileList, project);
        Model viewModel = dialog.getViewModel();
        EcoreUtils.addListener(viewModel.getEList(),
                adapter, 2);
        int ok = dialog.open();
        if (ok == PushButtonDialog.OK) {
            try {
                model.save(true);
                EcoreUtils.removeListener(viewModel.getEList(), adapter, 2);
                Iterator iterator = deletedAlarmProfiles.iterator();
                Model caModel = pdm.getCAModel();
                while (iterator.hasNext()) {
                    String deletedKey = (String) iterator.next();
                    List resList = ResourceDataUtils.getMoList(caModel.getEList());
                    for (int i = 0; i < resList.size(); i++) {
                        EObject obj = (EObject) resList.get(i);
                        List associatedAlarmList = ResourceDataUtils.
                        	getAssociatedAlarms(project, obj);
                        if (associatedAlarmList != null) {
	                        for (int j = 0; j < associatedAlarmList.size(); j++) {
	                            String alarm = (String) associatedAlarmList.get(j);
	                            if (deletedKey.equals(alarm)) {
	                                associatedAlarmList.remove(j);
	                            }
	                        }
                        }
                    }
                }
                FormatConversionUtils.convertToResourceFormat((EObject) caModel
						.getEList().get(0), ClassEditorConstants.EDITOR_TYPE);
				caModel.save(true);
            } catch (Exception e) {
                WorkspacePlugin.LOG.error("AlarmProfile save failed", e);
            }
        }
    }
}

/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/handler/LibSelectionHandler.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.handler;

import org.eclipse.emf.common.notify.Notification;

import com.clovis.cw.project.data.NotificationHandler;
import com.clovis.cw.project.data.ProjectDataModel;

public class LibSelectionHandler extends NotificationHandler {

	/**
	 * 
	 * @param n - Notification Object
	 * @param changedObj - Object which is changed
	 * @param dependentObj - The dependent object(s) which has to be 
	 * updated
	 * @param the features to updated in dependent object(s)
	 */
	public static void processNotifications(Notification n, Object changedObj,
            Object dependentObj, String [] featureNames,
            String [] referencePaths, ProjectDataModel pdm)
    {
		UpdateProject update = new UpdateProject(pdm);
		update.updateLibSelection();
	}

}

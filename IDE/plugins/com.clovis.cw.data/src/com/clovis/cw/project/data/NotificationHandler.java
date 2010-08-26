/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.data/src/com/clovis/cw/project/data/NotificationHandler.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.project.data;

import org.eclipse.emf.common.notify.Notification;

/**
 * 
 * @author shubhada
 *
 * Notification Handler interface for handling notifications
 * for dependency purpose
 */
public class NotificationHandler
{
    /**
     * Blank implementation subclasses should override to provide implementation
     * @param n - Notification Object
     * @param changedObj - Object which is changed
     * @param dependentObj - The dependent object(s) which has to be 
     * updated
     * @param the features to updated in dependent object(s)
     */
    public static void processNotifications(Notification n, Object changedObj,
            Object dependentObj, String[] featureNames,
            String [] referencePaths, ProjectDataModel pdm)
    {
        
    }
}

/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.data/src/com/clovis/cw/project/data/NodeChangeListener.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.project.data;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IPath;
import org.eclipse.core.runtime.Path;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.impl.AdapterImpl;
import org.eclipse.emf.ecore.EAttribute;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.data.ICWProject;

/**
 * @author pushparaj
 *
 * This will creates workspace directory structure
 */
public class NodeChangeListener extends AdapterImpl
{
    private IProject         _project;

    /**
     * Constructor
     *
     * @param project
     *            Project
     */
    public NodeChangeListener(IProject project)
    {
        _project = project;
    }
    /**
     * @param notification
     *            Notofication event
     */
    public void notifyChanged(Notification notification)
    {
        switch (notification.getEventType()) {
        case Notification.SET:
            Object notifier = notification.getNotifier();
            if (notifier instanceof EObject) {
                EObject obj = (EObject) notifier;
                String className = obj.eClass().getName();
                if (className.equals("Node")) {
                    Object feature = notification.getFeature();
                    if (feature != null && feature instanceof EAttribute) {
                        String featureName = ((EAttribute) feature).getName();
                        if (featureName.equals("name")) {
                            String oldName = (String) notification
                                    .getOldValue();
                            String newName = (String) notification
                                    .getNewValue();
                            try {
                                if (!oldName.equals(newName)) {
                                    if (_project.exists(new Path(oldName))) {
                                    _project.getFolder(new Path(oldName)).move(
                                            new Path(newName), true, null);
                                    }
                                }
                            } catch (Exception e) {
                                e.printStackTrace();
                            }
                        }
                    }
                }
            }
            break;
        case Notification.ADD:
            if (notification.getNewValue() instanceof EObject) {
                EObject obj = (EObject) notification.getNewValue();
                String name = obj.eClass().getName();
                if (name.equals("Node")) {
                    EcoreUtils.addListener(obj, this, 2);
                    try {
                        createNodeFolder(EcoreUtils.getName(obj));
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }
            }
            break;
        case Notification.REMOVE:
            if (notification.getOldValue() instanceof EObject) {
                EObject obj = (EObject) notification.getOldValue();
                String name = obj.eClass().getName();
                if (name.equals("Node")) {
                    EcoreUtils.removeListener(obj, this, 2);
                    try {
                        String nodeName = EcoreUtils.getName(obj);
                        _project.getFolder(new Path(nodeName)).delete(true,
                                true, null);
                    } catch (CoreException e) {
                        e.printStackTrace();
                    }
                }
            }
            break;
        }
    }
    /**
     * Creates directory structure for nodes
     * @param nodeName  Node name
     */
    private void createNodeFolder(String nodeName) throws CoreException
    {
        IPath nodePath = new Path(nodeName);
        if (!_project.exists(nodePath)) {
            _project.getFolder(nodePath).create(true, true, null);
        _project.getFolder(
                nodePath.append(ICWProject.CW_PROFILES_FOLDER_NAME))
                .create(true, true, null);
        /*_project.getFolder(
                nodePath.append(ICWProject.CW_SAF_COMPONENTS_FOLDER_NAME))
                .create(true, true, null);
        IPath compPath = nodePath
                .append(ICWProject.CW_ASP_COMPONENTS_FOLDER_NAME);
        _project.getFolder(compPath).create(true, true, null);
        _project.getFolder(compPath.append(ICWProject
               .CW_BOOT_TIME_COMPONENTS_FOLDER_NAME)).create(true, true, null);
        _project.getFolder(compPath.append(ICWProject
               .CW_BUILD_TIME_COMPONENTS_FOLDER_NAME)).create(true, true, null);*/
        }
    }
}

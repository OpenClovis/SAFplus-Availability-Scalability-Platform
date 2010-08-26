/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/action/ActionClassFactory.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.action;

import java.lang.reflect.Method;
import java.util.HashMap;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.TreeViewer;

import com.clovis.common.utils.ClovisUtils;
import com.clovis.common.utils.UtilsPlugin;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;

/**
 *
 * @author shubhada
 * Action Factory which creates the Action per EClass
 */
public class ActionClassFactory
{
    private HashMap _eclassInstanceMap = new HashMap();
    private static ActionClassFactory actionFactory = null;
    private static final Log LOG = Log.getLog(UtilsPlugin.getDefault());

    /**
     *
     * @return static instance of the ActionClassFactory
     */
    public static ActionClassFactory getActionClassFactory()
    {
        if (actionFactory == null) {
            actionFactory =  new ActionClassFactory();
            return actionFactory;
        } else {
            return actionFactory;
        }
    }
    /**
     * Maintains the action instances per eClass and returns them.
     * If not found in map, creates new instance
     * @param eClass Eclass
     * @return Validator istance per EClass
     */
    public TreeAction getAction(EClass eClass, TreeViewer treeViewer, Dialog dialog, EObject rootObj)
    {
        TreeAction action = (TreeAction) _eclassInstanceMap.
            get(eClass);
        if (action == null) {
            String custom =
                EcoreUtils.getAnnotationVal(eClass, null, "actionClass");
            if (custom != null) {
                return getCustomActionClass(custom, treeViewer, dialog, rootObj);
            } else {
                action = new TreeAction(treeViewer, dialog, rootObj);
                _eclassInstanceMap.put(eClass, action);
                return action;
            }
        } else {
            return action;
        }

    }
    /**
     * Get Custom Action Class for given EClass.
     * @param customActionClass Name and full path of Action class.
     * @param treeViewer  - TreeViewer with which action is associated
     * @param dialog - Preference Dialog of TreeViewer
     * @param rootObj - PreferenceTree rootObj 
     * @return Custom Action (null in case of errors)
     */
    private TreeAction getCustomActionClass(
            String customActionClass, TreeViewer treeViewer, Dialog dialog, EObject rootObj)
    {
        TreeAction action = null;
        Class actionClass = ClovisUtils.loadClass(customActionClass);
        try {
            Class[] argType = {
                    TreeViewer.class,
                    Dialog.class,
                    EObject.class
            };
            if (actionClass != null) {
                Method met  = actionClass.
                    getMethod("createAction", argType);
                Object[] args = {treeViewer, dialog, rootObj};
                action = (TreeAction) met.invoke(null, args);
            }
        } catch (NoSuchMethodException e) {
            LOG.error("Custom editor does not have createAction() method", e);
        } catch (Throwable th) {
            LOG.
            error("Unhandled error while creating action class:" + customActionClass, th);
        }
        return action;
    }
}

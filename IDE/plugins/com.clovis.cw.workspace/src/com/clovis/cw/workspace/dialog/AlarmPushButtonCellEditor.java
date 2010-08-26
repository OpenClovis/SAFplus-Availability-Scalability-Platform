/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/dialog/AlarmPushButtonCellEditor.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.dialog;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.common.notify.NotifyingList;

import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Composite;

import org.eclipse.jface.viewers.CellEditor;

import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.ui.factory.PushButtonCellEditor;
import com.clovis.cw.editor.ca.dialog.AlarmRuleDialog;
/**
 * @author pushparaj
 *
 * CellEditor for push button
 */
public class AlarmPushButtonCellEditor extends PushButtonCellEditor
{
    /**
     * Constructor.
     * @param p   Parent composite
     * @param env Environment
     */
    private AlarmPushButtonCellEditor(Composite p, Environment env, EStructuralFeature ref)
    {
        super(p, (EReference) ref, env);
    }
    /**
    *
    * @param parent  Composite
    * @param feature EStructuralFeature
    * @param env Environment
    * @return cell Editor
    */
   public static CellEditor createEditor(Composite parent,
           EStructuralFeature feature, Environment env)
   {
       return new AlarmPushButtonCellEditor(parent, env, feature);
   }
   /**
    * Open Dialog.
    * @param parent Parent
    * @return null.
    */
   protected Object openDialogBox(Control parent)
   {
       
       /*EObject eObject = (EObject) getValue();
       NotifyingList list = (NotifyingList) _parentEnv.getValue("model");
       new AlarmRuleDialog(parent.getShell(), eObject, list).open();*/
       return null;
   }
}

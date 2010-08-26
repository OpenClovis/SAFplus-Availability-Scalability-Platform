/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/dialog/AssociateResourcePushButtonCellEditor.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.dialog;

import java.util.List;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.dialogs.DialogPage;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.DialogCellEditor;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.cw.editor.ca.ResourceDataUtils;
import com.clovis.cw.editor.ca.dialog.AssociateResourcesDialog;
import com.clovis.cw.project.data.ProjectDataModel;
/**
 * @author Shubhada
 *
 * CellEditor for Associate Resource for the component editor
 */
public class AssociateResourcePushButtonCellEditor extends DialogCellEditor
{
    private Environment _parentEnv = null;
    private EStructuralFeature _feature = null;
    /**
     * Constructor.
     * @param p   Parent composite
     * @param env Environment
     */
    private AssociateResourcePushButtonCellEditor(Composite p, Environment env, EStructuralFeature ref)
    {
        super(p);
        _parentEnv = env;
        _feature = ref;
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
       return new AssociateResourcePushButtonCellEditor(parent, env, feature);
   }
   /**
    * Open Dialog.
    * @param parent Parent
    * @return null.
    */
   protected Object openDialogBox(Control parent)
   {
       
       IProject project = (IProject) _parentEnv.getValue("project");
       Shell shell = (Shell) _parentEnv.getValue("shell");
       StructuredSelection sel = (StructuredSelection) _parentEnv.
           getValue("selection");
       EObject selObj = (EObject) sel.getFirstElement();
       ProjectDataModel pdm =  ProjectDataModel.getProjectDataModel(project);
       List editorList = pdm.getCAModel().getEList();
       List resourcesList = ResourceDataUtils.getResourcesList(editorList);
       AssociateResourcesDialog dialog = new AssociateResourcesDialog(
               shell, project, (EList) resourcesList, selObj);
       dialog.open();
       return null;
   }
   /**
    * Creates the button for this cell editor under the given parent control.
    * <p>
    * The default implementation of this framework method creates the button 
    * display on the right hand side of the dialog cell editor. Subclasses
    * may extend or reimplement.
    * </p>
    *
    * @param parent the parent control
    * @return the new button control
    */
   protected Button createButton(Composite parent) {
       Button result = super.createButton(parent);
       result.setText("Edit..."); //$NON-NLS-1$
       return result;
   }
   /**
    * @param value Object
    * Blank implementation of update contents to avoid blank label
    */
   protected void updateContents(Object value)
   {
   }
   /**
    * When Cell Editor is activated, the Tooltip message
    * is displayed on the Message Area of the Containing 
    * dialog 
    */
   public void activate()
   {
      /* String tooltip =
           EcoreUtils.getAnnotationVal(_feature, null, "tooltip");
       Object container = _parentEnv.getValue("container");
       if (tooltip != null && container!= null) {
           if (container instanceof PreferenceDialog) {
           ((PreferenceDialog) container).setMessage(tooltip,
                   IMessageProvider.INFORMATION);
           } else if (container instanceof TitleAreaDialog) {
           ((TitleAreaDialog) container).setMessage(tooltip,
                   IMessageProvider.INFORMATION);
           } else if (container instanceof DialogPage) {
               ((DialogPage) container).setMessage(tooltip,
                       IMessageProvider.INFORMATION);
           }
       }*/
       super.activate();
   }
   
}

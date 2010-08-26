package com.clovis.cw.workspace.dialog;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.ui.dialog.PushButtonDialog;
import com.clovis.common.utils.ui.factory.PushButtonCellEditor;

/**
 * PushButtonCellEditor for PM config interval
 * @author Pushparaj
 *
 */
public class PMConfigPushButtonCellEditor extends PushButtonCellEditor{
	
	private EReference _ref = null;
    private Environment _parentEnv = null;
    /**
    *
    * @param parent Composite
    * @param ref EReference
    * @param parentEnv Environment
    */
   public PMConfigPushButtonCellEditor(Composite parent, EReference ref,
           Environment parentEnv)
   {
       super(parent, ref, parentEnv);
       _ref = ref;
       _parentEnv = parentEnv;
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
      return new PMConfigPushButtonCellEditor(parent, (EReference) feature, env);
  }
  /**
   * @param cellEditorWindow - Control
   * @return null
   */
  protected Object openDialogBox(Control cellEditorWindow)
  {
	  EObject obj = (EObject) _parentEnv.getValue("model");
	  new PushButtonDialog(getControl().
	            getShell(), _ref.getEReferenceType(), (EList<EObject>)EcoreUtils.getValue(obj, _ref.getName()), _parentEnv).open();
      return null;
  }
 
}

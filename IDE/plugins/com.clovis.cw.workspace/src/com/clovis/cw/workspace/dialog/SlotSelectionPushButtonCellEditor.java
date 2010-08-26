/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/dialog/SlotSelectionPushButtonCellEditor.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.dialog;

import java.util.List;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;

import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.ui.dialog.SelectionListDialog;
import com.clovis.common.utils.ui.factory.PushButtonCellEditor;

/**
*
* @author shubhada
* PushButtonCellEditor which displays the values to be chosen in a List format
* from which user can choose the values, on clicking the push button. 
*/
public class SlotSelectionPushButtonCellEditor extends PushButtonCellEditor
{
    private EReference _ref = null;
    private Environment _parentEnv = null;
    /**
    *
    * @param parent Composite
    * @param ref EReference
    * @param parentEnv Environment
    */
   public SlotSelectionPushButtonCellEditor(Composite parent, EReference ref,
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
      return new
      SlotSelectionPushButtonCellEditor(parent, (EReference) feature, env);
  }
  /**
   * @param cellEditorWindow - Control
   * @return null
   */
  protected Object openDialogBox(Control cellEditorWindow)
  {
      List selList = new ClovisNotifyingListImpl();
      List origList = new ClovisNotifyingListImpl();
      StructuredSelection sel = (StructuredSelection) _parentEnv.
          getValue("selection");
      EObject selObj = (EObject) sel.getFirstElement();
      EObject nodeTypesObj = (EObject) selObj.eGet(_ref);
      if (nodeTypesObj == null) {
          nodeTypesObj = EcoreUtils.createEObject(_ref.getEReferenceType(), true);
          selObj.eSet(_ref, nodeTypesObj);
      }
      EReference classTypeRef = (EReference) nodeTypesObj.eClass().
          getEStructuralFeature("classType");
      selList = (List) nodeTypesObj.eGet(classTypeRef);
      List nodesList = BootTime.getInstance().getNodesList(); 
      for (int i = 0; i < nodesList.size(); i++) {
          EObject nodeObj = (EObject) nodesList.get(i);
          EObject eobj = EcoreUtils.createEObject(
                  classTypeRef.getEReferenceType(), true);
          EcoreUtils.setValue(eobj, "name", EcoreUtils.getName(nodeObj));
          origList.add(eobj);
      }
      new SelectionListDialog(getControl().
              getShell(), "Select Node Types", selList, origList).open();
      return null;
  }
}

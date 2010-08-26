/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/dialog/SlotsPushButtonCellEditor.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.dialog;

import java.util.HashMap;
import java.util.Iterator;
import java.util.List;

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
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;

/**
*
* @author shubhada
* PushButtonCellEditor which does some specific actions 
* before opening pushbutton dialog on clicking the push button. 
*/
public class SlotsPushButtonCellEditor extends PushButtonCellEditor
{
    private EReference _ref = null;
    private Environment _parentEnv = null;
    /**
    *
    * @param parent Composite
    * @param ref EReference
    * @param parentEnv Environment
    */
   public SlotsPushButtonCellEditor(Composite parent, EReference ref,
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
      SlotsPushButtonCellEditor(parent, (EReference) feature, env);
  }
  /**
   * @param cellEditorWindow - Control
   * @return null
   */
  protected Object openDialogBox(Control cellEditorWindow)
  {
      EObject selObj = (EObject) _parentEnv.
          getValue("model");
      int maxSlots = 0;
      List resList = BootTime.getInstance().getResourceList();
      EObject rootObject = (EObject) resList.get(0);
      List list = (EList) rootObject.eGet(rootObject.eClass().getEStructuralFeature(ClassEditorConstants.CHASSIS_RESOURCE_REF_NAME));
      EObject resObj = (EObject) list.get(0);
      maxSlots = ((Integer) EcoreUtils.getValue(resObj,
                  ClassEditorConstants.CHASSIS_NUM_SLOTS)).intValue();
      List slotList = (List) EcoreUtils.getValue(selObj, "slot");
      HashMap SlotObjectNumberMap = new HashMap();
      Iterator iterator = slotList.iterator();
      while (iterator.hasNext()) {
          EObject slot = (EObject) iterator.next();
          int slotNumber = ((Integer) EcoreUtils.getValue(
                  slot, "slotNumber")).intValue();
          if (slotNumber > maxSlots) {
              iterator.remove();
              SlotObjectNumberMap.remove(slot);
          } else {
              SlotObjectNumberMap.put(slot, String.valueOf(slotNumber));
          }
      }
      for (int i = 0; i < maxSlots; i++) {
          EObject slotObj = EcoreUtils.createEObject(
                  _ref.getEReferenceType(), true);
          EcoreUtils.setValue(slotObj, "slotNumber", String.valueOf(i + 1));
          if (!SlotObjectNumberMap.containsValue(String.valueOf(i + 1))) {
              slotList.add(slotObj);
//                Select all the node types for this slot initially
              EReference slotsRef = (EReference) slotObj.eClass().
                  getEStructuralFeature("classTypes");
              EObject nodeTypesObj = (EObject) slotObj.eGet(slotsRef);
              if (nodeTypesObj == null) {
                  nodeTypesObj = EcoreUtils.createEObject(slotsRef.getEReferenceType(), true);
                  slotObj.eSet(slotsRef, nodeTypesObj);
              }
              EReference classTypeRef = (EReference) nodeTypesObj.eClass().
                  getEStructuralFeature("classType");
              List nodeTypeList = (List) nodeTypesObj.eGet(classTypeRef);
              List nodesList = BootTime.getInstance().getNodesList(); 
              for (int j = 0; j < nodesList.size(); j++) {
                  EObject nodeObj = (EObject) nodesList.get(j);
                  EObject eobj = EcoreUtils.createEObject(
                          classTypeRef.getEReferenceType(), true);
                  EcoreUtils.setValue(eobj, "name", EcoreUtils.getName(nodeObj));
                  nodeTypeList.add(eobj);
              }
          } 
      }
      new PushButtonDialog(getControl().getShell(),
              _ref.getEReferenceType(), getValue(), _parentEnv).open();
  
  return null;
  }
}

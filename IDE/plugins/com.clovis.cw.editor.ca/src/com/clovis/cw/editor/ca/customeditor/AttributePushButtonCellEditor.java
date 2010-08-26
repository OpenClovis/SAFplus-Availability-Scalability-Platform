/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/customeditor/AttributePushButtonCellEditor.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.customeditor;

import java.util.HashMap;
import java.util.List;
import java.util.Vector;

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
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.dialog.WebEMSDialog;

/**
 * 
 * @author swapnesh
 * Attribute Push Button cell editor to show the list of selectable
 * attributes for a category
 */
public class AttributePushButtonCellEditor extends PushButtonCellEditor
{
	private EReference _ref = null;
    private Environment _env = null;
    /**
    *
    * @param parent Composite
    * @param ref EReference
    * @param parentEnv Environment
    */
   public AttributePushButtonCellEditor(Composite parent, EReference ref,
           Environment parentEnv)
   {
       super(parent, ref, parentEnv);
       _ref = ref;
       _env = parentEnv;
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
      AttributePushButtonCellEditor(parent, (EReference) feature, env);
  }
  /**
   * @param cellEditorWindow - Control
   * @return null
   */
  protected Object openDialogBox(Control cellEditorWindow)
  {
      List selList = new ClovisNotifyingListImpl();
      List origList = new ClovisNotifyingListImpl();
      List resourceList = WebEMSDialog.getInstance().getResourceList();
      String resName = WebEMSDialog.getInstance().
      	getSelectedNodePreference();
      List attrList = WebEMSDialog.getAllAttributesOfResource(resName,
              resourceList);
      List attrNamesList = new Vector();
      HashMap attrNameTypeMap = new HashMap();
      for (int i = 0; i < attrList.size(); i++) {
    	  EObject attrObj = (EObject) attrList.get(i);
    	  attrNamesList.add(EcoreUtils.getName(attrObj));
    	  String attrType = EcoreUtils.getValue(attrObj, ClassEditorConstants.
                  ATTRIBUTE_TYPE).toString();
    	  int multiplicity = Integer.parseInt(EcoreUtils.getValue(attrObj,
                  ClassEditorConstants.ATTRIBUTE_MULTIPLICITY).toString());
          attrNameTypeMap.put(EcoreUtils.getName(attrObj),
                  WebEMSDialog.getAttrType(attrType, multiplicity));
      }
      List categoryList = (List) _env.getValue("model");
      StructuredSelection sel = (StructuredSelection) _env.
      	getValue("selection");
      EObject selCategory = (EObject) sel.getFirstElement();
      processAttributesList(categoryList, attrNamesList, selCategory);
      for (int i = 0; i < attrNamesList.size(); i++) {
          String attrName = (String) attrNamesList.get(i);
    	  EObject attrObj = EcoreUtils.createEObject(
    			  _ref.getEReferenceType(), true);
    	  origList.add(attrObj);
    	  EcoreUtils.setValue(attrObj, "NAME", attrName);
          if (attrNameTypeMap.get(attrName) != null) {
              EcoreUtils.setValue(attrObj, "TYPE", (String)
                      attrNameTypeMap.get(attrName));
          }
      }
      selList = (List) EcoreUtils.getValue(selCategory, "ATTRIBUTE");
      new SelectionListDialog(getControl().
              getShell(), "Select Attributes for category", selList, origList).open();
      return null;
  }
  
  /**
   * 
   * @param categoryList - List of All categories except selected category
   * @param attrList - List of all attributes of the resource
   */
  private void processAttributesList(List categoryList, List attrList,
		  EObject selCatObj)
  {
	  for (int i = 0; i < categoryList.size(); i++) {
			EObject catObj = (EObject) categoryList.get(i);
			if (!catObj.equals(selCatObj)) {
				List catAttrList = (List) EcoreUtils.getValue(catObj, "ATTRIBUTE");
				for (int j = 0; j < catAttrList.size(); j++) {
					EObject catAttr = (EObject) catAttrList.get(j);
					attrList.remove(EcoreUtils.getName(catAttr));
				}
			}
	  }
  }

}

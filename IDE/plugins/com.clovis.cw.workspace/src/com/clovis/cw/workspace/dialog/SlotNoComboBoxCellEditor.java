/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/dialog/SlotNoComboBoxCellEditor.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.dialog;

import java.util.List;
import java.util.Vector;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ComboBoxCellEditor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
/**
 *
 * @author shubhada
 * Combo Box cell editor to display physical slot numbers.
 */
public class SlotNoComboBoxCellEditor extends ComboBoxCellEditor
{
   private Environment _env = null;
   private EStructuralFeature _feature = null;
	/**
     *
     * @param parent Composite
     * @param feature EStructuralFeature
     * @param env Environment
     */
    public SlotNoComboBoxCellEditor(Composite parent,
            EStructuralFeature feature, Environment env)
    {
        super(parent, new String[0], SWT.BORDER | SWT.READ_ONLY);
        _env = env;
        _feature = feature;
    }

    /**
     * Create Editor Instance.
     * @param parent  Composite
     * @param feature EStructuralFeature
     * @param env     Environment
     * @return cell Editor
     */
    public static CellEditor createEditor(Composite parent,
            EStructuralFeature feature, Environment env)
    {
        return new SlotNoComboBoxCellEditor(parent, feature, env);
    }
    /**
     *@param value - Value to be set
     */
    protected void doSetValue(Object value)
    {
        setItems(getComboValues());
        int index = 0;
        String[] items = this.getItems();
        for (int i = 0; i < items.length; i++) {
            if (items[i].equals(value)) {
                index = i;
            }
        }
        super.doSetValue(new Integer(index));
    }
   /**
    *@return the Value
    */
   protected Object doGetValue()
   {
       if (((Integer) super.doGetValue()).intValue() == -1) {
           return new String("1");
       } else {
           return getItems()[((Integer) super.doGetValue()).intValue()];
       }
   }
   /**
    * @return Combo Values
    */
   protected String[] getComboValues()
   {
       List comboValues = new Vector();
       List resList = BootTime.getInstance().getResourceList();
       int maxSlots = 0;
       EObject rootObject = (EObject) resList.get(0);
       List list = (EList) rootObject.eGet(rootObject.eClass().getEStructuralFeature(ClassEditorConstants.CHASSIS_RESOURCE_REF_NAME));
       EObject resObj = (EObject) list.get(0);
       maxSlots = ((Integer) EcoreUtils.getValue(resObj,
                   ClassEditorConstants.CHASSIS_NUM_SLOTS)).intValue();
       for (int i = 0; i < maxSlots; i++) {
           comboValues.add(String.valueOf(i + 1));
       }
       String [] values = new String[comboValues.size()];
       for (int i = 0; i < comboValues.size(); i++) {
           values[i] = (String) comboValues.get(i);
       }
       return values;
   }
   /**
    * When Cell Editor is activated, the Tooltip message
    * is displayed on the Message Area of the Containing 
    * dialog 
    */
	public void activate()
	{
		/*Object container = _env.getValue("container");
		if (container!= null && container instanceof PreferenceDialog) {
			String tooltip =
	            EcoreUtils.getAnnotationVal(_feature, null, "tooltip");
			((PreferenceDialog) container).setMessage(tooltip,
					IMessageProvider.INFORMATION);
		} else if (container!= null && container instanceof TitleAreaDialog) {
			String tooltip =
	            EcoreUtils.getAnnotationVal(_feature, null, "tooltip");
			((TitleAreaDialog) container).setMessage(tooltip,
					IMessageProvider.INFORMATION);
		}*/
		super.activate();
	}
   
}

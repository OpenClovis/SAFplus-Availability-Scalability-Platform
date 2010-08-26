/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/AssociatedDOComboBoxCellEditor.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca;

import java.util.List;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ComboBoxCellEditor;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;


/**
 * @author Manish
 *
 * AssociatedDOComboBoxCellEditor which puts DO list of Resource in the combobox
 */
public class AssociatedDOComboBoxCellEditor extends ComboBoxCellEditor implements ClassEditorConstants
{
    private Environment        _env     = null;
    /**
     * Constructor.
     *
     * @param parent
     *            Composite
     * @param feature
     *            EStructuralFeature
     * @param env
     *            Environment
     */
    public AssociatedDOComboBoxCellEditor(Composite parent,
            EStructuralFeature feature, Environment env)
    {
        super(parent, new String[0],SWT.BORDER | SWT.READ_ONLY);
        _env = env;
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
        return new AssociatedDOComboBoxCellEditor(parent, feature, env);
    }
    /**
     * @return Combo Values
     */
    protected String[] getComboValues()
    {
    	EObject resourceObj = (EObject) _env.getValue("topLevelObject");  
    	if (resourceObj != null) {
        	EClass eclass = resourceObj.eClass();
        	EReference dosRef = (EReference)eclass.getEStructuralFeature(DEVICE_OBJECT);
        	List doList = (List)resourceObj.eGet(dosRef);        	
        	String[] values = new String[doList.size()];
	        for (int i = 0; i < doList.size(); i++) {
	             EObject obj = (EObject) doList.get(i);
	              values[i] = (String) EcoreUtils.getValue(obj,
                          ClassEditorConstants.DEVICE_ID);
	        }
	        return values;
        }
        return new String[0];
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
		   return new String("");
	   } else {
		   return getItems()[((Integer) super.doGetValue()).intValue()];
	   }
   }
}

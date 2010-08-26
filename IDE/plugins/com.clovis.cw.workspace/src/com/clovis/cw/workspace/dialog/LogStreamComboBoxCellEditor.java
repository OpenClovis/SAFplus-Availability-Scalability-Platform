/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/dialog/LogStreamComboBoxCellEditor.java $
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
import org.eclipse.jface.viewers.ComboBoxCellEditor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;

import com.clovis.common.utils.menu.Environment;

/**
 * @author Pushparaj
 *
 * LogStreamComboBoxCellEditor which puts specific items in to the
 */
public class LogStreamComboBoxCellEditor extends ComboBoxCellEditor
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
    public LogStreamComboBoxCellEditor(Composite parent,
            EStructuralFeature feature, Environment env)
    {
        super(parent, new String[0], SWT.BORDER | SWT.READ_ONLY);
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
        return new LogStreamComboBoxCellEditor(parent, feature, env);
    }
    /**
     * @return Combo Values
     */
    protected String[] getComboValues()
    {
        String values[] = null;
    	Environment parentEnv = _env.getParentEnv();
        EObject eobj = (EObject) parentEnv.getValue("model");
        if (eobj != null && eobj.eClass().getName().equals("LOG")) {
			EReference formatRef = (EReference) eobj.eClass().getEStructuralFeature("format");
			List formatObjs = (List) eobj.eGet(formatRef);
			values = new String[formatObjs.size()];
			for (int i = 0; i < formatObjs.size(); i++) {
				EObject formatObj = (EObject) formatObjs.get(i);
				values[i] = (String) formatObj.eGet(formatObj.eClass().getEStructuralFeature("name"));
			}
			return values;
		}
        return new String[0];
    }

    /**
	 * @param value -
	 *            Value to be set
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
    	   try {
    		   return getItems()[((Integer) super.doGetValue()).intValue()];
    	   }
    	   catch(Exception e) {
    		   return "";
    	   }
       }
   }
}

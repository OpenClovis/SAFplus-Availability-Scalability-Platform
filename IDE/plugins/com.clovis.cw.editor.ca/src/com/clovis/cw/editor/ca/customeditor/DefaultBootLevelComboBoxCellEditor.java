/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/customeditor/DefaultBootLevelComboBoxCellEditor.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.customeditor;

import java.util.List;
import java.util.Vector;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ComboBoxCellEditor;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;

/**
 * 
 * @author shubhada
 *
 * ComboBox to show the user defined boot levels as selectable
 * combo box items
 */
public class DefaultBootLevelComboBoxCellEditor extends ComboBoxCellEditor
{
    private Environment _env = null;
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
    public DefaultBootLevelComboBoxCellEditor(Composite parent,
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
        return new DefaultBootLevelComboBoxCellEditor(parent, feature, env);
    }
    /**
     * @return Combo Values
     */
    protected String[] getComboValues()
    {
        List comboValues = new Vector();
        // Add 5 to the combo box because this is ASP bootlevel which
        //can also be default boot level
        comboValues.add("5");
        
        EObject selBootConfigObj = (EObject)((StructuredSelection) _env.
                getValue("selection")).getFirstElement();
        EObject bootLevelsObj = (EObject) EcoreUtils.getValue(selBootConfigObj,
                "bootLevels");
        List bootLevels = (List) EcoreUtils.getValue(bootLevelsObj,
            "bootLevel");
        for (int i = 0; i < bootLevels.size(); i++) {
            EObject bootLevelObj = (EObject) bootLevels.get(i);
            int level = ((Integer) EcoreUtils.getValue(bootLevelObj,
                    "level")).intValue();
            comboValues.add(String.valueOf(level));
        }
        String [] values = new String[comboValues.size()];
        for (int i = 0; i < comboValues.size(); i++) {
            values[i] = (String) comboValues.get(i);
        }
        return values;
        
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

/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/TypesComboBoxCellEditor.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca;

import java.util.List;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ComboBoxCellEditor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.widgets.Composite;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.ui.table.FormView;
import com.clovis.cw.editor.ca.constants.SafConstants;
import com.clovis.cw.editor.ca.dialog.NodeProfileDialog;

/**
 * @author shubhada
 *
 * ClovisComboBoxCellEditor which puts Types in to combo Box.
 * Types are taken from the component editor.
 */
public class TypesComboBoxCellEditor extends ComboBoxCellEditor
{
   private Environment _env = null;
   private EStructuralFeature _feature = null;
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
    public TypesComboBoxCellEditor(Composite parent,
            EStructuralFeature feature, Environment env)
    {
        super(parent, new String[0], SWT.BORDER | SWT.READ_ONLY);
        _env = env;
        _feature = feature;

        CCombo comboBox = (CCombo) getControl();
		comboBox.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent event) {
				focusLost();
			}
		});
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
        return new TypesComboBoxCellEditor(parent, feature, env);
    }
    /**
     * @return Combo Values
     */
    protected String[] getComboValues()
    {
        EObject eobj = (EObject) _env.getValue("model");
        List objList = null;
        if (eobj != null) {
            if (eobj.eClass().getName().equals(
                    SafConstants.NODE_INSTANCELIST_ECLASS)) {
                objList = NodeProfileDialog.getInstance().getNodeList();
            } else if (eobj.eClass().getName().equals(
                    SafConstants.SG_INSTANCELIST_ECLASS)) {
                objList = NodeProfileDialog.getInstance().
                    getServiceGroupsList();
            }
            String [] comboValues = new String[objList.size()];
            for (int i = 0; i < objList.size(); i++) {
                EObject obj = (EObject) objList.get(i);
                String type = EcoreUtils.getName(obj);
                comboValues[i] = type;
            }
            return comboValues;
          }
        return new String[0];
      }


    /**
     *@param value - Value to be set
     */
    protected void doSetValue(Object value)
    {
        setItems(getComboValues());
        int index = -1;
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
    /**
     * Dont Deactivate editor in FormView.
     */
    public void deactivate()
    {
        if (_env instanceof FormView) {
            fireCancelEditor();
        } else {
            super.deactivate();
        }
    }
}

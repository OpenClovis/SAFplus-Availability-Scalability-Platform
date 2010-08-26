/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/CustomComboBoxCellEditor.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca;

import java.util.List;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
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
 * ClovisComboBoxCellEditor which puts specific items in to the
 */
public class CustomComboBoxCellEditor extends ComboBoxCellEditor
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
    public CustomComboBoxCellEditor(Composite parent,
            EStructuralFeature feature, Environment env)
    {
        super(parent, new String[0], SWT.BORDER | SWT.READ_ONLY);
        _env = env;

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
        return new CustomComboBoxCellEditor(parent, feature, env);
    }
    /**
     * @return Combo Values
     */
    protected String[] getComboValues()
    {
      EObject eobj = (EObject) _env.getValue("model");
        if (eobj != null) {
            if (eobj.eClass().getName().equals(SafConstants.
                    SERVICEUNIT_INSTANCELIST_ECLASS)) {
            	EObject nodeObj = eobj.eContainer().eContainer();
            	String name = (String) EcoreUtils
                        .getValue(nodeObj, "type");
                List objects = (List) NodeProfileDialog.getInstance().
                        getNodeSuMap().get(name);
                if (objects != null) {
                    String[] values = new String[objects.size()];
                    for (int i = 0; i < objects.size(); i++) {
                        EObject obj = (EObject) objects.get(i);
                        values[i] = EcoreUtils.getName(obj);
                    }
                    return values;
                }
            } else if (eobj.eClass().getName().equals(SafConstants.
                    COMPONENT_INSTANCELIST_ECLASS)) {
               EObject suObj = eobj.eContainer().eContainer();
               String name = (String) EcoreUtils.
                       getValue(suObj, "type");
               List objects = (List) NodeProfileDialog.getInstance().
                       getSuCompMap().get(name);
               if (objects != null) {
                    String[] values = new String[objects.size()];
                    for (int i = 0; i < objects.size(); i++) {
                        EObject obj = (EObject) objects.get(i);
                        values[i] = EcoreUtils.getName(obj);
                    }
                    return values;
               }

            } else if (eobj.eClass().getName().equals(SafConstants.SI_INSTANCELIST_ECLASS)) {
            	EObject serviceObj = eobj.eContainer().eContainer();
                String name = (String) EcoreUtils.
                        getValue(serviceObj, "type");
                List objects = (List) NodeProfileDialog.getInstance().
                    getSgSiMap().get(name);
                if (objects != null) {
                     String[] values = new String[objects.size()];
                     for (int i = 0; i < objects.size(); i++) {
                         EObject obj = (EObject) objects.get(i);
                         values[i] = EcoreUtils.getName(obj);
                     }
                     return values;
                }
             } else if (eobj.eClass().getName()
                     .equals(SafConstants.CSI_INSTANCELIST_ECLASS)) {
                 EObject siObj = eobj.eContainer().eContainer();
                 String name = (String) EcoreUtils.
                         getValue(siObj, "type");
                 List objects = (List) NodeProfileDialog.getInstance().
                     getSiCsiMap().get(name);
                 if (objects != null) {
                      String[] values = new String[objects.size()];
                      for (int i = 0; i < objects.size(); i++) {
                          EObject obj = (EObject) objects.get(i);
                          values[i] = EcoreUtils.getName(obj);
                      }
                      return values;
                 }
             }
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

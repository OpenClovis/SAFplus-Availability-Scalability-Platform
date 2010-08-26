/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/editor/CComboBoxCellEditor.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.editor;

import java.util.List;

import org.eclipse.emf.ecore.EEnum;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.dialogs.DialogPage;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.viewers.ComboBoxCellEditor;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.widgets.Composite;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.ui.factory.CellEditorFactory;
/**
 * @author nadeem
 * ComboBoxCellEditor for Form/Table and PropertyViews. This
 * class overrides ComboBoxCellEditor to handle deactivation
 * in form view. It also provided direct support for EEnum.
 */
public class CComboBoxCellEditor extends ComboBoxCellEditor
{
    private final int _viewType;
    protected Environment _env;
    protected EStructuralFeature _feature;
    /**
     * Convert value from integer to String.
     * @return String Value.
     */
    protected Object doGetValue()
    {
        int index = ((Integer) super.doGetValue()).intValue();
        return index != -1 ? getItems()[index] : null;
    }
    /**
     * Convert value from String to Integer before
     * calling super implementation.
     * @param value Value to be set
     */
    protected void doSetValue(Object value)
    {
        int index  = -1;
        String val = (value instanceof EEnumLiteral)
            ? ((EEnumLiteral) value).getName() : value.toString();
        String[] items = getItems();
        for (int i = 0; i < items.length; i++) {
            if (val.equals(items[i])) {
                index = i;
                break;
            }
        }
        super.doSetValue(new Integer(index));
    }
    /**
     * Dont Deactivate editor in FormView.
     */
    public void deactivate()
    {
        if (_viewType != CellEditorFactory.FORM_VIEW) {
            super.deactivate();
        } else {
            fireCancelEditor();
        }
    }
    /**
     * When Cell Editor is activated, the Tooltip message
     * is displayed on the Message Area of the Containing 
     * dialog 
     */
    public void activate()
    {
        /*String tooltip =
            EcoreUtils.getAnnotationVal(_feature, null, "tooltip");
        Object container = _env.getValue("container");
        if (tooltip != null && container!= null) {
            if (container instanceof PreferenceDialog) {
            ((PreferenceDialog) container).setMessage(tooltip,
                    IMessageProvider.INFORMATION);
            } else if (container instanceof TitleAreaDialog) {
            ((TitleAreaDialog) container).setMessage(tooltip,
                    IMessageProvider.INFORMATION);
            } else if (container instanceof DialogPage) {
                ((DialogPage) container).setMessage(tooltip,
                        IMessageProvider.INFORMATION);
            }
        }*/
        super.activate();
    }
    /**
     * Get String array for EnumLiterals.
     * @param eEnum EEnum
     * @return string array
     */
    private static String[] getLiterals(EEnum eEnum)
    {
        List list = eEnum.getELiterals();
        String[] vals = new String[list.size()];
        for (int i = 0; i < list.size(); i++) {
            vals[i] = list.get(i).toString();
        }
        return vals;
    }
    /**
     * Constructor for EEnum.
     * @param p      Parent Composite.
     * @param eEnum  EEnum
     * @param style  SWT Style.
     * @param view   ViewType (Form/table/property)
     */
    public CComboBoxCellEditor(Composite p, EEnum eEnum, int style, int view,
            EStructuralFeature feature, Environment env)
    {
        this(p, getLiterals(eEnum), style, view, feature, env);
    }
    /**
     * Constructor.
     * @param p      Parent Composite.
     * @param vals   Possible Values.
     * @param style  SWT Style.
     * @param view   ViewType (Form/table/property)
     */
    public CComboBoxCellEditor(Composite p, String[] vals, int style, int view,
            EStructuralFeature feature, Environment env)
    {
        super(p, vals, style);
        _viewType = view;
        _feature = feature;
        _env = env;
        
        //This code is not required always. Sometimes combox values 
        //are not set after selection. This code forcefully set the selected value.
        //This is temporary fix.
        CCombo comboBox = (CCombo) getControl();
        comboBox.addSelectionListener(new SelectionAdapter() {
            public void widgetDefaultSelected(SelectionEvent event) {
            }

            public void widgetSelected(SelectionEvent event) {
            	focusLost();
            }
        });
    }
}

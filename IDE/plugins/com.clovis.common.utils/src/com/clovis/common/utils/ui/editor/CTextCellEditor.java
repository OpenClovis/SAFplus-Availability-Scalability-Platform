/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/editor/CTextCellEditor.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.editor;

import org.eclipse.swt.widgets.Composite;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.dialogs.DialogPage;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.viewers.TextCellEditor;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.ui.factory.CellEditorFactory;
/**
 * @author shubhada
 * TextCellEditor class is extended to provide support for
 * editing Integer values. Deactivate method is modified to
 * handle FormView in which it is not required to make control
 * invisible after editing.
 */
public class CTextCellEditor extends TextCellEditor
{
    protected int _viewType;
    protected Environment _env;
    protected EStructuralFeature _feature;
    /**
     * Overrides the method to check whether value is Integer
     * @param value Object
     */
    protected void doSetValue(Object value)
    {
        super.doSetValue(value.toString());
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
     * Constructor
     * @param parent   Composite.
     * @param style    SWT Style.
     * @param viewType View Type form,table, property.
     * @param feature EStructuralFeature
     * @param env Environment
     */
    public CTextCellEditor(Composite parent, int style, int viewType,
            EStructuralFeature feature, Environment env)
    {
        super(parent, style);
        _viewType = viewType;
        _feature = feature;
        _env = env;
    }
}

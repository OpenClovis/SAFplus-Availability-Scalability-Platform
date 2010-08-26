/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/factory/PushButtonCellEditor.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.factory;

import org.eclipse.emf.ecore.EReference;

import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Table;

import org.eclipse.jface.viewers.DialogCellEditor;

import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.ui.dialog.PushButtonDialog;
/**
 * @author shubhada
 *
 * CellEditor with a push button which will open a dialog
 * when clicked on button.
 */
public class PushButtonCellEditor extends DialogCellEditor
{
    protected EReference  _ref       = null;
    protected Environment _parentEnv = null;
    /**
     * @param parent Composite
     * @param ref    EReference
     * @param parentEnv ParentEnvironment
     */
    public PushButtonCellEditor(Composite parent, EReference ref,
            Environment parentEnv)
    {
        super(parent);
        _ref       = ref;
        _parentEnv = parentEnv;
    }
    /**
     * When Cell Editor is activated, the Tooltip message
     * is displayed on the Message Area of the Containing 
     * dialog 
     */
    public void activate()
    {
        /*String tooltip =
            EcoreUtils.getAnnotationVal(_ref, null, "tooltip");
        Object container = _parentEnv.getValue("container");
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
     * Creates the button for this cell editor under the given parent control.
     * <p>
     * The default implementation of this framework method creates the button 
     * display on the right hand side of the dialog cell editor. Subclasses
     * may extend or reimplement.
     * </p>
     *
     * @param parent the parent control
     * @return the new button control
     */
    protected Button createButton(Composite parent) {
        Button result = super.createButton(parent);
        result.setText("Edit..."); //$NON-NLS-1$
        return result;
    }
    
    /**
     * @param value Object
     * Blank implementation of update contents to avoid blank label
     */
    protected void updateContents(Object value)
    {
    }
    /**
     * @param cellEditorWindow - Control
     * @return Object
     * Method where we have to specify which
     * dialog to be opened on clicking push button
     */
    protected Object openDialogBox(Control cellEditorWindow)
    {
        new PushButtonDialog(getControl().getShell(),
            _ref.getEReferenceType(), getValue(), _parentEnv).open();
        return null;
    }
    /** 
     * Override in order to enable the control while cell editor
     * is deactivating. otherwise edit button will disappear.
     * 
     * @see org.eclipse.jface.viewers.CellEditor#deactivate()
     */
    public void deactivate() {
    	super.deactivate();
    	if(getControl() != null && !(getControl().getParent() instanceof Table))
    		getControl().setVisible(true);
	}
}

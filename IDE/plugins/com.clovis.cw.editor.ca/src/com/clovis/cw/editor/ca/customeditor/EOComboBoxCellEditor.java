/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/customeditor/EOComboBoxCellEditor.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.customeditor;

import java.util.List;

import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ComboBoxCellEditor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Tree;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.ui.table.FormView;
import com.clovis.cw.editor.ca.dialog.RMDDialog;
/**
 * 
 * @author shubhada
 *
 * Combo Box cell editor to show the user defined EO's.
 */
public class EOComboBoxCellEditor extends ComboBoxCellEditor
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
    public EOComboBoxCellEditor(Composite parent,
            EStructuralFeature feature, Environment env)
    {
        super(parent, new String[0], SWT.BORDER | SWT.READ_ONLY);
        _env = env;
        _feature = feature;

        ((CCombo) getControl()).addModifyListener(new ModifyListener() {
        	public void modifyText(ModifyEvent e) {
        		updatePreferenceTree();
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
        return new EOComboBoxCellEditor(parent, feature, env);
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
     * Updates the preference Tree based on the selection
     * of the Combo Box.
     */
    private void updatePreferenceTree() {
		PreferencePage page = (PreferencePage) ((FormView) _env)
				.getValue("container");
		page.setTitle(((CCombo)getControl()).getText());
		Tree tree = null;
		Shell shell = page.getShell();
		if(shell.getData() instanceof RMDDialog) {
			tree = ((RMDDialog) shell.getData()).getTreeViewer().getTree();
		}
		tree.getSelection()[0].setText(((CCombo)getControl()).getText());
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
    * @return Combo Values
    */
   protected String[] getComboValues()
   {
	   List comboValues = null;

	   Shell shell = getControl().getShell();
	   if(shell.getData() instanceof RMDDialog) {
	        comboValues = RMDDialog.getInstance().getEOs();
	   }

	   String [] values = (String[]) comboValues.toArray(new String[] {});
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

/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/CpmPushButtonCellEditor.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.customeditor;


import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;

import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.ui.factory.PushButtonCellEditor;
import com.clovis.cw.editor.ca.dialog.LibsPushButtonDialog;

/**
 * @author Pushparaj
 *
 * PushButtonCellEditor for extenal libraries
 */
public class LibsPushButtonCellEditor extends PushButtonCellEditor
{
private EReference _ref = null;
private Environment _parentEnv = null;

    /**
     * @param parent - parent composite
     * @param ref -EReference
     * @param env - Parent Environment
     */
    public LibsPushButtonCellEditor(Composite parent, EReference ref,
            Environment env)
    {
        super(parent, ref, env);
        _ref = ref;
        _parentEnv = env;
    }
    /**
     * @param cellEditorWindow - Control
     * @return null
     */
    protected Object openDialogBox(Control cellEditorWindow) {
		new LibsPushButtonDialog(getControl().getShell(), _ref
				.getEReferenceType(), getValue(), _parentEnv).open();
		return null;
	}
    /**
	 * 
	 * @param parent
	 *            Composite
	 * @param feature
	 *            EStructuralFeature
	 * @param env
	 *            Environment
	 * @return cell Editor
	 */
    public static CellEditor createEditor(Composite parent,
            EStructuralFeature feature, Environment env)
    {
        return new LibsPushButtonCellEditor(parent, (EReference) feature, env);
    }
}
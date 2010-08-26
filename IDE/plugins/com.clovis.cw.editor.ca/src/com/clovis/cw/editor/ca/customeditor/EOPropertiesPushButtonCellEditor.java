/**
 * 
 */
package com.clovis.cw.editor.ca.customeditor;

import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;

import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.ui.factory.PushButtonCellEditor;
import com.clovis.cw.editor.ca.dialog.EOPropertiesDialog;

/**
 * Cell editor for the EO Properties field in SAF Component.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class EOPropertiesPushButtonCellEditor extends PushButtonCellEditor {

	/**
	 * Constructor.
	 * 
	 * @param parent
	 * @param ref
	 * @param parentEnv
	 */
	public EOPropertiesPushButtonCellEditor(Composite parent,
			EStructuralFeature ref, Environment parentEnv) {
		super(parent, (EReference) ref, parentEnv);
	}

	/**
	 * @param parent
	 * @param feature
	 * @param env
	 * @return
	 */
	public static CellEditor createEditor(Composite parent,
			EStructuralFeature feature, Environment env) {
		return new EOPropertiesPushButtonCellEditor(parent, feature, env);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.common.utils.ui.factory.PushButtonCellEditor#openDialogBox(org.eclipse.swt.widgets.Control)
	 */
	@Override
	protected Object openDialogBox(Control cellEditorWindow) {
		new EOPropertiesDialog(getControl().getShell(), _ref
				.getEReferenceType(), getValue(), _parentEnv).open();
		return null;
	}
}

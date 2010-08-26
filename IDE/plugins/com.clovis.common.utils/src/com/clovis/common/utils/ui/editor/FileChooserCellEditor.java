package com.clovis.common.utils.ui.editor;

import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.DialogCellEditor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Table;

import com.clovis.common.utils.menu.Environment;

/**
 * Cell Editor for selecting the File Path.
 * @author Suraj Rajyaguru
 *
 */
public class FileChooserCellEditor extends DialogCellEditor {

	/**
	 * Constructor
	 * @param parent
	 */
	public FileChooserCellEditor(Composite parent) {
		super(parent);
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
        return new FileChooserCellEditor(parent);
    }

    /* (non-Javadoc)
     * @see org.eclipse.jface.viewers.DialogCellEditor#createButton(org.eclipse.swt.widgets.Composite)
     */
    protected Button createButton(Composite parent) {
        Button result = new Button(parent, SWT.DOWN);
        result.setText("Browse..."); //$NON-NLS-1$
        return result;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.DialogCellEditor#openDialogBox(org.eclipse.swt.widgets.Control)
	 */
	protected Object openDialogBox(Control cellEditorWindow) {
		FileDialog fileDialog = new FileDialog(getControl().getShell());
		fileDialog.setFilterExtensions(new String[] {"*.h"});
		return fileDialog.open();
	}
	/** 
     * Override in order to enable the control while cell editor
     * is deactivating. otherwise browse button will disappear.
     * 
     * @see org.eclipse.jface.viewers.CellEditor#deactivate()
     */
    public void deactivate() {
    	super.deactivate();
    	if(getControl() != null && !(getControl().getParent() instanceof Table))
    		getControl().setVisible(true);
	}
}

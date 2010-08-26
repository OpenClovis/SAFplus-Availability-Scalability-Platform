package com.clovis.cw.editor.ca;

import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.TextCellEditor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.eclipse.swt.widgets.Tree;

import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.ui.table.FormView;
import com.clovis.cw.editor.ca.dialog.MemoryConfigurationDialog;
import com.clovis.cw.editor.ca.dialog.NodeProfileDialog;
import com.clovis.cw.editor.ca.dialog.RMDDialog;

/**
 * Text Cell Editor used for the preference dialog to reflect the
 * change in the tree of the dialog.
 * @author Suraj Rajyaguru
 */
public class PreferenceTextCellEditor extends TextCellEditor {

	private Environment _env;

	/**
	 * Constructor
	 * @param parent
	 * @param feature
	 * @param env
	 */
	public PreferenceTextCellEditor(Composite parent,
			EStructuralFeature feature,  Environment env) {
		super(parent, SWT.BORDER);
		_env = env;
        ((Text) getControl()).addModifyListener(new ModifyListener() {
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
        return new PreferenceTextCellEditor(parent, feature, env);
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

	/**
     * Updates the preference Tree based on the Text change
     * of the Editor.
     */
    private void updatePreferenceTree() {
		PreferencePage page = (PreferencePage) ((FormView) _env)
				.getValue("container");
		page.setTitle(((Text) getControl()).getText());
		Shell shell = page.getShell();
		Tree tree = null;
		if (shell.getData() instanceof NodeProfileDialog) {
			tree = ((NodeProfileDialog) shell.getData()).getTreeViewer()
					.getTree();
		} else if (shell.getData() instanceof RMDDialog) {
			tree = ((RMDDialog) shell.getData()).getTreeViewer().getTree();
		} else if (shell.getData() instanceof MemoryConfigurationDialog) {
			tree = ((MemoryConfigurationDialog) shell.getData()).getTreeViewer().getTree();
		}
		if(tree.getSelection().length > 0)
			tree.getSelection()[0].setText(((Text) getControl()).getText());
	}
}

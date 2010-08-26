package com.clovis.cw.editor.ca.dialog.handler;

import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Tree;

import com.clovis.cw.editor.ca.action.TreeAction;
import com.clovis.cw.editor.ca.dialog.GenericFormPage;
import com.clovis.cw.editor.ca.dialog.GenericPreferenceDialog;
import com.clovis.cw.editor.ca.dialog.PreferenceUtils;
import com.clovis.cw.editor.ca.dialog.PreferenceUtils.PreferenceSelectionData;

/**
 * Handler class for New Menu Action. This handler is used in Preference Dialog's
 * pop-up menus
 * @author Pushparaj
 *
 */
public class NewMenuHandler extends AbstractMenuHandler {
	
	/**
	 * @see com.clovis.cw.editor.ca.dialog.handler.AbstractMenuHandler#execute(com.clovis.cw.editor.ca.action.TreeAction)
	 */
	protected void execute(TreeAction action) {
		action.newSelected(null);
	}
	/**
	 * @see org.eclipse.core.commands.IHandler#isEnabled()
	 */
	public boolean isEnabled() {
		if (Display.getDefault().getFocusControl() instanceof Tree) {
			Tree tree = (Tree) Display.getDefault().getFocusControl();
			GenericPreferenceDialog dialog = (GenericPreferenceDialog) tree
					.getData("dialog");
			TreeViewer viewer = dialog.getTreeViewer();
			PreferenceSelectionData psd = PreferenceUtils
					.getCurrentSelectionData(viewer);
			PreferencePage page = psd.getPrefPage();
			if (!(page instanceof GenericFormPage)) {
				String selectedItemText = psd.getTreeItem().getText();
				if (!selectedItemText.endsWith("List")
						|| ((PreferencePage) ((PreferenceDialog) dialog)
								.getSelectedPage()).isValid() == false) {
					return false;
				}
			} else {
				if (((PreferencePage) ((PreferenceDialog) dialog)
						.getSelectedPage()).isValid() == false) {
					return false;
				}
			}
		}
		return true;
	}
}

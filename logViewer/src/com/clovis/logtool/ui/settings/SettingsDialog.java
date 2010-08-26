/**
 * 
 */
package com.clovis.logtool.ui.settings;

import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.swt.widgets.Shell;

/**
 * @author srajyaguru
 *
 */
public class SettingsDialog extends PreferenceDialog {

	public SettingsDialog(Shell shell, PreferenceManager prefManager) {
		super(shell, prefManager);
		createContent();
	}

	private void createContent() {
		PreferenceManager manager = getPreferenceManager();
		manager.addToRoot(new PreferenceNode("Location",
				new ConfigureLocationPage()));
	}

	@Override
	protected void configureShell(Shell shell) {
		super.configureShell(shell);
		shell.setText("Settings");
	}

}

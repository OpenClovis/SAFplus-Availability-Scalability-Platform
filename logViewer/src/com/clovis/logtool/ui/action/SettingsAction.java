/**
 * 
 */
package com.clovis.logtool.ui.action;

import java.io.IOException;
import java.net.URL;
import java.util.Properties;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceStore;
import org.eclipse.jface.resource.ImageDescriptor;

import com.clovis.logtool.ui.LogDisplay;
import com.clovis.logtool.ui.settings.SettingsDialog;
import com.clovis.logtool.utils.ConfigConstants;
import com.clovis.logtool.utils.LogConstants;
import com.clovis.logtool.utils.LogUtils;

/**
 * Settings Action for the log tool application.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class SettingsAction extends Action {

	/**
	 * Constructs Setting Action instance.
	 */
	public SettingsAction() {
		setText("Se&ttings@Ctrl+T");
		setToolTipText("Settings");

		try {
			setImageDescriptor(ImageDescriptor.createFromURL(new URL(
					"file:icons/logSettings.png")));
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.action.Action#run()
	 */
	public void run() {
		SettingsDialog dialog = new SettingsDialog(LogDisplay.getInstance()
				.getShell(), new PreferenceManager());

		PreferenceStore prefStore = new PreferenceStore("config/settings");
		try {
			prefStore.load();
		} catch (IOException e) {
			e.printStackTrace();
		}
		dialog.setPreferenceStore(prefStore);

		dialog.open();

		try {
			prefStore.save();
		} catch (IOException e) {
			LogUtils
					.displayMessage(LogConstants.DISPLAY_MESSAGE_ERROR,
							"Unable to Store Settings",
							"Check wether the settings file exist and you have the write permission.");
		}
		loadDataStructures(prefStore);
	}

	/**
	 * It loads the current data structures maintained by the application if
	 * required.
	 * 
	 * @param prefStore
	 *            the Preference Store
	 */
	private void loadDataStructures(PreferenceStore prefStore) {
		Properties currentSettings = new Properties();
		currentSettings.putAll(LogUtils.getSettings(false));

		LogUtils.getSettings(true);

		String msgIdFile = ConfigConstants.SETTINGS_FILE_MESSAGEID;
		if (!prefStore.getString(msgIdFile).equals(
				currentSettings.getProperty(msgIdFile))) {
			LogUtils.getMessageIdMapping(true);
		}
	}
}

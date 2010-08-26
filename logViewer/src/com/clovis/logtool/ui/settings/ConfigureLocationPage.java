/**
 * 
 */
package com.clovis.logtool.ui.settings;

import org.eclipse.jface.preference.DirectoryFieldEditor;
import org.eclipse.jface.preference.FieldEditorPreferencePage;
import org.eclipse.jface.preference.FileFieldEditor;
import org.eclipse.jface.preference.IPreferenceStore;

import com.clovis.logtool.utils.ConfigConstants;

/**
 * @author srajyaguru
 *
 */
public class ConfigureLocationPage extends FieldEditorPreferencePage {

	public ConfigureLocationPage() {
		super("Configure Location", GRID);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.FieldEditorPreferencePage#createFieldEditors()
	 */
	@Override
	protected void createFieldEditors() {
		IPreferenceStore ps = getPreferenceStore();

		if (ps.getString(ConfigConstants.SETTINGS_DIR_USER).equals("")) {
			ps.setValue(ConfigConstants.SETTINGS_DIR_USER,
					ConfigConstants.DIR_DEFAULT_USER);
		}

		ps.setDefault(ConfigConstants.SETTINGS_DIR_USER,
				ConfigConstants.DIR_DEFAULT_USER);

		DirectoryFieldEditor userDirFE = new DirectoryFieldEditor(
				ConfigConstants.SETTINGS_DIR_USER, "User Directory:",
				getFieldEditorParent());
		userDirFE.setEmptyStringAllowed(false);
		addField(userDirFE);

		FileFieldEditor msgIdFE = new FileFieldEditor(
				ConfigConstants.SETTINGS_FILE_MESSAGEID,
				"MessageId Mapping File:", getFieldEditorParent());
		addField(msgIdFE);
	}
}

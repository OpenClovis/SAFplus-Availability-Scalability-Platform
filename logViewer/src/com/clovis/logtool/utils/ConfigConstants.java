/**
 * 
 */
package com.clovis.logtool.utils;

/**
 * Defines various configuration constants for log tool application.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public interface ConfigConstants {

	String PATH_SEPARATOR = System.getProperty("file.separator");

	// Configurable Location Constants
	public static final String SETTINGS_DIR_USER = "ConfigureLocation.userDirectory";

	public static final String SETTINGS_FILE_MESSAGEID = "ConfigureLocation.messageIdMapping";

	// Location Constants
	public static final String DIR_CONFIG = "config";

	public static final String DIR_CLOVIS = ".clovis";

	public static final String DIR_LOGVIEWER = "logviewer";

	public static final String DIR_DEFAULT_USER = System
			.getProperty("user.home")
			+ PATH_SEPARATOR + DIR_CLOVIS + PATH_SEPARATOR + DIR_LOGVIEWER;

	public static final String FILE_CONSTANTS = DIR_CONFIG + PATH_SEPARATOR
			+ "constants";

	public static final String FILE_SETTINGS = DIR_CONFIG + PATH_SEPARATOR
			+ "settings";

	public static final String FILE_RECORDFIELDS = DIR_CONFIG + PATH_SEPARATOR
			+ "recordFields";

	public static final String FILE_UICOLUMNS = DIR_CONFIG + PATH_SEPARATOR
			+ "UIColumns";

	public static final String FILE_NAME_FILTER = "filter";
}

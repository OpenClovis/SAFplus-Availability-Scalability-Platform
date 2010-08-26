/**
 * 
 */
package com.clovis.cw.workspace.migration;

import java.io.File;

import com.clovis.cw.data.ICWProject;

/**
 * Defines the constants for the Migration Framework.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public interface MigrationConstants {

	// Migration Types

	final static int TYPE_HANDLER = 0;

	final static int TYPE_ADD_ATTR = 10;

	final static int TYPE_ADD_ELEMENT_CHILD = 11;

	final static int TYPE_ADD_ELEMENT_SIBLING = 12;

	final static int TYPE_ADD_ELEMENT_ROOT = 13;

	final static int TYPE_ADD_TEXT = 14;

	final static int TYPE_RENAME_ATTR = 20;

	final static int TYPE_RENAME_ELEMENT = 21;

	final static int TYPE_CHANGEVAL_ATTR = 30;

	final static int TYPE_CHANGEVAL_ATTR_MATCH = 31;

	final static int TYPE_CHANGEVAL_ELEMENT = 32;

	final static int TYPE_CHANGEVAL_ELEMENT_MATCH = 33;

	final static int TYPE_REMOVE_ATTR = 40;

	final static int TYPE_REMOVE_ELEMENT = 41;

	final static int TYPE_MOVE_ATTR = 50;

	final static int TYPE_MOVE_ELEMENT_CHILD = 51;

	final static int TYPE_MOVE_ELEMENT_SIBLING = 52;

	final static int TYPE_MOVE_ELEMENT_PATH = 53;

	final static int TYPE_MOVE_ATTRTOELEMENT_PATH = 54;

	final static int TYPE_REMOVE_FILE = 80;

	final static int TYPE_REPLACE_STR_FILE = 90;

	final static int TYPE_REPLACE_STR_ALLFILES = 91;

	final static int CHECKSTRING_CONTAIN = 0;

	final static int CHECKSTRING_DOESNOT_CONTAIN = 1;

	final static int CHECKELEMENT_CONTAIN = 10;

	final static int CHECKELEMENT_DOESNOT_CONTAIN = 11;


	//Problem Severity
	final static String PROBLEM_SEVERITY_ERROR = "ERROR";

	final static String PROBLEM_SEVERITY_WARNING = "WARNING";


	//Back-up Folders
	final static String BACKUP_FOLDER = ICWProject.PLUGIN_MIGRATION_FOLDER + File.separator + "backup";

	final static String BACKUP_FOLDER_2302_30 = BACKUP_FOLDER + File.separator + "2302_30";

	final static String BACKUP_FOLDER_31 = BACKUP_FOLDER + File.separator + "31";
}

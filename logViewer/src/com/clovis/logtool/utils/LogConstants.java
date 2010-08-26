/**
 * 
 */
package com.clovis.logtool.utils;

/**
 * Defines various constants for log tool application.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public interface LogConstants {

	// Field Index constants.
	public static final int FIELD_INDEX_RECORDNUMBER = 0;

	public static final int FIELD_INDEX_FLAG = 1;

	public static final int FIELD_INDEX_SEVERITY = 2;

	public static final int FIELD_INDEX_STREAMID = 3;

	public static final int FIELD_INDEX_COMPONENTID = 4;

	public static final int FIELD_INDEX_SERVICEID = 5;

	public static final int FIELD_INDEX_MESSAGEID = 6;

	public static final int FIELD_INDEX_TIMESTAMP = 7;

	// Column Index Constants.
	public static final int COLUMN_INDEX_RECORDNUMBER = 0;

	public static final int COLUMN_INDEX_SEVERITY = 1;

	public static final int COLUMN_INDEX_STREAMID = 2;

	public static final int COLUMN_INDEX_COMPONENTID = 3;

	public static final int COLUMN_INDEX_SERVICEID = 4;

	public static final int COLUMN_INDEX_TIMESTAMP = 5;

	public static final int COLUMN_INDEX_MESSAGEID = 6;

	public static final int COLUMN_INDEX_MESSAGE = 7;

	// Type Constants
	public static final int TYPE_BYTE = 0;

	public static final int TYPE_SHORT = 1;

	public static final int TYPE_INT = 2;

	public static final int TYPE_LONG = 3;

	public static final int TYPE_STRING = 4;

	public static final int TYPE_DATE = 5;

	// MessageID Constants
	public static final int MESSAGEID_BINARY = 0;

	public static final int MESSAGEID_ASCII = 1;

	// Viewer Constants
	public static final int VIEWER_DISPLAYBATCH_SIZE = 50;

	// Display Message Error Constants
	public static final int DISPLAY_MESSAGE_CONFIRM = 0;

	public static final int DISPLAY_MESSAGE_ERROR = 1;

	public static final int DISPLAY_MESSAGE_INFORMATION = 2;

	public static final int DISPLAY_MESSAGE_QUESTION = 3;

	public static final int DISPLAY_MESSAGE_WARNING = 4;

	// File Header Constants
	public static final int FILE_HEADER_SIZE = 31;

	public static final String CFG_FILE_HEADER_STRING = "OpenClovis_Log_Configfile";

	public static final String LOG_FILE_HEADER_STRING = "OpenClovis_Logfile";
}

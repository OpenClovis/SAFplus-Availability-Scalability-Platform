/**
 * 
 */
package com.clovis.logtool.utils;

import java.io.File;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.List;
import java.util.Properties;

import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.widgets.Shell;

import com.clovis.logtool.record.filter.RecordFilter;
import com.clovis.logtool.ui.LogDisplay;

/**
 * Contains the utility methods for various operations for Log Tool.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class LogUtils {

	/**
	 * Stores the information for message id to message format mapping.
	 */
	private static Properties _messageIdMapping;

	/**
	 * Stores the Constants.
	 */
	private static Properties _constants;

	/**
	 * Stores the settings.
	 */
	private static Properties _settings;

	/**
	 * Stores the details of the record fields.
	 */
	private static List<RecordField> _recordFields;

	/**
	 * Stores the details of the UI columns.
	 */
	private static List<UIColumn> _UIColumns;

	/**
	 * Severity Constants.
	 */
	public static String[] severityString = new String[] { "0:Off",
			"1:Emergency", "2:Alert", "3:Critical", "4:Error", "5:Warning",
			"6:Notice", "7:Informational", "8:Debug", "9:End" }; 

	/**
	 * Private Constructor.
	 */
	private LogUtils() {
	}

	/**
	 * Parses the unsigned byte value from the given byte sequence.
	 * 
	 * @param bytes
	 *            the byte sequence to be parsed
	 * @return unsigned byte value
	 */
	public static short getUnSignedByteFromBytes(byte[] bytes, int index) {
		return (short) (bytes[index] & 0xff);
	}

	/**
	 * Parses the short value from the given byte sequence.
	 * 
	 * @param bytes
	 *            the byte sequence to be parsed
	 * @return short value parsed from the byte sequence
	 */
	public static short getShortFromBytesLE(byte[] bytes, int index) {
		return (short) ((bytes[index + 0] & 0xff)
				| ((bytes[index + 1] & 0xff) << 8));
	}

	/**
	 * Parses the short value from the given byte sequence.
	 * 
	 * @param bytes
	 *            the byte sequence to be parsed
	 * @return short value parsed from the byte sequence
	 */
	public static short getShortFromBytesBE(byte[] bytes, int index) {
		return (short) ((bytes[index + 1] & 0xff)
				| ((bytes[index + 0] & 0xff) << 8));
	}

	/**
	 * Parses the unsigned short value from the given byte sequence.
	 * 
	 * @param bytes
	 *            the byte sequence to be parsed
	 * @return unsigned short value
	 */
	public static int getUnSignedShortFromBytesLE(byte[] bytes, int index) {
		return ((bytes[index + 0] & 0xff)
				| ((bytes[index + 1] & 0xff) << 8));
	}

	/**
	 * Parses the unsigned short value from the given byte sequence.
	 * 
	 * @param bytes
	 *            the byte sequence to be parsed
	 * @return unsigned short value
	 */
	public static int getUnSignedShortFromBytesBE(byte[] bytes, int index) {
		return ((bytes[index + 1] & 0xff)
				| ((bytes[index + 0] & 0xff) << 8));
	}

	/**
	 * Parses the int value from the given byte sequence.
	 * 
	 * @param bytes
	 *            the byte sequence to be parsed
	 * @return int value parsed from the byte sequence
	 */
	public static int getIntFromBytesLE(byte[] bytes, int index) {
		return ((bytes[index + 0] & 0xff)
				| ((bytes[index + 1] & 0xff) << 8)
				| ((bytes[index + 2] & 0xff) << 16)
				| ((bytes[index + 3] & 0xff) << 24));
	}

	/**
	 * Parses the int value from the given byte sequence.
	 * 
	 * @param bytes
	 *            the byte sequence to be parsed
	 * @return int value parsed from the byte sequence
	 */
	public static int getIntFromBytesBE(byte[] bytes, int index) {
		return ((bytes[index + 3] & 0xff)
				| ((bytes[index + 2] & 0xff) << 8)
				| ((bytes[index + 1] & 0xff) << 16)
				| ((bytes[index + 0] & 0xff) << 24));
	}

	/**
	 * Parses the unsigned int value from the given byte sequence.
	 * 
	 * @param bytes
	 *            the byte sequence to be parsed
	 * @return unsigned int value
	 * @param bytes
	 * @return
	 */
	public static long getUnSignedIntFromBytesLE(byte[] bytes, int index) {
		return (((long) (bytes[index + 0] & 0xff))
				| ((long) (bytes[index + 1] & 0xff) << 8)
				| ((long) (bytes[index + 2] & 0xff) << 16)
				| ((long) (bytes[index + 3] & 0xff) << 24));
	}

	/**
	 * Parses the unsigned int value from the given byte sequence.
	 * 
	 * @param bytes
	 *            the byte sequence to be parsed
	 * @return unsigned int value
	 * @param bytes
	 * @return
	 */
	public static long getUnSignedIntFromBytesBE(byte[] bytes, int index) {
		return (((long) (bytes[index + 3] & 0xff))
				| ((long) (bytes[index + 2] & 0xff) << 8)
				| ((long) (bytes[index + 1] & 0xff) << 16)
				| ((long) (bytes[index + 0] & 0xff) << 24));
	}

	/**
	 * Parses the long value from the given byte sequence.
	 * 
	 * @param bytes
	 *            the byte sequence to be parsed
	 * @return long value parsed from the byte sequence
	 */
	public static long getLongFromBytesLE(byte[] bytes, int index) {
		return (((long) (bytes[index + 0] & 0xff))
				| ((long) (bytes[index + 1] & 0xff) << 8)
				| ((long) (bytes[index + 2] & 0xff) << 16)
				| ((long) (bytes[index + 3] & 0xff) << 24)
				| ((long) (bytes[index + 4] & 0xff) << 32)
				| ((long) (bytes[index + 5] & 0xff) << 40)
				| ((long) (bytes[index + 6] & 0xff) << 48)
				| ((long) (bytes[index + 7] & 0xff) << 56));
	}

	/**
	 * Parses the long value from the given byte sequence.
	 * 
	 * @param bytes
	 *            the byte sequence to be parsed
	 * @return long value parsed from the byte sequence
	 */
	public static long getLongFromBytesBE(byte[] bytes, int index) {
		return (((long) (bytes[index + 7] & 0xff))
				| ((long) (bytes[index + 6] & 0xff) << 8)
				| ((long) (bytes[index + 5] & 0xff) << 16)
				| ((long) (bytes[index + 4] & 0xff) << 24)
				| ((long) (bytes[index + 3] & 0xff) << 32)
				| ((long) (bytes[index + 2] & 0xff) << 40)
				| ((long) (bytes[index + 1] & 0xff) << 48)
				| ((long) (bytes[index + 0] & 0xff) << 56));
	}

	/**
	 * Parses the String value from the given byte sequence.
	 * 
	 * @param bytes
	 *            the byte sequence
	 * @param index
	 *            the start index
	 * @return the String
	 */
	public static String getStringFromBytes(byte[] bytes, int index) {
		for(int i=index ; i<bytes.length ; i++) {
			if(bytes[i] == '\0') {
				return new String(bytes, index, i - index);
			}
		}
		return null;
	}

	/**
	 * Returns the required byte subset from the given byte sequence as per the
	 * range specified with startIndex and endIndex.
	 * 
	 * @param source
	 *            the byte sequence for getting the subset
	 * @param startIndex
	 *            the start position in the source
	 * @param endIndex
	 *            the end position in the source
	 * @return the byte sequence as per the required range
	 */
	public static byte[] getBytesSubset(byte[] source, int startIndex,
			int endIndex) {
		byte[] bytesSubset = new byte[endIndex - startIndex + 1];
		System.arraycopy(source, startIndex, bytesSubset, 0, endIndex
				- startIndex + 1);
		return bytesSubset;
	}

	/**
	 * Returns the message id mapping.
	 * 
	 * @return the message id mapping
	 */
	public static Properties getMessageIdMapping(boolean forceRead) {
		if (forceRead || _messageIdMapping == null) {
			_messageIdMapping = ConfigReader.readProperties(getSettings(false)
					.getProperty(ConfigConstants.SETTINGS_FILE_MESSAGEID), false);

			if(_messageIdMapping == null) {
				LogUtils.displayMessage(LogConstants.DISPLAY_MESSAGE_WARNING,
						"MessageId Mapping not provided",
						"Messages will be parsed in a default way.");

				_messageIdMapping = new Properties();
			}
		}
		return _messageIdMapping;
	}

	/**
	 * Returns the Constants.
	 * 
	 * @return the constants
	 */
	public static Properties getConstants() {
		if (_constants == null) {
			_constants = ConfigReader.readProperties(ConfigConstants.FILE_CONSTANTS, false);

			if(_constants == null) {
				LogUtils.displayMessage(LogConstants.DISPLAY_MESSAGE_ERROR,
						"Configuration file missing.",
						"File \"" + ConfigConstants.FILE_CONSTANTS + "\" is missing."
						+ "\nApplication will exit now.");

				System.exit(-1);
			}
		}
		return _constants;
	}

	/**
	 * Returns the Settings.
	 * 
	 * @return the Settings
	 */
	public static Properties getSettings(boolean forceRead) {
		if (forceRead || _settings == null) {
			_settings = ConfigReader.readProperties(ConfigConstants.FILE_SETTINGS, false);

			if(_settings == null) {
				LogUtils.displayMessage(LogConstants.DISPLAY_MESSAGE_ERROR,
						"Configuration file missing.",
						"File \"" + ConfigConstants.FILE_SETTINGS + "\" is missing."
						+ "\nApplication will exit now.");

				System.exit(-1);
			} else {
				if(_settings.getProperty(ConfigConstants.SETTINGS_DIR_USER).equals("")) {
					_settings.setProperty(ConfigConstants.SETTINGS_DIR_USER,
							ConfigConstants.DIR_DEFAULT_USER);
				}
			}
		}
		return _settings;
	}

	/**
	 * Returns the Filter Map.
	 * 
	 * @return the Filter Map
	 */
	@SuppressWarnings("unchecked")
	public static HashMap<String, RecordFilter> getFilterMap() {

		Object object = ConfigReader.readXML(FilterMapXMLClass.class,
				getSettings(false).getProperty(
						ConfigConstants.SETTINGS_DIR_USER)
						+ ConfigConstants.PATH_SEPARATOR
						+ ConfigConstants.FILE_NAME_FILTER, false);

		HashMap<String, RecordFilter> filterMap = object != null ? ((FilterMapXMLClass) object)
				.getFilterMap()
				: null;

		if (filterMap == null) {
			LogUtils.displayMessage(LogConstants.DISPLAY_MESSAGE_INFORMATION,
					"No previous Filters",
					"New filter file will be created in the user directory.");
			filterMap = new HashMap<String, RecordFilter>();
		}
		return filterMap;
	}

	/**
	 * Saves the filter Map to the file.
	 * 
	 * @param filterMap
	 *            the filter map
	 */
	public static void saveFilterMap(HashMap<String, RecordFilter> filterMap) {
		FilterMapXMLClass fmxc = new FilterMapXMLClass();
		fmxc.setFilterMap(filterMap);

		String userDir = getSettings(false).getProperty(
				ConfigConstants.SETTINGS_DIR_USER);
		if(!new File(userDir).exists())
			new File(userDir).mkdirs();

		ConfigWriter.writeXML(fmxc, userDir
				+ ConfigConstants.PATH_SEPARATOR
				+ ConfigConstants.FILE_NAME_FILTER);
	}

	/**
	 * Returns the Fields of records.
	 * 
	 * @return record fields
	 */
	@SuppressWarnings("unchecked")
	public static List<RecordField> getRecordFields() {
		if (_recordFields == null) {
			Properties properties = ConfigReader.readProperties(
					ConfigConstants.FILE_RECORDFIELDS, false);

			if(properties == null) {
				LogUtils.displayMessage(LogConstants.DISPLAY_MESSAGE_ERROR,
						"Configuration file missing.",
						"File \"" + ConfigConstants.FILE_RECORDFIELDS + "\" is missing."
						+ "\nApplication will exit now.");

				System.exit(-1);
			}
			_recordFields = parseDataFromProperties(properties, "recordFields");
		}
		return _recordFields;
	}

	/**
	 * Returns the UI Columns.
	 * 
	 * @return UI Columns
	 */
	@SuppressWarnings("unchecked")
	public static List<UIColumn> getUIColumns() {
		if (_UIColumns == null) {
			Properties properties = ConfigReader.readProperties(
					ConfigConstants.FILE_UICOLUMNS, false);

			if(properties == null) {
				LogUtils.displayMessage(LogConstants.DISPLAY_MESSAGE_ERROR,
						"Configuration file missing.",
						"File \"" + ConfigConstants.FILE_UICOLUMNS + "\" is missing."
						+ "\nApplication will exit now.");

				System.exit(-1);
			}
			_UIColumns = parseDataFromProperties(properties, "UIColumns");
		}
		return _UIColumns;
	}

	/**
	 * Parses the data from the Properties.
	 * 
	 * @param properties
	 *            the properties object
	 * @param type
	 *            the type specifies either record field or UI column
	 * @return
	 */
	@SuppressWarnings("unchecked")
	private static List parseDataFromProperties(Properties properties,
			String type) {
		List dataList = new ArrayList();
		for (int i = 0; i < properties.size(); i++) {
			dataList.add(new Object());
		}
		Enumeration enumeration = properties.propertyNames();
		while (enumeration.hasMoreElements()) {
			String key = enumeration.nextElement().toString();
			String value = properties.getProperty(key);
			if (type.equals("recordFields")) {
				dataList
						.set(Integer.parseInt(key), new RecordField(key, value));
			} else if (type.equals("UIColumns")) {
				dataList.set(Integer.parseInt(key), new UIColumn(key, value));
			}
		}
		return dataList;
	}

	/**
	 * Method to display various messages to user.
	 * 
	 * @param type
	 *            the type of the message
	 * @param title
	 *            the Title for the message Dialog
	 * @param message
	 *            the Message to be displayed
	 */
	public static boolean displayMessage(int type, String title, String message) {
		Shell shell = LogDisplay.getInstance().getShell();

		switch (type) {
		case LogConstants.DISPLAY_MESSAGE_CONFIRM:
			return MessageDialog.openConfirm(shell, title, message);

		case LogConstants.DISPLAY_MESSAGE_ERROR:
			MessageDialog.openError(shell, title, message);
			break;

		case LogConstants.DISPLAY_MESSAGE_INFORMATION:
			MessageDialog.openInformation(shell, title, message);
			break;

		case LogConstants.DISPLAY_MESSAGE_QUESTION:
			return MessageDialog.openQuestion(shell.getShell(), title, message);

		case LogConstants.DISPLAY_MESSAGE_WARNING:
			MessageDialog.openWarning(shell, title, message);
			break;
		}
		return false;
	}
}

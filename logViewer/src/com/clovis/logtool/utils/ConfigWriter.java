/**
 * 
 */
package com.clovis.logtool.utils;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.ObjectOutputStream;
import java.util.Properties;

import org.exolab.castor.xml.MarshalException;
import org.exolab.castor.xml.Marshaller;
import org.exolab.castor.xml.ValidationException;

/**
 * Writes the configuration properties from file.
 * 
 * @author Suraj Rajyaguru
 */
public class ConfigWriter {

	/**
	 * Main method for testing.
	 * 
	 * @param args
	 *            the string arguments
	 */
	public static void main(String args[]) {
		Properties properties = createRecordFieldsProperties();
		writeProperties(properties, ConfigConstants.FILE_RECORDFIELDS);

		properties = createUIColumnsProperties();
		writeProperties(properties, ConfigConstants.FILE_UICOLUMNS);

		properties = createConstansProperties();
		writeProperties(properties, ConfigConstants.FILE_CONSTANTS);
	}

	/**
	 * Writes the properties to the given file.
	 * 
	 * @param properties
	 *            the properties to be written
	 * @param file
	 *            the file to write the properties
	 */
	public static void writeProperties(Properties properties, String file) {
		try {

			FileOutputStream fos = new FileOutputStream(file);
			properties.store(fos, "No Comments");
			fos.close();

		} catch (FileNotFoundException e) {
			LogUtils.displayMessage(LogConstants.DISPLAY_MESSAGE_ERROR,
					"File Not Found", "File \"" + file + "\" does not exist.");

		} catch (IOException e) {
			LogUtils.displayMessage(LogConstants.DISPLAY_MESSAGE_ERROR,
					"Error writing File", "Unable to write \"" + file + "\" file.");
		}
	}

	/**
	 * Writes the object to the given file.
	 * 
	 * @param object
	 *            the object to be written
	 * @param file
	 *            the file to write the object
	 */
	public static void writeObject(Object object, String file) {
		try {

			ObjectOutputStream oos = new ObjectOutputStream(new FileOutputStream(file));
			oos.writeObject(object);
			oos.close();

		} catch (FileNotFoundException e) {
			LogUtils.displayMessage(LogConstants.DISPLAY_MESSAGE_ERROR,
					"File Not Found", "File \"" + file + "\" does not exist.");

		} catch (IOException e) {
			LogUtils.displayMessage(LogConstants.DISPLAY_MESSAGE_ERROR,
					"Error writing File", "Unable to write \"" + file + "\" file.");
		}
	}

	/**
	 * Writes the object to the given file in XML format.
	 * 
	 * @param object
	 *            the object to be written
	 * @param file
	 *            the file to write the object
	 */
	public static void writeXML(Object object, String file) {
		try {

			Marshaller.marshal(object, new FileWriter(file));

		} catch (MarshalException e) {
			LogUtils.displayMessage(LogConstants.DISPLAY_MESSAGE_ERROR,
					"Error writing File", "Unable to write \"" + file + "\" file.");

		} catch (ValidationException e) {
			LogUtils.displayMessage(LogConstants.DISPLAY_MESSAGE_ERROR,
					"Error writing File", "Unable to write \"" + file + "\" file.");

		} catch (IOException e) {
			LogUtils.displayMessage(LogConstants.DISPLAY_MESSAGE_ERROR,
					"Error writing File", "Unable to write \"" + file + "\" file.");
		}
	}

	/**
	 * Method for testing.
	 * 
	 * @return the properties
	 */
	private static Properties createConstansProperties() {
		Properties properties = new Properties();

		properties.setProperty("FileHeader.Length", "0");
		properties.setProperty("Record.Length", "256");
		properties.setProperty("RecordHeader.Length", "20");

		return properties;
	}

	/**
	 * Method for testing.
	 * 
	 * @return the properties
	 */
	private static Properties createUIColumnsProperties() {
		Properties properties = new Properties();

		properties.setProperty("7", "Flag:1:0:0:false");
		properties.setProperty("0", "Severity:1:1:20:true");
		properties.setProperty("1", "StreamId:2:2:15:true");
		properties.setProperty("2", "ComponentId:2:3:15:true");
		properties.setProperty("3", "ServiceId:1:2:15:true");
		properties.setProperty("4", "TimeStamp:5:5:35:true");
		properties.setProperty("5", "MessageId:3:6:15:true");
		properties.setProperty("6", "Message:4:-1:178:true");

		return properties;
	}

	/**
	 * Method for testing.
	 * 
	 * @return the properties
	 */
	private static Properties createRecordFieldsProperties() {
		Properties properties = new Properties();

		properties.setProperty("0", "Flag:0:0:0");
		properties.setProperty("1", "Severity:0:1:1");
		properties.setProperty("2", "StreamId:1:2:3");
		properties.setProperty("3", "ComponentId:1:4:5");
		properties.setProperty("4", "ServiceId:1:6:7");
		properties.setProperty("5", "TimeStamp:3:8:15");
		properties.setProperty("6", "MessageId:2:16:19");

		return properties;
	}
}

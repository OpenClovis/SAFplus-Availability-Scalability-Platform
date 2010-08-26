/**
 * 
 */
package com.clovis.logtool.utils;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.util.Properties;

import org.exolab.castor.xml.MarshalException;
import org.exolab.castor.xml.Unmarshaller;
import org.exolab.castor.xml.ValidationException;

/**
 * Reads the configuration properties from file.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class ConfigReader {

	/**
	 * Main method for testing.
	 * 
	 * @param args
	 *            the string arguments
	 */
	public static void main(String args[]) {
		String file = "config" + System.getProperty("file.separator")
				+ "recordFields";
		readProperties(file, true);
	}

	/**
	 * Reads and returns the properties from the given file.
	 * 
	 * @param file
	 *            the properties file
	 * @param handleError
	 *            flag that specify wether to hadle error or not
	 * @return the properties
	 */
	public static Properties readProperties(String file, boolean handleError) {
		try {

			FileInputStream fis = new FileInputStream(file);
			Properties properties = new Properties();
			properties.load(fis);
			fis.close();
			return properties;

		} catch (FileNotFoundException e) {
			if(handleError) {
				LogUtils
						.displayMessage(LogConstants.DISPLAY_MESSAGE_ERROR,
								"File Not Found", "File \"" + file
										+ "\" is not found.");
			}

		} catch (IOException e) {
			if(handleError) {
				LogUtils.displayMessage(LogConstants.DISPLAY_MESSAGE_ERROR,
						"Unable To Read File", "Error reading File \"" + file
								+ "\".");
			}
		}

		return null;
	}

	/**
	 * Reads and returns the object from the given file.
	 * 
	 * @param file
	 *            the object file
	 * @param handleError
	 *            flag that specify wether to hadle error or not
	 * @return the object
	 */
	public static Object readObject(String file, boolean handleError) {
		try {

			ObjectInputStream ois = new ObjectInputStream(new FileInputStream(file));
			Object object = ois.readObject();
			ois.close();
			return object;

		} catch (FileNotFoundException e) {
			if(handleError) {
				LogUtils
						.displayMessage(LogConstants.DISPLAY_MESSAGE_ERROR,
								"File Not Found", "File \"" + file
										+ "\" is not found.");
			}

		} catch (IOException e) {
			if(handleError) {
				LogUtils.displayMessage(LogConstants.DISPLAY_MESSAGE_ERROR,
						"Unable To Read File", "Error reading File \"" + file
								+ "\".");
			}

		} catch (ClassNotFoundException e) {
			if(handleError) {
				LogUtils.displayMessage(LogConstants.DISPLAY_MESSAGE_ERROR,
						"Class Not Found", "Class for the object not found.");
			}
		}

		return null;
	}

	/**
	 * Reads and returns the object from the given XML format file.
	 * 
	 * @param clazz
	 *            the class of the object
	 * @param file
	 *            the object file
	 * @param handleError
	 *            flag that specify wether to hadle error or not
	 * @return the object
	 */
	public static Object readXML(Class clazz, String file, boolean handleError) {
		try {

			Object object = Unmarshaller.unmarshal(clazz, new FileReader(file));
			return object;

		} catch (MarshalException e) {
			if (handleError) {
				LogUtils.displayMessage(LogConstants.DISPLAY_MESSAGE_ERROR,
						"Unable To Read File", "Error reading File \"" + file
								+ "\".");
			}

		} catch (ValidationException e) {
			if (handleError) {
				LogUtils.displayMessage(LogConstants.DISPLAY_MESSAGE_ERROR,
						"Unable To Read File", "Error reading File \"" + file
								+ "\".");
			}

		} catch (FileNotFoundException e) {
			if (handleError) {
				LogUtils
						.displayMessage(LogConstants.DISPLAY_MESSAGE_ERROR,
								"File Not Found", "File \"" + file
										+ "\" is not found.");
			}
		}

		return null;
	}
}

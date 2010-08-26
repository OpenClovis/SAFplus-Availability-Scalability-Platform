/**
 * 
 */
package com.clovis.logtool.utils;

import java.util.StringTokenizer;

/**
 * Represents the Field of Record.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class RecordField {

	/**
	 * Index of field.
	 */
	private int index;

	/**
	 * Name of field.
	 */
	private String name;

	/**
	 * Type of field.
	 */
	private int type;

	/**
	 * Start Index of field.
	 */
	private int startIndex;

	/**
	 * End index of field.
	 */
	private int endIndex;

	/**
	 * Constructs the Record Field instance.
	 * 
	 * @param index
	 *            the index of Field
	 * @param details
	 *            the string from which other details are parse
	 */
	public RecordField(String index, String details) {
		this.index = Integer.parseInt(index);
		parseRecordFieldDetails(details);
	}

	/**
	 * Parses the details of field record.
	 * 
	 * @param details
	 *            the record field details
	 */
	private void parseRecordFieldDetails(String details) {
		StringTokenizer strToken = new StringTokenizer(details, ":");

		name = strToken.nextToken();
		type = Integer.parseInt(strToken.nextToken());
		startIndex = Integer.parseInt(strToken.nextToken());
		endIndex = Integer.parseInt(strToken.nextToken());
	}

	/**
	 * Returns the End Index of Record field.
	 * 
	 * @return the endIndex
	 */
	public int getEndIndex() {
		return endIndex;
	}

	/**
	 * Returns the Index of Record field.
	 * 
	 * @return the Index of Record field
	 */
	public int getIndex() {
		return index;
	}

	/**
	 * Returns the Name of Record field.
	 * 
	 * @return the Name of Record field
	 */
	public String getName() {
		return name;
	}

	/**
	 * Returns the Start Index of Record field.
	 * 
	 * @return the Start Index of Record field
	 */
	public int getStartIndex() {
		return startIndex;
	}

	/**
	 * Returns the Type of Record field.
	 * 
	 * @return the Type of Record field
	 */
	public int getType() {
		return type;
	}
}

/**
 * 
 */
package com.clovis.logtool.utils;

import java.util.StringTokenizer;

/**
 * Represents the Column of UI.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class UIColumn {

	/**
	 * Index of Column.
	 */
	private int index;

	/**
	 * Name of Column.
	 */
	private String name;

	/**
	 * Type of Column.
	 */
	private int type;

	/**
	 * Field Index of Column.
	 */
	private int fieldIndex;

	/**
	 * Width of Column.
	 */
	private int width;

	/**
	 * Show Flag for Column.
	 */
	private boolean showFlag;

	/**
	 * Constructs UI Column instance.
	 * 
	 * @param index
	 *            the index of column
	 * @param details
	 *            the details for column
	 */
	public UIColumn(String index, String details) {
		this.index = Integer.parseInt(index);
		parseUIColumnDetails(details);
	}

	/**
	 * Parses the details of UI Column.
	 * 
	 * @param details
	 *            the details of column
	 */
	private void parseUIColumnDetails(String details) {
		StringTokenizer strToken = new StringTokenizer(details, ":");

		name = strToken.nextToken();
		type = Integer.parseInt(strToken.nextToken());
		fieldIndex = Integer.parseInt(strToken.nextToken());
		width = Integer.parseInt(strToken.nextToken());
		showFlag = Boolean.parseBoolean(strToken.nextToken());
	}

	/**
	 * Returns the Field Index.
	 * 
	 * @return the Field Index
	 */
	public int getFieldIndex() {
		return fieldIndex;
	}

	/**
	 * Returns the Column Index.
	 * 
	 * @return the Column Index
	 */
	public int getIndex() {
		return index;
	}

	/**
	 * Returns the Name.
	 * 
	 * @return the Name
	 */
	public String getName() {
		return name;
	}

	/**
	 * Returns the Show flag.
	 * 
	 * @return the Show flag
	 */
	public boolean isShowFlag() {
		return showFlag;
	}

	/**
	 * Returns the Type.
	 * 
	 * @return the Type
	 */
	public int getType() {
		return type;
	}

	/**
	 * Returns the Width.
	 * 
	 * @return the width
	 */
	public int getWidth() {
		return width;
	}
}

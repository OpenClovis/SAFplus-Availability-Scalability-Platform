/**
 * 
 */
package com.clovis.logtool.utils;

import java.util.HashMap;

import com.clovis.logtool.record.filter.RecordFilter;

/**
 * Container Class to contain filter map for storing it in the XML file.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class FilterMapXMLClass {

	/**
	 * Filter Map.
	 */
	private HashMap<String, RecordFilter> _filterMap = new HashMap<String, RecordFilter>();

	/**
	 * Default Constructor.
	 */
	public FilterMapXMLClass() {
	}

	/**
	 * Returns the Filter Map.
	 * 
	 * @return the Filter Map
	 */
	public HashMap<String, RecordFilter> getFilterMap() {
		return _filterMap;
	}

	/**
	 * Sets the Filter Map.
	 * 
	 * @param map
	 *            the Filter Map
	 */
	public void setFilterMap(HashMap<String, RecordFilter> map) {
		_filterMap = map;
	}
}

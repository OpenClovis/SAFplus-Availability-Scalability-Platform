package com.clovis.logtool.record.filter;

import java.util.List;

import com.clovis.logtool.record.Record;

/**
 * RecordFilter defines an interface to filter the records
 * based on some given criteria.
 * 
 * @author ravi
 *
 */
public class RecordFilter {

	/**
	 * List of Filter objects, each containing some matching criteria.
	 */
	protected List _filterObjectList;

	/**
	 * The and/or flag for the filter.
	 */
	protected boolean _andFlag;

	/**
	 * Default Constructor.
	 */
	public RecordFilter() {
	}

	/**
	 * Creates the Record Filter Instance.
	 * 
	 * @param filterObjectList
	 *            the filter object list
	 * @param andFlag
	 *            the and flag
	 */
	public RecordFilter(List filterObjectList, boolean andFlag) {
		_filterObjectList = filterObjectList;
		_andFlag = andFlag;
	}

	/**
	 * Applies filtering criteria on a record.
	 * 
	 * @param record
	 *            record on which the filter will be applied
	 * @return true if the record satisfies the given criteria otherwise false
	 */
	public boolean applyFilter(Record record) {
		return false;
	}

	/**
	 * Returns the and flag.
	 * 
	 * @return the and flag
	 */
	public boolean isAndFlag() {
		return _andFlag;
	}

	/**
	 * Returns the filter object list.
	 * 
	 * @return the filter object list
	 */
	public List getFilterObjectList() {
		return _filterObjectList;
	}

	/**
	 * Sets the and flag for the filter.
	 * 
	 * @param flag
	 *            the and flag
	 */
	public void setAndFlag(boolean flag) {
		_andFlag = flag;
	}

	/**
	 * Sets the filter object list for the filter.
	 * 
	 * @param objectList
	 *            the filter object list
	 */
	public void setFilterObjectList(List objectList) {
		_filterObjectList = objectList;
	}
}

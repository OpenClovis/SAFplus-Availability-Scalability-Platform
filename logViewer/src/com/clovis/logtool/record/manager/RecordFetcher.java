/**
 * 
 */
package com.clovis.logtool.record.manager;

import java.util.List;

import com.clovis.logtool.record.Record;
import com.clovis.logtool.record.factory.RecordFactory;
import com.clovis.logtool.record.filter.RecordFilter;
import com.clovis.logtool.record.parser.RecordParser;
import com.clovis.logtool.stream.Stream;

/**
 * Abstract implementation for fetching the records.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public abstract class RecordFetcher {

	/**
	 * Record Factory instance.
	 */
	protected RecordFactory _recordFactory;

	/**
	 * Record Parser Instance.
	 */
	protected RecordParser _recordParser;

	/**
	 * Record Filter Instance.
	 */
	protected RecordFilter _recordFilter;

	/**
	 * Fetches the record batch from the Stream.
	 * 
	 * @return the list of records
	 */
	public abstract List<Record> fetchRecordBatch(boolean nextFlag);

	/**
	 * Sets the Stream from which records are to be fetched.
	 * 
	 * @param stream
	 *            the Stream
	 */
	public void setStream(Stream stream) {
		_recordFactory.setStream(stream);
	}

	/**
	 * Sets the record position from which the next record is to be fetched.
	 * 
	 * @param recordPositon
	 *            the position of record
	 */
	public void setRecordPositon(int recordPosition) {
		_recordFactory.setRecordPosition(recordPosition);
	}

	/**
	 * Sets the Filter Objects for the Record Filter.
	 * 
	 * @param filterObjectList
	 *            the filter object list
	 */
	public void setFilterData(List filterObjectList, boolean andFlag) {
		_recordFilter.setFilterObjectList(filterObjectList);
		_recordFilter.setAndFlag(andFlag);
	}

	/**
	 * Clears the information associated with the previous fetch.
	 */
	public void clearFetchData() {
		setFilterData(null, false);
	}

	/**
	 * Sets the Fetch Boundry to First position or Last position.
	 * 
	 * @param firstFlag
	 *            the boundry flag, true for first and false for last
	 */
	public void setFetchBoundry(boolean firstFlag) {
		if (firstFlag) {
			_recordFactory.setAbsolutePosition(0);
		} else {
			int streamSize = _recordFactory.getStream().getSize();
			_recordFactory.setAbsolutePosition(streamSize
					- (streamSize % _recordFactory.getRecordLength()));
		}
	}
}

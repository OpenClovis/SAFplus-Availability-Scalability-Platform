/**
 * 
 */
package com.clovis.logtool.record.manager;

import java.util.HashMap;
import java.util.List;

import com.clovis.logtool.record.Record;

/**
 * Abstract implementation which specifies the behaviour for managing the
 * records.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public abstract class RecordManager {

	/**
	 * Contains the current set of record for the batch.
	 */
	protected List<Record> _recordBatch;

	/**
	 * Record fetcher which fetches the record for this.
	 */
	protected RecordFetcher _recordFetcher;

	/**
	 * Holds the Configuration info.
	 */
	protected RecordConfiguration _recordConfiguration;

	/**
	 * Constructor.
	 * 
	 * @param recordConfiguration
	 */
	public RecordManager(RecordConfiguration recordConfiguration) {
		_recordConfiguration = recordConfiguration;
	}

	/**
	 * Returns the list of record for UI to show in response to the user action.
	 * 
	 * @param nextFlag
	 *            Determines wether the next batch of record to be read or the
	 *            previous batch.
	 * @return the list of records
	 */
	public abstract List<Record> getUIRecordBatch(boolean nextFlag);

	/**
	 * Fetches the record batch from the Stream.
	 * 
	 * @return the list of records
	 */
	public void fetchRecordBatch(boolean nextFlag) {
		_recordBatch = _recordFetcher.fetchRecordBatch(nextFlag);
	}

	/**
	 * Returns the Record Fetcher for this.
	 * 
	 * @return the Record Fetcher
	 */
	public RecordFetcher getRecordFetcher() {
		return _recordFetcher;
	}

	/**
	 * Clears the current record batch.
	 */
	public void clearRecordBatch() {
		_recordBatch = null;
		_recordFetcher.clearFetchData();
	}

	/**
	 * Returns the Mapping for the particular field.
	 * 
	 * @param filedIndex
	 *            the index of field
	 * @return the Map containg the mapping information
	 */
	public abstract HashMap<String, String> getFieldMapping(int fieldIndex);

	/**
	 * Returns the configuration info for the records.
	 * 
	 * @return the configuration info
	 */
	public RecordConfiguration getRecordConfiguration() {
		return _recordConfiguration;
	}
}

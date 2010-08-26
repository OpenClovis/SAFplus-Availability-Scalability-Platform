/**
 * 
 */
package com.clovis.logtool.record.manager;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.util.ArrayList;
import java.util.List;

import com.clovis.logtool.record.LogRecordHeader;
import com.clovis.logtool.record.Record;
import com.clovis.logtool.record.factory.RecordFactory;
import com.clovis.logtool.record.filter.RecordFilter;
import com.clovis.logtool.record.parser.RecordParser;
import com.clovis.logtool.utils.LogConstants;
import com.clovis.logtool.utils.LogUtils;

/**
 * Concrete implementation used for fetching log records.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class LogRecordFetcher extends RecordFetcher {

	/**
	 * Holds the last next flag value to keep track of fetch direction.
	 */
	private boolean _lastFetchFlag = true;

	/**
	 * Holds the last fetching position.
	 */
	private int _lastFetchPosition;

	/**
	 * Holds the top record number of the last batch.
	 */
	private int _topRecordNumber;

	/**
	 * Construct the Log record fetcher.
	 * 
	 * @param recordLength
	 *            the record length
	 */
	public LogRecordFetcher(int recordLength) {
		try {
			Class clazz = Class
					.forName("com.clovis.logtool.record.factory.LogRecordFactory");
			Constructor constructor = clazz
					.getConstructor(new Class[] { int.class });
			_recordFactory = (RecordFactory) constructor
					.newInstance(new Object[] { recordLength });

			clazz = Class
					.forName("com.clovis.logtool.record.parser.LogRecordParser");
			constructor = clazz.getConstructor(new Class[] { int.class });
			_recordParser = (RecordParser) constructor
					.newInstance(new Object[] { recordLength });

			clazz = Class
					.forName("com.clovis.logtool.record.filter.LogRecordFilter");
			_recordFilter = (RecordFilter) clazz.newInstance();
		} catch (InstantiationException e) {
			System.err.println("Error: Could not create instance of class " + e.getMessage());
			System.exit(-1);
		} catch (IllegalAccessException e) {
			System.err.println("Error: Could not create instance of class " + e.getMessage());
			System.exit(-1);
		} catch (ClassNotFoundException e) {
			System.err.println("Error: Could not create instance of class " + e.getMessage());
			System.exit(-1);
		} catch (SecurityException e) {
			System.exit(-1);
		} catch (NoSuchMethodException e) {
			System.exit(-1);
		} catch (IllegalArgumentException e) {
			System.exit(-1);
		} catch (InvocationTargetException e) {
			System.exit(-1);
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.logtool.record.manager.RecordFetcher#fetchRecordBatch()
	 */
	public List<Record> fetchRecordBatch(boolean nextFlag) {
		if (_recordFactory.getStream() == null) {
			LogUtils.displayMessage(LogConstants.DISPLAY_MESSAGE_WARNING,
					"No Stream", "Stream is not opened");
			return null;
		}

		int startPosition = _recordFactory.getPosition();
		if (_lastFetchFlag != nextFlag) {
			_recordFactory.setAbsolutePosition(_lastFetchPosition);
		}
		int fetchPosition = _recordFactory.getPosition();

		int count = 0;
		byte bytes[] = null;
		int recordNumber;

		ArrayList<Record> recordBatch = new ArrayList<Record>();
		while (count < LogConstants.VIEWER_DISPLAYBATCH_SIZE) {

			bytes = nextFlag ? _recordFactory.getNextRecord() : _recordFactory
					.getPreviousRecord();
			if (bytes == null) {
				break;
			}

			Record record = _recordParser.parseRecord(bytes);
			if(record == null) {
				LogUtils.displayMessage(LogConstants.DISPLAY_MESSAGE_ERROR,
						"Parsing Error", "Error while parsing log records." +
								"\nThe Binary Log Viewer is only for use on binary log files." +
								"\nIf you wish to view an ASCII log file use your favorite editor.");
				break;
			}

			if (nextFlag) {
				recordNumber = (_recordFactory.getPosition() - _recordFactory
						.getRecordLength())
						/ _recordFactory.getRecordLength();

			} else {
				recordNumber = _recordFactory.getPosition()
						/ _recordFactory.getRecordLength();
			}
			((LogRecordHeader) record.getHeader())
					.setRecordNumber(recordNumber);

			long timeStamp = ((Long) record.getField(
					LogConstants.FIELD_INDEX_TIMESTAMP, false)).longValue();

			if (timeStamp > 0 && _recordFilter.applyFilter(record)) {
				if (nextFlag) {
					recordBatch.add(record);
				} else {
					recordBatch.add(0, record);
				}

				count++;
			}
		}

		if (recordBatch.size() == 0) {
			LogUtils.displayMessage(LogConstants.DISPLAY_MESSAGE_INFORMATION,
					"No Records", "There are no records to display.");
			_recordFactory.setAbsolutePosition(startPosition);
		} else {
			_lastFetchPosition = fetchPosition;
			_lastFetchFlag = nextFlag;
			_topRecordNumber = ((LogRecordHeader) recordBatch.get(0)
					.getHeader()).getRecordNumber();
		}

		return recordBatch;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.logtool.record.manager.RecordFetcher#clearFetchData()
	 */
	@Override
	public void clearFetchData() {
		super.clearFetchData();
		_lastFetchFlag = true;
		_recordFactory.setRecordPosition(_topRecordNumber);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.logtool.record.manager.RecordFetcher#setFetchBoundry(boolean)
	 */
	@Override
	public void setFetchBoundry(boolean firstFlag) {
		super.setFetchBoundry(firstFlag);

		if(firstFlag) {
			_lastFetchFlag = true;
		} else {
			_lastFetchFlag = false;
		}
	}
}

/**
 * 
 */
package com.clovis.logtool.record.manager;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.util.HashMap;
import java.util.List;

import com.clovis.logtool.record.Record;
import com.clovis.logtool.utils.LogConstants;

/**
 * Concrete implementation for managing the log records.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class LogRecordManager extends RecordManager {

	/**
	 * Constructs the Log record manager.
	 */
	public LogRecordManager(RecordConfiguration recordConfiguration) {
		super(recordConfiguration);

		try {
			Class clazz = Class
					.forName("com.clovis.logtool.record.manager.LogRecordFetcher");
			Constructor constructor = clazz
					.getConstructor(new Class[] { int.class });
			_recordFetcher = (RecordFetcher) constructor
					.newInstance(new Object[] { _recordConfiguration
							.getRecordLength() });
		} catch (ClassNotFoundException e) {
			System.err.println("Error: Could not create instance of class "
					+ e.getMessage());
			System.exit(-1);
		} catch (InstantiationException e) {
			System.err.println("Error: Could not create instance of class "
					+ e.getMessage());
			System.exit(-1);
		} catch (IllegalAccessException e) {
			System.err.println("Error: Could not create instance of class "
					+ e.getMessage());
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
	 * @see com.clovis.logtool.record.manager.RecordManager#getUIRecordBatch(boolean)
	 */
	public List<Record> getUIRecordBatch(boolean nextFlag) {
		fetchRecordBatch(nextFlag);
		return _recordBatch;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.logtool.record.manager.RecordManager#getFieldMapping(int)
	 */
	@Override
	public HashMap<String, String> getFieldMapping(int fieldIndex) {

		switch (fieldIndex) {

		case LogConstants.FIELD_INDEX_COMPONENTID:
			return _recordConfiguration.getComponentMap();

		case LogConstants.FIELD_INDEX_STREAMID:
			return _recordConfiguration.getStreamMap();
		}

		return null;
	}
}

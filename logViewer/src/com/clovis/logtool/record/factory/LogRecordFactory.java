/**
 * 
 */
package com.clovis.logtool.record.factory;

import java.util.ArrayList;
import java.util.List;

import com.clovis.logtool.stream.Stream;

/**
 * Concrete implementation of the Record Factory for creating Log Records.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class LogRecordFactory extends RecordFactory {

	/**
	 * Constructor.
	 * 
	 * @param recordLength
	 *            the length of the record.
	 */
	public LogRecordFactory(int recordLength) {
		super(recordLength);
		_fileHeaderLength = _recordLength;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.logtool.record.RecordFactory#getNextRecord()
	 */
	public byte[] getNextRecord() {
		byte[] bytes = new byte[_recordLength];
		if (_stream.read(bytes)) {
			return bytes;
		} else {
			return null;
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.logtool.record.factory.RecordFactory#getPreviousRecord()
	 */
	public byte[] getPreviousRecord() {
		byte[] bytes = new byte[_recordLength];
		if (_stream.setRelativePosition(-_recordLength)) {
			if (_stream.read(bytes)) {
				_stream.setRelativePosition(- _recordLength);
				return bytes;
			}
		}
		return null;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.logtool.record.RecordFactory#getNextRecordBatch(int)
	 */
	public List<byte[]> getNextRecordBatch(int n) {
		ArrayList<byte[]> recordBatch = new ArrayList<byte[]>();
		byte[] bytes;
		for (int i = 0; i < n; i++) {
			bytes = new byte[_recordLength];
			if (_stream.read(bytes)) {
				recordBatch.add(bytes);
			} else {				
				break;
			}
		}
		return recordBatch;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.logtool.record.RecordFactory#setRecordPosition(int)
	 */
	public boolean setRecordPosition(int position) {
		int streamPosition = position * _recordLength;
		if (_stream != null) {
			return _stream.setPosition(streamPosition);
		}
		return false;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.logtool.record.factory.RecordFactory#setRelativeRecordPosition(int)
	 */
	public boolean setRelativeRecordPosition(int position) {
		int streamPosition = position * _recordLength;
		if (_stream != null) {
			return _stream.setRelativePosition(streamPosition);
		}
		return false;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.logtool.record.factory.RecordFactory#setStream(com.clovis.logtool.stream.Stream)
	 */
	public void setStream(Stream stream) {
		super.setStream(stream);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.logtool.record.factory.RecordFactory#getRecordPosition()
	 */
	@Override
	public int getPosition() {
		return _stream.getPosition();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.logtool.record.factory.RecordFactory#setAbsolutePosition(int)
	 */
	@Override
	public boolean setAbsolutePosition(int position) {
		if (_stream != null) {
			return _stream.setPosition(position);
		}
		return false;
	}
}

/**
 * 
 */
package com.clovis.logtool.record;

import com.clovis.logtool.utils.LogUtils;

/**
 * Represents the header part for the record of type Log Record.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class LogRecordHeader {

	/**
	 * Record Number for the Record.
	 */
	private int _recordNumber;

	/**
	 * Flag for the Record.
	 */
	private short _flag;

	/**
	 * Severity for the record.
	 */
	private long _severity;

	/**
	 * Stream Id for the Record.
	 */
	private int _streamId;

	/**
	 * Component Id for the Record.
	 */
	private long _componentId;

	/**
	 * Service Id for the Record.
	 */
	private int _serviceId;

	/**
	 * TimeStamp for the Record.
	 */
	private long _timeStamp;

	/**
	 * Message Id for the Record.
	 */
	private int _messageId;

	/**
	 * Parses the various header fields from byte array.
	 * 
	 * @param bytes
	 *            the byte sequence for the header
	 */
	public LogRecordHeader(byte[] bytes) {
		_flag = LogUtils.getUnSignedByteFromBytes(bytes, 0);

		if (getEndianNess() == 0) {
			_severity = LogUtils.getUnSignedIntFromBytesBE(bytes, 1);
			_streamId = LogUtils.getUnSignedShortFromBytesBE(bytes, 5);
			_componentId = LogUtils.getUnSignedIntFromBytesBE(bytes, 7);
			_serviceId = LogUtils.getUnSignedShortFromBytesBE(bytes, 11);
			_messageId = LogUtils.getUnSignedShortFromBytesBE(bytes, 13);
			_timeStamp = LogUtils.getLongFromBytesBE(bytes, 15);
		} else {
			_severity = LogUtils.getUnSignedIntFromBytesLE(bytes, 1);
			_streamId = LogUtils.getUnSignedShortFromBytesLE(bytes, 5);
			_componentId = LogUtils.getUnSignedIntFromBytesLE(bytes, 7);
			_serviceId = LogUtils.getUnSignedShortFromBytesLE(bytes, 11);
			_messageId = LogUtils.getUnSignedShortFromBytesLE(bytes, 13);
			_timeStamp = LogUtils.getLongFromBytesLE(bytes, 15);
		}
	}

	/**
	 * Returns the Flag for the record.
	 * 
	 * @return the Flag
	 */
	public short getFlag() {
		return _flag;
	}

	/**
	 * Returns the Severity for the record.
	 * 
	 * @return the Severity
	 */
	public long getSeverity() {
		return _severity;
	}

	/**
	 * Returns the Stream Id for the record.
	 * 
	 * @return the Stream Id
	 */
	public int getStreamId() {
		return _streamId;
	}

	/**
	 * Returns the Component Id for the record.
	 * 
	 * @return the Component Id
	 */
	public long getComponentId() {
		return _componentId;
	}

	/**
	 * Returns the Service Id for the record.
	 * 
	 * @return the Service Id
	 */
	public int getServiceId() {
		return _serviceId;
	}

	/**
	 * Returns the Time Stamp for the record.
	 * 
	 * @return the TimeStamp
	 */
	public long getTimeStamp() {
		return _timeStamp;
	}

	/**
	 * Returns the Message Id for the record.
	 * 
	 * @return the Message Id
	 */
	public int getMessageId() {
		return _messageId;
	}

	/**
	 * Returns the Endian-ness for the record.
	 * 
	 * @return the endian-ness
	 */
	public byte getEndianNess() {
		return (byte) ((_flag & 0x0001) == 0 ? 0 : 1);
	}

	/**
	 * Returns the Record Number for the record.
	 * 
	 * @return the Record Number
	 */
	public int getRecordNumber() {
		return _recordNumber;
	}

	/**
	 * Sets the Record Number for the record.
	 * 
	 * @param recordNumber
	 *            the Record Number to set
	 */
	public void setRecordNumber(int recordNumber) {
		_recordNumber = recordNumber;
	}
}

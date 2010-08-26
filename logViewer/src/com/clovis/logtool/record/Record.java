/**
 * 
 */
package com.clovis.logtool.record;

/**
 * Abstract implementation for the record type.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public abstract class Record {

	/**
	 * Header part of the record.
	 */
	protected Object _header;

	/**
	 * Data part of the record.
	 */
	protected Object _data;

	/**
	 * Constructs the record with given header and data.
	 * 
	 * @param header
	 *            the header for the record
	 * @param data
	 *            the data for the record
	 */
	public Record(Object header, Object data) {
		_header = header;
		_data = data;
	}

	/**
	 * Returns the data part for this message.
	 * 
	 * @return Data
	 */
	public Object getData() {
		return _data;
	}

	/**
	 * Returns the header part for this message.
	 * 
	 * @return Header
	 */
	public Object getHeader() {
		return _header;
	}

	/**
	 * Returns the field at the given index.
	 * 
	 * @param fieldIndex
	 *            the index of field
	 * @param UIFlag
	 *            the flag specifies whether to return the UI representation for
	 *            the value or the value itself. True for UI representation,
	 *            false otherwise
	 * @return the Field
	 */
	public abstract Object getField(int fieldIndex, boolean UIFlag);
}

/**
 * 
 */
package com.clovis.logtool.record.factory;

import java.util.List;

import com.clovis.logtool.stream.Stream;

/**
 * Abstract implementation of the Record Factory to create the records.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public abstract class RecordFactory {

	/**
	 * Stream associated with this Record Factory from which records are to be
	 * read.
	 */
	protected Stream _stream;

	/**
	 * Length of the record. Records of this length will be read from this
	 * record factory.
	 */
	protected int _recordLength;

	/**
	 * Length of the file header.
	 */
	protected int _fileHeaderLength;

	/**
	 * Constructor.
	 * 
	 * @param recordLength
	 *            the length of the record.
	 */
	public RecordFactory(int recordLength) {
		_recordLength = recordLength;
	}

	/**
	 * Reads next record from the associated stream.
	 * 
	 * @return record in the form of byte sequence
	 */
	public abstract byte[] getNextRecord();

	/**
	 * Reads previous record from the associated stream.
	 * 
	 * @return record in the form of byte sequence
	 */
	public abstract byte[] getPreviousRecord();

	/**
	 * Reads n number of records from the associated stream.
	 * 
	 * @param n
	 *            the number of records to be read
	 * @return batch of records in list form
	 */
	public abstract List<byte[]> getNextRecordBatch(int n);

	/**
	 * Associates the given stream with this record factory.
	 * 
	 * @param stream
	 *            the stream to be assigned to this record factory
	 */
	public void setStream(Stream stream) {
		_stream = stream;
		_stream.open(_fileHeaderLength);
	}

	/**
	 * Allows to set the current position for the factory. This in turns
	 * sets the corresponding stream's position.
	 * 
	 * @param position
	 *            the position to be set
	 * @return true if the operation is successful, and false otherwise
	 */
	public abstract boolean setAbsolutePosition(int position);

	/**
	 * Allows to set the current record position for the factory. This in turns
	 * sets the corresponding stream's position.
	 * 
	 * @param position
	 *            the position to be set
	 * @return true if the operation is successful, and false otherwise
	 */
	public abstract boolean setRecordPosition(int position);


	/**
	 * Allows to set the record position relative to current position for the factory.
	 * 
	 * @param position
	 *            the relative position to be set
	 * @return true if the operation is successful, and false otherwise
	 */
	public abstract boolean setRelativeRecordPosition(int position);

	/**
	 * Returns the Position in the Stream associated with this Record
	 * Factory.
	 * 
	 * @return the Position
	 */
	public abstract int getPosition();

	/**
	 * Returns the Stream associated with this Record Factory.
	 * 
	 * @return the Stream
	 */
	public Stream getStream() {
		return _stream;
	}

	/**
	 * Returns the Record Length for this Record Factory.
	 * 
	 * @return the Record Length
	 */
	public int getRecordLength() {
		return _recordLength;
	}

	/**
	 * Returns the File Header Length for this Record Factory.
	 * 
	 * @return the File Header Length
	 */
	public int getFileHeaderLength() {
		return _fileHeaderLength;
	}
}

/**
 * 
 */
package com.clovis.logtool.record.parser;

import java.util.List;

import com.clovis.logtool.record.Record;

/**
 * Abstract implementation for record parsing.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public interface RecordParser {

	/**
	 * Parses the record from the given byte sequence.
	 * 
	 * @param bytes
	 *            the byte sequence from which the record is to be parsed
	 * @return record parsed from the byte sequence
	 */
	public Record parseRecord(byte[] bytes);

	/**
	 * Parses the batch of record from the given list of record in the byte
	 * sequence format.
	 * 
	 * @param list
	 *            record list in the form of byte sequence
	 * @return list of records parsed from the given list in byte sequence form
	 */
	public List<Record> parseRecordBatch(List<byte[]> list);
}

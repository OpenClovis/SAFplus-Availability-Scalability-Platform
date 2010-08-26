/**
 * 
 */
package com.clovis.logtool.record.parser;

import java.util.ArrayList;
import java.util.List;

import com.clovis.logtool.record.LogRecord;
import com.clovis.logtool.record.LogRecordHeader;
import com.clovis.logtool.record.Record;
import com.clovis.logtool.record.TLV;
import com.clovis.logtool.utils.LogConstants;
import com.clovis.logtool.utils.LogUtils;

/**
 * Concrete implementation of the Record Parser to parse the Log Records.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class LogRecordParser implements RecordParser {

	/**
	 * Length of the record. Records of this length will be read from this
	 * record factory.
	 */
	protected int _recordLength;

	/**
	 * Constructor.
	 * 
	 * @param recordLength
	 *            the length of the record.
	 */
	public LogRecordParser(int recordLength) {
		_recordLength = recordLength;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.logtool.record.parser.RecordParser#parseRecord(byte[])
	 */
	public Record parseRecord(byte[] bytes) {
		Record record = null;
		Object data = null;
		LogRecordHeader header = null;

		int recordHeaderLength = Integer.parseInt(LogUtils.getConstants()
				.getProperty("RecordHeader.Length"));

		try {
			header = new LogRecordHeader(LogUtils.getBytesSubset(bytes, 0,
					recordHeaderLength - 1));

			long messageId = header.getMessageId();
			if(messageId == LogConstants.MESSAGEID_ASCII) {
				data = LogUtils.getStringFromBytes(bytes, recordHeaderLength);
				record = new LogRecord(header, data);
			} else if(messageId == LogConstants.MESSAGEID_BINARY) {
				int dataLength = (int) (header.getEndianNess() == 0 ? LogUtils.getUnSignedIntFromBytesBE(bytes,
						recordHeaderLength) : LogUtils.getUnSignedIntFromBytesLE(bytes, recordHeaderLength));
				data = LogUtils.getBytesSubset(bytes, recordHeaderLength + 4,
						recordHeaderLength + 4 + dataLength);
				record = new LogRecord(header, data);
			} else{
				data = getTLVDataFromBytes(LogUtils.getBytesSubset(bytes,
						recordHeaderLength, _recordLength - 1), header.getEndianNess());
				record = new LogRecord(header, data);
			}
			return record;
		} catch(Exception e) {
			return null;
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.logtool.record.parser.RecordParser#parseRecordBatch(java.util.List)
	 */
	public List<Record> parseRecordBatch(List<byte[]> list) {
		ArrayList<Record> recordList = new ArrayList<Record>();

		for (int i = 0; i < list.size(); i++) {
			recordList.add(parseRecord(list.get(i)));
		}
		return recordList;
	}

	/**
	 * Parses the data part of the record into the TLV form.
	 * 
	 * @param bytes
	 *            the byte sequence to be parsed
	 * @param n
	 *            the number of TLV to be parsed
	 * @return list of the parsed TLVs
	 */
	private List<TLV> getTLVDataFromBytes(byte[] bytes, byte endianNess) {
		ArrayList<TLV> tlvList = new ArrayList<TLV>();

		int i = 0;
		while (true) {
			int tag = endianNess == 0 ? LogUtils.getUnSignedShortFromBytesBE(bytes,
					i) : LogUtils.getUnSignedShortFromBytesLE(bytes, i);
			if (tag == 0) {
				break;
			}

			int length = endianNess == 0 ? LogUtils.getUnSignedShortFromBytesBE(
					bytes, i + TLV.TLV_LENGTH_START_INDEX) : LogUtils
					.getUnSignedShortFromBytesLE(bytes, i
							+ TLV.TLV_LENGTH_START_INDEX);

			byte[] value = LogUtils.getBytesSubset(bytes, i
					+ TLV.TLV_VALUE_START_INDEX, i + TLV.TLV_VALUE_START_INDEX
					+ length - 1);

			tlvList.add(new TLV(tag, length, value, endianNess));
			i += TLV.TLV_VALUE_START_INDEX + length;
		}
		return tlvList;
	}
}

package com.clovis.logtool.record;

import com.clovis.logtool.utils.LogUtils;

/**
 * Represents the TLV data i.e. data in the Tag, Length and Value form.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class TLV {

	// Constants for the TLV structure.

	public static final int TLV_LENGTH_START_INDEX = 2;

	public static final int TLV_VALUE_START_INDEX = 4;


	// Constants for the Tag of TLV used by Logger.
	static final int LOGGER_TAG_END = 0;

	static final int LOGGER_TAG_BASIC_SIGNED = 1;

	static final int LOGGER_TAG_BASIC_UNSIGNED = 2;

	static final int LOGGER_TAG_STRING = 3;


	// Constants for the Tag of TLV used by LogTool.

	static final int TAG_END = 0;

	static final int TAG_BASIC = 1;

	static final int TAG_BYTE_SIGNED = 2;

	static final int TAG_BYTE_UNSIGNED = 3;

	static final int TAG_SHORT_SIGNED = 4;

	static final int TAG_SHORT_UNSIGNED = 5;

	static final int TAG_INT_SIGNED = 6;

	static final int TAG_INT_UNSIGNED = 7;

	static final int TAG_LONG = 8;

	static final int TAG_STRING = 9;

	/**
	 * Tag part for the Data.
	 */
	private int _tag;

	/**
	 * Length part for the Data.
	 */
	private int _length;

	/**
	 * Value part for the Data.
	 */
	private Object _value;

	/**
	 * Constructs the data in the TLV form.
	 * 
	 * @param tag
	 *            the Tag for the data
	 * @param length
	 *            the Length for the data
	 * @param value
	 *            the Value for the data
	 */
	public TLV(int tag, int length, byte[] value, byte endian) {
		_tag = getLogToolTag(tag, length);
		_length = length;
		_value = parseValueFromBytes(_tag, value, endian);
	}

	/**
	 * Returns the tag value for log tool corresponding to tag and length
	 * specified by logger.
	 * 
	 * @param tag
	 *            the tag
	 * @param length
	 *            the length
	 * @return the tag used by log tool
	 */
	private int getLogToolTag(int tag, int length) {
		int logToolTag = 0;
		switch(tag){
		case LOGGER_TAG_END:
			logToolTag = TAG_END;
			break;
		case LOGGER_TAG_BASIC_SIGNED:
			switch(length) {
			case 1:
				logToolTag = TAG_BYTE_SIGNED;
				break;
			case 2:
				logToolTag = TAG_SHORT_SIGNED;
				break;
			case 4:
				logToolTag = TAG_INT_SIGNED;
				break;
			case 8:
				logToolTag = TAG_LONG;
				break;
			}
			break;
		case LOGGER_TAG_BASIC_UNSIGNED:
			switch(length) {
			case 1:
				logToolTag = TAG_BYTE_UNSIGNED;
				break;
			case 2:
				logToolTag = TAG_SHORT_UNSIGNED;
				break;
			case 4:
				logToolTag = TAG_INT_UNSIGNED;
				break;
			case 8:
				logToolTag = TAG_LONG;
				break;
			}
			break;
		case LOGGER_TAG_STRING:
			logToolTag = TAG_STRING;
			break;
		}
		return logToolTag;
	}

	/**
	 * Parses the value from the given byte sequence.
	 * 
	 * @param tag
	 *            the Tag for the data
	 * @param bytes
	 *            the byte sequence to be parsed to obtain the Value
	 * @return the Value for the data
	 */
	private Object parseValueFromBytes(int tag, byte[] bytes, byte endian) {
		Object obj = null;
		switch (tag) {

		case TAG_BYTE_SIGNED:
			obj = new Byte(bytes[0]);
			break;

		case TAG_BYTE_UNSIGNED:
			obj = new Short((short) bytes[0]);
			break;

		case TAG_SHORT_SIGNED:
			obj = endian == 0 ? new Short(LogUtils
					.getShortFromBytesBE(bytes, 0)) : new Short(LogUtils
					.getShortFromBytesLE(bytes, 0));
			break;

		case TAG_SHORT_UNSIGNED:
			obj = endian == 0 ? new Integer(LogUtils
					.getUnSignedShortFromBytesBE(bytes, 0)) : new Integer(
					LogUtils.getUnSignedShortFromBytesLE(bytes, 0));
			break;

		case TAG_INT_SIGNED:
			obj = endian == 0 ? new Integer(LogUtils
					.getIntFromBytesBE(bytes, 0)) : new Integer(LogUtils
					.getIntFromBytesLE(bytes, 0));
			break;

		case TAG_INT_UNSIGNED:
			obj = endian == 0 ? new Long(LogUtils.getUnSignedIntFromBytesBE(
					bytes, 0)) : new Long(LogUtils.getUnSignedIntFromBytesLE(
					bytes, 0));
			break;

		case TAG_LONG:
			obj = endian == 0 ? new Long(LogUtils.getLongFromBytesBE(bytes, 0))
					: new Long(LogUtils.getLongFromBytesLE(bytes, 0));
			break;

		case TAG_STRING:
			obj = new String(bytes);
			break;

		default:
			// obj = Use TLV parser from user
		}
		return obj;
	}

	/**
	 * Returns the Tag part for the data.
	 * 
	 * @return the Tag
	 */
	public int getTag() {
		return _tag;
	}

	/**
	 * Returns the Length part for the data.
	 * 
	 * @return the Length
	 */
	public int getLength() {
		return _length;
	}

	/**
	 * Returns the Value part for the data.
	 * 
	 * @return the Value
	 */
	public Object getValue() {
		return _value;
	}
}

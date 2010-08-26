/**
 * 
 */
package com.clovis.logtool.record.manager;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.util.HashMap;

import com.clovis.logtool.utils.LogUtils;

/**
 * Parses the Configuration file.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class RecordConfiguration {

	private byte _endian;

	private long _recordLength;

	private HashMap<String, String> _streamMap;

	private HashMap<String, String> _componentMap;


	private static final int endMarker = 0;

	private static final int endianStart = 287;

	private static final int recordLenStart = 292;

	private static final int streamHeadStart = 320;

	private static final int componentHeadStart = 324;

	/**
	 * Initializes the fields.
	 */
	public RecordConfiguration(String fileName) {

		try {
			RandomAccessFile file = new RandomAccessFile(fileName, "r");

			byte[] byteBytes = new byte[1];
			byte[] intBytes = new byte[4];

			file.seek(endianStart);
			file.read(byteBytes);
			_endian = (byte) ((byteBytes[0] & 0x0001) == 0 ? 0 : 1);

			file.seek(recordLenStart);
			file.read(intBytes);
			_recordLength = _endian == 0 ? LogUtils
					.getUnSignedIntFromBytesBE(intBytes, 0) : LogUtils
					.getUnSignedIntFromBytesLE(intBytes, 0);

			_streamMap = parseStreamMapping(file);
			_componentMap = parseComponentMapping(file);

			file.close();

		} catch (FileNotFoundException e) {

		} catch (IOException e) {

		}
	}

	/**
	 * Parses the component mapping from the configuration file.
	 * 
	 * @param fileName
	 *            the name of configuration file
	 * @return the component mapping
	 */
	public HashMap<String, String> parseComponentMapping(RandomAccessFile file) {

		HashMap<String, String> componentMap = new HashMap<String, String>();

		try {
			byte[] shortBytes = new byte[2];
			byte[] intBytes = new byte[4];

			file.seek(componentHeadStart);
			file.read(shortBytes);
			int nextEntry = _endian == 0 ? LogUtils
					.getUnSignedShortFromBytesBE(shortBytes, 0) : LogUtils
					.getUnSignedShortFromBytesLE(shortBytes, 0);

			int componentNameLen;
			long componentId;
			String componentName;

			while (nextEntry != endMarker) {

				file.seek(nextEntry);
				file.read(shortBytes);
				nextEntry = _endian == 0 ? LogUtils
						.getUnSignedShortFromBytesBE(shortBytes, 0) : LogUtils
						.getUnSignedShortFromBytesLE(shortBytes, 0);

				file.read(shortBytes);
				componentNameLen = _endian == 0 ? LogUtils
						.getUnSignedShortFromBytesBE(shortBytes, 0) : LogUtils
						.getUnSignedShortFromBytesLE(shortBytes, 0);

				byte[] readBuf = new byte[componentNameLen];
				file.read(readBuf);
				componentName = new String(readBuf);

				file.read(intBytes);
				componentId = _endian == 0 ? LogUtils
						.getUnSignedIntFromBytesBE(intBytes, 0) : LogUtils
						.getUnSignedIntFromBytesLE(intBytes, 0);

				componentMap.put(String.valueOf(componentId), componentName);
			}

			return componentMap;

		} catch (IOException e) {
			return null;
		}
	}

	/**
	 * Parses the stream mapping from the configuration file.
	 * 
	 * @param fileName
	 *            the name of configuration file
	 * @return the stream mapping
	 */
	public HashMap<String, String> parseStreamMapping(RandomAccessFile file) {

		HashMap<String, String> streamMap = new HashMap<String, String>();

		try {
			byte[] shortBytes = new byte[2];

			file.seek(streamHeadStart);
			file.read(shortBytes);
			int nextEntry = _endian == 0 ? LogUtils
					.getUnSignedShortFromBytesBE(shortBytes, 0) : LogUtils
					.getUnSignedShortFromBytesLE(shortBytes, 0);

			int streamId, streamNameLen;
			String streamName;

			while (nextEntry != endMarker) {

				file.seek(nextEntry);
				file.read(shortBytes);
				nextEntry = _endian == 0 ? LogUtils
						.getUnSignedShortFromBytesBE(shortBytes, 0) : LogUtils
						.getUnSignedShortFromBytesLE(shortBytes, 0);

				file.read(shortBytes);
				streamNameLen = _endian == 0 ? LogUtils.getUnSignedShortFromBytesBE(
						shortBytes, 0) : LogUtils.getUnSignedShortFromBytesLE(
						shortBytes, 0);

				byte[] readBuf = new byte[streamNameLen];
				file.read(readBuf);
				streamName = new String(readBuf);

				file.read(shortBytes);
				streamId = _endian == 0 ? LogUtils
						.getUnSignedShortFromBytesBE(shortBytes, 0) : LogUtils
						.getUnSignedShortFromBytesLE(shortBytes, 0);

				streamMap.put(String.valueOf(streamId), streamName);
			}

			return streamMap;

		} catch (IOException e) {
			return null;
		}
	}

	/**
	 * Returns the component map.
	 * 
	 * @return the component map
	 */
	public HashMap<String, String> getComponentMap() {
		return _componentMap;
	}

	/**
	 * Returns the record length.
	 * 
	 * @return the record length
	 */
	public int getRecordLength() {
		return (int) _recordLength;
	}

	/**
	 * returns the stream map.
	 * 
	 * @return the stream map
	 */
	public HashMap<String, String> getStreamMap() {
		return _streamMap;
	}
}

/**
 * 
 */
package com.clovis.logtool.stream;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.BufferUnderflowException;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.channels.FileChannel.MapMode;

/**
 * Concrete stream based on the file stored in the corresponding file system.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class FileStream extends Stream {

	/**
	 * Memory mapped buffer for the faster access of the data from the file.
	 */
	private MappedByteBuffer _stream;

	/**
	 * Constructs the file stream with the given stream details.
	 * 
	 * @param stream
	 *            the stream details
	 */
	public FileStream(String streamDetail) {
		super(streamDetail);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.logtool.stream.Stream#read(byte[])
	 */
	public boolean read(byte[] bytes) {
		try {
			_stream.get(bytes);
			return true;
		} catch (BufferUnderflowException e) {
		}
		return false;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.logtool.stream.Stream#read(int, byte[])
	 */
	public boolean read(int index, byte[] bytes) {
		try {
			setPosition(index);
			_stream.get(bytes);
			return true;
		} catch (BufferUnderflowException e) {
		} catch (IllegalArgumentException e) {
		}
		return false;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.logtool.stream.Stream#open()
	 */
	public boolean open(int fileHeaderLength) {
		try {
			FileChannel fc = new FileInputStream(new File(_streamDetail))
					.getChannel();
			_stream = fc.map(MapMode.READ_ONLY, fileHeaderLength, fc.size()
					- fileHeaderLength);
			_size = _stream.capacity();
			fc.close();
			return true;
		} catch (FileNotFoundException e) {
                        System.err.println("Error: Could not create open file " + e.getMessage());
                        System.exit(-1);
		} catch (IOException e) {
                        System.err.println("Error: Could not create open file " + e.getMessage());
                        System.exit(-1);
		}
		return false;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.logtool.stream.Stream#close()
	 */
	public boolean close() {
		_stream = null;
		return true;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.logtool.stream.Stream#setPosition(int)
	 */
	public boolean setPosition(int position) {
		try {
			_stream.position(position);
		} catch (IllegalArgumentException e) {
			return false;
		}
		return true;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.logtool.stream.Stream#setRelativePosition(int)
	 */
	public boolean setRelativePosition(int position) {
		try {
			_stream.position(_stream.position() + position);
		} catch (IllegalArgumentException e) {
			return false;
		}
		return true;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.logtool.stream.Stream#nextStream()
	 */
	public Stream nextStream() {
		if (_srp != null) {
			String nextStreamDetail = _srp.getNextStream();
			Stream stream = new FileStream(nextStreamDetail);
			return stream;
		}
		return null;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.logtool.stream.Stream#getPosition()
	 */
	@Override
	public int getPosition() {
		return _stream.position();
	}
}

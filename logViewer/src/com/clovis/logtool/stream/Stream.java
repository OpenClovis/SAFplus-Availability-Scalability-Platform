/**
 * 
 */
package com.clovis.logtool.stream;

/**
 * Abstract implementation for the stream which is the source for data.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public abstract class Stream {

	/**
	 * Details of the stream in the string format.
	 */
	protected String _streamDetail;

	/**
	 * Size of this Stream.
	 */
	protected int _size;

	/**
	 * Stream roll policy to get the next stream.
	 */
	protected StreamRollPolicy _srp;

	/**
	 * Constructs the stream. It initializes the stream detail and stream roll
	 * policy for the stream.
	 * 
	 * @param streamDetail
	 *            the stream details
	 */
	public Stream(String streamDetail) {
		_streamDetail = streamDetail;
	}

	/**
	 * Opens the stream. It should take care for creating the stream as well as
	 * allocating the other resources like creating the buffer for faster access
	 * of data from the source.
	 * 
	 * @param startOffset
	 *            the starting position for actual data.
	 * @return true if the operation is successful, and false otherwise
	 */
	public abstract boolean open(int startOffset);

	/**
	 * De-allocates all the resources like buffer associated with the stream. It
	 * assumes that the stream resources are not now required means stream is to
	 * be closed.
	 * 
	 * @return true if the operation is successful, and false otherwise
	 */
	public abstract boolean close();

	/**
	 * Reads the number of bytes equal to the length of the bytes array from the
	 * stream starting at the current position for the stream.
	 * 
	 * @param bytes
	 *            the array into which the data is to be read
	 * @return true if the operation is successful, and false otherwise
	 */
	public abstract boolean read(byte[] bytes);

	/**
	 * Reads the number of bytes equal to the length of the bytes array from the
	 * stream starting at the index position for the stream.
	 * 
	 * @param index
	 *            the start position for the reading
	 * @param bytes
	 *            the array into which the data is to be read
	 * @return true if the operation is successful, and false otherwise
	 */
	public abstract boolean read(int index, byte[] bytes);

	/**
	 * Allows to set the current position for the stream.
	 * 
	 * @param position
	 *            the position to be set
	 * @return true if the operation is successful, and false otherwise
	 */
	public abstract boolean setPosition(int position);

	/**
	 * Allows to set the position for the stream relative to current position.
	 * 
	 * @param position
	 *            the relative position to be set
	 * @return true if the operation is successful, and false otherwise
	 */
	public abstract boolean setRelativePosition(int position);

	/**
	 * Allows to get the current position for the stream.
	 * 
	 * @return Position
	 */
	public abstract int getPosition();

	/**
	 * Returns the next stream.
	 * 
	 * @return the next stream
	 */
	public abstract Stream nextStream();

	/**
	 * Sets the Stream Roll Policy for this stream.
	 * 
	 * @param srp
	 *            the Stream Roll Policy
	 */
	public void setStreamRollPolicy(StreamRollPolicy srp) {
		_srp = srp;
	}

	/**
	 * Returns the size of the Stream
	 * 
	 * @return the size
	 */
	public int getSize() {
		return _size;
	}
}

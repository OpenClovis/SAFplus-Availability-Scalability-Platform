/**
 * 
 */
package com.clovis.logtool.stream;

/**
 * Defines the interface for the roll policy for the stream.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public interface StreamRollPolicy {

	/**
	 * Returns the details of the next stream for the roll policy.
	 * 
	 * @return the details of the next stream
	 */
	String getNextStream();
}

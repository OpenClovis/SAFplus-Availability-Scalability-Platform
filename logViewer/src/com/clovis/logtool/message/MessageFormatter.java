/**
 * 
 */
package com.clovis.logtool.message;

import com.clovis.logtool.record.Record;

/**
 * Formats the message for the given record.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public interface MessageFormatter {

	/**
	 * Formats the message for the given record in a human readable form.
	 * 
	 * @param record
	 *            for which message is to be formatted
	 * @return the formatted message
	 */
	public abstract String format(Record record);
}

package com.clovis.logtool.message;

import java.util.Arrays;
import java.util.List;

import com.clovis.logtool.record.LogRecordHeader;
import com.clovis.logtool.record.Record;
import com.clovis.logtool.record.TLV;
import com.clovis.logtool.utils.LogConstants;
import com.clovis.logtool.utils.LogUtils;

/**
 * Formats the log message for the given log record.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class LogMessageFormatter implements MessageFormatter {

	/**
	 * Formats the log message for the given log record by combining it with the
	 * message format and data part for TLV type record. For ASCII and binary it
	 * just gives the String equivalent of the same.
	 * 
	 * @param record
	 *            for which message is to be formatted
	 * @return the log message
	 */
	@SuppressWarnings("unchecked")
	public String format(Record record) {
		String message = null;
		long messageId = ((LogRecordHeader) record.getHeader()).getMessageId();

		if(messageId == LogConstants.MESSAGEID_ASCII) {
			message = record.getData().toString();
		} else if(messageId == LogConstants.MESSAGEID_BINARY) {
			message = Arrays.toString((byte[]) record.getData());
		} else{
			try {
				String messageFormat = LogUtils.getMessageIdMapping(false)
						.getProperty(Long.toString(messageId));
				StringBuffer messageBuf = messageFormat != null
					? new StringBuffer(messageFormat) : new StringBuffer(
						"Message format for this message id is not configured.");

				List<TLV> TLVList = ((List<TLV>) record.getData());
				for (int i = 0, x; i < TLVList.size(); i++) {

					if (messageFormat != null) {
						x = messageBuf.indexOf("%TLV");

						if (x == -1) {
							messageBuf.replace(0, messageBuf.length(),
								"Unable to parse the message with TLVs "
									+ "because of descrepancy between Message"
									+ "format and data.");
							break;
						}

						messageBuf.replace(x, x + 4, TLVList.get(i).getValue()
								.toString());
					} else {
						messageBuf.append("   TLV - " + (i + 1) + " : "
								+ TLVList.get(i).getValue().toString());
					}
				}

				message = messageBuf.toString();
			} catch (Exception e) {
				message = "Unable to parse the message with TLVs.";
			}
		}
		return message;
	}
}

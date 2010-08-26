/**
 * 
 */
package com.clovis.logtool.record;

import java.util.HashMap;

import com.clovis.logtool.ui.LogDisplay;
import com.clovis.logtool.ui.RecordPanel;
import com.clovis.logtool.utils.LogConstants;
import com.clovis.logtool.utils.LogUtils;

/**
 * Concrete implementation of the Record class to create LogRecord.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class LogRecord extends Record {

	/**
	 * Constructs Log Record with the given header and data.
	 * 
	 * @param header
	 *            the header for the record
	 * @param data
	 *            the data for the record
	 */
	public LogRecord(LogRecordHeader header, Object data) {
		super(header, data);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.logtool.record.Record#getField(int)
	 */
	public Object getField(int fieldIndex, boolean UIFlag) {
		Object obj = null;

		switch (fieldIndex) {

		case LogConstants.FIELD_INDEX_RECORDNUMBER:
			obj = new Integer(((LogRecordHeader)_header).getRecordNumber());
			break;

		case LogConstants.FIELD_INDEX_FLAG:
			obj = new Short(((LogRecordHeader)_header).getFlag());
			break;

		case LogConstants.FIELD_INDEX_SEVERITY:

			if(UIFlag) {
				long severity = ((LogRecordHeader) _header).getSeverity();

				if(severity > LogUtils.severityString.length) {
					obj = new Long(severity);
				} else {
					obj = LogUtils.severityString[(int)severity];
				}

			} else {
				obj = new Long(((LogRecordHeader) _header).getSeverity());
			}
			break;

		case LogConstants.FIELD_INDEX_STREAMID:

			if(UIFlag) {
				RecordPanel recordPanel = (RecordPanel) LogDisplay
						.getInstance().getRecordPanelFolder().getSelection()
						.getControl();

				String id = String.valueOf(((LogRecordHeader)_header).getStreamId());
				HashMap<String, String> map = recordPanel.getRecordManager().getFieldMapping(fieldIndex);

				if(map != null && map.containsKey(id)) {
					String name = map.get(id);
					obj = id + ":" + name;
				} else {
					obj = id;
				}

			} else {
				obj = new Integer(((LogRecordHeader)_header).getStreamId());
			}
			break;

		case LogConstants.FIELD_INDEX_COMPONENTID:

			if(UIFlag) {
				RecordPanel recordPanel = (RecordPanel) LogDisplay
						.getInstance().getRecordPanelFolder().getSelection()
						.getControl();

				String id = String.valueOf(((LogRecordHeader)_header).getComponentId());
				HashMap<String, String> map = recordPanel.getRecordManager().getFieldMapping(fieldIndex);

				if(map != null && map.containsKey(id)) {
					String name = map.get(id);
					obj = id + ":" + name;
				} else {
					obj = id;
				}

			} else {
				obj = new Long(((LogRecordHeader)_header).getComponentId());
			}
			break;

		case LogConstants.FIELD_INDEX_SERVICEID:
			obj = new Integer(((LogRecordHeader)_header).getServiceId());
			break;

		case LogConstants.FIELD_INDEX_TIMESTAMP:
			obj = new Long(((LogRecordHeader)_header).getTimeStamp());
			break;

		case LogConstants.FIELD_INDEX_MESSAGEID:
			obj = new Integer(((LogRecordHeader)_header).getMessageId());
			break;
		}

		return obj;
	}
}

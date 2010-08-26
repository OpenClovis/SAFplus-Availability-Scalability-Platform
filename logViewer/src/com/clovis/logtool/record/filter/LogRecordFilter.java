package com.clovis.logtool.record.filter;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.clovis.logtool.record.LogRecord;
import com.clovis.logtool.record.LogRecordHeader;
import com.clovis.logtool.record.Record;
import com.clovis.logtool.ui.LogDisplay;
/**
 * Filters the record(s) from log based on the given criteria.
 * 
 * @author ravi
 * 
 */

public class LogRecordFilter extends RecordFilter {

	/**
	 * Used for serialization.
	 */
	private static final long serialVersionUID = 1L;

	/**
	 * Applies filtering criteria on a record.
	 * 
	 * @param record
	 *            the parsed record   
	 * @param andFlag   
	 * 		andFlag is true for 'and' operation and false for 'or'
	 * 		operation on filterObjList 
	 * @return 
	 * 		true if match is succesfull otherwise false
	 */

	public boolean applyFilter(Record record) {
		
		if(_filterObjectList == null || _filterObjectList.size() == 0) {
			return true;
		}

		boolean result = false;
		//Check for each matching criteria
		for (int i = 0; _filterObjectList!=null && i < _filterObjectList.size(); i++) {

			FilterObject filter = (FilterObject) _filterObjectList.get(i);
			LogRecordHeader header = (LogRecordHeader) record.getHeader();
			int fieldIndex = filter.getFieldIndex();
			//int fieldType = filter.getFieldType();
			String filterCriteria = filter.getFilterCriteria();
			boolean negateFlag = filter.isNegateFlag();
			
			short shortValue = 0;
			int intValue = 0;
			long longValue = 0;
			String inputStr = "";
			switch (fieldIndex) {

			case 0:
				longValue = header.getRecordNumber();
				result = rangeFilter(longValue, filterCriteria);
				break;
			case 1:
				shortValue = header.getFlag();
				result = rangeFilter(shortValue, filterCriteria);
				break;
			case 2:
				longValue = header.getSeverity();
				result = rangeFilter(longValue, filterCriteria);
				break;
			case 3:
				intValue = header.getStreamId();
				result = rangeFilter(intValue, filterCriteria);
				break;
			case 4:
				longValue = header.getComponentId();
				result = rangeFilter(longValue, filterCriteria);
				break;
			case 5:
				intValue = header.getServiceId();
				result = rangeFilter(intValue, filterCriteria);
				break;
			case 6:
				intValue = header.getMessageId();
				result = rangeFilter(intValue, filterCriteria);
				break;
			case 7:
				longValue = header.getTimeStamp();
				result = rangeFilter((longValue / 1000000000) * 1000000000, filterCriteria);
				break;
			default:
				inputStr = LogDisplay.getInstance().getMessageFormatter()
						.format((LogRecord) record);
				result = regExpFilter(inputStr, filterCriteria);
				if(negateFlag)
					result = !result;
				break;
			}
			// andFlag = true for 'and' operation
			if(_andFlag && !result) // if any of the matching criteria fails then return 				
				return false;
			//andFlag = false for 'or' operation
			if(!_andFlag && result)// if any of the matching criteria succeeds then return
				return true;
		}
		//If none of the match is successful for 'or' operation
		if(!_andFlag)
			return false;
		//If all the match are successful for 'and' operation
		return true;
	}

	/**
	 * Filters the record based on a given range for a header field.
	 * @param fieldValue
	 * 			the value to be searched within range 
	 * @param range
	 * 			range to make the search 
	 * @return
	 * 		true when the value lies within range
	 */
	public boolean rangeFilter(long fieldValue, String range) {

		int index = range.indexOf(':');
		if(-1 == index) {
			if (fieldValue == Long.parseLong(range))
				return true;
			else
				return false;
		}
		
		String startFieldValue = range.substring(0, index);
		String endFieldValue = range.substring(index + 1, range.length());
		
		if (index == 0){
			long end = Long.parseLong(endFieldValue);
			if (fieldValue <= end)
				return true;
		}else if (index == range.length()-1){
				long start = Long.parseLong(startFieldValue);
				if (fieldValue >= start)
					return true;
		}else{
				long start = Long.parseLong(startFieldValue);
				long end = Long.parseLong(endFieldValue);
				if (fieldValue >= start && fieldValue <= end)
					return true;
		}	
		return false;
	}
	
	public boolean rangeFilter(int fieldValue, String range) {

		int index = range.indexOf(':');
		if(-1 == index) {
			if (fieldValue == Integer.parseInt(range))
				return true;
			else
				return false;
		}
		
		String startFieldValue = range.substring(0, index);
		String endFieldValue = range.substring(index + 1, range.length());
		
		if (index == 0){
			int end = Integer.parseInt(endFieldValue);
			if (fieldValue <= end)
				return true;
		}else if (index == range.length()-1){
				int start = Integer.parseInt(startFieldValue);
				if (fieldValue >= start)
					return true;
		}else{
				int start = Integer.parseInt(startFieldValue);
				int end = Integer.parseInt(endFieldValue);
				if (fieldValue >= start && fieldValue <= end)
					return true;
		}	
		return false;
	}

	public boolean rangeFilter(short fieldValue, String range) {

		int index = range.indexOf(':');
		if(-1 == index) {
			if (fieldValue == Short.parseShort(range))
				return true;
			else
				return false;
		}
		
		String startFieldValue = range.substring(0, index);
		String endFieldValue = range.substring(index + 1, range.length());
		
		if (index == 0){
			short end = Short.parseShort(endFieldValue);
			if (fieldValue <= end)
				return true;
		}else if (index == range.length()-1){
				short start = Short.parseShort(startFieldValue);
				if (fieldValue >= start)
					return true;
		}else{
				short start = Short.parseShort(startFieldValue);
				short end = Short.parseShort(endFieldValue);
				if (fieldValue >= start && fieldValue <= end)
					return true;
		}	
		return false;
	}
	/**
	 * Filters the record based on given regular expression
	 * for the message part of the record.
	 * @param inputStr
	 * 			the input string 	
	 * @param regExp
	 * 			the regular expression for input string
	 * @return
	 * 		true when match is successfull
	 */
	public boolean regExpFilter(String inputStr, String regExp){
		
		Pattern pattern = Pattern.compile(regExp, Pattern.CASE_INSENSITIVE);
		Matcher matcher = pattern.matcher(inputStr);
        return matcher.find();
        	
	}
}

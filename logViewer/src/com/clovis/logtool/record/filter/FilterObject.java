package com.clovis.logtool.record.filter;


/**
 * Defines the fields of a Filtering object
 * 
 * @author ravi
 * 
 */
public class FilterObject {

	/**
	 * Type of the FilterCriteria [range or regular expression].
	 */
	private int fieldType;

	/**
	 * The field index of the header to match this criteria.
	 */
	private int fieldIndex;

	/**
	 * Filtering criteria for the record. It can be range of values or a regular
	 * expression.
	 */
	private String filterCriteria;

	/**
	 * True when negation is applied on filterCriteria.
	 */
	private boolean negateFlag;

	/**
	 * Default Constructor.
	 */
	public FilterObject() {
	}

	/**
	 * Creates the instance of Filter Object.
	 */
	public FilterObject(int fieldType, int fieldIndex, String filterCriteria,
			boolean negateFlag) {

		this.fieldType = fieldType;
		this.fieldIndex = fieldIndex;
		this.filterCriteria = filterCriteria;
		this.negateFlag = negateFlag;
	}

	public int getFieldType() {
		return fieldType;
	}

	public int getFieldIndex() {
		return fieldIndex;
	}

	public String getFilterCriteria() {
		return filterCriteria;
	}

	public boolean isNegateFlag() {
		return negateFlag;
	}

	public void setFieldIndex(int fieldIndex) {
		this.fieldIndex = fieldIndex;
	}

	public void setFieldType(int fieldType) {
		this.fieldType = fieldType;
	}

	public void setFilterCriteria(String filterCriteria) {
		this.filterCriteria = filterCriteria;
	}

	public void setNegateFlag(boolean negateFlag) {
		this.negateFlag = negateFlag;
	}
}

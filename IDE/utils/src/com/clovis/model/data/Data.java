package com.clovis.model.data;

import  java.util.Vector;
/**
 * Actual place holder of cell values.
 * @author <a href="nadeem@clovissolutions.com">Nadeem</a>
 */
public interface Data
{
    /**
     * Returns the value stored in the cell
     * @return the value of the cell
     */
    String getValue();
    /**
     * Sets the cell value.
     * @param text is new value for the cell.
     */
    void setValue(String val);
    /**
     * Returns the base value
     * @return base value.
     */
    String getBaseValue();
    /**
     * Sets the Base cell value.
     * @param text is previous value for the cell.
     */
    void setBaseValue(String val);
    /**
     * returns the enumeration of the cell
     * @return the enum of values for a column 
     */
    Vector getChoices();
    /**
     * sets the enumeration for the cell
     * @param enu is the enumeration.
     */
    void setChoices(Vector enu);
    /**
     * Returns the cell status Enable or disable.
     * @return status of cell
     */
    boolean isEnabled();
    /**
     * Sets the cell status Enable or disable.
     * @param isEnabled is new status for the cell.
     */
    void setEnabled(boolean enable);
}

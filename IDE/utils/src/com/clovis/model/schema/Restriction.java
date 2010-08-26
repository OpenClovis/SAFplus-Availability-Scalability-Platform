package com.clovis.model.schema;

import  java.util.Vector;
/**
 * This clas represents the enumeration.
 * 
 * @author <a href=mailto:nadeem@clovissolutions.com>Nadeem</a>
 */
public class Restriction
{
    /**
     * The enumeration from XSD.
     */
    private Vector _values = new Vector();
    /**
     * Gives the list of Enumeration values
     * 
     * @return Vector of Enumeration values
     */
    public Vector getValues()
    {
        return _values;
    }
    /**
     * Set the new set of Enumeration values
     *
     * @param enumVector contains possible enum values.
     */
    
    public void setValues(Vector enumVector)
    {
        _values = enumVector;                        
    }
    /**
     * Gives a String representation of a this object
     * 
     * @return String representation of a Restriction object
     */
    public String toString() { return " Restriction: " + _values.toString(); }
}

package com.clovis.model.data;

import  java.util.Vector;

import  com.clovis.model.schema.Restriction;
import  com.clovis.model.schema.SimpleElement;
/**
 * Actual place holder of cell values.
 * @author <a href="nadeem@clovissolutions.com">Nadeem</a>
 */
public class DataImpl implements Data
{
    private Row             _row;
    private SimpleElement   _column;
    private boolean         _enable;
    private Vector          _choices;
    private String          _value;
    private String          _baseValue;
    /**
     * Constructor.
     */
    public DataImpl(Row row, SimpleElement column, String value)
    {
        _row       = row;
        _column    = column;
        _value     = value;
        _baseValue = value;
        Restriction rest = column.getRestriction();
        _choices   = (rest != null) ? rest.getValues() : null;
    }
    /**
     * Returns the value stored in the cell
     * @return the value of the cell
     */
    public String getValue()                  { return _value;        }
    /**
     * Sets the cell value.
     * @param text is new value for the cell.
     */
    public void setValue(String val)          { _value = val;         }
    /**
     * Returns the base value
     * @return base value.
     */
    public String getBaseValue()            { return _baseValue;    }
    /**
     * Sets the Base cell value.
     * @param val is previous value for the cell.
     */
    public void setBaseValue(String val)    { _baseValue = val;     }
    /**
     * returns the choices of the cell
     * @return the choices of values for a column 
     */
    public Vector getChoices()              { return _choices;      }
    /**
     * sets the choices for the cell
     * @param choices is the possible choices.
     */
    public void setChoices(Vector choices)
    {
        _choices = choices;
        if (_choices != null && _choices.size() > 0) {
            setValue((String) _choices.get(0));
        }
    }
    /**
     * String representation.
     * @return String representation.
     */
    public String toString()
    {
        StringBuffer buff = new StringBuffer(_column.getName());
        buff.append(':')
            .append(_value).append(", ")
            .append(_baseValue).append(", ")
            .append(_choices).append(", ").append(_enable);
        return buff.toString();
    }
    /**
     * Returns the cell status Enable or disable.
     * @return status of cell
     */
    public boolean isEnabled()              { return _enable;       }
    /**
     * Sets the cell status Enable or disable.
     * @param isEnabled is new status for the cell.
     */
    public void setEnabled(boolean enable)  { _enable = enable;     }
}

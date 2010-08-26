package com.clovis.model.event;

import  com.clovis.model.data.Row;
import  com.clovis.model.data.Model;
/**
 * This class represent changes in Cell.
 *
 * @author <a href="mailto:nadeem@clovissolutions.com">Nadeem</a>
 */
public class CellModelEvent
    extends ModelEvent
{
    public static final int VALUE_CHANGED       = 7;
    
    /** Column Identifier. */
    private Object _columnid;

    /** New Value for the cell */
    private String _newVal;

    /** Old value for the cell. */
    private String _oldVal;

    /** Row identifier. */
    private Object _rowid;

    /**
     * Constructs a CellModelEvent. This notifies that complete table has
     * changed.
     * 
     * @param model is the view data model.
     * @param rowid is the record identifier
     * @param columnid is the column identifier
     * @param oldVal is the old value for the cell.
     * @param newVal is the new value for the cell.
     * @param action value changed action
     */
    public CellModelEvent(Model model, 
                          Object rowid, 
                          Object columnid, 
                          String oldVal, 
                          String newVal, 
                          int action)
    {
        super(model, action);
        _rowid    = rowid;
        _columnid = columnid;
        _oldVal   = oldVal;
        _newVal   = newVal;
        _type     = CELL_MODEL_EVENT;
    }
    /**
     * Returns the field Name.
     * 
     * @return column name.
     */
    public Object getColumnId()
    {
        return _columnid;
    }
    /**
     * Returns the new Value for the cell.
     * 
     * @return new value.
     */
    public String getNewValue()
    {
        return _newVal;
    }
    /**
     * Returns the Old Value for the cell.
     * 
     * @return old value.
     */
    public String getOldValue()
    {
        return _oldVal;
    }
    /**
     * Returns the key in the Event
     * 
     * @return row identifier.
     */
    public Object getRowId()
    {
        return _rowid;
    }
    /**
     * returns the String representation of CellModelEvent.
     * 
     * @return string representation of event.
     */
    public String toString()
    {
        StringBuffer buf = new StringBuffer("CellModelEvent: Action : ");
        buf.append(_action)
           .append("\nRow   : ").append(_rowid)
           .append("\nColumn: ").append(_columnid)
           .append("\nChange: ").append(_newVal).append ("<>").append(_oldVal); 
        return buf.toString();
    }
}

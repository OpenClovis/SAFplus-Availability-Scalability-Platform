package com.clovis.model.event;

import  java.util.Vector;

import  com.clovis.model.data.Row;
import  com.clovis.model.data.Model;
/**
 * The <code>RowModelEvent</code> describes changes happened to a set of rows.
 * Like a set of row added or deleted. For one row operation simple
 * ModelEvent will serve the purpose.
 * 
 * @author <a href="mailto:nadeem@clovissolutions.com">Nadeem</a>
 */
public class RowModelEvent
    extends ModelEvent
{
    public static final int ROW_ADDED       = 11;
    public static final int ROW_DELETED     = 12;

    /** List of Rows which got inserted/deleted */
    protected Vector _rows;
    /** List of all row identifiers which has undergone some changes */
    protected Vector _rowids;
    /**
     * Constructs a RowModelEvent. This notifies that complete table has
     * changed.
     * 
     * @param model is the view data model.
     * @param rows can be either rowids/row itself.
     * @param action is the action assiciated with event e.g, row deleted,
     *        inserted, updated etc.
     */
    public RowModelEvent(Model model, Row[] rows, int action)
    {
        super(model, action);
        if (rows != null && rows.length > 0) {
            _rows   = new Vector();
            _rowids = new Vector();
            for (int i = 0; i < rows.length; i++) {
                _rows.add(rows[i]);
                _rowids.add(rows[i].getRowId());
            }
        }
        _type = ROW_MODEL_EVENT;
    }
    /**
     * Creates a RowModelEvent for a single Row
     * @param model is the dataModel
     * @param row is the Row changed.
     * @param action is the action associated with the event.
     */
    public RowModelEvent(Model model, Row row, int action)
    {
        this(model, new Row[] {row}, action);
    }
    /**
     * Returns all rows.
     * 
     * @return list of all row which got deleted/
     */
    public Vector getRows()
    {
        return _rows;
    }
    /**
     * Returns row identifiers
     * 
     * @return list of all row identifiers which has undergone changes.
     */
    public Vector getRowIds()
    {
        return _rowids;
    }
    /**
     * returns the String representation of RowModelEvent.
     * 
     * @return string representation of event.
     */
    public String toString()
    {
        StringBuffer buf = new StringBuffer();
        buf.append("RowModelEvent : Action : " + _action);
        for (int i = 0; _rows != null && i < _rows.size(); i++) {
            buf.append("\n\t").append(i).append(" Row:").append(_rows.get(i));
        }
        return buf.toString();
    }
}

package com.clovis.model.data;

import  java.util.Vector;
import  java.util.LinkedHashMap;

import  com.clovis.model.common.ModelContext;

import  com.clovis.model.event.ModelEvent;
import  com.clovis.model.event.EventHandler;
import  com.clovis.model.event.RowModelEvent;
import  com.clovis.model.event.ModelListener;
import  com.clovis.model.schema.ComplexElement;
/**
 * The <code>Model</code> interface defines Model.
 * @author <a href="nadeem@clovissolutions.com">Nadeem</a>
 */
public class ModelImpl implements Model
{
    protected ComplexElement    _table;
    protected ColumnModel       _columnModel;
    protected LinkedHashMap     _rowSet        = new LinkedHashMap();
    protected ModelContext      _modelContext;
    protected EventHandler      _eventHandler;
    /**
     * Constructor.
     * @param table ComplexElement
     * @param mc    ModelContext
     */
    public ModelImpl(ComplexElement table, ModelContext mc)
    {
        _table         = table;
        _modelContext  = mc;
        _eventHandler  = _modelContext.getEventHandler();
        _columnModel = ColumnManager.getColumnModel(_table);

        //Fire ModelCreated Event.
        fireEvent(new ModelEvent(this, ModelEvent.MODEL_CREATED));
    }
    /**
     * Returns the Column Model for the Model.
     * @return column model for the model.
     */
    public ColumnModel getColumnModel() { return _columnModel;              }
    /**
     * Returns the Row object for the given row id
     * @param rowid is the row identifier.
     * @return row
     */
    public Row getRow(String rowId)     { return (Row) _rowSet.get(rowId);  }
    /**
     * Returns the number of Rows in the Model.
     * @return row count.
     */
    public int getRowCount()            { return _rowSet.size();            }
    /**
     * Returns the ComplexElement Organization for the model
     * @return table name.
     */
    public ComplexElement getTable()    { return _table;                    }
    /**
     * Returns all the row identifier stored by the Model.
     * 
     * @return set of all identifiers in the model.
     */
    public String[] getRowIds()
    {
        return (String[]) _rowSet.keySet().toArray(new String[0]);
    }
    /**
     * Return all rows in the model
     * @return Array of all rows
     */ 
    public Row[] getRows()
    {
        return (Row[]) _rowSet.values().toArray(new Row[0]);
    }
    /**
     * Add Model Listener to EventHandler.
     * @param listener Model Listener
     */
    public void addModelListener(ModelListener listener)
    {
        _eventHandler.addModelListener(listener);
    }
    /**
     * Remove Model Listener from EventHandler.
     * @param listener Model Listener
     */
    public void removeModelListener(ModelListener listener)
    {
        _eventHandler.removeModelListener(listener);
    }
    /**
     * Retuns the value stored at the cell identified by the rowidentifier.
     * @param rowid is the unique row identifier
     * @param columnid is the unique columnidentifier.
     * @return the value stored in the model[rowid][columnid];
     */
    public String getValue(String rowid, String columnid)
    {
        Row row = getRow(rowid);
        return (row != null) ? row.getValue(columnid) : null;
    }
    /**
     * Sets the value at model[rowid][columnid];
     * @param  rowid is the unique row identifier
     * @param  colid is the unique columnidentifier.
     * @param  data value stored in the model[rowid][columnid];
     * @return Value
     * @throws Exception Error
     */
    public String setValue(String rowid, String colId, String data) 
        throws Exception
    {
        Row row = getRow(rowid);
        return (row != null) ? row.setValue(colId, data, true) : null;
    }
    /**
     * Inserts Rows at the end of the model.
     * 
     * @param rows contains rows to be added.
     */
    public void addRow(Row[] rows)
    {
        Vector addedRows = new Vector();
        for (int i = 0; i < rows.length; i++) {
            Row row = (Row) _rowSet.put(rows[i].getRowId(), rows[i]);
            if (row == null) {
                addedRows.add(rows[i]);
            }
        }
        // Fire Event.
        Row[] rowsArr = (Row[]) addedRows.toArray(new Row[0]);
        fireEvent(new RowModelEvent(this, rowsArr, RowModelEvent.ROW_ADDED));
    }
    /**
     * Deletes all rows identified by there ids
     * 
     * @param rowids is the list unique identifier for the rows
     * @return removed row 
     */
    public void removeRow(String[] rowids)
    {
        Vector deletedRows = new Vector();
        for (int i = 0; i < rowids.length; i++) {
            Row row = (Row) _rowSet.remove(rowids[i]);
            if (row != null) {
                deletedRows.add(row);
            }
        }
        // Fire Event.
        fireEvent(new RowModelEvent(this,
                                    (Row[]) deletedRows.toArray(new Row[0]), 
                                    RowModelEvent.ROW_DELETED));
    }
    /**
     * Returns the ModelCreated for the Model.
     * @return ModelContext for the model.
     */
    public ModelContext getModelContext()   { return _modelContext;            }
    /**
     * Fire Change Events.
     * @param event ModelEvent
     */
    public void fireEvent(ModelEvent event) { _eventHandler.fireEvent(event);  }
}

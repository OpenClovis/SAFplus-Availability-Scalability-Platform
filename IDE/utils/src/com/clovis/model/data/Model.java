package com.clovis.model.data;

import  com.clovis.model.event.ModelEvent;
import  com.clovis.model.event.ModelListener;
import  com.clovis.model.common.ModelContext;
import  com.clovis.model.schema.ComplexElement;
/**
 * The <code>Model</code> interface defines Model.
 * @author <a href="nadeem@clovissolutions.com">Nadeem</a>
 */
public interface Model
{
    /**
     * Returns the Column Model for the Model.
     * @return column model for the model.
     */
    ColumnModel getColumnModel();
    /**
     * Returns the Row object for the given row id
     * @param rowid is the row identifier.
     * @return row
     */
    Row getRow(String rowid);
    /**
     * Returns the number of Rows in the Model.
     * @return row count.
     */
    int getRowCount();
    /**
     * Returns the ComplexElement Organization for the model
     * @return table name.
     */
    ComplexElement getTable();
    /**
     * Inserts Rows at the end of the model.
     * 
     * @param rows contains rows to be added.
     */
    void addRow(Row[] rows);
    /**
     * Deletes all rows identified by there ids
     * 
     * @param rowids is the list unique identifier for the rows
     */
    void removeRow(String[] rowids);
    /**
     * Returns all the row identifier stored by the Model.
     * 
     * @return set of all identifiers in the model.
     */
    String[] getRowIds();
    /**
     * Return all rows in the model
     * @return Array of all rows
     */ 
    Row[] getRows();
    /**
     * Add Model Listener.
     */
    void addModelListener(ModelListener listener);
    /**
     * Remove Model Listener.
     */
    void removeModelListener(ModelListener listener);
    /**
     * Retuns the value stored at the cell identified by the rowidentifier.
     * @param rowid is the unique row identifier
     * @param columnid is the unique columnidentifier.
     * @return the value stored in the model[rowid][columnid];
     */
    String getValue(String rowid, String columnid);
    /**
     * Fire Change Events.
     * @param event ModelEvent
     */
    void fireEvent(ModelEvent event);
    /**
     * Get ModelContext.
     * @return ModelContext.
     */
    ModelContext getModelContext();
    /**
     * Sets the value at model[rowid][columnid];
     * @param  rowid is the unique row identifier
     * @param  colid is the unique columnidentifier.
     * @param  data value stored in the model[rowid][columnid];
     * @return Value
     * @throws Exception Error
     */
    String setValue(String rowid, String colId, String data) throws Exception;
}

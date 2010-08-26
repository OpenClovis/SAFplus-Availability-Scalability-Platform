package com.clovis.model.data;

import  java.util.Vector;
import  java.util.Hashtable;

import  com.clovis.model.schema.SimpleElement;
import  com.clovis.model.schema.ComplexElement;
/**
 * Implementation of ColumnModel.
 * @author <a href="nadeem@clovissolutions.com">Nadeem</a>
 */
public class ColumnManager implements ColumnModel
{
    private Vector _columns;
    /**
     * creates the instance of the ColumnManager
     * @param columns for the table.
     */
    public ColumnManager(Vector columns)
    {
        _columns = columns;
    }
    /**
     * Returns the size of columns
     * 
     * @return number of columns
     */
    public int getColumnCount()         { return _columns.size(); }
    /**
     * Returns the Coulmn of the ColumnManager.
     * 
     * @param index is the field location.
     * @return the column for a given column index.
     */
    public SimpleElement getColumn(int index)
    {
        return (SimpleElement) _columns.get(index); 
    }
    /**
     * Returns the index of the SimpleElement.
     * 
     * @param columnId is the name of the column.
     * @return index of the column, -1 if not found.
     */
    public int indexOf(String columnId)
    {
        if (columnId != null) {
            int size = getColumnCount();
            for (int i = 0; i < size; i++) {
                if (getColumn(i).getName().equals(columnId)) {
                    return i;
                }
            }
        }
        return -1;
    }
    /**
     * Remove the column in the column model
     * 
     * @param index is the location of column to be removed.
     */
    public void removeColumn(int index) { _columns.remove(index); }
    /**
     * Remove the column in the column model
     * 
     * @param c is the column to be removed.
     */
    public void removeColumn(SimpleElement c)  { _columns.remove(c); }
    /**
     * Adds the column in the column model
     * 
     * @param c is the column to be added.
     */
    public void addColumn(SimpleElement c)     { _columns.add(c); }
    /**
     * Static Method for getting ColumnModel for a table
     *
     * @param table ComplexElement for which columnModel is required
     * @return the columnModel corresponding to table.
     */
    public static ColumnModel getColumnModel(ComplexElement table) 
    {
        return new ColumnManager(new Vector(table.getSimpleChildren().values()));
    }
}

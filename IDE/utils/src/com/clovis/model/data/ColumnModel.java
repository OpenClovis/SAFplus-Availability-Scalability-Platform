package com.clovis.model.data;

import  com.clovis.model.schema.SimpleElement;
/**
 * ColumnModel is an interface to get the columns of the Model.
 * @author <a href="nadeem@clovissolutions.com">Nadeem</a>
 */
public interface ColumnModel
{
    /**
     *  Adds the column in the column model
     * @param column is the column to be added.
     */
    void addColumn(SimpleElement column);
    /**
     * Remove the column in the column model
     * @param column is the column to be removed.
     */
    void removeColumn(SimpleElement column);
    /**
     * Remove the column in the column model from specified index
     * 
     * @param index is the location of column to be removed.
     */
    void removeColumn(int index);
    /**
     * Returns the SimpleElement for the column index
     * 
     * @param index is column location.
     * @return SimpleElement for the column index.
     */
    SimpleElement getColumn(int index);
    /**
     * Returns the size of columns
     * 
     * @return number of columns
     */
    int getColumnCount();
    /**
     * Gets Index of a SimpleElement.
     */
    int indexOf(String columnName);
}

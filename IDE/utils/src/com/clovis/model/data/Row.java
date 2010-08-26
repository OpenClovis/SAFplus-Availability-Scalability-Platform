package com.clovis.model.data;

import  java.util.Vector;

import  com.clovis.model.event.CellModelEvent;
import  com.clovis.model.schema.SimpleElement;
/**
 * One Row (instance) of Complex Element.
 * @author <a href="nadeem@clovissolutions.com">Nadeem</a>
 */
public class Row
{
    private Model       _model;
    private String      _rowId;
    private ColumnModel _columnModel;
    private Data[]      _data;
    /**
     * Creates an instance of Row.
     * @param columnModel is the Column Model of the table.
     * @param id is the Row identifier.
     */
    public Row(Model model)
    {
        this(model, null);
    }
    /**
     * Creates an instance of Row.
     * @param model is the data model for the row.
     * @param id is the Row identifier.
     */
    public Row(Model model, String id)
    {
        super();
        _model       = model;
        _columnModel = model.getColumnModel();
        _rowId       = id;
        _data        = new DataImpl[_columnModel.getColumnCount()];
        for (int i = 0; i < _data.length; i++) {
        	_data[i] = new DataImpl(this, _columnModel.getColumn(i), null);
        }
    }

    /**
     * Retuns the Data stored at the cell identified by the column id
     * 
     * @param columnid is the unique columnidentifier.
     * @return the data stored in the Row[columnindex];
     */
    public Data getData(String columnid)
    {
        return getData(_columnModel.indexOf(columnid));
    }
    /**
     * Returns the Data stores at the cell for given column index
     * 
     * @param columnIndex is the index of the column.
     * @return the data stored in the Row[columnindex];
     */
    public Data getData(int columnIndex)
    {
        return _data[columnIndex];
    }
    /**
     * Sets the row id for the Row
     * 
     * @param rowid is the Row identifier.
     */
    public void setRowId(String rowid)
    {
        _rowId = rowid;
    }
    /**
     * Returns the identifier for the row
     * 
     * @return id for the row.
     */
    public String getRowId()
    {
        return _rowId;
    }
    /**
     * Sets Enumeration.
     * @param columnid Name of the column
     * @param newEnum Vector of new Values
     * @throws Exception if column not found
     */
    public void setEnum(String columnid,
                        Vector newEnum)
        throws Exception
    {
        int index = _columnModel.indexOf(columnid);
        Data data = getData(index);
        if (data == null) {
            throw new Exception("No Data found for " + columnid);
        }
        if (newEnum == null) {
            newEnum = new Vector();
        }
        Vector oldEnum = data.getChoices();
        String oldVal = data.getValue();
        if (!newEnum.equals(oldEnum)) {
            data.setChoices(newEnum);
            fireEvent(_columnModel.getColumn(index), data, oldVal); 
        } else if (newEnum.size() > 0) {
            String newVal = (String) newEnum.get(0);
            if (newVal != null && !newVal.equals(oldVal)) {
                setValue(columnid, newVal);
            }
        }
    }

    /**
     * Sets the value.
     * @param columnid is the column identifier.
     * @param value is the new value to be set.
     * @return previous value.
     * @throws Exception in case of error
     */
    public String setValue(String columnid, 
                           String value)
                    throws Exception
    {
        return setValue(columnid, value, false);
    }
    /**
     * Sets the value.
     * 
     * @param columnid is the column identifier.
     * @param value is the new value to be set.
     * @param isFinal if previous value has to be set.
     * @return previous value
     * @throws Exception in case of error
     */
    public String setValue(String columnid, 
                           String value, boolean isFinal)
        throws Exception
    {
        int index = _columnModel.indexOf(columnid);
        Data data = getData(index);
        String oldVal = data.getValue();
        data.setValue(value);
        if (isFinal) {
            data.setBaseValue(value);
        } else {
            // Fire Event Only if value has changed.
            boolean toFire = (oldVal != null) ?
                !(oldVal.equals(value)) : (value != null);
            if (toFire) {
                fireEvent(_columnModel.getColumn(index), data, oldVal);
            }
        }
        return oldVal;
    }
    /**
     * Returns the object stored in specified Column.
     * 
     * @param columnid is the column identifier.
     * @return object stored in the column.
     */
    public String getValue(String columnid)
    {
        return getData(columnid).getValue();
    }
   /**
    * Sets the Enabled and Disabled state for a field.
    * @param columnid is the column identifier.
    * @param status is the new status to be set.
    */
    public void setEnabled(String columnid, 
                           boolean status)
    {
        getData(columnid).setEnabled(status);
    }
    /**
     * Get the Enabled and Disabled state for a field
     * @param columnid Column Identifier
     * @return Enabled/Disabled
     */
    public boolean getEnabled(String columnid)
    {
        return getData(columnid).isEnabled();
    }
    /**
     * Returns the column model for the row.
     * @return column model for the row,
     */
    public ColumnModel getColumnModel()
    {
        return _columnModel;
    }
    /**
     * Returns the parent component i.e Model
     * @return the model of the row.
     */
    public Model getModel()
    {
        return _model;
    }
    /**
     * Sets the Model
     * 
     * @param model Model to be set
     */
    public void setModel(Model model)
    {
        _model = model;
    }
    /**
     * String representation.
     * @return String representation.
     */
    public String toString()
    {
        StringBuffer buff = new StringBuffer();
        for (int i = 0; i < _data.length; i++) {
            buff.append("\n\t\t").append(_data[i].toString());
        }
        return buff.toString();
    }
    /**
     * Fires the Cell changed event.
     * @param column is the column 
     * @param data is the data
     * @param oldVal is the old value
     */
    public void fireEvent(SimpleElement column, Data data, String oldVal)
    {
        if (_model != null) {
            _model.fireEvent(new CellModelEvent(_model, getRowId(), 
                                column.getName(), oldVal, 
                                data.getValue(), CellModelEvent.VALUE_CHANGED));
        }
    }
}

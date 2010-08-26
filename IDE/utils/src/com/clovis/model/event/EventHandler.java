package com.clovis.model.event;

import  java.util.Vector;

import  com.clovis.model.data.Model;
/**
 * This class has all the logic for Event Handling.
 * @author <a href="mailto:nadeem@clovissolutions.com">Nadeem</a>
 */
public class EventHandler
{
    private Vector _listener = new Vector();
    /**
     * Add Model Listener.
     * @param listener Model Listener
     */
    public void addModelListener(ModelListener listener)
    {
        _listener.add(listener);
    }
    /**
     * Remove Model Listener.
     * @param listener Model Listener
     */
    public void removeModelListener(ModelListener listener)
    {
        _listener.remove(listener);
    }
    /**
     * Fire Change Events.
     * @param event ModelEvent
     */
    public void fireEvent(ModelEvent event)
    {
        switch(event.getType()) {
            case ModelEvent.MODEL_EVENT:
                handleModelEvent(event);                        break;
            case ModelEvent.ROW_MODEL_EVENT:
                handleRowModelEvent((RowModelEvent) event);     break;
            case ModelEvent.CELL_MODEL_EVENT:
                handleCellModelEvent((CellModelEvent) event);   break;
        }
    }
    /**
     * Model Event Handling.
     * @param event ModelEvent.
     */
    private void handleModelEvent(ModelEvent event)
    {
        switch (event.getAction()) {
            case ModelEvent.MODEL_CREATED:
                for (int i = 0; i < _listener.size(); i++) {
                    ((ModelListener) _listener.get(i)).modelCreated(event);
                }
                break;
        }
    }
    /**
     * Cell Model Event Handling.
     * @param event CellModelEvent.
     */
    private void handleCellModelEvent(CellModelEvent event)
    {
        switch (event.getAction()) {
            case CellModelEvent.VALUE_CHANGED:
                for (int i = 0; i < _listener.size(); i++) {
                    ((ModelListener) _listener.get(i)).valueChanged(event);
                }
                break;
        }
    }
    /**
     * Row Event Handling.
     * @param event RowEvent.
     */
    private void handleRowModelEvent(RowModelEvent event)
    {
        switch (event.getAction()) {
            case RowModelEvent.ROW_ADDED:
                for (int i = 0; i < _listener.size(); i++) {
                    ((ModelListener) _listener.get(i)).rowAdded(event);
                }
                break;
            case RowModelEvent.ROW_DELETED:
                for (int i = 0; i < _listener.size(); i++) {
                    ((ModelListener) _listener.get(i)).rowDeleted(event);
                }
                break;
        }
    }
}

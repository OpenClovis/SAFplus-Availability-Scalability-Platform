package com.clovis.model.event;
/**
 * Model Listener interface.
 * 
 * @author <a href="mailto:nadeem@clovissolutions.com">Nadeem</a>
 */
public interface ModelListener
{
    /**
     * Data model created 
     *
     * @param event is ModelEvent.
     */
    void modelCreated(ModelEvent event);
    /**
     * Callback for row inserted event
     * 
     * @param event is RowModelEvent.
     */
    void rowAdded(RowModelEvent event);
    /**
     * Callback for Row deleted.
     * 
     * @param event is RowModelEvent.
     */
    void rowDeleted(RowModelEvent event);
    /**
     * Event for model cell value changed.
     * 
     * @param event is ModelEvent.
     */
    void valueChanged(CellModelEvent event);
}

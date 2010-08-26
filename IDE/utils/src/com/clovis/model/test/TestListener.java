package com.clovis.model.test;

import com.clovis.model.event.CellModelEvent;
import com.clovis.model.event.ModelEvent;
import com.clovis.model.event.ModelListener;
import com.clovis.model.event.RowModelEvent;
/**
 * Tests DataModel implementation.
 * @author <a href="nadeem@clovissolutions.com">Nadeem</a>
 */
public class TestListener
    implements ModelListener
{
    /**
     * Data model created 
     * @param event is ModelEvent.
     */
    public void modelCreated(ModelEvent event)      { printEvent(event); }
    /**
     * Callback for row inserted event
     * @param event is RowModelEvent.
     */
    public void rowAdded(RowModelEvent event)       { printEvent(event); }
    /**
     * Callback for Row deleted.
     * @param event is RowModelEvent.
     */
    public void rowDeleted(RowModelEvent event)     { printEvent(event); }
    /**
     * Event for model cell value changed.
     * @param event is ModelEvent.
     */
    public void valueChanged(CellModelEvent event)  { printEvent(event); }
    /**
     * Prints Event to std out.
     */
    private void printEvent(ModelEvent e) { System.out.println(e.toString()); }
}

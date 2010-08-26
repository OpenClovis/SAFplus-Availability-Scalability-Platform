package com.clovis.model.common;

import  com.clovis.model.schema.Schema;
import  com.clovis.model.event.EventHandler;
/**
 * Provides context for the model.
 * @author <a href="nadeem@clovissolutions.com">Nadeem</a>
 */
public class ModelContext
{
    private Schema       _schema;
    private EventHandler _eventHandler = new EventHandler();
    /**
     * Set Event Handler.
     * @param eh Event Handler.
     */
    public void setEventHandler(EventHandler eh)    { _eventHandler = eh;   }
    /**
     * Get Event Handler.
     * @return Event Handler.
     */
    public EventHandler getEventHandler()           { return _eventHandler; }
    /**
     * Set Schema.
     * @return Schema
     */
    public void setSchema(Schema schema)            { _schema = schema;     }
    /**
     * Set Schema.
     * @return Schema
     */
    public Schema getSchema()                       { return _schema;       }
}

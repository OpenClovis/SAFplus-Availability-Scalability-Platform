package com.clovis.model.event;

import  com.clovis.model.data.Model;
/**
 * The <code>ModelEvent</code> encloses the context and source of the event
 * happend to the model. <code>ModelEvent</code> describes that the
 * particular cell has undergone some changes.
 * 
 * @author <a href="mailto:nadeem@clovissolutions.com">Nadeem</a>
 */
public class ModelEvent
{
    // Type
    public static final int MODEL_EVENT         = 1;
    public static final int ROW_MODEL_EVENT     = 2;
    public static final int CELL_MODEL_EVENT    = 3;
    public static final int COLUMN_MODEL_EVENT  = 4;
    // Action
    public static final int MODEL_CREATED       = 1;
    public static final int MODEL_CHANGED       = 10;

    protected int   _action;
    protected Model _model;
    protected int   _type;
    /**
     * Constructs a ModelEvent.
     * 
     * @param model is the view data model.
     * @param action is the action for the model event.
     */
    public ModelEvent(Model model, int action)
    {
        _model    = model;
        _action   = action;
        _type     = MODEL_EVENT;
    }
    /**
     * Sets the action.
     * 
     * @param action is the action for the event.
     */
    public void setAction(int action) { _action = action; }
    /**
     * Returns the action for the model event.
     * 
     * @return action for the event.
     */
    public int getAction() { return _action; }
    /**
     * Returns the model.
     * 
     * @return model on which event happened.
     */
    public Model getModel() { return _model; }
    /**
     * Sets the model for the event.
     *
     * @param model for the event.
     */
    public void setModel(Model model) { _model = model; }
    /**
     * Returns the Type of the model event.
     * 
     * @return type for the event.
     */
    public int getType() { return _type; }
    /**
     * returns the String representation of ModelEvent.
     * 
     * @return string representation of event.
     */
    public String toString()
    {
        return "ModelEvent: Action=" + _action + ", Type=" + _type;
    }
}

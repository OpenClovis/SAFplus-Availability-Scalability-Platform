package com.clovis.model.schema;

/**
 * <code>SimpleElement</code> extends AbstractSimpleElement to add
 * restriction.
 * 
 * @author <a href=mailto:nadeem@clovissolutions.com>Nadeem</a>
 */
public class SimpleElement
    extends com.clovis.model.schema.castor.AbstractSimpleElement
{
    /** Points to the Restriction on this simple type if any */
    private Restriction _restriction;
    /**
     * Creates a new SimpleElement object.
     */
    protected SimpleElement() {}
    /**
     * Returns the restriction for this SimpleElement
     * 
     * @return The Restrction if any
     */
    public Restriction getRestriction() { return _restriction; }
    /**
     * Sets the restriction for this SimpleElement
     * @param restr The restriction to be set
     */
    public void setRestriction(Restriction restr) { _restriction = restr; }
    /**
     * Gives a String representation of a SimpleElement
     * 
     * @return The String representation of SimpleElement
     */
    public String toString()
    {
        StringBuffer buffer = new StringBuffer("Simple element: ");
        buffer.append(super.toString());
        if (_restriction != null) { buffer.append(_restriction.toString()); }
        return buffer.toString();
    }
}

package com.clovis.model.schema;

import  java.util.Enumeration;
import  java.util.Hashtable;

import  com.clovis.model.schema.castor.AbstractElement;
/**
 * <code>ComplexElement</code> extends AbstractComplexElement and provides
 * get/set methods for simple and complex elements.
 * 
 * @author <a href=mailto:nadeem@clovissolutions.com>Nadeem</a>
 */
public class ComplexElement
    extends com.clovis.model.schema.castor.AbstractElement
{
    private Hashtable _simpleChildList;
    private Hashtable _complexChildList;
    /**
     * Creates a new ComplexElement object.
     */
    protected ComplexElement()
    {
        _simpleChildList = new Hashtable();
        _complexChildList = new Hashtable();
    }
    /**
     * Returns the Simple Child with a specified name
     * 
     * @param name The name of the Element
     * @return The corresponding Simple Element
     */
    public SimpleElement getSimpleElement(String name)
    {
        return (SimpleElement) _simpleChildList.get(name);
    }
    /**
     * Returns the Complex Child with a specified name
     * 
     * @param name The name of the Element
     * @return The corresponding Complex Element
     */
    public ComplexElement getComplexElement(String name)
    {
        return (ComplexElement) _complexChildList.get(name);
    }
    /**
     * Returns the Simple Children Hashtable
     * 
     * @return The Simple children hash table
     */
    public Hashtable getSimpleChildren()
    {
        return _simpleChildList;
    }
    /**
     * Returns the Complex Child hashtable
     * 
     * @return The Complex children hash table
     */
    public Hashtable getComplexChildren()
    {
        return _complexChildList;
    }
    /**
     * Adds a new Element to the Complex Element depending on 
     * the type of the object it is added to the appropriate list
     * 
     * @param elementName The name of the Element
     * @param element The ComplexElement to be added
     */
    public void addElement(String elementName, AbstractElement element)
    {
        if (element instanceof ComplexElement) {
            _complexChildList.put(elementName, element);
        } else if (element instanceof SimpleElement) {
            _simpleChildList.put(elementName, element);
        }
    }
    /**
     * String Representation of ComplexElements.
     * @return String Representation of ComplexElements.
     */
    public String toString()
    {
        StringBuffer buffer = new StringBuffer("Complex element[");
        buffer.append(getName()).append("]: ").append(super.toString());
        //Print Simple Children
        Enumeration simpleEls = getSimpleChildren().elements();
        while (simpleEls.hasMoreElements()) {
            buffer.append("\n").append(simpleEls.nextElement().toString());
        }
        return buffer.toString();
    }
}

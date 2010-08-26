package com.clovis.model.schema;
/**
 * Factory to create Elements ComplexElement and SimpleElement.
 * This class can be extended to use extended Complex and SimpleElements like
 * Table and Columns.
 */
public class ElementFactory
{
    private static ElementFactory defaultFactory = new ElementFactory();
    /**
     * Get Default.
     */
    public static ElementFactory getDefault()   { return defaultFactory;       }
    /**
     * Create Simple Element.
     */
    public SimpleElement createSimpleElement()  { return new SimpleElement();  }
    /**
     * Create Complex Element.
     */
    public ComplexElement createComplexElement(){ return new ComplexElement(); }
}

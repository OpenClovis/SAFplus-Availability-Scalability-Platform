package com.clovis.model.data;

import  java.util.Vector;

import  com.clovis.model.schema.SimpleElement;
/**
 * Column for a Table.
 * @author <a href="nadeem@clovissolutions.com">Nadeem</a>
 */
public class Column extends SimpleElement 
{
    /**
     * Clear Enumeration Vector.
     */
    public void clearEnumeration() { getRestriction().setValues(new Vector()); }
}

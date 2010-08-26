package com.clovis.model.schema;

import  java.io.InputStream;
import  java.io.FileInputStream;

import  java.util.Hashtable;
import  java.util.Enumeration;

import  javax.xml.parsers.SAXParserFactory;

import  com.clovis.common.log.Log;
/**
 * <code>Schema</code> is the class which provides factory methods for 
 * Simple, Complex and Restriction objects 
 *
 * @author <a href=mailto:nadeem@clovissolutions.com>Nadeem</a>
 */
public class Schema
    extends Hashtable
{
    private static Hashtable schemaHash = new Hashtable();
    private static Log log = new Log(Schema.class.getName());
    /**
     * Hidden Constructor.
     */
    private Schema() {}
    /**
     * Parse and return Schema instance.
     * @param  is Schema InputStream
     * @param elementFactory Factory for creating Element.
     * @return Schema instance.
     * @throws Exception Thrown if parsing fails
     */
    public static Schema parse(InputStream is, ElementFactory ef)
        throws Exception
    {
        log.trace("parse()");
        Schema schema = (Schema) schemaHash.get(is);
        if (schema == null) {
            schema = new Schema();
            SAXParserFactory.newInstance().newSAXParser().
                parse(is, new SchemaParser(schema, ef));
        }
        return schema;
    }
    /**
     * Parse and return Schema instance.
     * @param  is Schema InputStream
     * @return Schema instance.
     * @throws Exception Thrown if parsing fails
     */
    public static Schema parse(InputStream is)
        throws Exception
    {
        return Schema.parse(is, ElementFactory.getDefault());
    }
    /**
     * String representation of Complex Elements in an easy-to-debug way.
     */
    public String toString()
    {
        StringBuffer buff = new StringBuffer("\n");
        Enumeration enums = elements();
        while (enums.hasMoreElements()) {
            buff.append(enums.nextElement().toString()).append("\n");
        }
        return buff.toString();
    }
    /**
     * Get ComplexElement given Name.
     * @param name Name of ComplexElement.
     * @return ComplexElement.
     */
    public ComplexElement getComplexElement(String name)
    {
        return (ComplexElement) get(name);
    }
    /**
     * Test Method.
     * 
     * @param  args The Schema file name
     * @throws Exception If schema conversion fails
     */
    public static void main(String[] args)
        throws Exception
    {
        System.out.println("Schema: " + args[0]);
        log.trace(Schema.parse(new FileInputStream(args[0])).toString());
    }
}

package com.clovis.model.schema;

import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import java.util.Enumeration;

import org.exolab.castor.xml.JavaNaming;
/**
 * <code>AttributeTable</code> Provides a way to get/set attributes
 * read from schema to complex element/simple element classes.
 *
 * @author <a href="mailto:nadeem@clovissolutions.com">Nadeem</a>
 */
public class AttributeTable
    extends java.util.Hashtable
{
    /**
     * Sets the field values of an Element.
     * 
     */
    public final void setFields()
    {
        Enumeration attribEnum = keys();
        while (attribEnum.hasMoreElements()) {
            String attName = (String) attribEnum.nextElement();
            String attValue = (String) get(attName);
            setField (attName, attValue);
        }
    }
    /**
     * Sets the field value of an Element for a specifoc feld
     * 
     * @param attName The name of the field
     * @param attValue The Value of the attribute
     */
    public final void setField (String attName, String attValue)
    {
        Class objectClass = getClass ();

        /*  Now find out the field in the corresponding class
            First look in the AbstractSimpleElement class
            If not found look in super class AbstractElement
        */
        String attribFieldName = JavaNaming.toJavaMemberName(attName);
        String attribMethodName = "set"
                                  + JavaNaming.toJavaClassName(attName);

        if (!attribFieldName.startsWith("_")) {
            attribFieldName = "_" + attribFieldName;
        }

        Method attribMethod = null;

        Class elementClass = objectClass;
        while ((elementClass != null) && (attribMethod == null)){
            try {
                Field attribField = elementClass.getDeclaredField(
                                      attribFieldName);
                Class attribClass = attribField.getType();
                Class[] methodArgs = { attribClass };

                // Now find out the set method for this field
                attribMethod = objectClass.getMethod(
                                 attribMethodName, methodArgs);
                Object[] invokeArgs = new Object[1];
                String attribClassName = attribClass.getName();

                if (attribClassName.equals("java.lang.String")) {
                    invokeArgs[0] = new String(attValue);
                } else if (attribClassName.equals("int")) {
                    invokeArgs[0] = new Integer(attValue);
                } else if (attribClassName.equals("double")) {
                    invokeArgs[0] = new Double(attValue);
                } else if (attribClassName.equals("boolean")) {
                    invokeArgs[0] = new Boolean(attValue);
                }

                // Invoke the set method for the field
                attribMethod.invoke(this, invokeArgs);
            } catch (Exception nsfe) {
                attribMethod = null;
            }
            elementClass = elementClass.getSuperclass();
        }
    }
}

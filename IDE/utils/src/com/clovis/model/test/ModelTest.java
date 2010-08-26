package com.clovis.model.test;

import java.io.FileInputStream;
import java.io.FileReader;
import java.io.InputStream;
import java.io.Reader;
import java.io.InputStreamReader;

import com.clovis.model.common.ModelContext;
import com.clovis.model.data.Model;
import com.clovis.model.event.EventHandler;
import com.clovis.model.parser.ModelFactory;
import com.clovis.model.schema.ComplexElement;
import com.clovis.model.schema.Schema;
/**
 * Tests DataModel implementation.
 * @author <a href="nadeem@clovissolutions.com">Nadeem</a>
 */
public class ModelTest
{
	/**
	 * Worker method
	 * @param name       Name of complex Element.
	 * @param is         Schema InputStream.
	 * @param reader     Data XML Reader.
	 * @throws Exception On Error.
	 */
	private static void test(String name, InputStream is, Reader reader)
		throws Exception
	{
        Schema schema = Schema.parse(is);
        ModelContext mc = new ModelContext();
        EventHandler eh = new EventHandler();
        mc.setSchema(schema);
        mc.setEventHandler(eh);
        eh.addModelListener(new TestListener());
        ComplexElement complexElement = schema.getComplexElement(name);
        Model model = ModelFactory.createModel(complexElement, mc);
        ModelFactory.populateModel(model, reader);		
	}
    /**
     * Test Method.
     * @param arg ..
     */
    public static void main(String[] args) 
        throws Exception
    {
    	String      name;
    	InputStream is;
    	Reader      reader;
        if (args.length == 3) {
        	name   = args[0];
        	is     = new FileInputStream(args[1]);
        	reader = new FileReader(args[2]);            
        } else {
            System.err.println("Usage: name schema-path xml-path");
            name   = "junk";
            is     = ModelTest.class.getResourceAsStream("TestSchema.xsd");
            InputStream xmlIs  = ModelTest.class.getResourceAsStream("TestData.xml");
            reader = new InputStreamReader(xmlIs);
        }
        ModelTest.test(name, is, reader);
    }
}

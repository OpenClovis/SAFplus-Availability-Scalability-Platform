package com.clovis.model.parser;

import  java.io.Reader;
import  java.io.IOException;

import  org.xml.sax.InputSource;
import  org.xml.sax.SAXException;
import  javax.xml.parsers.SAXParserFactory;
import  javax.xml.parsers.ParserConfigurationException;

import  com.clovis.common.log.Log;

import  com.clovis.model.data.Model;
import  com.clovis.model.data.ModelImpl;

import  com.clovis.model.common.ModelContext;
import  com.clovis.model.schema.ComplexElement;
/**
 * The <code>ModelFactory</code> creates the Vector of rows for a given
 * table model and data xml
 * 
 * @author <a href="mailto:nadeem@clovissolutions.com">Nadeem</a>
 */
public class ModelFactory
{
    /**
     * SAXParserFactory object.
     */
    private static SAXParserFactory spf = SAXParserFactory.newInstance();
    private static Log log = new Log(ModelFactory.class.getName());
    /**
     * Create a Model.
     * @param table ComplexElement.
     * @param mc    Context for Model.
     */
    public static Model createModel(ComplexElement table, ModelContext mc)
    {
        return new ModelImpl(table, mc);
    }
    /**
     * Creates the vector of Rows.
     * 
     * @param  model is the data model.
     * @param  xml is the data xml.
     * @throws IOException From InputSource constructor.
     * @throws ParserConfigurationException from XMLReader
     * @throws SAXException from XMLReader.parser
     */
    public static void populateModel(Model model, Reader xml)
        throws IOException, ParserConfigurationException, SAXException
    {
    	spf.newSAXParser().parse(new InputSource(xml), new DataParser(model));
    }
}

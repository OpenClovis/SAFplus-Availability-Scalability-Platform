package com.clovis.model.parser;

import  java.util.Stack;
import  java.util.Hashtable;
import  java.util.Vector;

import  org.xml.sax.Attributes;
import  org.xml.sax.SAXException;
import  org.xml.sax.helpers.DefaultHandler;

import  com.clovis.common.log.Log;

import  com.clovis.model.data.Row;
import  com.clovis.model.data.Model;
import  com.clovis.model.data.ModelImpl;
import  com.clovis.model.data.ColumnModel;
import  com.clovis.model.schema.ComplexElement;
/**
 * This class acts as a processor for each Table element It does the SAX
 * parsing of the element in the input XML and constructs rows.
 * @author <a href="mailto:nadeem@clovissolutions.com">Nadeem</a>
 */
public class DataParser
    extends DefaultHandler
{
    public  static final String ROW_KEY_TAG  = "CwKey";
    public  static final String XML_WRAPPER  = "CwWrapper";
    private static Log   log = new Log(DefaultHandler.class.getName());
    
    private Hashtable      _modelHash       = new Hashtable();
    private int            _xmlStackSize    = 0;
    private String         _tableName       = null;
    private Vector         _rows            = null;
    private Stack          _fields          = new Stack();
    private Model          _model           = null;
    private ColumnModel    _columnModel     = null;
    private StringBuffer   _tempBuffer      = new StringBuffer();
    private boolean        _isInvalid       = false;
    private String         _ignoredElement  = null;
    /**
     * Creates a new ElementProcessor object.
     * 
     * @param model     Model
     */
    DataParser(Model model)
    {
        _model       = model;
        _columnModel = _model.getColumnModel();
        _tableName   = _model.getTable().getName();
    }
    /**
     * Get Model for ComplexElement. One ComplexElement may have many
     * rows in xml, All of them will share the same model.
     * This method creates the model if it does not exist.
     */
    private Model getModel(ComplexElement table)
    {
        Model model = (Model) _modelHash.get(table.getName());
        if (model == null) {
            model = new ModelImpl(table, _model.getModelContext());
            _modelHash.put(table.getName(), model);
        }
        return model;
    }
    /**
     * Overrides the SAX start element method Takes appropriate action on
     * a field starting or table starting
     * 
     * @param uri   The Namespace URL
     * @param name  The name of the element
     * @param qName The qualified name of the element
     * @param atts  The attributes
     */
    public void startElement(String uri, 
                             String name, 
                             String qName, 
                             Attributes atts)
    {
        _xmlStackSize++;
        _tempBuffer.setLength(0);
        if (_isInvalid || qName.equals(DataParser.XML_WRAPPER)) {
            return;
        }
        int stackSize = _fields.size();
        if (stackSize == 0) {
            if (_tableName.equals(qName)) {
                _fields.push(new Row(_model));
            } else {
                _isInvalid = true;
                _ignoredElement = qName;
                log.warn("Element can't be at top, Ignoring " + qName);
            }
        } else {
            Row currentRow = (Row) _fields.peek();
            ComplexElement current = 
                (ComplexElement) currentRow.getModel().getTable();
            if (current.getSimpleElement(qName) != null) {
                // Simple Child
                _isInvalid = false;
            } else {
                ComplexElement child = current.getComplexElement(qName);
                if (child != null) {
                    // Complex Child
                    _isInvalid = false;
                    _fields.push(new Row(getModel(child)));
                } else {
                    _isInvalid = true;
                    _ignoredElement = qName;
                    log.warn("Element not in schema, Ignoring " + qName);
                }
            }
        }
    }
    /**
     * This is an overloaded method of the  SAXParser This method is
     * called during the parsing when PC DATA starts   Here this method
     * gathers the field values
     * 
     * @param pcData PC Data
     * @param start Start position
     * @param length Length
     */
    public void characters(char[] pcData, 
                           int start, 
                           int length)
    {
        if (_tableName != null) {
            _tempBuffer.append(pcData, start, length);
        }
    }
    /**
     * Overrides the SAX end element method Takes appropriate action on a
     * field ending or table ending
     * 
     * @param  uri The Namespace URL
     * @param  name The name of the element
     * @param  qName The qualified name of the element
     * @throws SAXException On Parse errors.
     */
    public void endElement(String uri, 
                           String name, 
                           String qName)
        throws SAXException
    {
        if (_xmlStackSize == 0) {
            throw new SAXException("Stack Underflow.");
        }
        _xmlStackSize--;
        if (_isInvalid) {
            if (_ignoredElement.equals(qName)) {
                _isInvalid = false;
            }
            return;
        }
        if (qName.equals(DataParser.XML_WRAPPER)) {
            return;
        }
        Row row = (Row) _fields.peek();
        if (row.getModel().getTable().getName().equals(qName)) {
            // End of Row.
            _fields.pop();
            if (row.getRowId() != null) {
                row.getModel().addRow(new Row[] { row });
            } else {
                log.warn("Key not spcified for Row, Ignored.");
            }
        } else {
            // End of Simple Element.
            String val = _tempBuffer.toString();
            _tempBuffer.setLength(0);
            try {
                row.setValue(qName, val, true);
            } catch (Exception e) {
                log.warn("Row.setValue(" + qName + ", " + val + ")", e);
            }
            if (DataParser.ROW_KEY_TAG.equals(qName)) { row.setRowId(val); }
        }
    }
}

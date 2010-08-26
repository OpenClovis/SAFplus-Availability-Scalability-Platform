package com.clovis.model.schema;

import  java.util.Hashtable;

import  org.xml.sax.Attributes;
import  org.xml.sax.helpers.DefaultHandler;

import  com.clovis.common.log.Log;
import  com.clovis.model.schema.castor.AbstractElement;
/**
 * Package private class to parse schema.
 * @author <a href=mailto:nadeem@clovissolutions.com>Nadeem</a>
 */
class SchemaParser extends DefaultHandler
{
    private ComplexElement _complex;
    private Restriction    _restriction;
    private SimpleElement  _simple;
    private Schema         _complexElementTable = null;
    private Hashtable      _simpleElementTable;
    private ElementFactory _elementFactory;

    private Log _log = new Log(this.getClass().getName());

    /**
     * Creates a new SchemaParser object.
     * 
     * @throws Exception Thrown if Loading of the Classes fails
     */
    SchemaParser(Schema schema, ElementFactory ef) 
        throws Exception
    {
        _log.trace("SchemaParser()");
        _elementFactory      = ef;
        _complexElementTable = schema;
        _simpleElementTable  = new Hashtable();
    }
    /**
     * Overriden SAX method Notifies when a tag ends
     * 
     * @param uri The name space URL
     * @param localName The local name of the element
     * @param qName The qualified name of the element
     */
    public void endElement(String uri, 
                           String localName, 
                           String qName)
    {
        _log.trace("endElement:" + qName);
        try {
            if (qName.equals("xsd:element") ||
                qName.equals("xsd:simpleType") ||
                qName.equals("xsd:attribute")) {
                if (_simple != null) {
                    if (_complex != null) {
                        addElementToComplex();
                    } else {
                        _simple.setFields();
                        _simpleElementTable.put(_simple.getName(), _simple);
                    }
                    _simple = null;
                }
            } else if (qName.equals("xsd:complexType")) {
                _log.trace("End of complexType:" + _complex.getName());
                _complex = null;
            }
        } catch (Throwable throwable) {
            _log.error("endElement(): Exception is parsing.", throwable);
        }
    }
    /**
     * The overriden method of SAX parser Detects when an element starts
     * 
     * @param uri The name space URL
     * @param localName The local name of the element
     * @param qName The qualified name of the element
     * @param attributes The attributes object
     */
    public void startElement(String uri, 
                             String localName, 
                             String qName, 
                             Attributes attributes)
    {
        _log.trace("startElement(" + qName + ")");
        try {
            if (qName.equals("xsd:element")) {
                _simple  = createSimpleElement(null, attributes);
            } else if (qName.equals("xsd:attribute")) {
                _simple  = createSimpleElement(null, attributes);
            } else if (qName.equals("xsd:simpleType")) {
                _simple  = createSimpleElement(_simple, attributes);
            } else if (qName.equals("xsd:complexType")) {
                _complex = createComplexElement(attributes);
            } else if (qName.equals("xsd:restriction")) {
                _simple.setRestriction(createRestriction(attributes));
            } else if (qName.equals("xsd:enumeration")) {
                _simple.getRestriction().
                    getValues().add(attributes.getValue("value"));
            }
        } catch (Throwable throwable) {
            _log.error("startElement(): Exception is parsing.", throwable);
        }
    }
    /**
     * Adds an element (Simple/Complex) to a complex element.
     */
    private void addElementToComplex()
    {
        _log.trace("addElementToComplex()");
        _simple.setFields();
        String elementName = _simple.getName();
        if (elementName == null) {
            elementName = (String) _simple.get("ref");
            if (elementName != null) {
                findReferredObject(elementName, elementName, null);
            } else {
                _log.error("Either name or ref should be specified.");
            }
        } else {
            String elementType = _simple.getType();
            if (elementType == null) {
                _log.error("No type for :- " + elementName);
            } else {
                if (elementType.equals("xsd:string")
                    || elementType.equals("xsd:int")
                    || elementType.equals("xsd:integer")
                    || elementType.equals("xsd:long")
                    || elementType.equals("xsd:time")
                    || elementType.equals("xsd:boolean")
                    || elementType.equals("xsd:double")) {
                    _complex.addElement(elementName, _simple);
                    _log.trace(_complex.getName() + ": Adding " + elementName);
                } else {
                    findReferredObject(elementName, elementType, null);
                }
            }
        }
    }
    /**
     * Add attributes to Element.
     * @param element Element.
     * @param attrs   Attributes
     */
    private void addAttributes(AbstractElement element, Attributes attrs)
    {
        if (attrs != null) {
            for (int i = 0; i < attrs.getLength(); i++) {
                String name = attrs.getQName(i);
                if (name.equals("ref")) {
                    name = "name";
                }
                element.put(attrs.getQName(i), attrs.getValue(i));
            }
        }
    }
    /**
     * Creates a new Complex Element object Uses the specified class to create
     * the object
     * 
     * @return The newly created Complex Element
     */
    private ComplexElement createComplexElement(Attributes attrs)
    {
        _log.trace("In  createComplexElement()");
        ComplexElement complex = _elementFactory.createComplexElement();
        if (_simple != null) {
            complex.putAll(_simple);
            _simple = null;
        }
        addAttributes(complex, attrs);
        complex.setFields();
        _complexElementTable.put(complex.getName(), complex);
        _log.trace("Out createComplexElement(" + complex.getName() + ")");
        return complex;
    }
    /**
     * Creates a new Restriction object The specified class is loaded to
     * create the object
     * 
     * @param atts The Attribute list
     * @return The newly created Restriction object
     */
    private Restriction createRestriction(Attributes atts)
    {
        _log.trace("createRestriction()");
        Restriction restriction = new Restriction();
        _simple.put("type", atts.getValue("base"));
        return restriction;
    }
    /**
     * Creates a new Simple Element object The specified class is used to
     * create the object If a referenced object is passed, 
     * the attributes of this referred object is copied into the new object
     * 
     * @param refSimple The referenced Simple Element
     * @param atts The Attribute list
     * @return The newly created Simple Element object
     */
    private SimpleElement createSimpleElement(SimpleElement refSimple,
                                              Attributes atts)
    {
        _log.trace("createSimpleElement()");
        SimpleElement simple = _elementFactory.createSimpleElement();
        if (refSimple != null) {
            simple.putAll(refSimple);
        }
        addAttributes(simple, atts);
        return simple;
    }
    /**
     * Finds out a referred Simple or Complex Element and adds it into 
     * the currently processed Complex Element
     * 
     * @param elmName The name of the element
     * @param refName The name of referred element
     * @param atts The Attribute list
     */
    private void findReferredObject(String elmName,
                                    String refName,
                                    Attributes atts)
    {
        _log.trace("findReferredObject(" + elmName + ", " + refName + ")");
        Object refObject = _simpleElementTable.get(refName);

        if (refObject != null) {
            SimpleElement refSimple = (SimpleElement) refObject;
            _simple.putAll(refSimple);
            _simple.put("name", elmName);
            _simple.put("type", refSimple.getType());
            _simple.setFields();
            _simple.setRestriction(refSimple.getRestriction());
            _complex.addElement(elmName, _simple);
        } else {
            ComplexElement refCom = 
                (ComplexElement) _complexElementTable.get(elmName);
            if (refCom != null) {
                _complex.addElement(elmName, refCom);;
            } else {
                refCom = (ComplexElement) _complexElementTable.get(refName);
                if (refCom != null) {
                    ComplexElement clone = (ComplexElement) refCom.clone();
                    clone.setName(elmName);
                    _complex.addElement(elmName, clone);;
                    _complexElementTable.put(elmName, clone);
                } else { _log.error("Unresolved XSD Reference: " + refName); }
            }
        }
    }
}

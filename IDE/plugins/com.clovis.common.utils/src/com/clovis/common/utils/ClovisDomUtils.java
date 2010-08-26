/**
 * 
 */
package com.clovis.common.utils;

import java.io.File;
import java.io.FileWriter;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

/**
 * Dom Utils.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class ClovisDomUtils {

	/**
	 * Builds the document for the given file.
	 * 
	 * @param filePath
	 * @return
	 */
	public static Document buildDocument(String filePath) {
		try {
			DocumentBuilderFactory factory = DocumentBuilderFactory
					.newInstance();
			DocumentBuilder builder = factory.newDocumentBuilder();

			if (filePath.equals(""))
				return builder.newDocument();
			else
				return builder.parse(filePath);

		} catch (Exception e) {
		}

		return null;
	}

	/**
	 * Saves the given document to the given file.
	 * 
	 * @param document
	 * @param filePath
	 */
	public static void saveDocument(Document document, String filePath) {
		try {
			TransformerFactory factory = TransformerFactory.newInstance();
			factory.setAttribute("indent-number", new Integer(2));

			Transformer transformer = factory.newTransformer();
			transformer.setOutputProperty(OutputKeys.INDENT, "yes");

			normalizeElement(document.getDocumentElement());

			StreamResult result = new StreamResult(new FileWriter(new File(
					filePath)));
			DOMSource source = new DOMSource(document);
			transformer.transform(source, result);

		} catch (Exception e) {
		}
	}

	/**
	 * Normalizes the element.
	 * 
	 * @param element
	 */
	public static void normalizeElement(Element element) {
		Document doc = element.getOwnerDocument();

		Node next = (Node) element.getFirstChild(), child;
		while ((child = next) != null) {
			next = child.getNextSibling();

			if (child.getNodeType() == Node.TEXT_NODE) {
				String trimmedVal = child.getNodeValue().trim();

				if (trimmedVal.length() == 0) {
					element.removeChild(child);

				} else {
					element.replaceChild(doc.createTextNode(trimmedVal), child);
				}

			} else if (child.getNodeType() == Node.ELEMENT_NODE) {
				normalizeElement((Element) child);
			}
		}
	}

	/**
	 * Returns the nodes for the given path.
	 * 
	 * @param rootNode
	 * @param nodePath
	 * @return
	 */
	public static List<Node> getNodesForPath(Node rootNode, String nodePath) {

		ArrayList<Node> childList = new ArrayList<Node>(), nodeList = null;
		childList.add(rootNode);
		Iterator<Node> nodeIterator;

		NodeList tempList;
		Node node;

		String pathNodes[] = nodePath.split(",");
		for (int i = 0; i < pathNodes.length; i++) {

			nodeList = childList;
			nodeIterator = nodeList.iterator();
			childList = new ArrayList<Node>();

			while (nodeIterator.hasNext()) {
				node = nodeIterator.next();

				if (node.getNodeName().equals(pathNodes[i])) {
					tempList = node.getChildNodes();

					for (int k = 0; k < tempList.getLength(); k++) {
						childList.add(tempList.item(k));
					}

				} else {
					nodeIterator.remove();
				}
			}
		}

		return nodeList;
	}
}

/**
 * 
 */
package com.clovis.cw.workspace.migration;

import java.io.File;
import java.io.FileWriter;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.StringTokenizer;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.CoreException;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

import com.clovis.common.utils.ClovisUtils;
import com.clovis.common.utils.UtilsPlugin;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.genericeditor.GenericEditor;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.genericeditor.commands.AutoArrangeCommand;
import com.clovis.cw.genericeditor.editparts.BaseDiagramEditPart;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.WorkspacePlugin;
import com.clovis.cw.workspace.migration.handler.AbstractMigrationHandler;

/**
 * This is a utility class for various migration operations.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class MigrationUtils implements MigrationConstants {

	private static final Log LOG = Log.getLog(WorkspacePlugin.getDefault());

	/**
	 * Returns the project version of the selected project.
	 * 
	 * @param project
	 * @return
	 */
	public static String getProjectVersion(IProject project) {
		String version = "2.2.0";
		try {
			String comment = project.getDescription().getComment();
			version = comment.split(":")[1];
		} catch (Exception e) {
		}
		return version;
	}

	/**
	 * Returns the project update version of the selected project.
	 * 
	 * @param project
	 * @return
	 */
	public static int getProjectUpdateVersion(IProject project) {
		int updateVersion = 0;
		try {
			String comment = project.getDescription().getComment();
			updateVersion = comment.split(":").length > 2 ? Integer
					.parseInt(comment.split(":")[3]) : 0;
		} catch (Exception e) {
		}
		return updateVersion;
	}

	/**
	 * Checks wether the project can be migrated or not.
	 * 
	 * @param project
	 * @return
	 */
	public static boolean isMigrationRequired(IProject project) {
		String currentVersion = getProjectVersion(project);
		int currentUpdateVersion = getProjectUpdateVersion(project);

		String targetVersion = DataPlugin.getProductVersion();
		int targetUpdateVersion = DataPlugin.getProductUpdateVersion();

		return isMigrationRequired(currentVersion, targetVersion,
				currentUpdateVersion, targetUpdateVersion);
	}

	/**
	 * Checks wether the project can be migrated or not.
	 * 
	 * @param currentVersion
	 * @param targetVersion
	 * @param currentUpdateVersion
	 * @param targetUpdateVersion
	 * @return
	 */
	public static boolean isMigrationRequired(String currentVersion,
			String targetVersion, int currentUpdateVersion,
			int targetUpdateVersion) {

		StringTokenizer currentVersionTokenizer = new StringTokenizer(
				currentVersion, ".");
		StringTokenizer targetVersionTokenizer = new StringTokenizer(
				targetVersion, ".");

		int current, target;
		while (currentVersionTokenizer.hasMoreTokens()
				&& targetVersionTokenizer.hasMoreTokens()) {

			current = Integer.parseInt(currentVersionTokenizer.nextToken());
			target = Integer.parseInt(targetVersionTokenizer.nextToken());

			if (current < target) {
				return true;
			} else if (current > target) {
				return false;
			} else {
				continue;
			}
		}

		if (currentVersionTokenizer.hasMoreTokens()) {
			return false;
		} else if (targetVersionTokenizer.hasMoreTokens()) {
			return true;
		} else if (targetUpdateVersion > currentUpdateVersion) {
			return true;
		}

		return false;
	}

	/**
	 * Populates the list of element having the given name.
	 * 
	 * @param element
	 * @param elementName
	 * @param elementList
	 */
	public static void getElementListByTagName(Element element,
			String elementName, ArrayList<Element> elementList) {
		NodeList nodeList = element.getElementsByTagName(elementName);
		for (int i = 0; i < nodeList.getLength(); i++) {
			elementList.add((Element) nodeList.item(i));
		}
	}

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

			return builder.parse(filePath);

		} catch (Exception e) {
			reportProblem(PROBLEM_SEVERITY_ERROR,
					"Unable to create the document for '" + filePath + "'.",
					e);
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

			document.setXmlStandalone(true);
			normalizeElement(document.getDocumentElement());

			StreamResult result = new StreamResult(new FileWriter(new File(
					filePath)));
			DOMSource source = new DOMSource(document);
			transformer.transform(source, result);

		} catch (Exception e) {
			MigrationUtils
					.reportProblem(PROBLEM_SEVERITY_ERROR,
							"Unable to save the document for '" + filePath
									+ "'.", e);
		}
	}

	/**
	 * Add the attribute.
	 * 
	 * @param rootNode
	 * @param nodePath
	 * @param attrName
	 * @param attrValue
	 */
	public static void addAttribute(Node rootNode, String nodePath,
			String attrName, String attrValue) {

		if (nodePath.contains(",")) {

			Iterator<Node> itr = getNodesForPath(rootNode, nodePath).iterator();
			while (itr.hasNext()) {
				((Element) itr.next()).setAttribute(attrName, attrValue);
			}

		} else {
			NodeList nodeList = rootNode.getOwnerDocument()
					.getElementsByTagName(nodePath);
			for (int i = 0; i < nodeList.getLength(); i++) {
				((Element) nodeList.item(i)).setAttribute(attrName, attrValue);
			}
		}
	}

	/**
	 * Adds the element as child.
	 * 
	 * @param rootNode
	 * @param nodePath
	 * @param nodeName
	 */
	public static void addElementAsChild(Node rootNode, String nodePath,
			String nodeName) {

		Node node = rootNode.getOwnerDocument().createElement(nodeName);
		addNodeAsChild(rootNode, nodePath, node);
	}

	/**
	 * Adds the element as sibling.
	 * 
	 * @param rootNode
	 * @param nodePath
	 * @param nodeName
	 */
	public static void addElementAsSibling(Node rootNode, String nodePath,
			String nodeName) {

		Node node = rootNode.getOwnerDocument().createElement(nodeName);
		addNodeAsSibling(rootNode, nodePath, node);
	}

	/**
	 * Adds the element as root.
	 * 
	 * @param rootNode
	 * @param nodeName
	 * @param nameSpaceURI
	 */
	public static void addElementAsRoot(Node rootNode, String nodeName,
			String nameSpaceURI) {

		Document document = rootNode.getOwnerDocument();
		Element newRootNode = document.createElementNS(nameSpaceURI, nodeName);
		newRootNode.appendChild(rootNode);
		document.appendChild(newRootNode);
	}

	/**
	 * Adds the text node.
	 * 
	 * @param rootNode
	 * @param nodePath
	 * @param nodeValue
	 */
	public static void addText(Node rootNode, String nodePath, String nodeValue) {

		Node node = rootNode.getOwnerDocument().createTextNode(nodeValue);
		addNodeAsChild(rootNode, nodePath, node);
	}

	/**
	 * Adds the node as child.
	 * 
	 * @param rootNode
	 * @param nodePath
	 * @param node
	 */
	public static void addNodeAsChild(Node rootNode, String nodePath, Node node) {

		if (nodePath.contains(",")) {

			Iterator<Node> itr = getNodesForPath(rootNode, nodePath).iterator();
			while (itr.hasNext()) {
				itr.next().appendChild(node.cloneNode(true));
			}

		} else {
			NodeList nodeList = rootNode.getOwnerDocument()
					.getElementsByTagName(nodePath);
			for (int i = 0; i < nodeList.getLength(); i++) {
				nodeList.item(i).appendChild(node.cloneNode(true));
			}
		}
	}

	/**
	 * Adds the node as sibling.
	 * 
	 * @param rootNode
	 * @param nodePath
	 * @param node
	 */
	public static void addNodeAsSibling(Node rootNode, String nodePath,
			Node node) {

		Node refNode;
		if (nodePath.contains(",")) {

			Iterator<Node> itr = getNodesForPath(rootNode, nodePath).iterator();
			while (itr.hasNext()) {
				refNode = itr.next();
				refNode.getParentNode().insertBefore(node.cloneNode(true),
						refNode);
			}

		} else {
			NodeList nodeList = rootNode.getOwnerDocument()
					.getElementsByTagName(nodePath);
			for (int i = 0; i < nodeList.getLength(); i++) {
				refNode = nodeList.item(i);
				refNode.getParentNode().insertBefore(node.cloneNode(true),
						refNode);
			}
		}
	}

	/**
	 * Removes the attribute.
	 * 
	 * @param rootNode
	 * @param nodePath
	 * @param attrName
	 */
	public static void removeAttribute(Node rootNode, String nodePath,
			String attrName) {

		if (nodePath.contains(",")) {

			Iterator<Node> itr = getNodesForPath(rootNode, nodePath).iterator();
			while (itr.hasNext()) {
				((Element) itr.next()).removeAttribute(attrName);
			}

		} else {
			NodeList nodeList = rootNode.getOwnerDocument()
					.getElementsByTagName(nodePath);
			for (int i = 0; i < nodeList.getLength(); i++) {
				((Element) nodeList.item(i)).removeAttribute(attrName);
			}
		}
	}

	/**
	 * Removes the node.
	 * 
	 * @param rootNode
	 * @param nodePath
	 */
	public static void removeNode(Node rootNode, String nodePath) {

		Node refNode;
		if (nodePath.contains(",")) {

			Iterator<Node> itr = getNodesForPath(rootNode, nodePath).iterator();
			while (itr.hasNext()) {
				refNode = itr.next();
				refNode.getParentNode().removeChild(refNode);
			}

		} else {
			NodeList nodeList = rootNode.getOwnerDocument()
					.getElementsByTagName(nodePath);
			for (; nodeList.getLength() > 0;) {
				refNode = nodeList.item(0);
				refNode.getParentNode().removeChild(refNode);
			}
		}
	}

	/**
	 * Renames the attribute.
	 * 
	 * @param rootNode
	 * @param nodePath
	 * @param attrName
	 * @param newName
	 */
	public static void renameAttribute(Node rootNode, String nodePath,
			String attrName, String newName) {

		Node attrNode;
		if (nodePath.contains(",")) {

			Iterator<Node> itr = getNodesForPath(rootNode, nodePath).iterator();
			while (itr.hasNext()) {
				attrNode = ((Element) itr.next()).getAttributeNode(attrName);

				if (attrNode != null) {
					renameNode(attrNode, newName);
				}
			}

		} else {
			NodeList nodeList = rootNode.getOwnerDocument()
					.getElementsByTagName(nodePath);

			for (int i = 0; i < nodeList.getLength(); i++) {
				attrNode = ((Element) nodeList.item(i))
						.getAttributeNode(attrName);

				if (attrNode != null) {
					renameNode(attrNode, newName);
				}
			}
		}
	}

	/**
	 * Renames the node.
	 * 
	 * @param rootNode
	 * @param nodePath
	 * @param newName
	 */
	public static void renameNode(Node rootNode, String nodePath, String newName) {

		if (nodePath.contains(",")) {

			Iterator<Node> itr = getNodesForPath(rootNode, nodePath).iterator();
			while (itr.hasNext()) {
				renameNode(itr.next(), newName);
			}

		} else {
			NodeList nodeList = rootNode.getOwnerDocument()
					.getElementsByTagName(nodePath);
			for (int i = 0; i < nodeList.getLength(); i++) {
				renameNode(nodeList.item(i), newName);
			}
		}
	}

	/**
	 * Renames the node.
	 * 
	 * @param node
	 * @param newName
	 */
	public static void renameNode(Node node, String newName) {
		node.getOwnerDocument().renameNode(node, node.getNamespaceURI(),
				newName);
	}

	/**
	 * Changes the attribute value.
	 * 
	 * @param rootNode
	 * @param nodePath
	 * @param attrName
	 * @param attrValue
	 */
	public static void changeAttributeValue(Node rootNode, String nodePath,
			String attrName, String attrValue) {

		addAttribute(rootNode, nodePath, attrName, attrValue);
	}

	/**
	 * Changes the node Value.
	 * 
	 * @param rootNode
	 * @param nodePath
	 * @param newValue
	 */
	public static void changeNodeValue(Node rootNode, String nodePath,
			String newValue) {

		if (nodePath.contains(",")) {

			Iterator<Node> itr = getNodesForPath(rootNode, nodePath).iterator();
			while (itr.hasNext()) {
				changeNodeValue(itr.next(), newValue);
			}

		} else {
			NodeList nodeList = rootNode.getOwnerDocument()
					.getElementsByTagName(nodePath);
			for (int i = 0; i < nodeList.getLength(); i++) {
				changeNodeValue(nodeList.item(i), newValue);
			}
		}
	}

	/**
	 * Changes the node value.
	 * 
	 * @param node
	 * @param newValue
	 */
	public static void changeNodeValue(Node node, String newValue) {
		if (node.getNodeType() == Node.ELEMENT_NODE) {
			NodeList childList = node.getChildNodes();
			for (int i = 0; i < childList.getLength(); i++) {
				if (childList.item(i).getNodeType() == Node.TEXT_NODE) {
					childList.item(i).setNodeValue(newValue);
					break;
				}
			}
		} else {
			node.setNodeValue(newValue);
		}
	}

	/**
	 * Changes the attribute value if it matches the given value.
	 * 
	 * @param rootNode
	 * @param nodePath
	 * @param attrName
	 * @param matchValue
	 * @param newValue
	 */
	public static void changeAttributeValueMatch(Node rootNode,
			String nodePath, String attrName, String matchValue, String newValue) {

		Element element;
		if (nodePath.contains(",")) {

			Iterator<Node> itr = getNodesForPath(rootNode, nodePath).iterator();
			while (itr.hasNext()) {
				element = (Element) itr.next();
				if (element.getAttribute(attrName).equals(matchValue)) {
					element.setAttribute(attrName, newValue);
				}
			}

		} else {
			NodeList nodeList = rootNode.getOwnerDocument()
					.getElementsByTagName(nodePath);
			for (int i = 0; i < nodeList.getLength(); i++) {
				element = (Element) nodeList.item(i);
				if (element.getAttribute(attrName).equals(matchValue)) {
					element.setAttribute(attrName, newValue);
				}
			}
		}
	}

	/**
	 * Changes the node value if it matches the given value.
	 * 
	 * @param rootNode
	 * @param nodePath
	 * @param matchValue
	 * @param newValue
	 */
	public static void changeNodeValueMatch(Node rootNode, String nodePath,
			String matchValue, String newValue) {

		if (nodePath.contains(",")) {

			Iterator<Node> itr = getNodesForPath(rootNode, nodePath).iterator();
			while (itr.hasNext()) {
				changeNodeValueMatch(itr.next(), matchValue, newValue);
			}

		} else {
			NodeList nodeList = rootNode.getOwnerDocument()
					.getElementsByTagName(nodePath);
			for (int i = 0; i < nodeList.getLength(); i++) {
				changeNodeValueMatch(nodeList.item(i), matchValue, newValue);
			}
		}
	}

	/**
	 * Changes the node value if it matches the given value.
	 * 
	 * @param node
	 * @param matchValue
	 * @param newValue
	 */
	public static void changeNodeValueMatch(Node node, String matchValue,
			String newValue) {

		if (node.getNodeType() == Node.ELEMENT_NODE) {
			NodeList childList = node.getChildNodes();
			for (int i = 0; i < childList.getLength(); i++) {
				if (childList.item(i).getNodeType() == Node.TEXT_NODE) {
					if (childList.item(i).getNodeValue().equals(matchValue)) {
						childList.item(i).setNodeValue(newValue);
					}
					break;
				}
			}
		} else {
			if (node.getNodeValue().equals(matchValue)) {
				node.setNodeValue(newValue);
			}
		}
	}

	/**
	 * Changes the node value if it matches the given value.
	 * 
	 * @param node
	 * @param matchValue
	 * @param newValue
	 */
	public static void changeNodeValueMatchContain(Node node, String matchValue,
			String newValue) {

		if (node.getNodeType() == Node.ELEMENT_NODE) {
			NodeList childList = node.getChildNodes();
			for (int i = 0; i < childList.getLength(); i++) {
				if (childList.item(i).getNodeType() == Node.TEXT_NODE) {
					if (childList.item(i).getNodeValue().contains(matchValue)) {
						childList.item(i).setNodeValue(
								childList.item(i).getNodeValue().replaceAll(
										matchValue, newValue));
					}
					break;
				}
			}
		} else {
			if (node.getNodeValue().contains(matchValue)) {
				node.setNodeValue(newValue);
				node.setNodeValue(node.getNodeValue().replaceAll(matchValue,
						newValue));
			}
		}
	}

	public static void moveAttributeAtLevel(Node rootNode, String nodePath,
			int level) {
	}

	/**
	 * Moves the attribute to the target path. (Only for the single source and target)
	 * 
	 * @param rootNode
	 * @param nodePath
	 * @param targetPath
	 * @param attrName
	 */
	public static void moveAttribute(Node rootNode, String nodePath,
			String targetPath, String attrName) {

		Element node, targetNode;

		if (nodePath.contains(",")) {
			node = (Element) getNodesForPath(rootNode, nodePath).get(0);

		} else {
			node = (Element) rootNode.getOwnerDocument().getElementsByTagName(
					nodePath).item(0);
		}

		if (targetPath.contains(",")) {
			targetNode = (Element) getNodesForPath(rootNode, nodePath).get(0);

		} else {
			targetNode = (Element) rootNode.getOwnerDocument()
					.getElementsByTagName(nodePath).item(0);
		}

		if(node != null && targetNode != null) {
			targetNode.setAttributeNode(node.getAttributeNode(attrName));
		}
	}

	public static void moveNodeAtLevel(Node rootNode, String nodePath, int level) {
	}

	/**
	 * Moves the node as child of the target. (Only for the single source and target)
	 * 
	 * @param rootNode
	 * @param nodePath
	 * @param targetPath
	 */
	public static void moveNodeAsChild(Node rootNode, String nodePath,
			String targetPath) {

		Node node, targetNode;

		if (nodePath.contains(",")) {
			node = getNodesForPath(rootNode, nodePath).get(0);

		} else {
			node = rootNode.getOwnerDocument().getElementsByTagName(nodePath)
					.item(0);
		}

		if (targetPath.contains(",")) {
			targetNode = getNodesForPath(rootNode, nodePath).get(0);

		} else {
			targetNode = rootNode.getOwnerDocument().getElementsByTagName(
					nodePath).item(0);
		}

		if(node != null && targetNode != null) {
			targetNode.appendChild(node);
		}
	}

	/**
	 * Moves the node as sibling of the target. (Only for the single source and target)
	 * 
	 * @param rootNode
	 * @param nodePath
	 * @param targetPath
	 */
	public static void moveNodeAsSibling(Node rootNode, String nodePath,
			String targetPath) {

		Node node, targetNode;

		if (nodePath.contains(",")) {
			node = getNodesForPath(rootNode, nodePath).get(0);

		} else {
			node = rootNode.getOwnerDocument().getElementsByTagName(nodePath)
					.item(0);
		}

		if (targetPath.contains(",")) {
			targetNode = getNodesForPath(rootNode, nodePath).get(0);

		} else {
			targetNode = rootNode.getOwnerDocument().getElementsByTagName(
					nodePath).item(0);
		}

		if(node != null && targetNode != null) {
			targetNode.getParentNode().insertBefore(node, targetNode);
		}
	}

	/**
	 * Moves Node for the given Path.
	 * 
	 * @param rootNode
	 * @param nodePath
	 * @param relSource
	 * @param relTarget
	 */
	public static void moveNodeForPath(Node rootNode, String nodePath, String relSource, String relTarget) {

		Node node, targetNode;
		List<Node> sourceNodeList, targetNodeList;

		if (nodePath.contains(",")) {
			List<Node> nodeList = getNodesForPath(rootNode, nodePath);

			for(int i=0 ; i<nodeList.size() ; i++) {
				node = nodeList.get(i);
				sourceNodeList = getNodesForPath(node, relSource);
				targetNodeList = getNodesForPath(node, relTarget);
				if(targetNodeList.size() == 0) {
					continue;
				}
				targetNode = targetNodeList.get(0);

				Iterator<Node> itr = sourceNodeList.iterator();
				while(itr.hasNext()) {
					targetNode.appendChild(itr.next());
				}
			}

		} else {
			NodeList nodeList = rootNode.getOwnerDocument().getElementsByTagName(nodePath);

			for(int i=0 ; i<nodeList.getLength() ; i++) {
				node = nodeList.item(i);
				sourceNodeList = getNodesForPath(node, relSource);
				targetNodeList = getNodesForPath(node, relTarget);
				if(targetNodeList.size() == 0) {
					continue;
				}
				targetNode = targetNodeList.get(0);

				Iterator<Node> itr = sourceNodeList.iterator();
				while(itr.hasNext()) {
					targetNode.appendChild(itr.next());
				}
			}
		}
	}

	/**
	 * Moves Node for the given Path. Attributes are converted to Element.
	 * 
	 * @param rootNode
	 * @param nodePath
	 * @param relSource
	 * @param relTarget
	 */
	public static void moveAttrToElementForPath(Node rootNode, String nodePath, String relSource, String relTarget) {

		Node node, targetNode;
		Element newElement, sourceNode;
		List<Node> sourceNodeList;
		int index = relSource.lastIndexOf(",");
		String attrName = relSource.substring(index + 1, relSource.length());
		relSource = relSource.substring(0, index);

		if (nodePath.contains(",")) {
			List<Node> nodeList = getNodesForPath(rootNode, nodePath);

			for(int i=0 ; i<nodeList.size() ; i++) {
				node = nodeList.get(i);
				sourceNodeList = getNodesForPath(node, relSource);
				targetNode = getNodesForPath(node, relTarget).get(0);

				Iterator<Node> itr = sourceNodeList.iterator();
				while(itr.hasNext()) {
					sourceNode = (Element) itr.next();
					newElement = rootNode.getOwnerDocument().createElement(attrName);
					newElement.setTextContent(sourceNode.getAttribute(attrName));
					targetNode.appendChild(newElement);
					sourceNode.removeAttribute(attrName);
				}
			}

		} else {
			NodeList nodeList = rootNode.getOwnerDocument().getElementsByTagName(nodePath);

			for(int i=0 ; i<nodeList.getLength() ; i++) {
				node = nodeList.item(i);
				sourceNodeList = getNodesForPath(node, relSource);
				targetNode = getNodesForPath(node, relTarget).get(0);

				Iterator<Node> itr = sourceNodeList.iterator();
				while(itr.hasNext()) {
					sourceNode = (Element) itr.next();
					newElement = rootNode.getOwnerDocument().createElement(attrName);
					newElement.setTextContent(sourceNode.getAttribute(attrName));
					targetNode.appendChild(newElement);
					sourceNode.removeAttribute(attrName);
				}
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

	/**
	 * Calls the handler.
	 * 
	 * @param project
	 * @param detailsArray
	 */
	@SuppressWarnings("unchecked")
	public static void callHandler(IProject project, String[] detailsArray) {

		Class handlerClass = ClovisUtils.loadClass(detailsArray[0]);
		Class[] argType = { IProject.class };
		Object[] args = { project };

		try {
			if (handlerClass != null) {
				switch (detailsArray.length) {
				case 1:
					AbstractMigrationHandler handler = (AbstractMigrationHandler) handlerClass
							.getConstructor(argType).newInstance(args);
					handler.migrate();
					break;

				case 2:
					Method met = handlerClass.getMethod(detailsArray[1],
							argType);
					met.invoke(null, args);
					break;
				}
			}

		} catch (NoSuchMethodException e) {
			reportProblem(PROBLEM_SEVERITY_ERROR,
					"Class does not have migrate() method", e);
		} catch (Throwable th) {
			reportProblem(PROBLEM_SEVERITY_ERROR,
					"Unhandled error while calling migration handler:"
							+ detailsArray[0], th);
		}
	}

	/**
	 * Normalizes te element.
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
	 * Returns all the IDE files for the given project.
	 * 
	 * @param project
	 * @return
	 */
	public static List<String> getAllIDEFiles(IProject project) {
		List<String> fileList = new ArrayList<String>();

		String file;
		try {
			IResource resource[] = project.getFolder("configs").members();
			for(int i=0 ; i<resource.length ; i++) {
				file = resource[i].getLocation().toOSString();
				if(new File(file).isFile()) {
					fileList.add(file);
				}
			}

			resource = project.getFolder("models").members();
			for(int i=0 ; i<resource.length ; i++) {
				file = resource[i].getLocation().toOSString();
				if(new File(file).isFile()) {
					fileList.add(file);
				}
			}

			resource = project.getFolder("idl").members();
			for(int i=0 ; i<resource.length ; i++) {
				file = resource[i].getLocation().toOSString();
				if(new File(file).isFile()) {
					fileList.add(file);
				}
			}

		} catch (CoreException e) {
		}
		return fileList;
	}

	/**
	 * Auto Arranges the Editors. Editors should be opened before calling this
	 * method.
	 * 
	 * @param project
	 */
	public static void autoArrangeEditors(IProject project) {
		ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(project);

		GenericEditor editor = ((GenericEditorInput) pdm.getCAEditorInput())
				.getEditor();
		BaseDiagramEditPart part = (BaseDiagramEditPart) editor.getViewer()
				.getRootEditPart().getContents();
		new AutoArrangeCommand(part).execute();
		editor.propertyChange(null);
		editor.doSave(null);

		editor = ((GenericEditorInput) pdm.getComponentEditorInput())
				.getEditor();
		part = (BaseDiagramEditPart) editor.getViewer().getRootEditPart()
				.getContents();
		new AutoArrangeCommand(part).execute();
		editor.propertyChange(null);
		editor.doSave(null);
	}

	/**
	 * Reports the problem to the appropriate logging place.
	 * 
	 * @param severity
	 * @param message
	 * @param t
	 */
	public static void reportProblem(String severity, String message,
			Throwable t) {

		if (UtilsPlugin.isCmdToolRunning()) {
			System.out.println(severity + "\t\t" + message + "\t\t"
					+ (t != null ? t.getMessage() : ""));

		} else {

			if (severity.equals(PROBLEM_SEVERITY_ERROR)) {
				LOG.error("Migration : " + message, t);
			} else if (severity.equals(PROBLEM_SEVERITY_WARNING)) {
				LOG.warn("Migration : " + message, t);
			}
		}
	}
}

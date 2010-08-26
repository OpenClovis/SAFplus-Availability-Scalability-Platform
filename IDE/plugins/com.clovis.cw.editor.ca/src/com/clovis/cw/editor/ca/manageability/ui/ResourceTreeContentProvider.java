package com.clovis.cw.editor.ca.manageability.ui;

import java.util.ArrayList;
import java.util.List;
import java.util.Vector;

import org.eclipse.jface.viewers.IStructuredContentProvider;
import org.eclipse.jface.viewers.ITreeContentProvider;
import org.eclipse.jface.viewers.Viewer;

import com.clovis.cw.editor.ca.manageability.common.ResourceTreeNode;
import com.ireasoning.util.MibTreeNode;

public class ResourceTreeContentProvider implements ITreeContentProvider, IStructuredContentProvider {
	/**
	 * @param v -
	 *            Viewer
	 * @param oldInput
	 *            Object
	 * @param newInput
	 *            Object
	 */

	public void inputChanged(Viewer v, Object oldInput, Object newInput) {
	}

	/**
	 * Does nothing
	 */
	public void dispose() {
	}

	/**
	 * @param parent -
	 *            Object
	 * @return the top level elements in the tree.
	 */
	public Object[] getElements(Object parent) {
		Vector elements = (Vector) parent;
		if(elements.size() == 0) {
			ResourceTreeNode node = new ResourceTreeNode("Resource Type Browser", null, null, null);
			elements.add(node);
		}
		return elements.toArray();
	}

	/**
	 * @param child
	 *            Object
	 * @return the parent of TreeNode
	 */
	public Object getParent(Object child) {
		ResourceTreeNode node = (ResourceTreeNode) child;
		return node.getParent();
	}

	/**
	 * @param parent
	 *            Object Checks whether parent has any children
	 * @return true if parent has children else false.
	 */
	public boolean hasChildren(Object parent) {
		ResourceTreeNode node = (ResourceTreeNode) parent;
		if(node.getNode() != null)
			return (node.getNode().getChildNodes().size() > 0);
		else if(node.getChildNodes().size() > 0) {
			return true;
		}
		return true;
	}

	/**
	 * @param parent
	 *            Object
	 * @return the child nodes of parent TreeNode
	 */
	public Object[] getChildren(Object parent) {
		ResourceTreeNode node = (ResourceTreeNode) parent;
		Vector<ResourceTreeNode> elements = new Vector <ResourceTreeNode>();
		if (node.getNode() != null) {
			ArrayList<MibTreeNode> childs = node.getNode().getChildNodes();
			for (int i = 0; i < childs.size(); i++) {
				MibTreeNode child = childs.get(i);
				if (!child.isSnmpV2TrapNode() && (child.isTableNode() || (child.isGroupNode() && isGroupWithScalar(child)) /*|| isValidNode(child)*/)) {
					ResourceTreeNode resNode = new ResourceTreeNode(child
							.getName()
							+ "", child, node, node.getMibFileName());
					node.addChild(resNode);
					resNode.setParent(node);
					elements.add(resNode);
				} else {
					StringBuffer nodePath = new StringBuffer("");
					MibTreeNode compressedParentNode = getValidNode(child, nodePath);
					if(compressedParentNode != null) {
						ResourceTreeNode resNode = new ResourceTreeNode(nodePath.toString(), compressedParentNode, node, node.getMibFileName());
						node.addChild(resNode);
						resNode.setParent(node);
						elements.add(resNode);
					}
				}
			}
		} else if (node.getChildNodes().size() > 0) {
			ArrayList<ResourceTreeNode> childs = node.getChildNodes();
			for (int i = 0; i < childs.size(); i++) {
				elements.add(childs.get(i));
				childs.get(i).setParent(node);
			}
		}
		return elements.toArray();
	}
	/**
	 * Verify if the node is valid to display in the Tree
	 * @param node
	 * @return true if the node is valid to display in the Tree
	 */
	private boolean isValidNode(MibTreeNode node) {
		if (node.getChildNodeCount() == 0) {
			return false;
		} else {
			return isContainValidChild(node);
		}
	}
	/**
	 * Verify if the node is valid children to display in the Tree
	 * @param node
	 * @return true if node contains any valid child else false.
	 */
	private boolean isContainValidChild(MibTreeNode node) {
		List<MibTreeNode> children = node.getChildNodes();
		for (int j = 0; j < children.size(); j++) {
			MibTreeNode child = children.get(j);
			if (child.isTableNode()
					|| (child.isGroupNode() && isGroupWithScalar(child))) {
				return true;
			} else {
				if(isContainValidChild(child)) {
					return true;
				}
			}
		}
		return false;
	}
	/**
	 * Verify if the node is valid to display in the Tree
	 * @param node
	 * @return true if the node is valid to display in the Tree
	 */
	private MibTreeNode getValidNode(MibTreeNode node, StringBuffer nodePath) {
		if (node.getChildNodeCount() == 0) {
			return null;
		} else {
			return getValidChildNode(node, nodePath);
		}
	}
	/**
	 * Verify if the node is valid children to display in the Tree
	 * @param node
	 * @return true if node contains any valid child else false.
	 */
	private MibTreeNode getValidChildNode(MibTreeNode node, StringBuffer nodePath) {
		if(nodePath.toString().equals("")) {
			nodePath.append(node.getName());
		} else {
			nodePath.append(".").append(node.getName());
		}
		MibTreeNode compressedParentNode = node;
		List<MibTreeNode> children = node.getChildNodes();
		if(children.size() > 1) {
			if(isValidNode(compressedParentNode)) {
				return compressedParentNode;
			}
		}
		for (int j = 0; j < children.size(); j++) {
			MibTreeNode child = children.get(j);
			if (child.isTableNode()
					|| (child.isGroupNode() && isGroupWithScalar(child))) {
				return compressedParentNode;
			} else {
				compressedParentNode = getValidChildNode(child, nodePath);
				if(compressedParentNode != null) {
					return compressedParentNode;
				}
			}
		}
		return null;
	}
	/**
	 * Verify if the Group node contains any scalar node
	 * @param node
	 * @return true if node contains scalar nodes else false.
	 */
	private boolean isGroupWithScalar(MibTreeNode node){
		List children = node.getChildNodes();
		for (int k = 0; k < children.size(); k++) {
			MibTreeNode child = (MibTreeNode) children.get(k);
			if (child.isScalarNode()) {
				return true;
			}
		}
		return false;
	}
}
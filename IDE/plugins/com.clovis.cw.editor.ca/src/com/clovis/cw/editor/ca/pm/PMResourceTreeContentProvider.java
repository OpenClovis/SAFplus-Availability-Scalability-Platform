/**
 * 
 */
package com.clovis.cw.editor.ca.pm;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import org.eclipse.jface.viewers.IStructuredContentProvider;
import org.eclipse.jface.viewers.ITreeContentProvider;
import org.eclipse.jface.viewers.Viewer;

import com.clovis.cw.editor.ca.manageability.common.ResourceTreeNode;
import com.ireasoning.util.MibTreeNode;

/**
 * Content Provider for PM Editor Resource tree.
 * 
 * @author Suraj Rajyaguru.
 * 
 */
public class PMResourceTreeContentProvider implements ITreeContentProvider,
		IStructuredContentProvider {

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#getParent(java.lang.Object)
	 */
	public Object getParent(Object element) {
		ResourceTreeNode node = (ResourceTreeNode) element;
		return node.getParent();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#hasChildren(java.lang.Object)
	 */
	public boolean hasChildren(Object element) {
		ResourceTreeNode node = (ResourceTreeNode) element;

		if (node.getNode() != null)
			return (node.getNode().getChildNodes().size() > 0);
		else if (node.getChildNodes().size() > 0) {
			return true;
		}

		return true;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#getChildren(java.lang.Object)
	 */
	public Object[] getChildren(Object parent) {
		ResourceTreeNode node = (ResourceTreeNode) parent;
		Vector<ResourceTreeNode> elements = new Vector<ResourceTreeNode>();
		MibTreeNode column;

		if (node.getNode() != null) {
			ArrayList<MibTreeNode> childs = node.getNode().getChildNodes();

			for (int i = 0; i < childs.size(); i++) {
				MibTreeNode child = childs.get(i);

				if (!child.isSnmpV2TrapNode()) {
					if (child.isTableEntryNode()) {
						Iterator<MibTreeNode> tableColumns = child
								.getChildNodes().iterator();
						while (tableColumns.hasNext()) {
							column = tableColumns.next();
							ResourceTreeNode resNode = new ResourceTreeNode(
									column.getName().toString(), column, node,
									node.getMibFileName());
							node.addChild(resNode);
							resNode.setParent(node);
							elements.add(resNode);
						}

					} else if (child.isTableNode()
							|| (child.isGroupNode() && isGroupWithScalar(child))
							|| child.isScalarNode()) {
						ResourceTreeNode resNode = new ResourceTreeNode(child
								.getName()
								+ "", child, node, node.getMibFileName());
						node.addChild(resNode);
						resNode.setParent(node);
						elements.add(resNode);

					} else {
						StringBuffer compressedPath = new StringBuffer("");
						MibTreeNode compressedParentNode = getValidChild(child,
								compressedPath);

						if (compressedParentNode != null) {
							ResourceTreeNode resNode = new ResourceTreeNode(
									compressedPath.toString(),
									compressedParentNode, node, node
											.getMibFileName());
							node.addChild(resNode);
							resNode.setParent(node);
							elements.add(resNode);
						}
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

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.IStructuredContentProvider#getElements(java.lang.Object)
	 */
	public Object[] getElements(Object inputElement) {
		Vector elements = (Vector) inputElement;

		if (elements.size() == 0) {
			ResourceTreeNode mibResNode = new ResourceTreeNode("Mib Resources",
					null, null, null);
			elements.add(mibResNode);
		}

		return elements.toArray();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.IContentProvider#dispose()
	 */
	public void dispose() {
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.IContentProvider#inputChanged(org.eclipse.jface.viewers.Viewer,
	 *      java.lang.Object, java.lang.Object)
	 */
	public void inputChanged(Viewer viewer, Object oldInput, Object newInput) {
	}

	/**
	 * Returns valid child of the given node.
	 * 
	 * @param node
	 * @param compressedPath
	 * @return
	 */
	private MibTreeNode getValidChild(MibTreeNode node,
			StringBuffer compressedPath) {

		if (compressedPath.toString().equals("")) {
			compressedPath.append(node.getName());
		} else {
			compressedPath.append(".").append(node.getName());
		}

		List<MibTreeNode> children = node.getChildNodes();
		for (int j = 0; j < children.size(); j++) {
			MibTreeNode child = children.get(j);

			if (child.isTableNode()
					|| (child.isGroupNode() && isGroupWithScalar(child))) {
				return node;

			} else {
				node = getValidChild(child, compressedPath);
				if (node != null) {
					return node;
				}
			}
		}

		return null;
	}

	/**
	 * Verify if the Group node contains any scalar node
	 * 
	 * @param node
	 * @return true if node contains scalar nodes else false.
	 */
	private boolean isGroupWithScalar(MibTreeNode node) {
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

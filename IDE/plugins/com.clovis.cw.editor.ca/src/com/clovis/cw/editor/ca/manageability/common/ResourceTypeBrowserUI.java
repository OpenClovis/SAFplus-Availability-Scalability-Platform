package com.clovis.cw.editor.ca.manageability.common;

import java.io.File;
import java.util.Vector;

import org.eclipse.jface.viewers.CheckboxTreeViewer;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Composite;

import com.ireasoning.util.MibTreeNode;

/**
 * 
 * TreeViewer to display the Resource Types
 * @author Pushparaj
 *
 */
public class ResourceTypeBrowserUI extends CheckboxTreeViewer {

	public ResourceTypeBrowserUI(Composite parent, int style, ClassLoader loader) {
		super(parent, style);
		setLabelProvider(new ResourceTreeLabelProvider());
	}
	/**
	 * Add MIB root node
	 * @param node
	 */
	public void addRootMibNode(ResourceTreeNode node){
		Vector<ResourceTreeNode> elements = (Vector) getInput();
		ResourceTreeNode root = (ResourceTreeNode)elements.get(0);
		root.addChild(node);
		node.setParent(root);
		refresh();
	}
	/**
	 * Remove MIB root node
	 * @param node
	 */
	public void removeRootMibNode(ResourceTreeNode node){
		Vector<ResourceTreeNode> elements = (Vector) getInput();
		ResourceTreeNode root = (ResourceTreeNode)elements.get(0);
		root.removeChild(node);
		refresh();
	}
	/**
	 * Populates the Resource Type Browser
	 * @param node root node for the MIB
	 * @param fileName mib file name
	 */
	public void buildAvailableResourceTreeForMibNode(MibTreeNode node, String filePath) {
		String fileName = new File(filePath).getName();
		ResourceTreeNode mibNode = new ResourceTreeNode(fileName, null, null, filePath);
		ResourceTreeNode resNode = new ResourceTreeNode(node.getName() + "", node, mibNode, filePath);
    	mibNode.addChild(resNode);
    	addRootMibNode(mibNode);
	}
	
	/**
	 * Label Provider
	 * @author Pushparaj
	 *
	 */
	class ResourceTreeLabelProvider extends LabelProvider {
		/**
	     * @param obj Object
	     * @return image to be shown for the object
	     */
	    public Image getImage(Object obj)
	    {
	        return null;
	    }
	    /**
	     * @param obj
	     *            Object
	     * @return label for the tree node.
	     */
	     public String getText(Object obj)
	     {
	    	 ResourceTreeNode node = (ResourceTreeNode) obj;
	    	 return node.getName();
	     }
	}
	
}
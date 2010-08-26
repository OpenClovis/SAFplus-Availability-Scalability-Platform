package com.clovis.cw.editor.ca.manageability.common;


import java.util.ArrayList;

import com.ireasoning.util.MibTreeNode;

/**
 * 
 * @author Pushparaj
 *
 */
public class ResourceTreeNode {
	String _name, _mibFileName;
	MibTreeNode _mibNode;
	ResourceTreeNode _parent;
	ArrayList<ResourceTreeNode> _childNodes = new ArrayList<ResourceTreeNode>();
	public ResourceTreeNode(String name, MibTreeNode mibNode, ResourceTreeNode parent, String fileName){
		_name = name;
		_mibNode = mibNode;
		_parent = parent;
		_mibFileName = fileName;
	}
	public MibTreeNode getNode() {
		return _mibNode;
	}
	public String getName() {
		return _name;
	}
	public ResourceTreeNode getParent() {
		return _parent;
	}
	public void setParent(ResourceTreeNode parent) {
		_parent = parent;
	}
	public void addChild(ResourceTreeNode child){
		_childNodes.add(child);
	}
	public void removeChild(ResourceTreeNode child){
		_childNodes.remove(child);
	}
	public ArrayList<ResourceTreeNode> getChildNodes(){
		return _childNodes;
	}
	public String getMibFileName() {
		return _mibFileName;
	}
}

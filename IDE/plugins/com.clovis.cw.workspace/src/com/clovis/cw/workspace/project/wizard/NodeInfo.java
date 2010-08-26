/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/project/wizard/NodeInfo.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.workspace.project.wizard;

/**
 * 
 * @author pushparaj
 * Node info
 */
public class NodeInfo {
	private String nodeName = "Node";

	private String nodeClass = (String) AddNodeWizardPage.CLASS_TYPES.get(0);

	/**
	 * Create a task with an initial description
	 * 
	 */
	public NodeInfo() {

	}
	/**
	 * Returns Node name
	 */
	public String getNodeName() {
		return nodeName;
	}
	/**
	 * Returns Node Class
	 */
	public String getNodeClass()
	{
		return nodeClass;
	}

	/**
	 * Set Node Name
	 * 
	 * @param name Node Name
	 */
	public void setNodeName(String name) {
		nodeName = name;
	}
	/**
	 * Set Node Class
	 * 
	 * @param name Node Class Name
	 */
	public void setNodeClass(String name) {
		nodeClass = name;
	}
}

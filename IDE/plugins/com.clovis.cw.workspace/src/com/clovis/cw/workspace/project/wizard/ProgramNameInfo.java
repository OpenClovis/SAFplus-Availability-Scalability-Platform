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
 * @author matt
 * Class to track a program name, the node type it is associated with,
 * and the sequence of the program name within the node type.
 */
public class ProgramNameInfo {

	private String _nodeTypeName;
	private String _programName;

	protected ProgramNameInfo(String nodeTypeName, String programName)
	{
		_nodeTypeName = nodeTypeName;
		_programName = programName;
	}

	/**
	 * Return the name of the node type
	 * @return The node type name
	 */
	public String getNodeTypeName()
	{
		return _nodeTypeName;
	}

	/**
	 * Set the node type name
	 * @param The node type name
	 */
	public void setNodeTypeName(String nodeTypeName)
	{
		_nodeTypeName = nodeTypeName;
	}

	/**
	 * Return the program name
	 * @return The program name
	 */
	public String getProgramName()
	{
		return _programName;
	}
	
	/**
	 * Set the program name
	 * @param programName
	 */
	public void setProgramName(String programName)
	{
		_programName = programName;
	}
	
	/**
	 * Override the string representation of this object.
	 * @return The program name contained in this object.
	 */
	public String toString()
	{
		return _programName;
	}
}

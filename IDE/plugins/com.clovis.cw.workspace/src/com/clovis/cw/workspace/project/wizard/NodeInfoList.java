/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/project/wizard/NodeInfoList.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.workspace.project.wizard;

import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;
import java.util.Vector;

/**
 * 
 * @author pushparaj
 * Node Info List
 */
public class NodeInfoList {
	private Vector tasks = new Vector();
	private Set changeListeners = new HashSet();
	/**
	 * Constructor
	 */
	public NodeInfoList() {

	}
	
	/**
	 * Return the collection of tasks
	 */
	public Vector getTasks() {
		return tasks;
	}
	
	/**
	 * Add a new task to the collection of tasks
	 */
	public void addTask() {
		NodeInfo task = new NodeInfo();
		Iterator iter = tasks.iterator();
		
		// only allow 1 system controllers
		int sysControllerCount = 0;
		while (iter.hasNext())
		{
			NodeInfo info = (NodeInfo)iter.next();
			if (info.getNodeClass().equals((String) AddNodeWizardPage.CLASS_TYPES.get(0)))
				sysControllerCount++;
		}
		if (sysControllerCount > 0) task.setNodeClass((String) AddNodeWizardPage.CLASS_TYPES.get(1));

		task.setNodeName(getNewNodeName());
		tasks.add(tasks.size(), task);
		Iterator iterator = changeListeners.iterator();
		while (iterator.hasNext())
			((ITaskListViewer) iterator.next()).addTask(task);
	}

	/**
	 * @param task
	 */
	public void removeTask(NodeInfo task) {
		tasks.remove(task);
		Iterator iterator = changeListeners.iterator();
		while (iterator.hasNext())
			((ITaskListViewer) iterator.next()).removeTask(task);
	}
	/**
	 * @param task
	 */
	public void taskChanged(NodeInfo task, String oldValue) {
		Iterator iterator = changeListeners.iterator();
		while (iterator.hasNext())
			((ITaskListViewer) iterator.next()).updateTask(task, oldValue);
	}

	/**
	 * @param viewer
	 */
	public void removeChangeListener(ITaskListViewer viewer) {
		changeListeners.remove(viewer);
	}

	/**
	 * @param viewer
	 */
	public void addChangeListener(ITaskListViewer viewer) {
		changeListeners.add(viewer);
	}
	/**
	 * Returns unique Node Name
	 * @return Node Name
	 */
	private String getNewNodeName()
	{
		HashSet set = new HashSet();
		for(int i = 0; i < tasks.size(); i++)
		{
			NodeInfo node = (NodeInfo) tasks.get(i);
			set.add(node.getNodeName());
		}
		int i = 0;
		String newName = null;
		do {
			newName = "Node" + String.valueOf(i);
			i++;
		} while (set.contains(newName));
		return newName;
	}
}

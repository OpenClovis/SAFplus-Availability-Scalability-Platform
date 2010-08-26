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

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;
import java.util.Vector;

/**
 * 
 * @author matt
 * Program Name Info List
 */
public class ProgramNameInfoList {
	private Vector _tasks = new Vector();
	private Set _changeListeners = new HashSet();
	
	private Vector _nodeNames = new Vector();
	
	/**
	 * Constructor
	 */
	public ProgramNameInfoList() {

	}
	
	/**
	 * Return the collection of tasks
	 */
	public Vector getTasks() {
		return _tasks;
	}
	
	/**
	 * Add a new task to the collection of tasks
	 */
	public void addTask(ProgramNameInfo task) {
		_tasks.add(_tasks.size(), task);
		Iterator iterator = _changeListeners.iterator();
		while (iterator.hasNext())
			((ITaskListViewer) iterator.next()).addTask(task);
	}

	/**
	 * @param task
	 */
	public void removeTask(ProgramNameInfo task) {
		_tasks.remove(task);
		Iterator iterator = _changeListeners.iterator();
		while (iterator.hasNext())
			((ITaskListViewer) iterator.next()).removeTask(task);
	}

	/**
	 * @param task
	 */
	public void removeTasks(ArrayList tasks)
	{
		for (int i=0; i<tasks.size(); i++)
		{
			ProgramNameInfo progName = (ProgramNameInfo) tasks.get(i);
			_tasks.remove(progName);
			Iterator iterator = _changeListeners.iterator();
			while (iterator.hasNext())
				((ITaskListViewer) iterator.next()).removeTask(progName);
		}
	}

	/**
	 * @param task
	 */
	public void taskChanged(ProgramNameInfo task, String oldValue) {
		Iterator iterator = _changeListeners.iterator();
		while (iterator.hasNext())
			((ITaskListViewer) iterator.next()).updateTask(task, oldValue);
	}

	/**
	 * @param viewer
	 */
	public void removeChangeListener(ITaskListViewer viewer) {
		_changeListeners.remove(viewer);
	}

	/**
	 * @param viewer
	 */
	public void addChangeListener(ITaskListViewer viewer) {
		_changeListeners.add(viewer);
	}
	
	public void addNodeName(String nodeName)
	{
		_nodeNames.add(nodeName);
	}
}

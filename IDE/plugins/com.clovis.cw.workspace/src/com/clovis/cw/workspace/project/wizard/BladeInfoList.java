/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/project/wizard/BladeInfoList.java $
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
 * BladeInfo List
 */
public class BladeInfoList {
	private Vector tasks = new Vector();
	private Set changeListeners = new HashSet();
	
	/**
	 * Constructor
	 */
	public BladeInfoList() {

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
		BladeInfo task = new BladeInfo();
		task.setBladeName(getNewBladeName());
		tasks.add(tasks.size(), task);
		Iterator iterator = changeListeners.iterator();
		while (iterator.hasNext())
			((ITaskListViewer) iterator.next()).addTask(task);
	}

	/**
	 * @param task
	 */
	public void removeTask(BladeInfo task) {
		tasks.remove(task);
		Iterator iterator = changeListeners.iterator();
		while (iterator.hasNext())
			((ITaskListViewer) iterator.next()).removeTask(task);
	}
	/**
	 * @param task
	 */
	public void taskChanged(BladeInfo task, String oldValue) {
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
	 * Returns unique Blade Name
	 * @return Node Name
	 */
	private String getNewBladeName()
	{
		HashSet set = new HashSet();
		for(int i = 0; i < tasks.size(); i++)
		{
			BladeInfo node = (BladeInfo) tasks.get(i);
			set.add(node.getBladeName());
		}
		int i = 0;
		String newName = null;
		do {
			newName = "Blade" + String.valueOf(i);
			i++;
		} while (set.contains(newName));
		return newName;
	}
}

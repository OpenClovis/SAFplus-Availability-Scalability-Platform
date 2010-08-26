/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/project/wizard/NodeCellModifier.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.workspace.project.wizard;

import java.util.Arrays;
import java.util.List;
import java.util.Vector;
import java.util.regex.Pattern;

import org.eclipse.jface.viewers.ICellModifier;
import org.eclipse.swt.widgets.TableItem;

/**
 * 
 * @author pushparaj
 * CellModifier for Table
 */
public class NodeCellModifier implements ICellModifier {

	private List _columnNames;
	private List _classTypes;
	private NodeInfoList _taskList;
	/**
	 * Constructor 
	 * @param TableViewerExample an instance of a TableViewerExample 
	 */
	public NodeCellModifier(String[] names, List types, NodeInfoList tasks) {
		super();
		_columnNames = Arrays.asList(names);
		_classTypes = types;
		_taskList = tasks;
	}

	/**
	 * @see org.eclipse.jface.viewers.ICellModifier#canModify(java.lang.Object, java.lang.String)
	 */
	public boolean canModify(Object element, String property) {
		return true;
	}

	/**
	 * @see org.eclipse.jface.viewers.ICellModifier#getValue(java.lang.Object, java.lang.String)
	 */
	public Object getValue(Object element, String property) {

		// Find the index of the column
		int columnIndex = _columnNames.indexOf(property);

		Object result = null;
		NodeInfo task = (NodeInfo) element;
		switch (columnIndex) {
		case 0:  
			result = String.valueOf(task.getNodeName());
			break;
		case 1:
			result = new Integer(_classTypes.indexOf(task.getNodeClass()));
			break;
		default:
			result = "";
		}
		return result;
	}

	/**
	 * @see org.eclipse.jface.viewers.ICellModifier#modify(java.lang.Object, java.lang.String, java.lang.Object)
	 */
	public void modify(Object element, String property, Object value) {
		if(value == null || value.equals(""))
			return;
		// Find the index of the column 
		int columnIndex = _columnNames.indexOf(property);

		TableItem item = (TableItem) element;
		NodeInfo task = (NodeInfo) item.getData();
		String valueString;

		String oldValue = new String();
		
		switch (columnIndex) {
		case 0: 
			oldValue = task.getNodeName();
			valueString = ((String) value).trim();
			if(isValidName(String.valueOf(value)))
				task.setNodeName(valueString);
			break;
		case 1: 
			oldValue = task.getNodeClass();
			task.setNodeClass((String)_classTypes.get(((Integer) value).intValue()));
			break;
		default:
		}
		_taskList.taskChanged(task, oldValue);
	}
	/**
	 * 
	 * @return
	 */
	private boolean isValidName(String nodeName) {
		if (!Pattern.compile("^[a-zA-Z][a-zA-Z0-9_]{0,79}$").matcher(
                nodeName).matches()) {
			return false;
        }
		Vector tasks = _taskList.getTasks();
		for (int i = 0; i < tasks.size(); i++) {
			NodeInfo node = (NodeInfo) tasks.get(i);
			if (nodeName.equals(node.getNodeName())) {
				return false;
			}
		}
		return true;
	}
}

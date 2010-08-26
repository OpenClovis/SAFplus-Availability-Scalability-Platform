/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/project/wizard/BladeCellModifier.java $
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
public class BladeCellModifier implements ICellModifier {

	private List _columnNames;
	private List _bladeTypes;
	private BladeInfoList _taskList;
	/**
	 * Constructor 
	 * @param TableViewerExample an instance of a TableViewerExample 
	 */
	public BladeCellModifier(String[] names, String[] types, BladeInfoList tasks) {
		super();
		_columnNames = Arrays.asList(names);
		_bladeTypes = Arrays.asList(types);
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
		BladeInfo task = (BladeInfo) element;

		switch (columnIndex) {
		case 0:  
			result = new Integer(_bladeTypes.indexOf(task.getBladeType()));
			break;
		case 1:
			result = task.getBladeName();
			break;
		case 2:  
			result = String.valueOf(task.getNumberOfBlades());
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
		BladeInfo task = (BladeInfo) item.getData();
		String valueString;
		String oldValue = new String();

		switch (columnIndex) {
		case 0: 
			oldValue = task.getBladeType();
			task.setBladeType((String)_bladeTypes.get(((Integer) value).intValue()));
			break;
		case 1: 
			oldValue = task.getBladeName();
			valueString = ((String) value).trim();
			if(isValidName(String.valueOf(value)))
				task.setBladeName(valueString);
			break;
		case 2: 
			oldValue = String.valueOf(task.getNumberOfBlades());
			valueString = ((String) value).trim();
			task.setNumberOfBlades(Integer.parseInt(valueString));
			break;
		default:
		}
		_taskList.taskChanged(task, oldValue);
	}
	/**
	 * 
	 * @return
	 */
	private boolean isValidName(String bladeName) {
		if (!Pattern.compile("^[a-zA-Z][a-zA-Z0-9_]{0,79}$").matcher(
                bladeName).matches()) {
			return false;
        }
		Vector tasks = _taskList.getTasks();
		for (int i = 0; i < tasks.size(); i++) {
			BladeInfo blade = (BladeInfo) tasks.get(i);
			if (bladeName.equals(blade.getBladeName())) {
				return false;
			}
		}
		return true;
	}
}

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
import java.util.Iterator;
import java.util.List;
import java.util.regex.Pattern;

import org.eclipse.jface.viewers.ICellModifier;
import org.eclipse.swt.widgets.TableItem;

/**
 * 
 * @author matt
 * CellModifier for Table
 */
public class ProgramNameCellModifier implements ICellModifier
{

	private List _columnNames;
	private ProgramNameInfoList _programNames;
	private List _nodeTypeList;

	/**
	 * Constructor 
	 * @param 
	 */
	public ProgramNameCellModifier(String[] names, ProgramNameInfoList programNames, List nodeTypeList)
	{
		super();
		_columnNames = Arrays.asList(names);
		_programNames = programNames;
		_nodeTypeList = nodeTypeList;
	}

	/**
	 * @see org.eclipse.jface.viewers.ICellModifier#canModify(java.lang.Object, java.lang.String)
	 */
	public boolean canModify(Object element, String property)
	{
		return true;
	}

	/**
	 * @see org.eclipse.jface.viewers.ICellModifier#getValue(java.lang.Object, java.lang.String)
	 */
	public Object getValue(Object element, String property)
	{

		// Find the index of the column
		int columnIndex = _columnNames.indexOf(property);

		Object result = null;
		ProgramNameInfo programName = (ProgramNameInfo) element;
		switch (columnIndex) {
		case 0:
			result = _nodeTypeList.indexOf(programName.getNodeTypeName());
			break;
		case 1:
			result = String.valueOf(programName.getProgramName());
			break;
		default:
			result = "";
		}
		return result;
	}

	/**
	 * @see org.eclipse.jface.viewers.ICellModifier#modify(java.lang.Object, java.lang.String, java.lang.Object)
	 */
	public void modify(Object element, String property, Object value)
	{
		if(value == null || value.equals(""))
			return;
		// Find the index of the column 
		int columnIndex = _columnNames.indexOf(property);

		TableItem item = (TableItem) element;
		ProgramNameInfo programName = (ProgramNameInfo) item.getData();
		String valueString;
		String oldValue = new String();

		switch (columnIndex) {
		case 0: 
			oldValue = programName.getNodeTypeName();

			int index = ((Integer)value).intValue();
			if (index >= 0 && index < _nodeTypeList.size())
			{
				programName.setNodeTypeName((String)_nodeTypeList.get((index)));
			}
			break;
		case 1: 
			oldValue = programName.getProgramName();
			valueString = ((String) value).trim();
			if(isValidName(String.valueOf(value)))
				programName.setProgramName(valueString);
			break;
		default:
		}
		_programNames.taskChanged(programName, oldValue);
	}
	
	/**
	 * 
	 * @return
	 */
	private boolean isValidName(String nodeName)
	{
		if (!Pattern.compile("^[a-zA-Z][a-zA-Z0-9_]{0,79}$").matcher(
                nodeName).matches()) {
			return false;
        }
		
		Iterator iter = _programNames.getTasks().iterator();
		while (iter.hasNext())
		{
			ProgramNameInfo info = (ProgramNameInfo)iter.next();
			if (nodeName.equals(info.getProgramName())) {
				return false;
			}
		}
		return true;
	}
}

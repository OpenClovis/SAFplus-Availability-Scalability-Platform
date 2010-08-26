/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.data/src/com/clovis/cw/project/data/TrackingModel.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.project.data;

import java.util.ArrayList;
import java.util.List;

import com.clovis.common.utils.ecore.Model;

/**
 * Class for maintaining model changes list.
 * @author pushparaj
 *
 */
public class TrackingModel
{
	private Model _trackModel;
	private List _addList, _removeList, _modifyList; 
	public TrackingModel(Model model)
	{
		_trackModel = model;
		_addList = new ArrayList();
		_removeList = new ArrayList();
		_modifyList = new ArrayList();
	}
	public Model getTrackModel()
	{
		return _trackModel;
	}
	public List getAddedList()
	{
		return _addList;
	}
	public List getRemovedList()
	{
		return _removeList;
	}
	public List getModifiedList()
	{
		return _modifyList;
	}
	/**
	 * Clear all Lists
	 *
	 */
	public void clearAllLists()
	{
		_trackModel.getEList().clear();
		_addList.clear();
		_removeList.clear();
		_modifyList.clear();
	}
	public void save(boolean save)
	{
		_trackModel.save(save);
	}
}

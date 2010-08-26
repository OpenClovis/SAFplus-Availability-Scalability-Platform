/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/menu/EnvironmentNotifierImpl.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.menu;

import java.util.ArrayList;
import java.util.List;

/**
 * @author shubhada
 *
 * Implements the interface EnvironmentNotifier.
 */
public class EnvironmentNotifierImpl implements EnvironmentNotifier {
	private List listeners = new ArrayList();
	/**
	 * Adds the Environment variable to Listener List.
	 * @param env
	 */
	public void addListener(Object env) {
		
		listeners.add(env);
	}
	/**
	 * Removes Environment variable from the Listener List.
	 * @param env
	 */
	public void removeListener(Object env) {
		
		listeners.remove(env);
	}
	/**
	 * calls the required method on change in the Environment. 
	 *
	 */
	public void fireEnvironmentChange(Object obj) {
		for(int i=0 ; i < listeners.size() ; i++)
		{
			((IEnvironmentListener) listeners.get(i)).valueChanged(obj);
		}
		
	}

}

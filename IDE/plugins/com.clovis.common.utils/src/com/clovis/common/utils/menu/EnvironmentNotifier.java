/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/menu/EnvironmentNotifier.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This Interface will allow adding/deleting of listeners on the 
 * environment related parameters.  
 */
package com.clovis.common.utils.menu;
public interface EnvironmentNotifier 
{
	/**
	 * Adds the Environment variable to Listener List.
	 * @param listner Listener to be added.
	 */
	public void addListener(Object listener);
	/**
	 * Removes Environment variable from the Listener List.
	 * @param listner Listener to be removed 
	 */
	public void removeListener(Object listener);
	/**
	 * Does the required things on change in the Environment. 
	 * @param property Changed property.
	 */
	public void fireEnvironmentChange(Object property);
}

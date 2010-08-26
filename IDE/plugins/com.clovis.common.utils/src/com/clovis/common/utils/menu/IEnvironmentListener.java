/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/menu/IEnvironmentListener.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Listener interface to listen on changes in Environment
 *******************************************************************************/
package com.clovis.common.utils.menu;
public interface IEnvironmentListener {
	/**
	 * Does the necessary changes on change in Environment parameter
	 * @param obj
	 */
	public void valueChanged(Object obj);

}

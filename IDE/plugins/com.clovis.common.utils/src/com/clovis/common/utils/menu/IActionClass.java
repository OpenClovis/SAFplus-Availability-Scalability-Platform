/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/menu/IActionClass.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.menu;
/**
 * This is the interface required to be
 * implemented by all MenuAction Classes.
 * @author nadeem
 */
public interface IActionClass
{
    /**
     * Actual run method.
     * @param args Menu Argument.
     */
    boolean run(Object[] args);
    /**
     * Visible status of Menu.
     * @param  environment Environment
     * @return Visible status of Menu.
     */
    boolean isVisible(Environment environment);
    /**
     * Enable status of Menu.
     * @param  environment Environment
     * @return Enable status of Menu.
     */
    boolean isEnabled(Environment environment);
}

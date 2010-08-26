/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.menu;
/**
 * This is blank implementation for IActionClass
 * @author nadeem
 */
public abstract class IActionClassAdapter implements IActionClass {
    /**
     * Actual run method.
     * @param args Menu Argument.
     */
    public boolean run(Object[] args)                      { return true; }
    /**
     * Visible status of Menu.
     * @param  environment Environment
     * @return Visible status of Menu.
     */
    public boolean isVisible(Environment environment)      { return true; }
    /**
     * Enable status of Menu.
     * @param  environment Environment
     * @return Enable status of Menu.
     */
    public boolean isEnabled(Environment environment)      { return true; }
}

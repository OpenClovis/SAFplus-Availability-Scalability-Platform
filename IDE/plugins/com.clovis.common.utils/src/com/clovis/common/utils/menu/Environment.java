/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/menu/Environment.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.menu;

/**
 * This is the interface defining the Context for Menus
 * @author nadeem
 */
public interface Environment
{
    /**
     * Gets the value for a property.
     * @param  property  Name
     * @return           Value
     */
    Object getValue(Object property);
    /**
     * 
     * @param property - property to be set
     * @param value - Value to be set on the property
     */
    void setValue(Object property, Object value);
    /**
     * Returns Parent Environment.
     * @return Parent Environment.
     */
    Environment getParentEnv();
    /**
     *
     * @return the notifier instance
     */
    EnvironmentNotifier getNotifier();
}

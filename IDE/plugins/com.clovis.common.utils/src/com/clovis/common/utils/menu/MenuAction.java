/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/menu/MenuAction.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.menu;

import  java.util.HashMap;

import  org.eclipse.jface.action.Action;

import com.clovis.common.utils.ClovisUtils;
import  com.clovis.common.utils.menu.gen.MenuItem;
import  com.clovis.common.utils.menu.gen.ActionClass;
/**
 * Abstract Class for MenuAction.
 * @author nadeem
 */
public class MenuAction extends Action
{
    private static HashMap actionClassMap = new HashMap();
    protected Environment _env;
    protected MenuItem    _menuItem;
    /**
     * Gets Class for given ActionClass Name.
     * @param  name Name of the ActionClass.
     * @return Class for given ActionClass Name.
     */
    private IActionClass getActionClassInstance(String name)
    {

        if (!actionClassMap.containsKey(name)) {
            ClassLoader loader = (ClassLoader) _env.getValue("classloader");
            IActionClass instance = null;
            try {
                Class clazz = ClovisUtils.loadClass(name);
                if (clazz == null) {
                    clazz = (loader == null)
                    ?  Class.forName(name) : Class.forName(name, true, loader);
                }
                instance = (IActionClass) clazz.newInstance();
            } catch (Exception e) { e.printStackTrace(); }
            actionClassMap.put(name, instance);
        }
        return (IActionClass) actionClassMap.get(name);
    }
    /**
     * Constructor.
     * @param env       Environment
     * @param menuItem  MenuItem from XML.
     */
    public MenuAction(Environment env, MenuItem menuItem)
    {
        _env = env;
        _menuItem = menuItem;
        String label = _menuItem.getLabel();
        setToolTipText(_menuItem.getHint());
        setText((label != null) ? label : _menuItem.getName());
    }
    /**
     * @return the tooltip text
     */
    public String getToolTipText()
    {
        String tooltip = _menuItem.getTooltip();
        return (tooltip != null) ? tooltip : _menuItem.getName();
    }
    /**
     * @return Returns the Environment.
     */
    public Environment getEnv()
    {
        return _env;
    }
    /**
     * @param env The Environemnt to set.
     */
    public void setEnv(Environment env)
    {
        _env = env;
    }
    /**
     * Visible Status. Returns true only if all actionclasses are visible.
     * @return Visible Status of Menu
     */
    public boolean isVisible()
    {
        for (int i = 0; i < _menuItem.getActionClassCount(); i++) {
            ActionClass classInfo = _menuItem.getActionClass(i);
            IActionClass obj =
                getActionClassInstance(classInfo.getClassName());
            if (obj != null && !obj.isVisible(_env)) {
                return false;
            }
        }
        return true;
    }
    /**
     * Enable Status. Returns true only if all actionclasses are enabled.
     * @return Enable Status of Menu
     */
    public boolean isEnabled()
    {
        for (int i = 0; i < _menuItem.getActionClassCount(); i++) {
            ActionClass classInfo = _menuItem.getActionClass(i);
            IActionClass obj =
                getActionClassInstance(classInfo.getClassName());
            if (obj == null || !obj.isEnabled(_env)) {
                return false;
            }
        }
        return true;
    }
    /**
     * Returns MenuItem
     * @return _menuItem
     */
    public MenuItem getMenuItem()
    {
        return _menuItem;
    }
    /**
     * Run all actions one by one.
     * Calls run(Object args[]) method of each action classes in sequence. It
     * provides values of args from environment.
     */
    public void run()
    {
        
        for (int i = 0; i < _menuItem.getActionClassCount(); i++) {
            ActionClass classInfo  = _menuItem.getActionClass(i);
            String className = classInfo.getClassName();
            IActionClass obj = getActionClassInstance(className);
            Object args[] = new Object[classInfo.getArgumentCount()];
            for (int j = 0; j < classInfo.getArgumentCount(); j++) {
                args[j] = _env.getValue(classInfo.getArgument(j));
            }
            // Calling run on the ActionClass.
            boolean status = obj.run(args);
            if (!status) {
                System.err.println("Execution Failed for Action: " + className);
                break;
            }
        }
    }
}

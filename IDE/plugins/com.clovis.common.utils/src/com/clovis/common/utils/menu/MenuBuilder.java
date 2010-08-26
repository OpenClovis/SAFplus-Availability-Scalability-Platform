/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/menu/MenuBuilder.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.menu;

import  java.io.Reader;

import  org.eclipse.gef.EditPartViewer;
import  org.eclipse.gef.ContextMenuProvider;
import org.eclipse.swt.widgets.Composite;

import  com.clovis.common.utils.menu.gen.MenuBars;
/**
 * This Class is the builder class for generic menus This class
 * has to initialised with a menu configuration file.
 * @author nadeem
 */
public class MenuBuilder
{
	private Environment _env;
	private MenuBars     _menuBars;
    /**
     * Creates a new MenuBuilder object.
     * 
     * @param reader XML Reader having menu information
     * @param env    The environment in which the menu is shown
     * @exception Exception  Description of the Exception
     */
    public MenuBuilder(Reader reader, Environment env)
        throws Exception
    {
    	_env     = env;
        _menuBars = (MenuBars) MenuBars.unmarshalMenuBars(reader);
    }
    /**
     * Return menubar
     * @return menubar
     */
    public MenuBars getMenuBars() { return _menuBars; }
    /**
     * Gets Environment assosciated with this MenuBuilder
     * @return Environment.
     */
    public Environment getEnvironment() { return _env; }
    /**
     * Get a composite (toolbar) added to parent composite
     * @param  parent Parent composite
     * @param  index  Index of Menubar in the menu.
     * @return toolbar composite
     */
    public Composite getToolbar(Composite parent, int index)
    {
    	return new Toolbar(parent, this, index);
    }
    /**
     * Get ContextMenuProvider for given Editor.
     * @return MenuProvider
     */
    public ContextMenuProvider getContextMenuProvider(EditPartViewer viewer)
    {
    	return new EditorContextMenuProvider(viewer, this);
    }
}

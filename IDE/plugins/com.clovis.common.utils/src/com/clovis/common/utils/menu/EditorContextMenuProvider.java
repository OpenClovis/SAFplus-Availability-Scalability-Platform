/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/menu/EditorContextMenuProvider.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.menu;

import  org.eclipse.gef.EditPartViewer;
import  org.eclipse.gef.ContextMenuProvider;
import  org.eclipse.jface.action.IMenuManager;

import  com.clovis.common.utils.menu.gen.MenuBar;
import com.clovis.common.utils.menu.gen.MenuBars;
/**
 * Provides context menu for Editor.
 * @author nadeem
 */
public class EditorContextMenuProvider
    extends ContextMenuProvider
{
    private MenuBuilder _menuBuilder;
    private MenuAction  _defaultAction;
    /**
     * Constructor.
     * @param viewer  EditPartViewer
     * @param builder MenuBuilder
     */
    public EditorContextMenuProvider(EditPartViewer viewer,
                                     MenuBuilder builder)
    {
        super(viewer);
        _menuBuilder = builder;
    }
    /**
     * Gets Default Action.
     * @return Default Action
     */
    public MenuAction getDefaultAction()
    {
        return _defaultAction;
    }
    /**
     * Called when menu is about to show.
     * @param menuManager MenuManager (basically this)
     */
    public void buildContextMenu(IMenuManager menuManager)
    {
        if (_menuBuilder != null) {
            MenuBars menubars = _menuBuilder.getMenuBars();
            MenuBar menubar   = menubars.getMenuBar(0);
            Environment env = _menuBuilder.getEnvironment();
            for (int i = 0; i < menubar.getMenu().length; i++) {
                MenuAction menuAction = new MenuAction(env, menubar.getMenu(i));
                if (menuAction.isVisible()) {
                    menuManager.add(menuAction);
                    if (menuAction.getMenuItem().getDefault()) {
                        _defaultAction = menuAction;
                    }
                }
            }
        }
    }
}

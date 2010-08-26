/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor;

import java.util.List;

import org.eclipse.gef.EditPart;
import org.eclipse.gef.EditPartViewer;
import org.eclipse.gef.ui.actions.ActionRegistry;
import org.eclipse.gef.ui.actions.GEFActionConstants;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.actions.ActionFactory;

import com.clovis.common.utils.menu.MenuBuilder;
import com.clovis.common.utils.menu.EditorContextMenuProvider;
import com.clovis.cw.genericeditor.actions.AbstractCopyAction;
import com.clovis.cw.genericeditor.editparts.BaseEditPart;
import com.clovis.cw.genericeditor.model.Base;

/**
 * @author pushparaj
 * Provides Context menu for GenericEditor.
 */
public class GEContextMenuProvider extends EditorContextMenuProvider
{
    private ActionRegistry _actionRegistry;
    private EditPartViewer _viewer;
    /**
     * Constructor.
     * Sets viewer and registry for menu provided.
     */
    public GEContextMenuProvider(EditPartViewer viewer,
                                 MenuBuilder builder, ActionRegistry reg)
    {
        super(viewer, builder);
        _viewer = viewer;
        _actionRegistry = reg;
    }
    /**
     * Builds context menu.
     * @param manager Menu Manager.
     */
    public void buildContextMenu(IMenuManager manager)
    {
        // Add Common menu
        buildCommonMenu(manager);
    	// Add Specific Editor Menu.
        super.buildContextMenu(manager);
    }
    /**
     * Adds common menu requied by Generic Editor.
     * @param manager Menu Manager
     */
    private void buildCommonMenu(IMenuManager manager)
    {
    	//UNDO
        GEFActionConstants.addStandardActionGroups(manager);
        IAction action;
        action = _actionRegistry.getAction(ActionFactory.UNDO.getId());
        manager.appendToGroup(GEFActionConstants.GROUP_UNDO, action);
        //REDO
        action = _actionRegistry.getAction(ActionFactory.REDO.getId());
        manager.appendToGroup(GEFActionConstants.GROUP_UNDO, action);
        //ZOOM IN
        action = _actionRegistry.getAction(GEFActionConstants.ZOOM_IN);
        manager.appendToGroup(GEFActionConstants.GROUP_REST, action);
        //ZOOM OUT
        action = _actionRegistry.getAction(GEFActionConstants.ZOOM_OUT);
        manager.appendToGroup(GEFActionConstants.GROUP_REST, action);
        //CUT
        action = _actionRegistry.getAction(ActionFactory.CUT.getId());
        manager.appendToGroup(GEFActionConstants.GROUP_EDIT, action);
        //COPY
        action = _actionRegistry.getAction(ActionFactory.COPY.getId());
        manager.appendToGroup(GEFActionConstants.GROUP_EDIT, action);
        //PASTE
        action = _actionRegistry.getAction(ActionFactory.PASTE.getId());
        manager.appendToGroup(GEFActionConstants.GROUP_EDIT, action);
        //DELETE
        action = _actionRegistry.getAction(ActionFactory.DELETE.getId());
        manager.appendToGroup(GEFActionConstants.GROUP_EDIT, action);
        //SAVE
        action = _actionRegistry.getAction(ActionFactory.SAVE.getId());
        action.setImageDescriptor(ImageDescriptor.createFromFile(AbstractCopyAction.class,
		"icons/esave_edit.gif"));
        manager.appendToGroup(GEFActionConstants.GROUP_SAVE, action);
        //SAVE AS IMAGE
        action = _actionRegistry.getAction("saveAsImage");
        manager.appendToGroup(GEFActionConstants.GROUP_SAVE, action);
        //EXPAND & COLLAPSE
        List parts = _viewer.getSelectedEditParts();
        if(parts.size() == 1) {
        	EditPart part = (EditPart) parts.get(0);
        	if(part instanceof BaseEditPart) {
        		Base model = (Base) part.getModel();
        		if(model.isCollapsedParent()) {
        			action = _actionRegistry.getAction("expand");
        	        manager.appendToGroup(GEFActionConstants.GROUP_SAVE, action);
        		} else if(model.getSourceConnections().size() > 0){
        			action = _actionRegistry.getAction("collapse");
        	        manager.appendToGroup(GEFActionConstants.GROUP_SAVE, action);
        		}
        	}
        }
        //AUTO ARRANGE
        action = _actionRegistry.getAction("auto");
        manager.appendToGroup(GEFActionConstants.GROUP_SAVE, action);
    }
}

/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor.actions;

import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.actions.ActionFactory;
import org.eclipse.gef.editparts.ZoomManager;
import org.eclipse.gef.ui.actions.DeleteRetargetAction;
import org.eclipse.gef.ui.actions.GEFActionConstants;
import org.eclipse.gef.ui.actions.MatchHeightRetargetAction;
import org.eclipse.gef.ui.actions.MatchWidthRetargetAction;
import org.eclipse.gef.ui.actions.RedoRetargetAction;
import org.eclipse.gef.ui.actions.UndoRetargetAction;
import org.eclipse.gef.ui.actions.ZoomComboContributionItem;
import org.eclipse.gef.ui.actions.ZoomInRetargetAction;
import org.eclipse.gef.ui.actions.ZoomOutRetargetAction;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;


public class GEActionBarContributor
	extends org.eclipse.gef.ui.actions.ActionBarContributor
{

/**
 * @see org.eclipse.gef.ui.actions.ActionBarContributor#createActions()
 */
protected void buildActions() {
	addRetargetAction(new UndoRetargetAction());
	addRetargetAction(new RedoRetargetAction());
	addRetargetAction(new DeleteRetargetAction());
	
		
	addRetargetAction(new ZoomInRetargetAction());
	addRetargetAction(new ZoomOutRetargetAction());
	
	addRetargetAction(new MatchWidthRetargetAction());
	addRetargetAction(new MatchHeightRetargetAction());
}

/**
 * @see org.eclipse.gef.ui.actions.ActionBarContributor#declareGlobalActionKeys()
 */
protected void declareGlobalActionKeys() {
	addGlobalActionKey(ActionFactory.PRINT.getId());
	addGlobalActionKey(ActionFactory.SELECT_ALL.getId());
	addGlobalActionKey(ActionFactory.COPY.getId());
	addGlobalActionKey(ActionFactory.PASTE.getId());
}

/**
 * @see org.eclipse.ui.part.EditorActionBarContributor#contributeToToolBar(IToolBarManager)
 */
public void contributeToToolBar(IToolBarManager tbm) {
	tbm.add(getAction(ActionFactory.UNDO.getId()));
	tbm.add(getAction(ActionFactory.REDO.getId()));
	tbm.add(getAction(GEFActionConstants.ZOOM_IN));
	tbm.add(getAction(GEFActionConstants.ZOOM_OUT));
	tbm.add(new Separator());	
	String[] zoomStrings = new String[] {	ZoomManager.FIT_ALL, 
											ZoomManager.FIT_HEIGHT, 
											ZoomManager.FIT_WIDTH	};
	tbm.add(new ZoomComboContributionItem(getPage(), zoomStrings));
}

/**
 * @see org.eclipse.ui.part.EditorActionBarContributor#contributeToMenu(IMenuManager)
 */
public void contributeToMenu(IMenuManager menubar) {   
	super.contributeToMenu(menubar);
	/*MenuManager viewMenu = new MenuManager("Clovis Editor");
	viewMenu.add(getAction(ActionFactory.UNDO.getId()));
	viewMenu.add(getAction(ActionFactory.REDO.getId()));
	viewMenu.add(new Separator());
	viewMenu.add(getAction(GEFActionConstants.ZOOM_IN));
	viewMenu.add(getAction(GEFActionConstants.ZOOM_OUT));
	menubar.insertAfter(IWorkbenchActionConstants.M_EDIT, viewMenu);*/
}

}

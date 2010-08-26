package com.clovis.cw.editor.ca.dialog.handler;


import org.eclipse.core.commands.ExecutionEvent;
import org.eclipse.core.commands.ExecutionException;
import org.eclipse.core.commands.IHandler;
import org.eclipse.core.commands.IHandlerListener;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Tree;

import com.clovis.cw.editor.ca.action.ActionClassFactory;
import com.clovis.cw.editor.ca.action.TreeAction;
import com.clovis.cw.editor.ca.dialog.GenericPreferenceDialog;
import com.clovis.cw.editor.ca.dialog.PreferenceUtils;
import com.clovis.cw.editor.ca.dialog.PreferenceUtils.PreferenceSelectionData;

/**
 * This is the base class for menu handlers which are used in 
 * PreferenceDialog's pop-up menus.
 * @author Pushparaj
 *
 */
public abstract class AbstractMenuHandler implements IHandler{
	
	/**
	 * @see org.eclipse.core.commands.IHandler#addHandlerListener(org.eclipse.core.commands.IHandlerListener)
	 */
	public void addHandlerListener(IHandlerListener handlerListener) {}
	/**
	 * @see org.eclipse.core.commands.IHandler#dispose()
	 */
	public void dispose() { }
	/**
	 * @see org.eclipse.core.commands.IHandler#execute(org.eclipse.core.commands.ExecutionEvent)
	 */
	public Object execute(ExecutionEvent event) throws ExecutionException {
		Event e = (Event)event.getTrigger();
		Tree tree = (Tree)e.display.getFocusControl();
		GenericPreferenceDialog dialog = (GenericPreferenceDialog)tree.getData("dialog");
		EObject rootObject = dialog.getViewModel()
		.getEObject();
		TreeViewer treeViewer = dialog.getTreeViewer();
		PreferenceSelectionData psd = PreferenceUtils
				.getCurrentSelectionData(treeViewer);
		EObject nodeObj = (psd.getEObject() != null) ? psd.getEObject()
				: rootObject;
		TreeAction action = ActionClassFactory.getActionClassFactory()
				.getAction(nodeObj.eClass(), treeViewer, dialog, rootObject);
		action.init(treeViewer, dialog, rootObject);
		execute(action);
		return null;
	}
	/**
	 * @see org.eclipse.core.commands.IHandler#isHandled()
	 */
	public boolean isHandled(){
		return true;
	}
	/**
	 * @see org.eclipse.core.commands.IHandler#removeHandlerListener(org.eclipse.core.commands.IHandlerListener)
	 */
	public void removeHandlerListener(IHandlerListener handlerListener) {}
	/**
	 * invoke appropriate methods which are doing add/delete/duplicate in Tree
	 * @param action TreeAction
	 */
	protected abstract void execute(TreeAction action);
}

/**
 * 
 */
package com.clovis.cw.editor.ca.dialog;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.widgets.TreeItem;

/**
 * Selection change listener for the preference dialog tree.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class PreferenceSelectionChangedListener implements
		ISelectionChangedListener {

	private TreeItem _oldSelection;

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.ISelectionChangedListener#selectionChanged(org.eclipse.jface.viewers.SelectionChangedEvent)
	 */
	public void selectionChanged(SelectionChangedEvent event) {
		TreeViewer treeViewer = (TreeViewer) event.getSource();
		TreeItem treeItem = null;
		if (treeViewer.getTree().getSelection().length != 0) {
			treeItem = treeViewer.getTree().getSelection()[0];
		}

		if (_oldSelection != null && treeItem != null
				&& _oldSelection != treeItem) {
			PreferenceNode node = (PreferenceNode) _oldSelection.getData();
			GenericPreferencePage prefPage = (GenericPreferencePage) node
					.getPage();

			if (!prefPage.isErrorFree()) {
				if (MessageDialog.openQuestion(
						prefPage.getControl().getShell(),
						"Invalid Configuration",
						"The currently displayed page contains invalid configuration values."
								+ "\nDo you want to remove it?")) {

					EObject eObject = null;
					TreeItem parentTreeItem = _oldSelection.getParentItem();
					PreferenceNode parentPrefNode = null;

					if (prefPage instanceof GenericFormPage) {
						eObject = ((GenericFormPage) prefPage).getEObject();
						parentPrefNode = (parentTreeItem != null) ? (PreferenceNode) parentTreeItem
								.getData()
								: null;

					} else {
						if (parentTreeItem != null) {
							parentPrefNode = (PreferenceNode) parentTreeItem
									.getData();
							PreferencePage page = (PreferencePage) parentPrefNode
									.getPage();

							if (page instanceof GenericFormPage) {
								eObject = ((GenericFormPage) page).getEObject();
							}
						}
					}

					EList eList = PreferenceUtils.getContainerEList(eObject);
					eList.remove(eObject);

					if (parentTreeItem != null) {
						prefPage.setValid(true);
						parentPrefNode.remove(node);
					}

					_oldSelection = treeItem;
					PreferenceUtils.setTreeViewerSelection(treeViewer,
							(PreferenceNode) treeItem.getData());

				} else {
					PreferenceUtils.setTreeViewerSelection(treeViewer,
							(PreferenceNode) _oldSelection.getData());
				}

			} else {
				_oldSelection = treeItem;
			}

		} else {
			_oldSelection = treeItem;
		}
	}
}

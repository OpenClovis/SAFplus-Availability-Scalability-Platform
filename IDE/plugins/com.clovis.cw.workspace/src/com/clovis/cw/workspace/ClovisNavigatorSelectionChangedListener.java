/**
 * 
 */
package com.clovis.cw.workspace;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TreeSelection;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;

import com.clovis.cw.workspace.migration.MigrationDialog;
import com.clovis.cw.workspace.migration.MigrationUtils;

/**
 * Selection Change Listener for Clovis Navigator.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class ClovisNavigatorSelectionChangedListener implements
		ISelectionChangedListener {

	/**
	 * Selected Project.
	 */
	private IProject _project;

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.ISelectionChangedListener#selectionChanged(org.eclipse.jface.viewers.SelectionChangedEvent)
	 */
	public void selectionChanged(SelectionChangedEvent event) {

		if (event.getSelection() != null && !event.getSelection().isEmpty()) {
			IProject project = ((IResource) ((TreeSelection) event
					.getSelection()).getFirstElement()).getProject();

			if (_project == project || !project.isOpen())
				return;
			_project = project;

			if (MigrationUtils.isMigrationRequired(_project)) {
				Shell shell = Display.getCurrent().getActiveShell();

				if (MessageDialog
						.openQuestion(
								shell,
								"Project Migration Required",
								"Project ["
										+ _project.getName()
										+ "] is not in the latest version.\nDo you want to migrate it to the latest version?")) {

					new MigrationDialog(shell, _project).open();
				}
			}
		}
	}
}

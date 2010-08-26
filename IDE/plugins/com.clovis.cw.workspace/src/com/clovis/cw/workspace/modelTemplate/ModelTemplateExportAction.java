/**
 * 
 */
package com.clovis.cw.workspace.modelTemplate;

import java.io.File;
import java.io.IOException;
import java.net.URL;

import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.widgets.Shell;

import com.clovis.cw.workspace.WorkspacePlugin;

/**
 * Action class which handles the export of model templates.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class ModelTemplateExportAction extends Action {

	private Shell _shell;

	/**
	 * Constructor.
	 * 
	 * @param shell
	 */
	public ModelTemplateExportAction(Shell shell) {
		_shell = shell;
		setText("Export");
		setToolTipText("Exports the Model Template");

/*		URL url = FileLocator.find(WorkspacePlugin.getDefault().getBundle(),
				new Path("icons" + File.separator + "export.gif"), null);
*/
		URL url = WorkspacePlugin.getDefault().find(
				new Path("icons" + File.separator + "export.gif"));

		try {
/*			setImageDescriptor(ImageDescriptor.createFromURL(FileLocator
					.resolve(url)));
*/
			setImageDescriptor(ImageDescriptor.createFromURL(Platform
					.resolve(url)));
		} catch (IOException e) {
			WorkspacePlugin.LOG.warn("Unable to Load the Export Icon", e);
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.action.Action#run()
	 */
	public void run() {
		new ModelTemplateImportExportDialog(_shell,
				ModelTemplateConstants.DIALOG_TYPE_EXPORT, null).open();
	}
}

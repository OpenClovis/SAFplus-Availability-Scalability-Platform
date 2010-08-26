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

import com.clovis.cw.workspace.WorkspacePlugin;

/**
 * Action class which handles the refresh for the model template view.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class ModelTemplateRefreshAction extends Action {

	private ModelTemplateView _modelTemplateView;

	/**
	 * Constructor.
	 * 
	 * @param modelTemplateView
	 */
	public ModelTemplateRefreshAction(ModelTemplateView modelTemplateView) {
		_modelTemplateView = modelTemplateView;
		setText("Refresh");
		setToolTipText("Refreshes the Model Template View");

/*		URL url = FileLocator.find(WorkspacePlugin.getDefault().getBundle(),
				new Path("icons" + File.separator + "refresh.gif"), null);
*/
		URL url = WorkspacePlugin.getDefault().find(
				new Path("icons" + File.separator + "refresh.gif"));

		try {
/*			setImageDescriptor(ImageDescriptor.createFromURL(FileLocator
					.resolve(url)));
*/
			setImageDescriptor(ImageDescriptor.createFromURL(Platform
					.resolve(url)));
		} catch (IOException e) {
			WorkspacePlugin.LOG.warn("Unable to Load the Refresh Icon", e);
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.action.Action#run()
	 */
	@Override
	public void run() {
		_modelTemplateView.refreshModelTemplateView();
	}
}

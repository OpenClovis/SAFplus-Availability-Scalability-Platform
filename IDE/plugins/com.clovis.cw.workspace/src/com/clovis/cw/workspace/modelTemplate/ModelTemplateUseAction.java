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
 * Action class which handles the use of the model template into the editor.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class ModelTemplateUseAction extends Action {

	private ModelTemplateView _modelTemplateView;

	/**
	 * Constructor.
	 * 
	 * @param modelTemplateView
	 */
	public ModelTemplateUseAction(ModelTemplateView modelTemplateView) {
		_modelTemplateView = modelTemplateView;
		setText("Use");
		setToolTipText("Uses the Model Template");

/*		URL url = FileLocator.find(WorkspacePlugin.getDefault().getBundle(),
				new Path("icons" + File.separator + "useModelTemplate.png"), null);
*/
		URL url = WorkspacePlugin.getDefault().find(
				new Path("icons" + File.separator + "useModelTemplate.png"));

		try {
/*			setImageDescriptor(ImageDescriptor.createFromURL(FileLocator
					.resolve(url)));
*/
			setImageDescriptor(ImageDescriptor.createFromURL(Platform
					.resolve(url)));
		} catch (IOException e) {
			WorkspacePlugin.LOG.warn("Unable to Load the Use Action Icon", e);
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.action.Action#run()
	 */
	public void run() {
		ModelTemplateView.useModelTemplate(_modelTemplateView
				.getModelTemplateObject());
	}
}

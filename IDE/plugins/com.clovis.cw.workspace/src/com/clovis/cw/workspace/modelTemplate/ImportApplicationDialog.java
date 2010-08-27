/**
 * 
 */
package com.clovis.cw.workspace.modelTemplate;

import java.io.File;
import java.io.FilenameFilter;
import java.util.Arrays;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.swt.widgets.Shell;

/**
 * Application Import Dialog.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class ImportApplicationDialog extends ModelTemplateImportExportDialog {

	/**
	 * Constructor.
	 * 
	 * @param shell
	 */
	public ImportApplicationDialog(Shell shell) {
		super(shell, ModelTemplateConstants.DIALOG_TYPE_IMPORT, null);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.cw.workspace.modelTemplate.ModelTemplateImportExportDialog#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell shell) {
		super.configureShell(shell);
		shell.setText("Import Application Dialog");
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.cw.workspace.modelTemplate.ModelTemplateImportExportDialog#okPressed()
	 */
	@Override
	protected void okPressed() {
		String[] files = Arrays.asList(
				_modelTemplateFilesViewer.getCheckedElements()).toArray(
				new String[] {});
		_importExportType = "";
		importTemplates(_importExportLocation.getText(), files);

		for (String file : files) {

			File templateDir = new File(
					ModelTemplateConstants.MODEL_TEMPLATE_FOLDER_PATH
							+ File.separator
							+ file
									.substring(
											0,
											file
													.lastIndexOf(ModelTemplateConstants.MODEL_TEMPLATE_ARCHIEVE_EXT)));

			String templates[] = templateDir.list(new FilenameFilter() {
				public boolean accept(File dir, String name) {

					if (ModelTemplateUtils.isModelTemplateFile(name)) {
						return true;
					}
					return false;
				}
			});

			if (templates.length > 0) {
				EObject modelTemplateObject = ModelTemplateView
						.readModelTemplate(templates[0]);
				ModelTemplateView.useModelTemplate(modelTemplateObject);
			}
		}

		super.okClicked();
	}
}

/**
 * 
 */
package com.clovis.cw.workspace.modelTemplate;

import java.io.File;
import java.io.FileFilter;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FilenameFilter;
import java.io.IOException;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.eclipse.core.runtime.Path;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.jface.viewers.CheckboxTableViewer;
import org.eclipse.osgi.util.NLS;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.HelpEvent;
import org.eclipse.swt.events.HelpListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.DirectoryDialog;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.dialogs.IOverwriteQuery;
import org.eclipse.ui.internal.ide.IDEWorkbenchMessages;

import com.clovis.common.utils.ClovisFileUtils;
import com.clovis.common.utils.UtilsPlugin;
import com.clovis.common.utils.ui.table.TableContentProvider;
import com.clovis.cw.workspace.WorkspacePlugin;

/**
 * Dialog for importing and exporting templates.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class ModelTemplateImportExportDialog extends TitleAreaDialog implements
		IOverwriteQuery {

	private int _dialogType;

	protected String _importExportType = "";

	private ModelTemplateView _modelTemplateView;
	
	private String contextid = "";
	
	protected CheckboxTableViewer _modelTemplateFilesViewer;
	
	protected Text _importExportLocation;
	
	/**
	 * Constructor.
	 * 
	 * @param shell
	 * @param dialogType
	 * @param modelTemplateView 
	 */
	public ModelTemplateImportExportDialog(Shell shell, int dialogType, ModelTemplateView modelTemplateView) {
		super(shell);
		_dialogType = dialogType;
		_modelTemplateView = modelTemplateView;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.TitleAreaDialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent) {
		Composite composite = new Composite(parent, SWT.NONE);
		composite.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		composite.setLayout(new GridLayout());
		
		if (_dialogType == ModelTemplateConstants.DIALOG_TYPE_IMPORT) {
			createImportLocationComposite(composite);
			createModelTemplateFilesViewer(composite);
			setTitle("Select files to import");
			contextid = "com.clovis.cw.help.modeltemplate_import";

		} else if (_dialogType == ModelTemplateConstants.DIALOG_TYPE_EXPORT) {
			createModelTemplateFilesViewer(composite);
			createExportLocationComposite(composite);
			setTitle("Select files to export");
			contextid = "com.clovis.cw.help.modeltemplate_export";
		}

		setMessage("Selection should not be empty.");
		
		composite.addHelpListener(new HelpListener() {

			public void helpRequested(HelpEvent e) {
				PlatformUI.getWorkbench().getHelpSystem().displayHelp(
						contextid);
			}
		});
		
		return composite;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.TrayDialog#createButtonBar(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createButtonBar(Composite parent) {
		Control control = super.createButtonBar(parent);
		enableOKButton(false);
		return control;
	}

	/**
	 * Enables or disables the OK button.
	 * 
	 * @param status
	 */
	public void enableOKButton(boolean status) {
		getButton(IDialogConstants.OK_ID).setEnabled(status);
	}

	/**
	 * Creates the control to capture the import location.
	 * 
	 * @param composite
	 */
	private void createImportLocationComposite(Composite composite) {
		Composite locationComposite = new Composite(composite, SWT.NONE);
		locationComposite.setLayout(new GridLayout(3, false));
		locationComposite.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true,
				false));

		new Label(locationComposite, SWT.NONE).setText("From Directory:");

		_importExportLocation = new Text(locationComposite, SWT.BORDER);
		_importExportLocation.setLayoutData(new GridData(SWT.FILL, SWT.FILL,
				true, false));
		_importExportLocation.addListener(SWT.Modify, new ModelImportExportSelectionListener(this));
		Button browseButton = new Button(locationComposite, SWT.NONE);
		browseButton.setText("Browse...");

		browseButton.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {

				DirectoryDialog dialog = new DirectoryDialog(getShell());
				dialog.setFilterPath(UtilsPlugin.getDialogSettingsValue("IMPORTMIB"));
				String location = dialog.open();
				UtilsPlugin.saveDialogSettings("IMPORTMIB", location);
				if (location != null) {
					_importExportLocation.setText(location);
					File locationFile = new File(location);

					if (locationFile.exists()) {

						if(ModelTemplateUtils.isModelTempalteFolder(location)) {
							_modelTemplateFilesViewer.setInput(null);
							enableOKButton(false);
							setErrorMessage("User should not import from the Model Template folder.");
							return;
						}

						setErrorMessage("There are no files currently selected for import.");
						_modelTemplateFilesViewer.setInput(Arrays
								.asList(locationFile.list(new FilenameFilter() {

									public boolean accept(File dir, String name) {
										if (ModelTemplateUtils
												.isModelTemplateArchieve(name)) {
											return true;
										}
										return false;
									}
								})));
					}
				}
			}
		});

		File modelTemplateFolder = new File(
				ModelTemplateConstants.MODEL_TEMPLATE_FOLDER_PATH);
		if (!modelTemplateFolder.exists()) {
			modelTemplateFolder.mkdir();
		}
	}

	/**
	 * Creates the control to capture the export location.
	 * 
	 * @param composite
	 */
	private void createExportLocationComposite(Composite composite) {
		Composite locationComposite = new Composite(composite, SWT.NONE);
		locationComposite.setLayout(new GridLayout(3, false));
		locationComposite.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true,
				false));

		new Label(locationComposite, SWT.NONE).setText("To Directory:");

		_importExportLocation = new Text(locationComposite, SWT.BORDER);
		_importExportLocation.setLayoutData(new GridData(SWT.FILL, SWT.FILL,
				true, false));
		_importExportLocation.addListener(SWT.Modify, new ModelImportExportSelectionListener(this));
		Button browseButton = new Button(locationComposite, SWT.NONE);
		browseButton.setText("Browse...");

		browseButton.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {

				DirectoryDialog dialog = new DirectoryDialog(getShell());
				dialog.setFilterPath(UtilsPlugin.getDialogSettingsValue("EXPORTMIB"));
				String location = dialog.open();
				UtilsPlugin.saveDialogSettings("EXPORTMIB", location);
				
				if (location != null) {
					_importExportLocation.setText(location);

					if(ModelTemplateUtils.isModelTempalteFolder(location)) {
						enableOKButton(false);
						setErrorMessage("User should not export to the Model Template folder.");
					} else {
						if(_modelTemplateFilesViewer.getCheckedElements().length == 0) {
							enableOKButton(false);
							setErrorMessage("There are no files currently selected for export.");
						} else {
							enableOKButton(true);
							setErrorMessage(null);
						}
					}
				}
			}
		});

		File modelTemplateFolder = new File(
				ModelTemplateConstants.MODEL_TEMPLATE_FOLDER_PATH);
		if (!modelTemplateFolder.exists()) {
			modelTemplateFolder.mkdir();
		}

		File templateDirs[] = modelTemplateFolder.listFiles(new FileFilter() {
			public boolean accept(File file) {

				if (file.isDirectory()) {
					return true;
				}
				return false;
			}
		});

		List<String> templates = new ArrayList<String>();
		for (File templateDir : templateDirs) {

			String files[] = templateDir.list(new FilenameFilter() {
				public boolean accept(File dir, String name) {

					if (ModelTemplateUtils
							.isModelTemplateFile(name)) {
						return true;
					}
					return false;
				}
			});

			if (files.length > 0) {
				templates.add(templateDir.getName());
			}
		}

		_modelTemplateFilesViewer.setInput(templates);
	}

	/**
	 * Create the viewer for selecting model template files.
	 * 
	 * @param composite
	 */
	private void createModelTemplateFilesViewer(Composite composite) {
		_modelTemplateFilesViewer = CheckboxTableViewer.newCheckList(composite,
				SWT.BORDER);
		_modelTemplateFilesViewer.getTable().setLayoutData(
				new GridData(SWT.FILL, SWT.FILL, true, true));

		_modelTemplateFilesViewer
				.setContentProvider(new TableContentProvider());
		_modelTemplateFilesViewer
				.addCheckStateListener(new ModelTemplateImportExportCheckStateListener(
						this));
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell shell) {
		super.configureShell(shell);

		Rectangle bounds = Display.getCurrent().getClientArea();
		shell.setBounds((int) (1.5 * bounds.width / 5),
				(int) (1.5 * bounds.height / 5), 2 * bounds.width / 5,
				2 * bounds.height / 5);

		if (_dialogType == ModelTemplateConstants.DIALOG_TYPE_IMPORT) {
			shell.setText("Model Template Import Dialog");

		} else if (_dialogType == ModelTemplateConstants.DIALOG_TYPE_EXPORT) {
			shell.setText("Model Template Export Dialog");
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed() {
		String[] files = Arrays.asList(
				_modelTemplateFilesViewer.getCheckedElements()).toArray(
				new String[] {});
		_importExportType = "";

		if (_dialogType == ModelTemplateConstants.DIALOG_TYPE_IMPORT) {
			importTemplates(_importExportLocation.getText(), files);
			if(_modelTemplateView != null) {
				_modelTemplateView.refreshModelTemplateView();
			}

		} else if (_dialogType == ModelTemplateConstants.DIALOG_TYPE_EXPORT) {
			exportTemplates(_importExportLocation.getText(), files);
		}

		super.okPressed();
	}

	protected void okClicked() {
		super.okPressed();
	}

	/**
	 * Imports the model template files.
	 * 
	 * @param path
	 * @param templatesToImport
	 */
	protected void importTemplates(String path, String[] templatesToImport) {
		for (int i = 0; i < templatesToImport.length; i++) {

			String templateName = templatesToImport[i];
			String destinationPath = ModelTemplateConstants.MODEL_TEMPLATE_FOLDER_PATH
					+ File.separator + templateName.substring(0, templateName
					.lastIndexOf(ModelTemplateConstants.MODEL_TEMPLATE_ARCHIEVE_EXT));

			if (new File(destinationPath).exists()) {
				if (_importExportType.equals(NO_ALL))
					continue;

				if (!_importExportType.equals(ALL)) {
					_importExportType = queryOverwrite(destinationPath);

					if (_importExportType.equals(NO)
							|| _importExportType.equals(NO_ALL)) {
						continue;
					} else if (_importExportType.equals(IOverwriteQuery.CANCEL)) {
						break;
					}
				}
			}

			ClovisFileUtils.extractArchive(path + File.separator + templateName,
					ModelTemplateConstants.MODEL_TEMPLATE_FOLDER_PATH);
		}
	}

	/**
	 * Exports the model template files.
	 * 
	 * @param path
	 * @param templatesToExport
	 */
	private void exportTemplates(String path, String[] templatesToExport) {
		for (int i = 0; i < templatesToExport.length; i++) {

//			String timeStamp = new Timestamp(System.currentTimeMillis()).toString();
//			timeStamp = timeStamp.substring(0, timeStamp.lastIndexOf(".")).replace(" ", ":");

			String targetPath = path + File.separator + templatesToExport[i] + ".tgz";
			if (new File(targetPath).exists()) {
				if (_importExportType.equals(NO_ALL))
					continue;

				if (!_importExportType.equals(ALL)) {
					_importExportType = queryOverwrite(targetPath);

					if (_importExportType.equals(NO)
							|| _importExportType.equals(NO_ALL)) {
						continue;
					} else if (_importExportType.equals(IOverwriteQuery.CANCEL)) {
						break;
					}
				}
			}

			ClovisFileUtils.createArchive(ModelTemplateConstants.MODEL_TEMPLATE_FOLDER_PATH
					+ File.separator + templatesToExport[i], targetPath);
		}
	}

	/**
	 * Copies a file.
	 * 
	 * @param sourceFilePath
	 * @param destinationFilePath
	 */
	public void copyFile(String sourceFilePath, String destinationFilePath) {

		File destinationFile = new File(destinationFilePath);
		if (destinationFile.exists()) {

			if (_importExportType.equals(NO_ALL))
				return;
			if (!_importExportType.equals(ALL)) {
				_importExportType = queryOverwrite(destinationFilePath);
				if (_importExportType.equals(NO)
						|| _importExportType.equals(NO_ALL)
						|| _importExportType.equals(IOverwriteQuery.CANCEL)) {
					return;
				}
			}
		}

		try {
			FileInputStream fis = new FileInputStream(sourceFilePath);
			FileOutputStream fos = new FileOutputStream(destinationFilePath);
			FileChannel iChannel = fis.getChannel();
			FileChannel oChannel = fos.getChannel();

			MappedByteBuffer mBuffer = iChannel.map(
					FileChannel.MapMode.READ_ONLY, 0, iChannel.size());
			oChannel.write(mBuffer);

			iChannel.close();
			fis.close();
			oChannel.close();
			fos.close();
		} catch (IOException e) {
			WorkspacePlugin.LOG.error("Unable to copy the file "
					+ sourceFilePath, e);
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.dialogs.IOverwriteQuery#queryOverwrite(java.lang.String)
	 */
	public String queryOverwrite(String pathString) {

		Path path = new Path(pathString);

		String messageString;
		if (path.getFileExtension() == null || path.segmentCount() < 2) {
			messageString = NLS.bind(
					IDEWorkbenchMessages.WizardDataTransfer_existsQuestion,
					pathString);
		} else {
			messageString = NLS
					.bind(
							IDEWorkbenchMessages.WizardDataTransfer_overwriteNameAndPathQuestion,
							path.lastSegment(), path.removeLastSegments(1)
									.toOSString());
		}

		final MessageDialog dialog = new MessageDialog(getShell(),
				IDEWorkbenchMessages.Question, null, messageString,
				MessageDialog.QUESTION, new String[] {
						IDialogConstants.YES_LABEL,
						IDialogConstants.YES_TO_ALL_LABEL,
						IDialogConstants.NO_LABEL,
						IDialogConstants.NO_TO_ALL_LABEL,
						IDialogConstants.CANCEL_LABEL }, 0);
		String[] response = new String[] { YES, ALL, NO, NO_ALL,
				IOverwriteQuery.CANCEL };

		getShell().getDisplay().syncExec(new Runnable() {
			public void run() {
				dialog.open();
			}
		});

		return dialog.getReturnCode() < 0 ? IOverwriteQuery.CANCEL
				: response[dialog.getReturnCode()];
	}

	/**
	 * Returns the Dialog type.
	 * 
	 * @return the _dialogType
	 */
	public int getDialogType() {
		return _dialogType;
	}

	/**
	 * Returns the import export location.
	 * 
	 * @return
	 */
	public String getImportExportLocation() {
		return _importExportLocation.getText();
	}
}

/**
 * 
 */
package com.clovis.cw.workspace.migration;

import java.io.File;

import org.eclipse.core.resources.IProject;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.HelpEvent;
import org.eclipse.swt.events.HelpListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
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

import com.clovis.cw.data.DataPlugin;

/**
 * Dialog to provide interface for migrating the project.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class MigrationDialog extends TitleAreaDialog {

	private IProject _selectedProject;

	private Button _backupButton;
	
    private Text _backupLocationText;

    private Button _backupBrowseButton;

	private Text _currentVersionText;

	private Text _targetVersionText;

	private Text _currentUpdateVersionText;

	private Text _targetUpdateVersionText;

	private static final String DIALOG_TITLE = "Project Migration";

	/**
	 * Constructor.
	 * 
	 * @param parentShell
	 * @param selectedProject
	 */
	public MigrationDialog(Shell parentShell, IProject selectedProject) {
		super(parentShell);
		_selectedProject = selectedProject;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell shell) {
		super.configureShell(shell);
		shell.setText(DIALOG_TITLE);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed() {
		IProject[] projects = { _selectedProject };
		
		if (_backupButton.getSelection()) {

			String backupDir = _backupLocationText.getText().trim();
			
			if (backupDir.length() == 0)
			{
				String title = "Backup Directory Required";
				String msg = "Since you have chosen to backup the project a backup directory must be provided.";
				MessageDialog.openError(new Shell(), title, msg);
				return;
			}
			
			if (!MigrationManager.backupProjects(projects, backupDir))
			{
				String title = "Project Backup Error";
				String msg = "There was a problem backing up one or more migration projects."
							+ " See the Error Log for more details.";
				MessageDialog.openError(new Shell(), title, msg);
				return;
			}
		}

		super.okPressed();

		for (int i = 0; i < projects.length; i++) {
			Display.getDefault().syncExec(new MigrationThread(projects[i]));
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.TrayDialog#createButtonBar(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createButtonBar(Composite parent) {
		Label buttonBarSeparator = new Label(parent, SWT.HORIZONTAL
				| SWT.SEPARATOR);
		buttonBarSeparator
				.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));

		Control control = super.createButtonBar(parent);

		if (!MigrationUtils.isMigrationRequired(_selectedProject)) {
			setErrorMessage("Migration target version is not newer than the exsisting version.");
			getButton(IDialogConstants.OK_ID).setEnabled(false);
			_backupButton.setEnabled(false);
			_backupLocationText.setEnabled(false);
			_backupBrowseButton.setEnabled(false);
		}

		return control;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.TitleAreaDialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent) {
		Composite contents = new Composite(parent, SWT.NONE);
		GridLayout glayout = new GridLayout();
		glayout.numColumns = 2;
		contents.setLayout(glayout);
		contents.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

		Label projectLabel = new Label(contents, SWT.NONE);
		projectLabel.setLayoutData(new GridData(GridData.BEGINNING
				| GridData.FILL));
		projectLabel.setText("Selected project:");

		Text projectText = new Text(contents, SWT.SINGLE | SWT.BORDER);
		projectText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		projectText.setEnabled(false);
		projectText.setText(_selectedProject.getName());

		Label currentVersionLabel = new Label(contents, SWT.NONE);
		currentVersionLabel.setLayoutData(new GridData(GridData.BEGINNING
				| GridData.FILL));
		currentVersionLabel.setText("Current Project Version:");

		Composite currentVersionComposite = new Composite(contents, SWT.NONE);
		GridLayout currentVersionCompositeGL = new GridLayout(3, true);
		currentVersionCompositeGL.marginHeight = 0;
		currentVersionCompositeGL.marginWidth = 0;
		currentVersionComposite.setLayout(currentVersionCompositeGL);
		currentVersionComposite.setLayoutData(new GridData(
				GridData.FILL_HORIZONTAL));

		_currentVersionText = new Text(currentVersionComposite, SWT.SINGLE
				| SWT.BORDER);
		_currentVersionText
				.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		_currentVersionText.setEnabled(false);
		_currentVersionText.setText(MigrationUtils.getProjectVersion(_selectedProject));

		Label currentUpdateVersionLabel = new Label(currentVersionComposite,
				SWT.RIGHT);
		currentUpdateVersionLabel.setLayoutData(new GridData(
				GridData.FILL_HORIZONTAL));
		currentUpdateVersionLabel.setText("Update:");

		_currentUpdateVersionText = new Text(currentVersionComposite,
				SWT.SINGLE | SWT.BORDER);
		_currentUpdateVersionText.setLayoutData(new GridData(
				GridData.FILL_HORIZONTAL));
		_currentUpdateVersionText.setEnabled(false);
		_currentUpdateVersionText
				.setText(String.valueOf(MigrationUtils.getProjectUpdateVersion(_selectedProject)));

		Label targetVersionLabel = new Label(contents, SWT.NONE);
		targetVersionLabel.setLayoutData(new GridData(GridData.BEGINNING
				| GridData.FILL));
		targetVersionLabel.setText("Target Project Version:");

		Composite targetVersionComposite = new Composite(contents, SWT.NONE);
		GridLayout targetVersionCompositeGL = new GridLayout(3, true);
		targetVersionCompositeGL.marginHeight = 0;
		targetVersionCompositeGL.marginWidth = 0;
		targetVersionComposite.setLayout(targetVersionCompositeGL);
		targetVersionComposite.setLayoutData(new GridData(
				GridData.FILL_HORIZONTAL));

		_targetVersionText = new Text(targetVersionComposite, SWT.SINGLE
				| SWT.BORDER);
		_targetVersionText
				.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		_targetVersionText.setEnabled(false);
		_targetVersionText.setText(DataPlugin.getProductVersion());

		Label targetUpdateVersionLabel = new Label(targetVersionComposite,
				SWT.RIGHT);
		targetUpdateVersionLabel.setLayoutData(new GridData(
				GridData.FILL_HORIZONTAL));
		targetUpdateVersionLabel.setText("Update:");

		_targetUpdateVersionText = new Text(targetVersionComposite, SWT.SINGLE
				| SWT.BORDER);
		_targetUpdateVersionText.setLayoutData(new GridData(
				GridData.FILL_HORIZONTAL));
		_targetUpdateVersionText.setEnabled(false);
		_targetUpdateVersionText.setText(String.valueOf(DataPlugin
				.getProductUpdateVersion()));

		_backupButton = new Button(contents, SWT.CHECK);
		_backupButton.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		_backupButton.setSelection(true);
		_backupButton.setAlignment(SWT.LEFT);
		_backupButton.setText("Backup Selected Project?");

		// create composite with grid to hold backup location selector in single column
    	Composite preBuildComp = new Composite(contents, org.eclipse.swt.SWT.NONE);
    	GridData preBuildGrid = new GridData(GridData.FILL_HORIZONTAL);
    	preBuildGrid.horizontalSpan = 1;
    	preBuildGrid.minimumWidth = 250;
    	preBuildComp.setLayoutData(preBuildGrid);
    	GridLayout preBuildLayout = new GridLayout();
		preBuildLayout.marginHeight = 0;
		preBuildLayout.marginWidth = 0;
    	preBuildLayout.numColumns = 2;
    	preBuildComp.setLayout(preBuildLayout);
    	
    	_backupButton.addSelectionListener(new SelectionListener() {
			public void widgetSelected(SelectionEvent e) {
				if(_backupButton.getSelection()) {
					_backupLocationText.setEnabled(true);
					_backupBrowseButton.setEnabled(true);
					verifyBackupLocation(_backupLocationText.getText());
				} else {
					_backupLocationText.setEnabled(false);
					_backupBrowseButton.setEnabled(false);
					setErrorMessage(null);
					getButton(IDialogConstants.OK_ID).setEnabled(true);
				}
			}
			public void widgetDefaultSelected(SelectionEvent e) {
			}
    	});

    	_backupLocationText = new Text(preBuildComp, SWT.BORDER);
    	_backupLocationText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
    	_backupLocationText.addModifyListener(new ModifyListener() {
			public void modifyText(ModifyEvent e) {
				verifyBackupLocation(_backupLocationText.getText());
			}
    	});

    	_backupBrowseButton = new Button(preBuildComp, SWT.PUSH);
    	_backupBrowseButton.setText("Browse...");
    	_backupBrowseButton.addSelectionListener(new SelectionListener() {
			public void widgetSelected(SelectionEvent e) {
				DirectoryDialog dialog =
	                new DirectoryDialog(getShell(), SWT.NONE);
	            String fileName = dialog.open();
	            if (fileName != null) {
	            	_backupLocationText.setText(fileName);
	            } 
	            verifyBackupLocation(_backupLocationText.getText());
			}
			public void widgetDefaultSelected(SelectionEvent e) {
			}
		});

		_backupLocationText.setEnabled(true);
		_backupBrowseButton.setEnabled(true);
		
		setTitle("Project Migration");
		contents.addHelpListener(new HelpListener() {
			public void helpRequested(HelpEvent e) {
				PlatformUI.getWorkbench().getHelpSystem().displayHelp(
						"com.clovis.cw.help.migrate");
			}
		});
		return contents;
	}

	private void verifyBackupLocation(String location) {
		if(location.equals("") || !new File(location).exists()) {
			setErrorMessage("Specified Backup location is not a valid directory.");
			getButton(IDialogConstants.OK_ID).setEnabled(false);
		} else {
			setErrorMessage(null);
			getButton(IDialogConstants.OK_ID).setEnabled(true);
		}
	}
}

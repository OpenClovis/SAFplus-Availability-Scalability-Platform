/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/project/migration/MigrateProjectDialog.java $
 * $Author: bkpavan $
 * $Date: 2007/05/09 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.project.migration;

import java.io.FileWriter;
import java.io.IOException;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.resources.IWorkspace;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IPath;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.HelpEvent;
import org.eclipse.swt.events.HelpListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.part.FileEditorInput;

import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.workspace.ClovisNavigator;
import com.clovis.cw.workspace.WorkspacePlugin;
import com.clovis.cw.workspace.natures.SystemProjectNature;
import com.clovis.cw.workspace.project.FolderCreator;

/**
 * 
 * @author shubhada
 * Dialog to capture Migration info for projects 
 * 
 */
public class MigrateProjectDialog extends TitleAreaDialog
{
	private IProject _selectedProject;
	private Button _backupButton;
	private Text _newVersionText;
	private Text _oldVersionText;
	private static final String DIALOG_TITLE = "Migrate Project";
	private static final Log LOG = Log.getLog(WorkspacePlugin.getDefault());
	private static MigrateProjectDialog instance = null;

    /**
     * Gets current instance of Dialog
     * @return current instance of Dialog
     */
    public static MigrateProjectDialog getInstance()
    {
        return instance;
    }
    /**
     * Close the dialog.
     * Remove static instance
     * @return super.close()
     */
    public boolean close()
    {
        
        instance = null;
        return super.close();
    }
    /**
     * Open the dialog.
     * Set static instance to itself
     * @return super.open()
     */
    public int open()
    {
        instance = this;
        return super.open();
    }
    /**
     * 
     * @param parentShell - Parent Shell
     * constructor
     * @param selectedProject the project to migrate
     */
    public MigrateProjectDialog(Shell parentShell, IProject selectedProject)
    {
        super(parentShell);
        _selectedProject = selectedProject;
    }
    /**
     * On ok pressed, call the migration action with
     * selected projects
     */
    protected void okPressed()
    {
        IProject [] selProjs = {_selectedProject};
//      Take the back of of all these files
        if (_backupButton.getSelection()) {
            MigrationManager.createBackupFiles(selProjs);
        }
        MigrationManager manager = new MigrationManager(
                _oldVersionText.getText(),
                _newVersionText.getText());
        manager.readFilesAndUpdate(selProjs, false);

        updateScriptsAndTemplates();

		try {
            FileWriter logfile  = new FileWriter(Platform.
                 getInstanceLocation().getURL().getPath().
                 concat("migration_" + _oldVersionText.getText()
                         + "_" + _newVersionText.getText() + ".log"));
            MigrationManager.writeProblemsToLog(selProjs, logfile, manager);           
            logfile.close();
        } catch (IOException e) {
            LOG.error("Migration log cannot be written", e);
        }
        boolean logOpen = MessageDialog.openQuestion(null, "Migration Report",
                "Migration report is written to file '"
                + "migration_" + _oldVersionText.getText()
                + "_" + _newVersionText.getText() + ".log" + "'. Do you want to open the report?");
        if (logOpen) {
            openMigrationReport();
            
        }
        super.okPressed();
       
    }

    /**
     * Updates the Scripts and templates with the latest one.
     */
    private void updateScriptsAndTemplates() {
        FolderCreator fd = new FolderCreator(_selectedProject);

        try {
			fd.copyScript();
			fd.copyTemplates();

		} catch (CoreException e) {
			LOG.error("Migration : Failed to update templates or scripts.", e);

		} catch (IOException e) {
			LOG.error("Migration : Failed to update templates or scripts.", e);
		}
	}

    /**
     * Opens the Migration Report which presents the problem encountered
     * during migration of selected projects
     *
     */
    private void openMigrationReport()
    {
        try{    
            IWorkspace ws = ResourcesPlugin.getWorkspace();
            IProject project = ws.getRoot().getProject("Migration Report");
            if (project.exists()) {
            	project.delete(true, true, null);
                
            }
            project.create(null);
            if (!project.isOpen())
               project.open(null);
            IPath location = new Path(Platform.
                    getInstanceLocation().getURL().getPath().
                    concat("migration_" + _oldVersionText.getText()
                            + "_" + _newVersionText.getText() + ".log"));
            IFile file = project.getFile(location.lastSegment());
            file.createLink(location, IResource.NONE, null);
            IWorkbenchPage page = WorkspacePlugin.getDefault().
            getWorkbench()
                .getActiveWorkbenchWindow().getActivePage();
            if (page != null) {
               page.openEditor(new FileEditorInput(file),
                       "org.eclipse.ui.DefaultTextEditor");
            }
        } catch (CoreException e) {
            
        }
    }

    /**
     * @param parent - Parent Composite
     * Creates the controls in the Dialog Area
     */
    protected Control createDialogArea(Composite parent)
    {
        Composite contents = new Composite(parent, SWT.NONE);
        GridLayout glayout = new GridLayout();
        glayout.numColumns = 2;
        contents.setLayout(glayout);
        contents.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

        Label projectLabel = new Label(contents, SWT.NONE);
        projectLabel.setLayoutData(
            new GridData(GridData.BEGINNING | GridData.FILL));
        projectLabel.setText("Selected project:");

        Text projectText = new Text(contents,
                SWT.SINGLE | SWT.BORDER);
        projectText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        projectText.setEnabled(false);
        projectText.setText(_selectedProject.getName());

        Label oldVersionLabel = new Label(contents, SWT.NONE);
        oldVersionLabel.setLayoutData(
            new GridData(GridData.BEGINNING | GridData.FILL));
        oldVersionLabel.setText("Existing version of the project:");

        _oldVersionText = new Text(contents,
                SWT.SINGLE | SWT.BORDER);
        _oldVersionText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        _oldVersionText.setEnabled(false);
        String comment = null;
		try {
			comment = _selectedProject.getDescription().getComment();
		} catch (CoreException e1) {
			MessageDialog
			.openError(
					getShell(),
					"Version not Available",
					"Project Version not availbale for the currently selected project.");
		}
        String existingVersion = comment.substring(comment.indexOf(":") + 1, comment.length());
        _oldVersionText.setText(existingVersion);
        
        Label newVersionLabel = new Label(contents, SWT.NONE);
        newVersionLabel.setLayoutData(
            new GridData(GridData.BEGINNING | GridData.FILL));
        newVersionLabel.setText("Project will be migrated to version:");

        _newVersionText = new Text(contents,
            SWT.SINGLE | SWT.BORDER);
        _newVersionText.setLayoutData(new GridData(
                GridData.FILL_HORIZONTAL));
        _newVersionText.setEnabled(false);
        _newVersionText.setText(DataPlugin.getProductVersion());
        
        Label backupLabel = new Label(contents, SWT.NONE);
        backupLabel.setLayoutData(
            new GridData(GridData.BEGINNING | GridData.FILL));
        backupLabel.setText("Backup the selected project?");

        _backupButton = new Button(contents, SWT.CHECK);
        _backupButton.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        _backupButton.setSelection(true);
        
        setTitle("Project Migration");
        contents.addHelpListener(new HelpListener() {
			public void helpRequested(HelpEvent e) {
				PlatformUI.getWorkbench().getHelpSystem()
						.displayHelp("com.clovis.cw.help.migrate");
			}
		});
        return contents;
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
		buttonBarSeparator.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));

		Control control = super.createButtonBar(parent);

        boolean isValid = MigrationManager.isVersionValid(_oldVersionText.getText(),
                _newVersionText.getText());
        if (!isValid) {
            setErrorMessage("Migration target version is not newer than the exsisting version.");
            getButton(IDialogConstants.OK_ID).setEnabled(false);;
        }

        return control;
	}

    /**
     * @param shell - New Shell
     * Configures the shell properties
     */
    protected void configureShell(Shell shell)
    {
        super.configureShell(shell);
        shell.setText(DIALOG_TITLE);
    }

    /**
	 * Returns the currently selected project.
	 * 
	 * @return the currently selected project
	 */
	public static IProject getSelectedProject() {
		IWorkbenchPage page = WorkspacePlugin.getDefault().getWorkbench()
				.getActiveWorkbenchWindow().getActivePage();
		if (page != null) {

			ClovisNavigator navigator = ((ClovisNavigator) page
					.findView("com.clovis.cw.workspace.clovisWorkspaceView"));
			if (navigator == null) {
				MessageDialog
						.openWarning(
								null,
								"Empty Project Selection",
								"Open Clovis Workspace View and Select OpenClovis Project to proceed with it.");
				return null;
			}

			ISelection selection = navigator.getTreeViewer().getSelection();
			IProject project = null;

			if (selection instanceof IStructuredSelection) {
				IStructuredSelection sel = (IStructuredSelection) selection;

				if (sel.getFirstElement() instanceof IResource) {
					IResource resource = (IResource) sel.getFirstElement();
					project = resource.getProject();
				}
			}

			try {
				if (project != null
						&& project.exists()
						&& project.isOpen()
						&& project
								.hasNature(SystemProjectNature.CLOVIS_SYSTEM_PROJECT_NATURE)) {

					return project;

				} else {
					MessageDialog
							.openWarning(null, "Empty Project Selection",
									"Select OpenClovis Project to proceed with it.");
				}

			} catch (CoreException e) {
				WorkspacePlugin.LOG.warn(
						"Could not initialize the selected project", e);
			}
		}

		return null;
	}
}

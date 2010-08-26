package com.clovis.cw.workspace.dialog;


import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IFolder;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.Path;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.HelpEvent;
import org.eclipse.swt.events.HelpListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.PlatformUI;

import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.WorkspacePlugin;
import com.clovis.cw.workspace.project.CwProjectPropertyPage;

/**
 * Dialog for add/delete and rename template groups
 * 
 * @author Pushparaj
 * 
 */
public class TemplateGroupConfigurationDialog extends Dialog {

	private static final Log LOG = Log.getLog(CaPlugin.getDefault());

	/**
	 * Represents the Template Group list in UI.
	 */
	private org.eclipse.swt.widgets.List _templateGroupList;

	/**
	 * Represents the Template Group list in Back end.
	 */
	private List _templateGroupModel;

	private IProject _project;

	private Shell _shell;;

	/**
	 * Component to Template mapping.
	 */
	private Resource _compTemplateResource;

	/**
	 * Creates the instance of this dialog.
	 * 
	 * @param shell
	 *            the parent shell
	 */
	public TemplateGroupConfigurationDialog(Shell shell, IProject project) {
		super(shell);

		_shell = shell;
		_project = project;
		_templateGroupModel = getTemplateGroupList(_project);

		ProjectDataModel dataModel = ProjectDataModel
				.getProjectDataModel(_project);
		_compTemplateResource = dataModel.getComponentTemplateModel()
				.getResource();
	}

	@Override
	protected void createButtonsForButtonBar(Composite parent) {
		createButton(parent, 1234, "Close", true).addSelectionListener(
				new SelectionAdapter() {
					public void widgetSelected(SelectionEvent e) {
						close();
					}
				});
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent) {
		Composite composite = new Group(parent, SWT.SHADOW_IN);
		composite.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		composite.setLayout(new GridLayout());

		Group group = new Group(composite, SWT.BORDER);
		group.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		GridLayout groupLayout = new GridLayout(2, false);
		groupLayout.marginWidth = 0;
		groupLayout.marginHeight = 0;
		group.setLayout(groupLayout);

		Group filterListGroup = new Group(group, SWT.BORDER);
		GridLayout filterListGroupLayout = new GridLayout();
		filterListGroupLayout.marginWidth = 0;
		filterListGroupLayout.marginHeight = 0;
		filterListGroup.setLayout(filterListGroupLayout);

		_templateGroupList = new org.eclipse.swt.widgets.List(filterListGroup,
				SWT.V_SCROLL | SWT.H_SCROLL);
		loadFilterListItems();

		GridData listData = new GridData(SWT.FILL, SWT.FILL, true, true);
		Rectangle bounds = Display.getCurrent().getClientArea();
		listData.heightHint = (int) (1.5 * bounds.height / 5);
		listData.widthHint = bounds.width / 5;
		_templateGroupList.setLayoutData(listData);

		Composite buttonPanel = new Composite(group, SWT.NONE);
		RowLayout buttonPanelLayout = new RowLayout(SWT.VERTICAL);
		buttonPanelLayout.marginLeft = 0;
		buttonPanelLayout.marginRight = 0;
		buttonPanelLayout.fill = true;
		buttonPanel.setLayout(buttonPanelLayout);

		Button addButton = new Button(buttonPanel, SWT.PUSH);
		addButton.setText("Add");
		addButton.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				InputDialog addDialog = new InputDialog(_shell, "Add Template Group",
						"Enter Name", "Template", null);
				addDialog.open();
				String grpName = addDialog.getValue();
				if(grpName != null) {
					 createNewTemplateGroup(grpName);
				}
			}
		});

		Button renameButton = new Button(buttonPanel, SWT.PUSH);
		renameButton.setText("Rename");
		renameButton.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				if (_templateGroupList.getSelection().length == 0) {
					MessageDialog.openInformation(_shell, "Selection Not Available",
							"Select Template Group to rename.");
					return;
				}
				String oldVal = _templateGroupList.getSelection()[0].toString();
				InputDialog renameDialog = new InputDialog(_shell,
						"Rename Template Group", "Enter New Name", oldVal, null);

				renameDialog.open();
				String newVal = renameDialog.getValue();
				if(newVal != null) {
					 renameTemplateGroup(oldVal, newVal);
				}
			}
		});

		Button removeButton = new Button(buttonPanel, SWT.PUSH);
		removeButton.setText("Remove");
		removeButton.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				if (_templateGroupList.getSelection().length == 0) {
					MessageDialog.openInformation(_shell, "Selection Not Available",
							"Select Template Group to remove.");
					return;
				}
				String grpName = _templateGroupList.getSelection()[0].toString();
				deleteTemplateGroup(grpName);
			}
		});
		composite.addHelpListener(new HelpListener() {
			public void helpRequested(HelpEvent e) {
				PlatformUI.getWorkbench().getHelpSystem()
						.displayHelp("com.clovis.cw.help.template_create");
			}
		});
		return composite;
	}
	/**
	 * Creates new template group
	 * @param grpName
	 */
	private void createNewTemplateGroup(String grpName) {
		if (isValidTemplateName(grpName)) {
			String options[] = WorkspacePlugin.getCodeGenOptions();
			if(!isExistingTemplateGroup(options, grpName)){
				for (int i = 0; i < options.length; i++) {
					try {
						copyTemplatesToNewTemplateGroup(_project, options, grpName);
					} catch (CoreException e) {
						MessageDialog.openError(_shell, "Error",
						"Unable to create folder for Template Group.");
						return;
					} catch (IOException e) {
						MessageDialog.openError(_shell, "Error",
						"Unable to create folder for Template Group.");
						return;
					}
				}
				
			}else {
				MessageDialog.openError(_shell, "Error!", "Template Group Name already exists!!");
			}

		} else {
			MessageDialog.openError(_shell, "Error",
					"Invalid Template Group Name.");
			return;
		}
		_templateGroupModel.add(grpName);
		loadFilterListItems();
	}

	/**
	 * Rename template group
	 * @param oldVal
	 * @param newVal
	 */
	private void renameTemplateGroup(String oldVal, String newVal) {
		if (!isValidTemplateName(newVal)) {
			MessageDialog.openError(_shell, "Error",
					"Invalid Template Group Name.");
			return;
		}

		if (_templateGroupModel.contains(newVal)) {
			MessageDialog.openInformation(_shell, "Duplicate Template Group",
					"Template Group " + newVal + "Already Defined.");
			return;
		}

		try {
			String options[] = WorkspacePlugin.getCodeGenOptions();
			for (int i = 0; i < options.length; i++) {
				IFolder srcFolder = _project.getFolder(new Path(ICWProject.PROJECT_CODEGEN_FOLDER).append(options[i]).append(oldVal));
				IFolder dstFolder = _project.getFolder(new Path(ICWProject.PROJECT_CODEGEN_FOLDER).append(options[i]).append(newVal));
				if (srcFolder.exists()) {
					srcFolder.move(dstFolder.getFullPath(), true, null);
				}
			}

		} catch (CoreException e) {
			MessageDialog.openError(_shell, "Error",
					"Unable to rename Template Group.");
			return;

		}
		
		//Changing the _grpNameToBeDeleted instance to "default" in Xml file

		EObject rootObject = (EObject) _compTemplateResource.getContents().get(
				0);
		List mapList = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature("safComponent"));

		for (int i = 0; i < mapList.size(); i++) {
			EObject mapObj = (EObject) mapList.get(i);
			String templateGroupName = EcoreUtils.getValue(mapObj,
					"templateGroupName").toString();
			if (templateGroupName.equals(oldVal)) {
				EcoreUtils.setValue(mapObj, "templateGroupName", newVal);
			}
		}

		try {
			EcoreModels.save(_compTemplateResource);
		} catch (IOException e) {
			e.printStackTrace();
		}

		_templateGroupModel.set(_templateGroupModel.indexOf(oldVal), newVal);
		loadFilterListItems();
	}

	/**
	 * Deletes template group
	 * @param grpName
	 */
	private void deleteTemplateGroup(String grpName) {
		try {
			String options[] = WorkspacePlugin.getCodeGenOptions();
			for (int i = 0; i < options.length; i++) {
				IFolder dstFolder = _project.getFolder(new Path(ICWProject.PROJECT_CODEGEN_FOLDER).append(options[i]).append(grpName));
				if (dstFolder.exists()) {
					dstFolder.delete(true, true, null);
				}
			}
		} catch (CoreException e) {
			MessageDialog.openError(_shell, "Error",
					"Unable to delete Template Group.");
			return;
		}
		//Changing the _grpNameToBeDeleted instance to "default" in Xml file

		EObject rootObject = (EObject) _compTemplateResource.getContents().get(
				0);
		List mapList = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature("safComponent"));
		List indicesToBeDeleted = new ArrayList();

		for (int i = 0; i < mapList.size(); i++) {
			EObject mapObj = (EObject) mapList.get(i);
			String templateGroupName = EcoreUtils.getValue(mapObj,
					"templateGroupName").toString();
			if (templateGroupName.equals(grpName)) {
				indicesToBeDeleted.add(new Integer(i));
			}
		}

		for (int i = 0; i < indicesToBeDeleted.size(); i++) {
			mapList.remove((((Integer) indicesToBeDeleted.get(i)).intValue())
					- i);
		}

		try {
			EcoreModels.save(_compTemplateResource);
		} catch (IOException e) {
			e.printStackTrace();
		}

		_templateGroupModel.remove(_templateGroupList.getSelection()[0]);
		loadFilterListItems();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed() {
		super.okPressed();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell shell) {
		shell.setText("Manage Template Group(s)");
		super.configureShell(shell);
	}

	/**
	 * Updates the filter list with current view model value.
	 */
	private void loadFilterListItems() {
		_templateGroupList.setItems((String[]) _templateGroupModel
				.toArray(new String[] {}));
	}

	/**
	 * Validates the folder name. 
	 * @param newTemplateGroup
	 * @return
	 */
	private boolean isValidTemplateName(String newTemplateGroup) {
		Pattern p = Pattern.compile("^[a-zA-Z][a-zA-Z0-9_]*$");
		Matcher m = p.matcher(newTemplateGroup);
		return m.matches();
	}

	/**
	 * Returns the List of Template Groups available.
	 * @return
	 */
	private List getTemplateGroupList(IProject project) {
		if (project != null) {
			try {
				String codeGenOption = CwProjectPropertyPage
						.getCodeGenMode(project);
				String codeGenPath = project.getLocation().append(
						ICWProject.PROJECT_CODEGEN_FOLDER)
						.append(codeGenOption).toOSString();
				String templateGroups[] = new File(codeGenPath).list();
				List templateGroupList = new ArrayList();

				for (int i = 0; i < templateGroups.length; i++) {

					String filePath = codeGenPath + File.separator
							+ templateGroups[i];

					if (new File(filePath).isDirectory()) {
						String templateDirMarker = filePath + File.separator
								+ ICWProject.CW_PROJECT_TEMPLATE_GROUP_MARKER;

						if (!new File(filePath)
								.getName()
								.equals(
										ICWProject.PROJECT_TEMPLATE_FOLDER)
								&& new File(templateDirMarker).isFile())
							templateGroupList.add(templateGroups[i]);
					}
				}

				return templateGroupList;

			} catch (Exception e) {
				LOG.error("Error While Loading Template Group Configuration.",
						e);
			}
		}
		return null;
	}
	/**
	 * Copies required templates to the new template group folder
	 * @param newTemplateGroup
	 * @throws CoreException
	 * @throws IOException
	 */
	private void copyTemplatesToNewTemplateGroup(IProject project, String[] codegenOptions, String newTemplateGroup)
	throws CoreException, IOException {
		for (int i = 0; i < codegenOptions.length; i++) {
			IFolder defaultTemplateFolder = project.getFolder(new Path(ICWProject.PROJECT_CODEGEN_FOLDER).append(codegenOptions[i]).append(ICWProject.PROJECT_TEMPLATE_FOLDER));
			IFolder newTemplateFolder = project.getFolder(new Path(ICWProject.PROJECT_CODEGEN_FOLDER).append(codegenOptions[i]).append(newTemplateGroup));
			if(!newTemplateFolder.exists()) {
				newTemplateFolder.create(true, true, null);
			}
			copyFiles(defaultTemplateFolder.getLocation().toFile().listFiles(), newTemplateFolder);
		}
	}
	/**
	 * Copy files to destination folder
	 * @param files
	 * @param dstFolder
	 * @throws CoreException
	 * @throws IOException
	 */
	private void copyFiles(File files[], IFolder dstFolder) throws CoreException, IOException{
		for (int i = 0; i < files.length; i++) {
			if (!files[i].isDirectory()) {
				IFile dst = dstFolder.getFile(files[i].getName());
				if (dst.exists()) {
					dst.delete(true, true, null);
				}
				dst.create(new FileInputStream(files[i]), true, null);
			}
		}
	}
	/**
	 * Verify whether the template group name is already exist
	 * @param templateGroupName
	 * @param options
	 * @return true if templateGroupName already exist or false
	 */
	private boolean isExistingTemplateGroup(String options[], String templateGroupName){
		for (int i = 0; i < options.length; i++) {
			String templateFolder = _project.getLocation().append(ICWProject.PROJECT_CODEGEN_FOLDER).append(options[i]).append(templateGroupName).toOSString();
			if(new File(templateFolder).exists()) {
				return true;
			}
		}  
		return false;
	}
}

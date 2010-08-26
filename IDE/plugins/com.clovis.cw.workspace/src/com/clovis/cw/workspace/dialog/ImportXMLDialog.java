/*
 * @(#) $RCSfile: ImportXMLDialog.java,v $
 * $Revision: #3 $ $Date: 2007/04/30 $
 *
 * Copyright (C) 2005 -- Clovis Solutions.
 * Proprietary and Confidential. All Rights Reserved.
 *
 * This software is the proprietary information of Clovis Solutions.
 * Use is subject to license terms.
 *
 */
package com.clovis.cw.workspace.dialog;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.Path;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.DirectoryDialog;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;

import com.clovis.common.utils.UtilsPlugin;
import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.workspace.WorkspacePlugin;
import com.clovis.cw.workspace.project.ValidationConstants;
import com.clovis.cw.workspace.utils.ObjectAdditionHandler;

/**
 * 
 * @author shubhada
 * 
 * Dialog to capture the Project XML directory path 
 * 
 */
public class ImportXMLDialog extends TitleAreaDialog
{
    private IProject _project = null;
    private Text _xmlPathText = null;
    private static final Log LOG = Log.getLog(WorkspacePlugin.getDefault());
    
    /**
     * 
     * @param parentShell - Shell
     * @param project - IProject
     */
    public ImportXMLDialog(Shell parentShell, IProject project)
    {
        super(parentShell);
        _project = project;
    }
    /**
     * @param parent - Parent Composite
     * Creates the controls in the Dialog Area
     */
    protected Control createDialogArea(Composite parent)
    {
        Composite contents = new Composite(parent, org.eclipse.swt.SWT.NONE);
        GridLayout glayout = new GridLayout();
        glayout.numColumns = 3;
        contents.setLayout(glayout);
        contents.setLayoutData(new GridData(GridData.FILL_BOTH));
        
        Label oldVersionLabel = new Label(contents, SWT.NONE);
        oldVersionLabel.setLayoutData(
                new GridData(GridData.BEGINNING | GridData.FILL));
        oldVersionLabel.setText("Enter Project XML Directory");
        
        _xmlPathText = new Text(contents, SWT.SINGLE | SWT.BORDER);
        _xmlPathText.setEditable(false);
        _xmlPathText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));

        // Add Browse Button.
        Button button = new Button(contents, SWT.PUSH);
        button.setText("Browse...");
        button.addSelectionListener(new SelectionListener() {
            /**
             * Open File Dialog.
             * @param e Event
             */
            public void widgetSelected(SelectionEvent e)
            {
                DirectoryDialog dialog =
                    new DirectoryDialog(getShell(), SWT.NONE);
                dialog.setFilterPath(UtilsPlugin.getDialogSettingsValue("IMPORTXML"));
                String fileName = dialog.open();
                UtilsPlugin.saveDialogSettings("IMPORTXML", fileName);
                if (fileName != null) {
                    _xmlPathText.setText(fileName);
                }
            }
            /**
             * Does Nothig.
             * @param e Event
             */
            public void widgetDefaultSelected(SelectionEvent e)
            {
            }
        });
        setMessage("Enter the import details", IMessageProvider.INFORMATION);
        setTitle("Import Project XML Files");
        return contents;
    }
    /**
     * Import/Convert the Project XML files to OpenClovis internal format
     */
    protected void okPressed()
    {
        try {
            processProjectXMLFiles();
        } catch (CoreException e) {
            LOG.error("Unhandled Error while importing project XML files", e);
        } catch (IOException e) {
            LOG.error("Unhandled Error while importing project XML files", e);
        }
        super.okPressed();
    }
    
    private void processProjectXMLFiles() throws CoreException, IOException
    {
        String dir = _xmlPathText.getText();
        if (!dir.equals("")) {
            
            // Process resource information
            String resourceXMLPath = dir
                + File.separator
                + ICWProject.PROJECT_RESOURCE_DATA_FILE_NAME;
            File file = new File(resourceXMLPath);
            URI uri = URI.createFileURI(resourceXMLPath);
            if (file.exists()) {
                Resource resource =  EcoreModels.
                    getUpdatedResource(uri);
                ObjectAdditionHandler handler = new ObjectAdditionHandler(
                        _project, ValidationConstants.CAT_RESOURCE_EDITOR);
                handler.processProjectXMLData((EObject)
                        resource.getContents().get(0));
            } else {
                LOG.warn(ICWProject.PROJECT_RESOURCE_DATA_FILE_NAME +
                        " file is not found in directory " + dir);
            }
            
            // Process component information
            String componentXMLPath = dir
                + File.separator
                + ICWProject.PROJECT_COMPONENT_DATA_FILE_NAME;
            file = new File(componentXMLPath);
            uri = URI.createFileURI(componentXMLPath);
            if (file.exists()) {
                Resource resource =  EcoreModels.
                    getUpdatedResource(uri);
                ObjectAdditionHandler handler = new ObjectAdditionHandler(
                        _project, ValidationConstants.CAT_COMPONENT_EDITOR);
                handler.processProjectXMLData((EObject)
                    resource.getContents().get(0));
            } else {
                LOG.warn(ICWProject.PROJECT_COMPONENT_DATA_FILE_NAME +
                    " file is not found in directory " + dir);
            }
            
            // copy all the other files to their respective directories
            String alarmXMLPath = dir
                + File.separator
                + ICWProject.PROJECT_ALARM_DATA_FILE_NAME;
            file = new File(alarmXMLPath);
            if (file.exists()) {
                IFile projFile = _project.getFile(new Path(ICWProject.
                        CW_PROJECT_MODEL_DIR_NAME).append(ICWProject.
                                ALARM_PROFILES_XMI_DATA_FILENAME));
                if (projFile.exists()) {
                    projFile.delete(true, true, null);
                }
                projFile.create(new FileInputStream(file), true, null);
            } else {
                LOG.warn(ICWProject.PROJECT_ALARM_DATA_FILE_NAME +
                        " file is not found in directory " + dir);
            }
            
            String amfConfigXMLPath = dir
                + File.separator
                + ICWProject.PROJECT_AMF_CONFIGURATION_FILE_NAME;
            file = new File(amfConfigXMLPath);
            if (file.exists()) {
                IFile projFile = _project.getFile(new Path(ICWProject.
                    CW_PROJECT_CONFIG_DIR_NAME).append(ICWProject.
                            CPM_XML_DATA_FILENAME));
                if (projFile.exists()) {
                    projFile.delete(true, true, null);
                }
                projFile.create(new FileInputStream(file), true, null);
            } else {
                LOG.warn(ICWProject.PROJECT_AMF_CONFIGURATION_FILE_NAME +
                    " file is not found in directory " + dir);
            }
            
            String iocConfigXMLPath = dir
                + File.separator
                + ICWProject.PROJECT_IOC_CONFIGURATION_FILE_NAME;
            file = new File(iocConfigXMLPath);
            if (file.exists()) {
                IFile projFile = _project.getFile(new Path(ICWProject.
                        CW_PROJECT_CONFIG_DIR_NAME).append(ICWProject.
                                IOCBOOT_XML_DATA_FILENAME));
                if (projFile.exists()) {
                    projFile.delete(true, true, null);
                }
                projFile.create(new FileInputStream(file), true, null);
            } else {
                LOG.warn(ICWProject.PROJECT_IOC_CONFIGURATION_FILE_NAME +
                " file is not found in directory " + dir);
            }
            
            String slotInfoXMLPath = dir
                + File.separator
                + ICWProject.PROJECT_SLOT_CONFIGURATION_FILE_NAME;
            file = new File(slotInfoXMLPath);
            if (file.exists()) {
                IFile projFile = _project.getFile(new Path(ICWProject.
                        CW_PROJECT_CONFIG_DIR_NAME).append(ICWProject.
                                SLOT_INFORMATION_XML_FILENAME));
                if (projFile.exists()) {
                    projFile.delete(true, true, null);
                }
                projFile.create(new FileInputStream(file), true, null);
            } else {
                LOG.warn(ICWProject.PROJECT_SLOT_CONFIGURATION_FILE_NAME +
                " file is not found in directory " + dir);
            }
            
            String gmsConfigXMLPath = dir
                + File.separator
                + ICWProject.PROJECT_GMS_CONFIGURATION_FILE_NAME;
            file = new File(gmsConfigXMLPath);
            if (file.exists()) {
                IFile projFile = _project.getFile(new Path(ICWProject.
                        CW_PROJECT_CONFIG_DIR_NAME).append(ICWProject.
                                GMS_CONFIGURATION_XML_FILENAME));
                if (projFile.exists()) {
                    projFile.delete(true, true, null);
                }
                projFile.create(new FileInputStream(file), true, null);
            } else {
                LOG.warn(ICWProject.PROJECT_GMS_CONFIGURATION_FILE_NAME +
                " file is not found in directory " + dir);
            }
            
            String logXMLPath = dir
                + File.separator
                + ICWProject.PROJECT_LOG_CONFIGURATION_FILE_NAME;
            file = new File(logXMLPath);
            if (file.exists()) {
                IFile projFile = _project.getFile(new Path(ICWProject.
                        CW_PROJECT_CONFIG_DIR_NAME).append(ICWProject.
                                LOG_CONFIGS_XML_FILENAME));
                if (projFile.exists()) {
                    projFile.delete(true, true, null);
                }
                projFile.create(new FileInputStream(file), true, null);
            } else {
                LOG.warn(ICWProject.PROJECT_LOG_CONFIGURATION_FILE_NAME +
                " file is not found in directory " + dir);
            }
            
            String compileConfigXMLPath = dir
                + File.separator
                + ICWProject.PROJECT_COMPILE_CONFIGURATION_FILE_NAME;
            file = new File(compileConfigXMLPath);
            if (file.exists()) {
                IFile projFile = _project.getFile(new Path(ICWProject.
                        CW_PROJECT_CONFIG_DIR_NAME).append(ICWProject.
                                COMPILE_CONFIGS_XMI_FILENAME));
                if (projFile.exists()) {
                    projFile.delete(true, true, null);
                }
                projFile.create(new FileInputStream(file), true, null);
            } else {
                LOG.warn(ICWProject.PROJECT_COMPILE_CONFIGURATION_FILE_NAME +
                " file is not found in directory " + dir);
            }
            
            String idlXMLPath = dir
                + File.separator
                + ICWProject.PROJECT_IDL_FILE_NAME;
            file = new File(idlXMLPath);
            if (file.exists()) {
                IFile projFile = _project.getFile(new Path(ICWProject.
                        CW_PROJECT_IDL_DIR_NAME).append(ICWProject.
                                IDL_XML_DATA_FILENAME));
                if (projFile.exists()) {
                    projFile.delete(true, true, null);
                }
                projFile.create(new FileInputStream(file), true, null);
            } else {
                LOG.warn(ICWProject.PROJECT_IDL_FILE_NAME +
                " file is not found in directory " + dir);
            }
            
        
        }
    }
    /**
     * Configures the shell parameters
     * 
     * @param newShell - Shell
     */
	protected void configureShell(Shell newShell)
	{
		newShell.setText(_project.getName() + " - " + "Import XML Files");
		newShell.setSize(400, 250);
		super.configureShell(newShell);
	}
    
}

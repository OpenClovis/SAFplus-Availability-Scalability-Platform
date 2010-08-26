/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/dialog/ResourceClassModelPage.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.dialog;

import java.io.File;
import java.util.List;
import java.util.StringTokenizer;
import java.util.regex.Pattern;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.wizard.WizardDialog;
import org.eclipse.jface.wizard.WizardPage;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;

import com.clovis.common.utils.editor.EditorUtils;
import com.clovis.common.utils.ui.TextComboButtonListener;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * 
 * @author shubhada
 *
 * Wizard Page which captures the Blade details from the user
 */
public class ResourceClassModelPage extends WizardPage
{
	private IProject _project = null;
    private CCombo _bladeTypeCombo = null;
    private Text _maxInstancesText = null;
    public static String BLADE_TYPE_CUSTOM = "Default";
    
    /**
     * Constructor
     * @param pageName - Name of the page
     */
    protected ResourceClassModelPage(String pageName, IProject project)
    {
        super(pageName);
        _project = project;
    }
    /**
     * Creates the controls in the page
     * @param parent - Parent Composite
     */
    public void createControl(Composite parent)
    {
        Composite container = new Composite(parent, SWT.NONE);
        container.setLayoutData(new GridData(GridData.FILL_BOTH));
        GridLayout containerLayout = new GridLayout();
        containerLayout.numColumns = 2;
        container.setLayout(containerLayout);
        
        Label bladeTypeLabel = new Label(container, SWT.NONE);
        bladeTypeLabel.setLayoutData(
            new GridData(GridData.BEGINNING | GridData.FILL));
        bladeTypeLabel.setText("Select Blade Type");
        
        String templateFilePath = _project.getLocation().toOSString()
        						+ File.separator
        						+ ICWProject.CW_PROJECT_MODEL_DIR_NAME
        						+ File.separator
        						+ ICWProject.RESOURCE_TEMPLATE_FOLDER;
        File file = new File(templateFilePath);
        if(!file.exists())
        	file.mkdir();
        String [] templates = file.list();
        String [] bladeTypes = new String[templates.length];
        for (int i = 0; i < templates.length; i++)
        {
            String filePath = templateFilePath + File.separator + templates[i];
            if (new File(filePath).isDirectory()) {
              String templateDirMarker = filePath + File.separator + ICWProject.CW_PROJECT_TEMPLATE_GROUP_MARKER;
              if (new File(templateDirMarker).isFile())
        	bladeTypes[i] = new StringTokenizer(templates[i], ".").nextToken();
            }
        }
        _bladeTypeCombo = new CCombo(container, SWT.BORDER | SWT.SINGLE | SWT.READ_ONLY);
        for (int i = 0; i < bladeTypes.length; i++) {
            _bladeTypeCombo.add(bladeTypes[i]);
        }
        _bladeTypeCombo.add(BLADE_TYPE_CUSTOM);
        _bladeTypeCombo.setText(BLADE_TYPE_CUSTOM);
        GridData gd = new GridData();
        gd.horizontalAlignment = GridData.FILL;
        gd.grabExcessHorizontalSpace = true;
        _bladeTypeCombo.setLayoutData(gd);
        _bladeTypeCombo.addSelectionListener(new TextComboButtonListener() {
            /**
             * @param e - SelectionEvent
             */
            public void widgetSelected(SelectionEvent e)
            {
                SpecificBladeTypePage page = (SpecificBladeTypePage)
                    ((AddResourceWizard) getWizard()).
                    getBladeTypeDetailsPage();
                String selbladeType = _bladeTypeCombo.getText();
                if (!selbladeType.equals(BLADE_TYPE_CUSTOM)) {
                	List templateObjs = ((AddResourceWizard) getWizard()).
                		getObjectAdditionHandler().readTemplateFile(selbladeType);
                	int hardwareResCount = 0;
                	for (int i = 0; i < templateObjs.size(); i++) {
                		EObject eobj = (EObject) templateObjs.get(i);
                		if (eobj.eClass().getName().equals(
                				ClassEditorConstants.HARDWARE_RESOURCE_NAME)) {
                			hardwareResCount++;
                		}
                	}
                	page.getNameText().setText(selbladeType);
                    page.getNameText().setEnabled(false);
                    page.getHardwareText().setText(String.valueOf(hardwareResCount));
                    page.getHardwareText().setEnabled(false);
                } else {
                    ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
                    GenericEditorInput geInput = (GenericEditorInput) pdm.
                        getCAEditorInput();
                    List editorViewModelList = geInput.getModel().getEList();
                	String bladeName = EditorUtils.getNextValue("Blade",
                            editorViewModelList, ClassEditorConstants.CLASS_NAME);
                	page.getNameText().setText(bladeName);
                    page.getNameText().setEnabled(true);
                    page.getHardwareText().setText("0");
                    page.getHardwareText().setEnabled(true); 
                }
            }
        });
        
        Label maxInstLabel = new Label(container, SWT.NONE);
        maxInstLabel.setLayoutData(
            new GridData(GridData.BEGINNING | GridData.FILL));
        maxInstLabel.setText("Enter the maximum numbers of blades of above type");
        
        _maxInstancesText = new Text(container, SWT.BORDER);
        GridData gd1 = new GridData();
        gd1.horizontalAlignment = GridData.FILL;
        gd1.grabExcessHorizontalSpace = true;
        _maxInstancesText.setLayoutData(gd1);
        _maxInstancesText.setText("0");
        _maxInstancesText.addModifyListener(new TextComboButtonListener() {
            /**
             * @param e - ModifyEvent
             */
            public void modifyText(ModifyEvent e)
            {
                String text = _maxInstancesText.getText();
                checkMaxInstances(text);
                ((WizardDialog) getContainer()).updateButtons();
            }
        });
        setTitle("Enter Blade Details");
        this.setControl(container);
        
    }
    /**
     * @param msg - Message String
     */
    public void setMessage(String msg, int type)
    {
        super.setMessage(msg, type);
    }
    /**
     * 
     * @return the Selected Blade Type
     */
    public String getSelectedBladeType()
    {
        return _bladeTypeCombo.getText();
    }
    /**
     * 
     * @return the Maximum number of blades present in the system
     * of selected Blade type
     */
    public int getMaximumInstances()
    {
        return Integer.parseInt(_maxInstancesText.getText());
    }
    /**
     * @see org.eclipse.jface.wizard.IWizardPage#isPageComplete()
     */
    private boolean checkMaxInstances(String number)
    {
    	if (!Pattern.compile("^[0-9][0-9]*$").matcher(
                number).matches()) {
            setMessage(
              "The value entered for 'Maximum Number of Blades' is not a valid number", IMessageProvider.ERROR);
            return false;
        } else {
            setMessage("", IMessageProvider.NONE);
            return true;
        }
    }
    public boolean isPageComplete()
    {
        if (checkMaxInstances(_maxInstancesText.getText()))
		{
			return true;
		} else {
			return false;
		}
    }
}

/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/dialog/SpecificBladeTypePage.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.dialog;

import java.util.List;
import java.util.regex.Pattern;

import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.wizard.WizardDialog;
import org.eclipse.jface.wizard.WizardPage;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;

import com.clovis.common.utils.editor.EditorUtils;
import com.clovis.common.utils.ui.TextComboButtonListener;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.workspace.utils.ObjectAdditionHandler;

/**
 * 
 * @author shubhada
 *
 * Wizard page to collect Specific Blade Type Details 
 */
public class SpecificBladeTypePage extends WizardPage
{
    private List _editorViewModelList = null;
    private Text _bladeNameText = null;
    private Text _numHardwareText = null;
    private Text _numSoftwareText = null;
    
    /**
     * Constructor
     * 
     * @param pageName - Page Name
     */
    protected SpecificBladeTypePage(String pageName, List viewModelList)
    {
        super(pageName);
        _editorViewModelList = viewModelList;
        
    }
    /**
     * creates the page controls
     * @param parent - Parent Composite
     */
    public void createControl(Composite parent)
    {
        Composite container = new Composite(parent, SWT.NONE);
        container.setLayoutData(new GridData(GridData.FILL_BOTH));
        GridLayout containerLayout = new GridLayout();
        containerLayout.numColumns = 2;
        container.setLayout(containerLayout);
        
        Label nameLabel = new Label(container, SWT.NONE);
        nameLabel.setLayoutData(
            new GridData(GridData.BEGINNING | GridData.FILL));
        nameLabel.setText("Enter the blade name");
        
        
        String bladeName = EditorUtils.getNextValue("Blade",
                _editorViewModelList, ClassEditorConstants.CLASS_NAME);
        
        _bladeNameText = new Text(container, SWT.BORDER);
        GridData nameGd = new GridData();
        nameGd.horizontalAlignment = GridData.FILL;
        nameGd.grabExcessHorizontalSpace = true;
        _bladeNameText.setLayoutData(nameGd);
        _bladeNameText.setText(bladeName);
        _bladeNameText.addModifyListener(new TextComboButtonListener() {
            /**
             * @param e - ModifyEvent
             */
            public void modifyText(ModifyEvent e)
            {
                String text = _bladeNameText.getText();
                checkBladeName(text);
                ((WizardDialog) getContainer()).updateButtons();
            }
        });   
        
        Label numSoftwareLabel = new Label(container, SWT.NONE);
        numSoftwareLabel.setLayoutData(
            new GridData(GridData.BEGINNING | GridData.FILL));
        numSoftwareLabel.setText("Enter the number of managed resources");
        
        _numSoftwareText = new Text(container, SWT.BORDER);
        GridData gd1 = new GridData();
        gd1.horizontalAlignment = GridData.FILL;
        gd1.grabExcessHorizontalSpace = true;
        //gd1.grabExcessVerticalSpace = true;
        _numSoftwareText.setLayoutData(gd1);
        _numSoftwareText.setText("0");
        _numSoftwareText.addModifyListener(new TextComboButtonListener() {
            /**
             * @param e - ModifyEvent
             */
            public void modifyText(ModifyEvent e)
            {
                String text = _numSoftwareText.getText();
                checkNumberOfSW(text);
                ((WizardDialog) getContainer()).updateButtons();
            }
        });
        setTitle("Enter System Details");
        this.setControl(container);
    }
    /**
     * 
     * @param name - Blade Name
     * @return true if the Blade Name is valid else return false
     */
    private boolean checkBladeName(String name)
    {
        if (!Pattern.compile("^[a-zA-Z][a-zA-Z0-9_]{0,79}$").matcher(
                name).matches()) {
            setMessage("The blade name is invalid",
                    IMessageProvider.ERROR);
            return false;
        } else {
            if (!ObjectAdditionHandler.isUniqueObject(name, _editorViewModelList)) {
                setMessage("Duplicate value entered for blade name",
                        IMessageProvider.ERROR);
                return false;
            } else {
                setMessage("", IMessageProvider.NONE);
                return true;
            }
        }
        
    }
    /**
     * Validate Text field which contains Number of HW resources 
     * @param number text which contains no.of HW resources
     * @return true or false
     */
    private boolean checkNumberOfHW(String number)
    {
    	if (!Pattern.compile("^[0-9][0-9]*$").matcher(
                number).matches()) {
            setMessage(
              "The value entered for 'Number of Hardware Resources' is not a valid number",
              IMessageProvider.ERROR);
            return false;
        } else {
            setMessage("", IMessageProvider.NONE);
            return true;
        }
    }
    /**
     * Validate Text field which contains Number of SW resources 
     * @param number text which contains no.of SW resources
     * @return true or false
     */
    private boolean checkNumberOfSW(String number)
    {
    	if (!Pattern.compile("^[0-9][0-9]*$").matcher(
                number).matches()) {
            setMessage(
              "The value entered for 'Number of Software Resources' is not a valid number",
              IMessageProvider.ERROR);
            return false;
        } else {
            setMessage("", IMessageProvider.NONE);
            return true;
        }
    }
    /**
     * 
     * @return the number of hardware resources
     */
    public int getNumberOfHardwareResources()
    {
        return Integer.parseInt(_numHardwareText.getText());
    }
    /**
     * 
     * @return the number of software resources
     */
    public int getNumberOfSoftwareResources()
    {
        return Integer.parseInt(_numSoftwareText.getText());
    }
    /**
     * 
     * @return the number of hardware resources text box
     */
    public Text getHardwareText()
    {
        return _numHardwareText;
    }
    /**
     * 
     * @return the blade name text box
     */
    public Text getNameText()
    {
        return _bladeNameText;
    }
    /**
     * @see org.eclipse.jface.wizard.IWizardPage#isPageComplete()
     */
    public boolean isPageComplete()
    {
        if (checkBladeName(_bladeNameText.getText())
				&& checkNumberOfSW(_numSoftwareText.getText())) {
			return true;
		} else {
			return false;
		}
    }
    
}

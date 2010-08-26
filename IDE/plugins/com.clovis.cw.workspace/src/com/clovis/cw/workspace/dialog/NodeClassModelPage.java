/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/dialog/NodeClassModelPage.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.dialog;

import java.util.List;
import java.util.regex.Pattern;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.ecore.EEnum;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.wizard.WizardDialog;
import org.eclipse.jface.wizard.WizardPage;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;

import com.clovis.common.utils.editor.EditorUtils;
import com.clovis.common.utils.ui.TextComboButtonListener;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.utils.ObjectAdditionHandler;
/**
 * 
 * @author shubhada
 *
 * Wizard page to capture Node and other logical SAF entity
 * details from the user
 */
public class NodeClassModelPage extends WizardPage
{
    private IProject _project = null;
    private List _editorViewModelList = null;
    private Text _nameText = null;
    private Text _numCompText = null;
    private CCombo _nodeClassCombo = null;
    private EEnum _uiNodeClassEnum = null, _nodeClassEnum = null;
    /**
     * Constructor
     * @param pageName - Page Name
     */
    protected NodeClassModelPage(String pageName, IProject project)
    {
        super(pageName);
        _project = project;
        ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
        GenericEditorInput geInput = (GenericEditorInput) pdm.
            getComponentEditorInput();
        _editorViewModelList = geInput.getModel().getEList();
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
        
        Label nameLabel = new Label(container, SWT.NONE);
        nameLabel.setLayoutData(
            new GridData(GridData.BEGINNING | GridData.FILL));
        nameLabel.setText("Enter node name");
        
        String nodeName = EditorUtils.getNextValue(ComponentEditorConstants.NODE_NAME,
                _editorViewModelList, ClassEditorConstants.CLASS_NAME);
        
        _nameText = new Text(container, SWT.BORDER);
        GridData nameGd = new GridData();
        nameGd.horizontalAlignment = GridData.FILL;
        nameGd.grabExcessHorizontalSpace = true;
        _nameText.setLayoutData(nameGd);
        _nameText.setText(nodeName);
        _nameText.addModifyListener(new TextComboButtonListener() {
            /**
             * @param e - ModifyEvent
             */
            public void modifyText(ModifyEvent e)
            {
                String text = _nameText.getText();
                checkNodeName(text);
                ((WizardDialog) getContainer()).updateButtons();
            }
        });
        
        Label nodeClassLabel = new Label(container, SWT.NONE);
        nodeClassLabel.setLayoutData(
            new GridData(GridData.BEGINNING | GridData.FILL));
        nodeClassLabel.setText("Select Node Class");
        
        ObjectAdditionHandler handler = ((AddComponentWizard) getWizard()).
        	getObjectAdditionHandler();
        EPackage ePackage = handler.getEPackage();
        _uiNodeClassEnum = (EEnum) ePackage.getEClassifier(
        		"UIAMSNodeClassType");
        _nodeClassEnum = (EEnum) ePackage.getEClassifier(
			"AmsNodeClass");
        List literals = _uiNodeClassEnum.getELiterals();
        
        _nodeClassCombo = new CCombo(container, SWT.BORDER | SWT.SINGLE | SWT.READ_ONLY);
        
        GridData gd = new GridData();
        gd.horizontalAlignment = GridData.FILL;
        gd.grabExcessHorizontalSpace = true;
        _nodeClassCombo.setLayoutData(gd);
        for (int i = 0; i < literals.size(); i++) {
        	EEnumLiteral literal = (EEnumLiteral) literals.get(i);
        	_nodeClassCombo.add(literal.getName());
        }
        _nodeClassCombo.select(0);

        Label compLabel = new Label(container, SWT.NONE);
        compLabel.setLayoutData(
            new GridData(GridData.BEGINNING | GridData.FILL));
        compLabel.setText("Number of programs to autocreate");
        
        _numCompText = new Text(container, SWT.BORDER);
        GridData compGd = new GridData();
        compGd.horizontalAlignment = GridData.FILL;
        compGd.grabExcessHorizontalSpace = true;
        _numCompText.setLayoutData(compGd);
        _numCompText.setText("1");
        _numCompText.addModifyListener(new TextComboButtonListener() {
            /**
             * @param e - ModifyEvent
             */
            public void modifyText(ModifyEvent e)
            {
                String text = _numCompText.getText();
                if(checkNumberOfComp(text)) {
                	((AddComponentWizard) getWizard()).
                    getAssociateResourcePage().initComponentList(
                            Integer.parseInt(text), _editorViewModelList);
                }
                ((WizardDialog) getContainer()).updateButtons();
            }
        });
        setTitle("Enter Node Details");
        this.setControl(container);
    }
    /**
     * 
     * @return the Node Name
     */
    public String getNodeName()
    {
        return _nameText.getText();
    }
    /**
     * 
     * @return the Number of components under the node
     */
    public int getNumberOfComponents()
    {
        return Integer.parseInt(_numCompText.getText());
    }
    /**
     * 
     * @return the Selected class for the node
     */
    public String getNodeClassType()
    {
    	String selClass = _nodeClassCombo.getText();
    	EEnumLiteral uiLiteral = _uiNodeClassEnum.getEEnumLiteral(selClass);
    	EEnumLiteral literal = _nodeClassEnum.getEEnumLiteral(uiLiteral.getValue());
        return literal.getName();
    }
    /**
     * Check if the page is complete
     */
    public boolean isPageComplete()
    {
        if (checkNodeName(_nameText.getText())
				&& checkNumberOfComp(_numCompText.getText())) {
			return true;
		} else {
			return false;
		}
    }
    /**
	 * Checks whether Node name is according to the pattern and also checks for
	 * duplicate node name
	 * 
	 * @param name -
	 *            Node Name
	 * @return true if Node Name is valid else return false
	 */
    private boolean checkNodeName(String name)
    {
        if (!Pattern.compile("^[a-zA-Z][a-zA-Z0-9_]{0,79}$").matcher(
                name).matches()) {
            setMessage("The node name is invalid",
                    IMessageProvider.ERROR);
            return false;
        } else {
            if (!ObjectAdditionHandler.isUniqueObject(name, _editorViewModelList)) {
                setMessage("Duplicate value entered for node name",
                        IMessageProvider.ERROR);
                return false;
            } else {
                setMessage("", IMessageProvider.NONE);
                return true;
            }
        }
        
    }
    /**
     * Checks Number of SAF Components
     * @param number text which contains no.of comps
     * @return true or false
     */
    private boolean checkNumberOfComp(String number)
    {
    	if (!Pattern.compile("^[0-9][0-9]*$").matcher(
                number).matches()) {
            setMessage(
              "The value entered for 'Number of Components' is not a valid number",
              IMessageProvider.ERROR);
            return false;
        } else {
            setMessage("", IMessageProvider.NONE);
            return true;
        }
    }
}

/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/dialog/AddComponentWizard.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.dialog;

import org.eclipse.core.resources.IProject;
import org.eclipse.jface.wizard.IWizardPage;
import org.eclipse.jface.wizard.Wizard;

import com.clovis.cw.genericeditor.GECommandStack;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.project.ValidationConstants;
import com.clovis.cw.workspace.utils.ObjectAdditionHandler;
/**
 * 
 * @author shubhada
 *
 * Wizard to capture the Logical Component Information
 * of the user systems to generate the SAF entity
 * Classes   
 */
public class AddComponentWizard extends Wizard
{
    private IProject _project = null;
    private IWizardPage _page1 = null;
    private IWizardPage _page2 = null;
    private ObjectAdditionHandler _handler = null;
    public static final String DIALOG_TITLE = "Add New Node";
    /**
     * Constructor
     * @param project - Project
     */
    public AddComponentWizard(IProject project)
    {
        _project = project;
        _handler = new ObjectAdditionHandler(_project,
                ValidationConstants.CAT_COMPONENT_EDITOR);
        _page1 = new NodeClassModelPage("Node Type Page", _project);
        addPage(_page1);
        _page2 = new AssociateResourceWizardPage("Associate Resources", _project);
        addPage(_page2);
    }
    /**
     * Implementation which specifies what to do
     * when finish button is pressed
     */
    public boolean performFinish()
    {
        ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
        GenericEditorInput geInput = (GenericEditorInput) pdm.
            getComponentEditorInput();
        GECommandStack commandStack = (GECommandStack) geInput.getEditor().
            getCommandStack();
        AddNewComponentCommand command = new AddNewComponentCommand(_project,
        		_handler, (NodeClassModelPage) _page1, (AssociateResourceWizardPage) _page2);
        commandStack.execute(command);
        return true;
    }
    /**
     * Return the wizard title
     */
    public String getWindowTitle()
    {
        return DIALOG_TITLE;
        
    }
    /**
     * 
     * @return the ObjectAdditionHandler instance
     */
    public ObjectAdditionHandler getObjectAdditionHandler()
    {
    	return _handler;
    }
    /**
     * 
     * @return the AssociateResourceWizardPage
     */
    public AssociateResourceWizardPage getAssociateResourcePage()
    {
        return (AssociateResourceWizardPage) _page2;
    }
}

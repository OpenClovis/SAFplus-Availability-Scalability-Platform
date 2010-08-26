/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/dialog/AddResourceWizard.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.dialog;

import java.util.List;

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
 * Wizard to capture the Physical Resource Information
 * of the user systems to generate the Resource
 * Classes   
 *  
 */
public class AddResourceWizard extends Wizard
{
    private IProject _project = null;
    private IWizardPage _page1 = null;
    private IWizardPage _page2 = null;
    private ObjectAdditionHandler _handler = null;
    public static final String DIALOG_TITLE = "Add New Blade Type";
    /**
     * Constructor
     * @param project - Project
     */
    public AddResourceWizard(IProject project)
    {
        _project = project;
        _handler = new ObjectAdditionHandler(_project,
                ValidationConstants.CAT_RESOURCE_EDITOR);
        _page1 = new ResourceClassModelPage("Blade Type Page", _project);
        addPage(_page1);
        ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
        GenericEditorInput geInput = (GenericEditorInput) pdm.
            getCAEditorInput();
        List editorViewModelList = geInput.getModel().getEList();
        _page2 = new SpecificBladeTypePage("Specific Blade Details", editorViewModelList);
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
            getCAEditorInput();
        GECommandStack commandStack = (GECommandStack) geInput.getEditor().
            getCommandStack();
        AddNewResourceCommand command = new AddNewResourceCommand(_project, _handler,
                (ResourceClassModelPage) _page1, (SpecificBladeTypePage) _page2);
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
     * @return the Blade Type Details Page
     */
    public IWizardPage getBladeTypeDetailsPage()
    {
        return _page2;
    }
    /**
     * 
     * @return the ObjectAdditionHandler
     */
    public ObjectAdditionHandler getObjectAdditionHandler()
    {
    	return _handler;
    }
}

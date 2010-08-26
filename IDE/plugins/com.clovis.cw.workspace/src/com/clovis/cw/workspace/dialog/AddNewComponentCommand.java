/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/dialog/AddNewComponentCommand.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.dialog;

import java.util.List;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.gef.commands.Command;

import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.genericeditor.model.EditorModel;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.utils.ObjectAdditionHandler;
/**
 * 
 * @author shubhada
 *
 * Command Class to implement the undo and
 * redo actions for Addition of New Class
 * Objects in to the editor through wizard
 */
public class AddNewComponentCommand extends Command
{
    private IProject _project = null;
    private List _newObjList = null;
    private String _nodeName = null, _nodeClass = null;
    private int _numComponents;
    private List _compsList = null;
    private ObjectAdditionHandler _handler = null;
    /**
     * Constructor
     * @param project - Project
     * @param handler - ObjectAdditionHandler
     * @param page1 - First wizard page
     */
    public AddNewComponentCommand(IProject project,
    		ObjectAdditionHandler handler,
    		NodeClassModelPage page1, AssociateResourceWizardPage page2)
    {
        _project = project;
        _handler = handler;
        _nodeName = page1.getNodeName();
        _nodeClass = page1.getNodeClassType();
        _numComponents = page1.getNumberOfComponents();
        _compsList = page2.getComponentList();
    }
    /**
     * Executes the command
     */
    public void execute()
    {
        addComponent();
    }
    /**
     * Implementation of Redo
     */
    public void redo()
    {
        addComponent();
    }
    /**
     * Implementation of Undo
     */
    public void undo() {
    	EditorModel editorModel = getEditorModel();
		if (_newObjList != null) {
			for (int i = 0; i < _newObjList.size(); i++) {
				EObject eobj = (EObject) _newObjList.get(i);
				editorModel.removeEObject(eobj);
			}
		}
	}
    /**
	 * Adds the component template objects to the Editor
	 * 
	 */
    public void addComponent()
    {
        _newObjList = _handler.addDefaultComponentObjects(_nodeName, _nodeClass,
        		_numComponents, _compsList);
    }
    /**
     * 
     * @return the EditorModel of the Editor
     */
    private EditorModel getEditorModel()
    {
        ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
        GenericEditorInput geInput = (GenericEditorInput) pdm.
            getComponentEditorInput();
        return geInput.getEditor().getEditorModel();
    }
}

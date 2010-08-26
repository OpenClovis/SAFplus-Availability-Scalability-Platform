/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/dialog/AddNewResourceCommand.java $
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
import org.eclipse.emf.ecore.EReference;
import org.eclipse.gef.commands.Command;

import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
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
public class AddNewResourceCommand extends Command
{
    private IProject _project = null;
    private String _selBladeType = null;
    private int _bladeMaxInst;
    private List _newObjList = null;
    private String _bladeName = null;
    private int _numHardwareRes, _numSoftwareRes;
    private ObjectAdditionHandler _handler = null;
    /**
     * 
     * @param project - IProject
     * @param handler - ObjectAdditionHandler
     * @param page1 - ResourceClassModel
     * @param page2 - SpecificBladeTypePage
     */
    public AddNewResourceCommand(IProject project,
    		ObjectAdditionHandler handler,
    		ResourceClassModelPage page1,
            SpecificBladeTypePage page2)
    {
        _project = project;
        _handler = handler;
        _selBladeType = ((ResourceClassModelPage) page1).
            getSelectedBladeType();
        _bladeMaxInst = ((ResourceClassModelPage) page1).
            getMaximumInstances();
        _numHardwareRes = 0;
        _numSoftwareRes = page2.getNumberOfSoftwareResources();
        _bladeName = page2.getNameText().getText();
    }
    /**
     * Executes the command
     */
    public void execute()
    {
        addResource();
    }
    /**
     * Implementation of Redo
     */
    public void redo()
    {
        addResource();
    }
    /**
     * Implementation of Undo
     */
    public void undo() {
		//List viewModelList = getViewModelList();
    	EditorModel editorModel = getEditorModel();
		if (_newObjList != null) {
			for (int i = 0; i < _newObjList.size(); i++) {
				EObject eobj = (EObject) _newObjList.get(i);
				editorModel.removeEObject(eobj);
			}
		}
	}
    /**
	 * Adds the resource template objects to the Editor
	 * 
	 */
    public void addResource()
    {
        if (_selBladeType.equals(ResourceClassModelPage.BLADE_TYPE_CUSTOM)) {
            _newObjList = _handler.addDefaultResourceObjects(_numHardwareRes,
                    _numSoftwareRes, _bladeMaxInst, _bladeName);
        } else {
            List templateObjs = _handler.readTemplateFile(_selBladeType);
            EObject infoObj = (EObject) templateObjs.get(0);
            List objList = new ClovisNotifyingListImpl();
            List refList = infoObj.eClass().getEAllReferences();
            for (int i = 0; i < refList.size(); i++) {
                EReference ref = (EReference) refList.get(i);
                Object val = infoObj.eGet(ref);
                if (val != null) {
                    if (val instanceof EObject) {
                        objList.add(val);
                    } else if (val instanceof List) {
                        List valList = (List) val;
                        for (int j = 0; j < valList.size(); j++) {
                            objList.add(valList.get(j));
                           
                        }
                        
                    }
                }
            }
            _newObjList = _handler.processTemplateData(objList,
                    _selBladeType, _numSoftwareRes, _bladeMaxInst);
        }
    }
    /**
     * 
     * @return the EditorModel of the Editor
     */
    private EditorModel getEditorModel()
    {
        ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
        GenericEditorInput geInput = (GenericEditorInput) pdm.getCAEditorInput();
        return geInput.getEditor().getEditorModel();
    }
    
}

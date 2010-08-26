/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/project/ClovisProjectCreationWizard.java $
 * $Author: srajyaguru $
 * $Date: 2007/04/30 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.project;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import org.eclipse.core.resources.ICommand;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IProjectDescription;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.resources.IncrementalProjectBuilder;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;

import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.cw.genericeditor.GenericEditor;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.genericeditor.commands.AutoArrangeCommand;
import com.clovis.cw.genericeditor.editparts.BaseDiagramEditPart;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.ClovisNavigator;
import com.clovis.cw.workspace.WorkspacePlugin;
import com.clovis.cw.workspace.action.OpenComponentEditorAction;
import com.clovis.cw.workspace.action.OpenResourceEditorAction;
import com.clovis.cw.workspace.natures.SystemProjectNature;
import com.clovis.cw.workspace.project.wizard.BladeInfo;
import com.clovis.cw.workspace.project.wizard.NodeInfo;
import com.clovis.cw.workspace.project.wizard.ProgramNameInfo;
import com.clovis.cw.workspace.utils.ObjectAdditionHandler;

/**
 * @author pushparaj
 *
 * Clovis System Project Creation Wizard
 */
public class ClovisProjectCreationWizard extends ClovisNewProjectResourceWizard
{
	ProjectDataModel dataModel;
	private IProject newProject;
    /**
     * Callback of finish Button.
     * @return true If all settings are properly done.
     */
    public boolean performFinish()
    {
        if (!super.performFinish()) {
            return false;
        }
        newProject  = getNewProject();
        String   projectName = newProject.getName();
        try {
            updatePerspective();
            selectAndReveal(newProject);
            // Am putting this method above as the data files need to
            // be available before the data can be read from them
            new FolderCreator(newProject).createAll();
            /*MakefileGenerator.generateMake(newProject);
            MakefileGenerator.generateEnv(newProject);*/
            dataModel = ProjectDataModel.getProjectDataModel(newProject);
            newProject.refreshLocal(IResource.DEPTH_INFINITE, null);
            IProjectDescription description = newProject.getDescription();
            ICommand[] commands = description.getBuildSpec();

            // add builder to project
            ICommand command = description.newCommand();
            command.setBuilderName(
                "com.clovis.cw.workspace.builders.ClovisBuilder");
            
            command.setBuilding(IncrementalProjectBuilder.AUTO_BUILD, false);
            command.setBuilding(IncrementalProjectBuilder.FULL_BUILD, true);
            ICommand[] newCommands = new ICommand[commands.length + 1];
            newCommands[0] = command;
            System.arraycopy(commands, 0, newCommands, 1, commands.length);
            description.setBuildSpec(newCommands);
            
            // adding system project nature
            String natures[] = description.getNatureIds();
    		String newNatures[] = new String[natures.length + 1];
    		System.arraycopy(natures, 0, newNatures, 0, natures.length);
    		newNatures[natures.length] = SystemProjectNature.CLOVIS_SYSTEM_PROJECT_NATURE;
    		description.setNatureIds(newNatures);
    	
            newProject.setDescription(description, null);
                        
            OpenResourceEditorAction.openResourceEditor(newProject);
            updateResourceDefaultModel();
            OpenComponentEditorAction.openComponentEditor(newProject);
            updateComponentDefaultModel();
            ClovisNavigator navigator = ((ClovisNavigator)WorkspacePlugin.getDefault().getWorkbench()
        		   .getActiveWorkbenchWindow().getActivePage()
        		   .findView("com.clovis.cw.workspace.clovisWorkspaceView"));
            if(navigator != null)
            {
            	navigator.getViewer().refresh();
            }
            return true;
        } catch (Exception e) {
            WorkspacePlugin.LOG.error("Create Error:[" + projectName + "]", e);
        }
        return false;
    }
    
    private void updateResourceDefaultModel()
    {
    	GenericEditorInput caInput = (GenericEditorInput) dataModel.getCAEditorInput();
    	GenericEditor editor = caInput.getEditor();
    	ObjectAdditionHandler handler = new ObjectAdditionHandler(newProject,
                ValidationConstants.CAT_RESOURCE_EDITOR);
    	Vector tasks = _bladesList.getTasks();
    	for (int k = 0; k < tasks.size(); k++)
    	{
    		BladeInfo blade = (BladeInfo) tasks.get(k);
    		String bladeType = blade.getBladeType();
    		int numMaxInst = blade.getNumberOfBlades();
    		int numSWRes = blade.getNumberOfSW();
    		String bladeName = blade.getBladeName();
        	if(bladeType.equals("Default")) {
    			handler.addDefaultResourceObjects(0, numSWRes, numMaxInst, bladeName);
    		} else {
    			List templateObjs = handler.readTemplateFile(bladeType);
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
                handler.processTemplateData(objList,
                        bladeType, numSWRes, numMaxInst);
    		}
    	}
    	BaseDiagramEditPart editPart = (BaseDiagramEditPart) editor
		.getViewer().getRootEditPart().getContents();
		AutoArrangeCommand cmd = new AutoArrangeCommand(editPart);
		cmd.execute();
		editor.propertyChange(null);
    	editor.doSave(null);
    }
    private void updateComponentDefaultModel()
    {
    	GenericEditorInput compInput = (GenericEditorInput) dataModel.getComponentEditorInput();
    	GenericEditor editor = compInput.getEditor();
    	ObjectAdditionHandler handler = new ObjectAdditionHandler(newProject,
                ValidationConstants.CAT_COMPONENT_EDITOR);
    	List modelList = editor.getEditorModel().getEList();
    	Vector tasks = _nodesList.getTasks();
    	for (int k = 0; k < tasks.size(); k++)
    	{
    		NodeInfo node = (NodeInfo) tasks.get(k);
    		String nodeName = node.getNodeName();
    		String nodeClass = node.getNodeClass();
    		
    		// for this component node retrieve all of the program names
    		// that were entered in the third page of the wizard
    		ArrayList progNames = new ArrayList();
    		Iterator iter = _programNamesList.getTasks().iterator();
    		while (iter.hasNext())
    		{
    			ProgramNameInfo info = ((ProgramNameInfo) iter.next());
    			if (info.getNodeTypeName().equals(node.getNodeName()))
    			{
    				progNames.add(info.getProgramName());
    			}
    		}

    		List compsList = handler.createComponents(progNames.size(), modelList, progNames);
    		handler.addDefaultComponentObjects(nodeName, nodeClass, progNames.size(), compsList);
    	}
    	BaseDiagramEditPart editPart = (BaseDiagramEditPart) editor
		.getViewer().getRootEditPart().getContents();
		AutoArrangeCommand cmd = new AutoArrangeCommand(editPart);
		cmd.execute();
		editor.propertyChange(null);
    	editor.doSave(null);
    }
}

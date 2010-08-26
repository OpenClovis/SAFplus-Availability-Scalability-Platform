package com.clovis.cw.workspace.action;

import java.io.File;
import java.util.List;
import java.util.ArrayList;

import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.ui.IViewActionDelegate;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;

import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.genericeditor.GEDataUtils;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.dialog.AssociateTemplateGroupDialog;
import com.clovis.cw.workspace.project.CwProjectPropertyPage;


/**
 * Action class for associating component with template group
 * 
 * @author ravi
 *
 */
public class AssociateTemplateGroupAction  extends CommonMenuAction implements
IViewActionDelegate, IWorkbenchWindowActionDelegate, ICWProject {

	/**
     * Initializes the View.
     * @param view ViewPart
     */
    public void init(IViewPart view){
    }

	public void dispose() {
		
	}

	public void init(IWorkbenchWindow window) {
	
	}

    /**
     * Handling action
     * @param args 0 - EObject for Component from Selection
     * @return true if action is successfull else false.
     */
    public void run(IAction action)
    {
                        
        String templatesPath = _project.getLocation().toString() + 
        						System.getProperty("file.separator") + 
        						PROJECT_CODEGEN_FOLDER + System.getProperty("file.separator") + CwProjectPropertyPage.getCodeGenMode(_project);
                
        File templateGroups[] = new File(templatesPath).listFiles();
        List templateGroupList = new ArrayList();
        
        for(int i=0; i<templateGroups.length; i++){
        	
        	File templateFolder = templateGroups[i];
        	
        	if(templateFolder.isDirectory()){
        		String templateDirMarker = templateFolder.getAbsolutePath() + File.separator
						+ CW_PROJECT_TEMPLATE_GROUP_MARKER;
        		if(new File(templateDirMarker).isFile()){
        			templateGroupList.add(templateFolder.getName());
        		}
        	}
        }
        
        String templateGroupArray[] = new String[templateGroupList.size()];
        for(int i=0; i<templateGroupList.size();i++){
        	templateGroupArray[i] = (String) templateGroupList.get(i);
        }
        
        List safComponentList = GEDataUtils.getNodeListFromType(
				ProjectDataModel.getProjectDataModel(_project)
						.getComponentModel().getEList(), ComponentEditorConstants.SAFCOMPONENT_NAME);
        
        if(safComponentList.size() != 0) {
        	
        	if(templateGroupArray.length > 1 ) {
                
            	new AssociateTemplateGroupDialog(_shell, _project, templateGroupArray).open();
            
            }else{
            	
            	// No template group defined
            	MessageDialog.openError(_shell, "Error", "Only default template group is present.");
            }
        }else{
        	
        	// There is no SAFCompoent
        	MessageDialog.openError(_shell, "Error", "There is no SAF Component in the model.");
        }
        	
    }
	
}

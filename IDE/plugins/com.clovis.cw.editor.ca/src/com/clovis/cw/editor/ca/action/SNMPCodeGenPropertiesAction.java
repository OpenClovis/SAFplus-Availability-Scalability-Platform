package com.clovis.cw.editor.ca.action;

import java.io.File;
import java.net.URL;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.IActionClassAdapter;
import com.clovis.cw.genericeditor.editparts.AbstractComponentNodeEditPart;
import com.clovis.cw.genericeditor.editparts.BaseEditPart;
import com.clovis.cw.genericeditor.model.NodeModel;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.editor.ca.dialog.SNMPCodeGenConfigDialog;

/**
 * Action class to capture SNMP code generation properties
 * @author Pushparaj
 *
 */
public class SNMPCodeGenPropertiesAction extends IActionClassAdapter{
	
	private Shell _shell = null;
	/**
     * Visible only for SNMP Agent
     * @param environment Environment
     * @return true for snmp agent.
     */
	public boolean isVisible(Environment environment)
    {
    	boolean retValue = false;
        StructuredSelection selection =
            (StructuredSelection) environment.getValue("selection");
        
        if (selection.size() == 1 && selection.getFirstElement() instanceof AbstractComponentNodeEditPart) {
        	AbstractComponentNodeEditPart part = (AbstractComponentNodeEditPart) selection.getFirstElement();
        	NodeModel model = (NodeModel) part.getModel();
        	EObject obj = model.getEObject();
        	if(obj != null && obj.eClass().getName().equals(ComponentEditorConstants.SAFCOMPONENT_NAME)) {
        		boolean isSubAgent = Boolean.parseBoolean(String.valueOf(EcoreUtils.getValue(obj, ComponentEditorConstants.IS_SNMP_SUBAGENT)));
        		_shell = (Shell) environment.getValue("shell");
        		return isSubAgent;
        	}
        }         
        return retValue;        
    }
    /**
     * @see com.clovis.common.utils.menu.IActionClassAdapter#run(java.lang.Object[])
     */
    public boolean run(Object[] args) {
     	StructuredSelection selection = (StructuredSelection) args[1];
		BaseEditPart cep = (BaseEditPart) selection.getFirstElement();
		NodeModel model = (NodeModel) cep.getModel();
		EObject compObj = model.getEObject();
		IProject project = model.getRootModel().getProject();
		openSNMPConfigDialog(compObj, project, _shell);
		return true;
    }
    /**
     * Opens SNMP configuration dialog
     * @param compObj
     * @param project
     * @param shell
     */
    public static void openSNMPConfigDialog(EObject compObj, IProject project, Shell shell) {
    	String compName = EcoreUtils.getName(compObj);
    	EObject mapObj = null;
		Resource resource = null;
		URL caURL = DataPlugin.getDefault().find(
                new Path("model" + File.separator
                         + "compmibmap.ecore"));
        try {
            File ecoreFile = new Path(Platform.resolve(caURL).getPath())
                    .toFile();
            EPackage pack = EcoreModels.getUpdated(ecoreFile.getAbsolutePath());
            String dataFilePath = project.getLocation().toOSString()
                                  + File.separator
                                  + ICWProject.CW_PROJECT_MODEL_DIR_NAME
                                  + File.separator
                                  + "compmibmap.xmi";
            File xmiFile = new File(dataFilePath);
            URI uri = URI.createFileURI(dataFilePath);
            resource = xmiFile.exists() ? EcoreModels.
                    getUpdatedResource(uri) : EcoreModels.create(uri);
            NotifyingList list = (NotifyingList) resource.getContents();
            for (int i = 0; i < list.size(); i++) {
            	EObject obj = (EObject) list.get(i);
            	String name = (String) EcoreUtils.getValue(obj, "compName");
            	if(compName.equals(name)) {
            		mapObj = obj;
            		break;
            	}
            }
            if(mapObj == null) {
            	mapObj = EcoreUtils.createEObject((EClass)pack.getEClassifier("Map"), false);
            	EcoreUtils.setValue(mapObj, "compName", compName);
            	EcoreUtils.setValue(mapObj, "moduleName", "");
            	EcoreUtils.setValue(mapObj, "mibPath", "");
            	list.add(mapObj);
            }
            SNMPCodeGenConfigDialog dialog = new SNMPCodeGenConfigDialog(shell, mapObj, project);
    		if(dialog.open() == dialog.OK) 
    			resource.save(null);
        } catch(Exception e) {
        	e.printStackTrace();
        }
    }
}

package com.clovis.cw.workspace.dialog;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IFolder;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IPath;
import org.eclipse.core.runtime.Path;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * Dialog to delete a template group
 * @author ravi
 *
 */
public class DeleteTemplateGroupDialog extends TitleAreaDialog implements
		ICWProject {

	private IProject _project;
	private String[] _templateGroupArray;
	private Resource _compTemplateResource;
    private String _grpNameToBeDeleted;
	
	public DeleteTemplateGroupDialog(Shell parentShell, IProject _project, String[] templateGroupArray) {
		super(parentShell);
		this._project = _project;
		this._templateGroupArray = templateGroupArray;
		ProjectDataModel dataModel = ProjectDataModel.getProjectDataModel(_project);
		//_templateGroupModel.readTemplateGroupModel();
		_compTemplateResource = dataModel.getComponentTemplateModel().getResource();
	}
	
	
	
	protected Control createDialogArea(Composite parent) {
		Composite composite = new Composite(parent, SWT.NONE);
		composite.setLayoutData(new GridData(GridData.FILL_BOTH));
		composite.setLayout(new GridLayout(2, false));
		getShell().setText("Delete Template Group");
		
		
		new Label(composite, SWT.NONE).setText("Delete Template Group : ");
		CCombo combo = new CCombo(composite, SWT.READ_ONLY);
		combo.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		combo.setItems(_templateGroupArray);
		
		combo.select(0);
		

		return composite;
	}

	protected void okPressed() {
		Control controls[] = ((Composite) getDialogArea()).getChildren();
		_grpNameToBeDeleted = ((CCombo) controls[1]).getText();
		
		
		try{
			deleteTemplateGroup();
		}catch(CoreException e){
			e.printStackTrace();
		}catch(IOException e){
			e.printStackTrace();
		}
		
		//Changing the _grpNameToBeDeleted instance to "default" in Xml file
		
		EObject rootObject = (EObject) _compTemplateResource.getContents().get(0); 
		List mapList = (EList) rootObject.eGet(rootObject.eClass().getEStructuralFeature("safComponent"));
		List indicesToBeDeleted = new ArrayList();
		
		for(int i=0; i<mapList.size(); i++){
			EObject mapObj = (EObject) mapList.get(i);
			String templateGroupName = EcoreUtils.getValue(mapObj, "templateGroupName").toString();
			if(templateGroupName.equals(_grpNameToBeDeleted)){
				indicesToBeDeleted.add(new Integer(i));
			}
		}
		
		for(int i=0; i<indicesToBeDeleted.size(); i++){
			mapList.remove((((Integer) indicesToBeDeleted.get(i)).intValue())-i);
		}
				
		try {
 			EcoreModels.save(_compTemplateResource);
		} catch (IOException e) {
			e.printStackTrace();
		}
		
		super.okPressed();
	}
	

	private void deleteTemplateGroup()
		throws CoreException, IOException {
	
		String templateGroupPath = _project.getLocation()
				+ File.separator + PROJECT_TEMPLATE_FOLDER + File.separator
				+ _grpNameToBeDeleted;
			
		File templatesDir  = new File(templateGroupPath);
	    File[] templates   = templatesDir.listFiles();
	    IPath dstPrefix  = new Path(PROJECT_TEMPLATE_FOLDER).append(_grpNameToBeDeleted);
	    for (int i = 0; i < templates.length; i++) {
	        IFile dst = _project.getFile(
	                dstPrefix.append(templates[i].getName()));
	        if (dst.exists()) {
	            dst.delete(true, true, null);
	        }
	    }
	    IFolder dstFolder = _project.getFolder(dstPrefix);
        if (dstFolder.exists()) {
            dstFolder.delete(true, true, null);
        }
	}
}
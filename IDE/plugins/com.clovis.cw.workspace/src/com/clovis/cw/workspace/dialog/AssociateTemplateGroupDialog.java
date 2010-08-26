package com.clovis.cw.workspace.dialog;

import java.io.IOException;
import java.util.HashMap;
import java.util.List;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.custom.TableEditor;
import org.eclipse.swt.events.FocusAdapter;
import org.eclipse.swt.events.FocusEvent;
import org.eclipse.swt.events.HelpEvent;
import org.eclipse.swt.events.HelpListener;
import org.eclipse.swt.events.MouseAdapter;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.TableItem;
import org.eclipse.ui.PlatformUI;

import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.genericeditor.GEDataUtils;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * Dialog for associating component to templateGroup
 * 
 * @author ravi
 *
 */
public class AssociateTemplateGroupDialog extends TitleAreaDialog {

	
	private String[] _templateGroupArray;
	private List _safComponentList;
    private Resource _compTemplateResource;
    private EClass _safCompClass;
    private HashMap _templateGroupMap = new HashMap(); 

	final int compColumn = 0;
	final int groupColumn = 1;

	public AssociateTemplateGroupDialog(Shell parentShell, IProject project, String [] templateGroupArray) {
		super(parentShell);
        super.setShellStyle(SWT.CLOSE | SWT.RESIZE | SWT.APPLICATION_MODAL);
		_templateGroupArray = templateGroupArray;
		ProjectDataModel dataModel = ProjectDataModel.getProjectDataModel(project);
		_safComponentList = GEDataUtils.getNodeListFromType(
				dataModel.getComponentModel().getEList(), ComponentEditorConstants.SAFCOMPONENT_NAME);
		Model templateModel = dataModel.getComponentTemplateModel();
		_compTemplateResource = templateModel.getResource();
		_safCompClass = (EClass) templateModel.getEPackage().getEClassifier("SAFComponent");
		initTemplateGroupMap();
		
	}	
	private void initTemplateGroupMap() {
		
		for(int i=0 ; i<_safComponentList.size() ; i++) {
        	EObject eObj = (EObject) _safComponentList.get(i);
        	String compName = EcoreUtils.getName(eObj);
        	EObject rootObject = (EObject) _compTemplateResource.getContents().get(0);
        	EReference ref = (EReference) rootObject.eClass().getEStructuralFeature("safComponent");
        	List templateMapList = (EList) rootObject.eGet(ref);
        	for (int j = 0; j < templateMapList.size(); j++) {
        		EObject mapObj = (EObject) templateMapList.get(j);
        		String name = EcoreUtils.getValue(mapObj, "name").toString();
        		if (name.equals(compName)) {
        			String templateGrp = EcoreUtils.getValue(mapObj, "templateGroupName").toString();
        			_templateGroupMap.put(compName, templateGrp);
        		}
        	}
        }
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.TitleAreaDialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	protected Control createDialogArea(Composite parent) {

		getShell().setText("Associate Template Group");
    	setTitle("Associate Component to Template Group");

    	Composite composite = new Composite(parent, SWT.NONE);
		composite.setLayoutData(new GridData(GridData.FILL_BOTH));
		composite.setLayout(new GridLayout());

		final Table table = new Table(composite, SWT.NONE);
		table.setLayoutData(new GridData(GridData.FILL_BOTH));
		table.setHeaderVisible(true);
		table.setLinesVisible(true);

		final TableEditor editor = new TableEditor(table);
		editor.grabHorizontal = true;

		new TableColumn(table, SWT.NONE);
		new TableColumn(table, SWT.NONE);

		table.addMouseListener(new MouseAdapter() {
			public void mouseDown(MouseEvent e) {

				TableItem item;
				for (int i = 0; i < table.getItemCount(); i++) {
					item = table.getItem(i);

					if (item.getBounds(groupColumn).contains(e.x, e.y)) {

						if(editor.getEditor() != null){
							editor.getEditor().dispose();
						}

						final CCombo combo = new CCombo(table, SWT.READ_ONLY);
						combo.setItems(_templateGroupArray);

						combo.addFocusListener(new FocusAdapter() {

							public void focusLost(FocusEvent e) {
								Object data = combo.getData();

								if(data != null && data instanceof TableItem) {
									((TableItem) data).setText(groupColumn, combo.getText());
									combo.dispose();
								}
							}
						});

						combo.setText(item.getText(groupColumn));
						combo.setData(item);

						editor.setEditor(combo, item, groupColumn);
						break;
					}
				}
			}
		});

		TableItem tableItem;
		for(int i=0 ; i<_safComponentList.size() ; i++) {

			String compName = EcoreUtils.getName((EObject) _safComponentList.get(i));
    		String grpName = (String) _templateGroupMap.get(compName);

    		if(grpName == null) {
    			grpName = ICWProject.PROJECT_TEMPLATE_FOLDER;

    		} else {
    			boolean deleted = true;

    			for(int j=0; j<_templateGroupArray.length ; j++) {
    				if(grpName.equals(_templateGroupArray[j])){
    					deleted = false;
    					break;
    				}   					
    			}

    			if(deleted) {
    				grpName = ICWProject.PROJECT_DEFAULT_CODEGEN_OPTION;
    			}
    		}
 
    		tableItem = new TableItem(table, SWT.NONE);
    		tableItem.setText(new String[]{compName, grpName});
    	}

    	TableColumn[] tableColumns = table.getColumns();
		tableColumns[0].setText("Component Name");
		tableColumns[0].pack();
		tableColumns[1].setText("Template Group");
		tableColumns[1].pack();
		composite.addHelpListener(new HelpListener() {
			public void helpRequested(HelpEvent e) {
				PlatformUI.getWorkbench().getHelpSystem()
						.displayHelp("com.clovis.cw.help.template_associate");
			}
		});
		return composite;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	protected void okPressed() {
		Table table = (Table) ((Composite) getDialogArea()).getChildren()[0];

		EObject rootObject = (EObject) _compTemplateResource.getContents().get(0); 
		List mapList = (EList) rootObject.eGet(rootObject.eClass().getEStructuralFeature("safComponent"));
		mapList.clear();

		TableItem item;
		for(int i=0; i<table.getItemCount(); i++) {

			item = table.getItem(i);
			String grpName = item.getText(groupColumn);

			if(!grpName.equals(ICWProject.PROJECT_DEFAULT_CODEGEN_OPTION)) {
				String compName = item.getText(compColumn);

				EObject mapObj = EcoreUtils.createEObject(_safCompClass, true);
				EcoreUtils.setValue(mapObj, "name", compName);
				EcoreUtils.setValue(mapObj, "templateGroupName", grpName);

				mapList.add(mapObj);
			}
		}
		
		try {
 			EcoreModels.save(_compTemplateResource);
		} catch (IOException e) {
			e.printStackTrace();
		}
		
		super.okPressed();
		
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell) {
		super.configureShell(newShell);
		Rectangle bounds = Display.getCurrent().getClientArea();
		newShell.setBounds((int) (1.5 * bounds.width / 5), bounds.height / 5,
				2 * bounds.width / 5, 2 * bounds.height / 5);
	}
}

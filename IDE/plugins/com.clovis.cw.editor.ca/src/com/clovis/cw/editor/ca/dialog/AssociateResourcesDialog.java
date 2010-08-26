/*
 * @(#) $RCSfile: AssociateResourcesDialog.java,v $
 * $Revision: #7 $ $Date: 2007/05/09 $
 *
 * Copyright (C) 2005 -- Clovis Solutions.
 * Proprietary and Confidential. All Rights Reserved.
 *
 * This software is the proprietary information of Clovis Solutions.
 * Use is subject to license terms.
 *
 */
/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/AssociateResourcesDialog.java $
 * $Author: bkpavan $
 * $Date: 2007/05/09 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.editor.ca.dialog;

import java.util.HashMap;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.HelpEvent;
import org.eclipse.swt.events.HelpListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.List;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.PlatformUI;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.ui.list.ListView;
import com.clovis.cw.editor.ca.ComponentEditor;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.project.data.SubModelMapReader;
/**
 * @author pushparaj
 *
 * Dialog for Associate Resources for Components
 */
public class AssociateResourcesDialog extends TitleAreaDialog
{
    private EList     _associatedResourcesList = new ClovisNotifyingListImpl();
    private EList     _resourcesList;
    private ListView  _resourceListViewer;
    private ListView  _associateResourceListViewer;
    private EObject   _selComp;
    private Model     _viewModel;
    private ComponentEditor _editor;

    /**
     * Constructor
     *
     * @param parentShell Parent Shell
     * @param resourcesList Resources List
     * @param obj EObject
     */
    public AssociateResourcesDialog(Shell parentShell, IProject project, EList resourcesList,
            EObject obj)
    {
        super(parentShell);
        super.setShellStyle(SWT.CLOSE|SWT.MIN|SWT.MAX|SWT.RESIZE|SWT.SYSTEM_MODAL);
        _resourcesList = resourcesList;
        _selComp = obj;
        ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(project);
        GenericEditorInput geInput = (GenericEditorInput) pdm.getComponentEditorInput();
        _editor = (ComponentEditor) geInput.getEditor();
        EObject mapObj = _editor.getLinkViewModel().getEObject();
        EObject linkObj = SubModelMapReader.getLinkObject(mapObj, ComponentEditorConstants.
        		ASSOCIATE_RESOURCES_NAME);
        _viewModel = new Model(_editor.getLinkViewModel().getResource(), linkObj).getViewModel();
        String rdn = (String) EcoreUtils.getValue(obj, ModelConstants.RDN_FEATURE_NAME);
        NotifyingList associatedResourcesList = (NotifyingList) SubModelMapReader.
        	getLinkTargetObjects(_viewModel.getEObject(), rdn);
        if (associatedResourcesList == null) {
        	associatedResourcesList = (NotifyingList) SubModelMapReader.createLinkTargets(
        			_viewModel.getEObject(),
        			EcoreUtils.getName(obj), rdn);
        }
        HashMap resMap = new HashMap();
        
        for (int i=0; i< resourcesList.size(); i++)
        {
            EObject resObj = (EObject) resourcesList.get(i);
            resMap.put(EcoreUtils.getName(resObj), resObj);
            
        }
        for (int i=0; i< associatedResourcesList.size(); i++)
        {
            EObject resObj = (EObject) resMap.get(associatedResourcesList.get(i));
            if (resObj != null){
            	_associatedResourcesList.add(resObj);
                _resourcesList.remove(resObj);
            }
        }
        
    }
    /**
     * @see org.eclipse.jface.dialogs.Dialog#createDialogArea
     *      (org.eclipse.swt.widgets.Composite)
     */
    protected Control createDialogArea(Composite parent)
    {
        Composite composite = new Composite(parent, SWT.NONE);
        GridLayout layout = new GridLayout();
        GridData data = new GridData(GridData.FILL_BOTH);
        layout.numColumns = 5;
        composite.setLayout(layout);
        composite.setLayoutData(data);

        Group resourceGroup = new Group(composite, SWT.BORDER);
        resourceGroup.setText("Available Resources");
        GridLayout layout1 = new GridLayout();
        layout1.numColumns = 1;
        resourceGroup.setLayout(layout1);
        data = new GridData(GridData.FILL_HORIZONTAL | GridData.FILL_VERTICAL);
        data.horizontalSpan = 2;
        data.grabExcessHorizontalSpace = true;
        resourceGroup.setLayoutData(data);
        List resourceList = new List(resourceGroup, SWT.FULL_SELECTION
                                                    | SWT.HIDE_SELECTION
                                                    | SWT.V_SCROLL
                                                    | SWT.H_SCROLL | SWT.BORDER
                                                    | SWT.MULTI);
        _resourceListViewer = new ListView(resourceList);
        _resourceListViewer.setInput(_resourcesList);
        GridData listData = new GridData(SWT.FILL, SWT.FILL, true, true);
		Rectangle bounds = Display.getCurrent().getClientArea();
		listData.heightHint = (int) (1.5 * bounds.height / 9);
		listData.widthHint = bounds.width / 12;
		resourceList.setLayoutData(listData);
		
        Composite buttonControl = new Composite(composite, SWT.NONE);
        GridLayout layout3 = new GridLayout();
        layout3.numColumns = 1;
        buttonControl.setLayout(layout3);
        data = new GridData(GridData.HORIZONTAL_ALIGN_CENTER
                            | GridData.FILL_VERTICAL);
        data.horizontalSpan = 1;
        buttonControl.setLayoutData(data);
        Button rightButton = new Button(buttonControl, SWT.BORDER);
        rightButton.setText(">>");
        data = new GridData(GridData.FILL_HORIZONTAL);
        rightButton.setLayoutData(data);
        Button leftButton = new Button(buttonControl, SWT.BORDER);
        leftButton.setText("<<");
        data = new GridData(GridData.FILL_HORIZONTAL);
        leftButton.setLayoutData(data);

        Group associateResourceGroup = new Group(composite, SWT.BORDER);
        associateResourceGroup.setText("Associated Resources");
        GridLayout layout2 = new GridLayout();
        layout2.numColumns = 1;
        associateResourceGroup.setLayout(layout2);
        data = new GridData(GridData.FILL_BOTH);
        data.horizontalSpan = 2;
        associateResourceGroup.setLayoutData(data);
        List associateResourceList = new List(associateResourceGroup,
                SWT.FULL_SELECTION | SWT.HIDE_SELECTION | SWT.V_SCROLL
                | SWT.H_SCROLL | SWT.BORDER | SWT.MULTI);
        _associateResourceListViewer = new ListView(associateResourceList);
        _associateResourceListViewer.setInput(_associatedResourcesList);
        listData = new GridData(SWT.FILL, SWT.FILL, true, true);
		bounds = Display.getCurrent().getClientArea();
		listData.heightHint = (int) (1.5 * bounds.height / 9);
		listData.widthHint = bounds.width / 12;
		associateResourceList.setLayoutData(listData);
		
        rightButton.addSelectionListener(new AddListener());
        leftButton.addSelectionListener(new DeleteListener());

        _resourceListViewer.refresh();
        String title = "Associate Resources";
        setMessage(title + " for component", IMessageProvider.INFORMATION);
        setTitle(title);
        parent.getShell().setText(title);
        final String contextid = "com.clovis.cw.help.realize";
		composite.addHelpListener(new HelpListener() {

			public void helpRequested(HelpEvent e) {
				PlatformUI.getWorkbench().getHelpSystem().displayHelp(
						contextid);
			}
		});
        return composite;
    }
    /**
     * see org.eclipse.jface.dialogs.Dialog#okPressed()
     */
    protected void okPressed()
    {
    	String rdn = (String) EcoreUtils.getValue(_selComp, ModelConstants.RDN_FEATURE_NAME);
    	NotifyingList associatedResourcesList = (NotifyingList) SubModelMapReader.getLinkTargetObjects(
    			_viewModel.getEObject(), rdn);
    	associatedResourcesList.clear();
    	for (int i = 0; i < _associatedResourcesList.size(); i++) {
    		associatedResourcesList.add(EcoreUtils.getName(
    				(EObject) _associatedResourcesList.get(i)));
    	}
        _viewModel.save(false);
        _editor.propertyChange(null);
        super.okPressed();
    }
    /**
	 * 
	 * @author pushparaj
	 * 
	 * Listener for associating resources for component
	 */
    class AddListener implements SelectionListener
    {
        /**
         * @see org.eclipse.swt.events.SelectionListener#widgetSelected(
         *      org.eclipse.swt.events.SelectionEvent)
         */
        public void widgetSelected(SelectionEvent e)
        {
            StructuredSelection sel = (StructuredSelection) _resourceListViewer
                    .getSelection();
            java.util.List list = sel.toList();
            for (int i = 0; i < list.size(); i++) {
                EObject obj = (EObject) list.get(i);
                if (!_associatedResourcesList.contains(obj)) {
                    _associatedResourcesList.add(obj);
                    _resourcesList.remove(obj);
                }
            }
        }
        /**
         * @see org.eclipse.swt.events.SelectionListener#widgetDefaultSelected(
         *      org.eclipse.swt.events.SelectionEvent)
         */
        public void widgetDefaultSelected(SelectionEvent e)
        {
        }
    }

    /**
     *
     * @author pushparaj
     *
     * Listener for removing associated resources for component
     */
    class DeleteListener implements SelectionListener
    {
        /**
         * @see org.eclipse.swt.events.SelectionListener#widgetSelected(
         *      org.eclipse.swt.events.SelectionEvent)
         */
        public void widgetSelected(SelectionEvent e)
        {
            StructuredSelection sel = (StructuredSelection)
                _associateResourceListViewer.getSelection();
            _associatedResourcesList.removeAll(sel.toList());
            _resourcesList.addAll(sel.toList());
        }
        /**
         * @see org.eclipse.swt.events.SelectionListener#widgetDefaultSelected(
         * org.eclipse.swt.events.SelectionEvent)
         */
        public void widgetDefaultSelected(SelectionEvent e)
        {
        }
    }
}

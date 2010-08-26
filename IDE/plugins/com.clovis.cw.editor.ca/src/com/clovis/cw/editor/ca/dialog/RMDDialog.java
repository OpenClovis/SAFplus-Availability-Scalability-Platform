/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/RMDDialog.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.dialog;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.HelpEvent;
import org.eclipse.swt.events.HelpListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Cursor;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Link;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.eclipse.ui.PlatformUI;

import com.clovis.common.utils.ClovisUtils;
import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.project.data.DependencyListener;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * @author shubhada
 *
 * Dialog to capture RMD information
 */
public class RMDDialog extends GenericPreferenceDialog
{
    private EClass _idlSpecsClass = null;
    private EObject _idlObj = null;
    private Resource _idlResource = null;
    private Model _model       = null;
    private static final String IDL_SPECS_CLASS_NAME = "IDLSpecs";
    private static final String DIALOG_TITLE = "RMD";
    private static final Log LOG = Log.getLog(CaPlugin.getDefault());
    private static RMDDialog instance    = null;
    private DependencyListener _dependencyListener;
    
    /**
     * Gets current instance of Dialog
     * @return current instance of Dialog
     */
    public static RMDDialog getInstance()
    {
        return instance;
    }
    /**
     * Close the dialog.
     * Remove static instance
     * @return super.close()
     */
    public boolean close()
    {
        if (_viewModel != null) {
    		EcoreUtils.removeListener(_viewModel.getEObject(),
					_dependencyListener, -1);
    		_dependencyListener = null;
            _viewModel.dispose();
            _viewModel = null;
        }
        if (_model != null) {
			EcoreUtils.removeListener(_model.getEList(), ProjectDataModel
					.getProjectDataModel(_project).getModelTrackListener(), -1);
		}
        instance = null;
        return super.close();
    }
    /**
     * Open the dialog.
     * Set static instance to itself
     * @return super.open()
     */
    public int open()
    {
        instance = this;
        return super.open();
    }
    /**
     * @param parentShell - Shell
     * @param project - IProject
     */
    public RMDDialog(Shell parentShell, IProject project, PreferenceManager pManager)
    {
        super(parentShell, pManager, project);
        super.setShellStyle(SWT.CLOSE|SWT.MIN|SWT.MAX|SWT.RESIZE|SWT.SYSTEM_MODAL);
        readEcoreFiles();
        readResource(_project);
        addPreferenceNodes();
    }

    /**
     * Adds the Preference Node to the Preference Tree
     * for this dialog.
     */
    protected void addPreferenceNodes() {
    	PreferenceNode EOListNode = new PreferenceNode("Service Group List", 
    		new BlankPreferencePage("Service Group List"));
		_preferenceManager.addToRoot(EOListNode);
		
		/*Model model = new Model(_idlResource, _idlObj);
		_viewModel  = model.getViewModel();*/
		ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
		_dependencyListener = new DependencyListener(pdm, DependencyListener.VIEWMODEL_OBJECT);
		EcoreUtils.addListener(_viewModel.getEObject(), _dependencyListener, -1);
		
		EObject eObj = _viewModel.getEObject();
		EStructuralFeature nodeFeature = eObj.eClass()
			.getEStructuralFeature("Service");
		EList nodeList = (EList) eObj.eGet(nodeFeature);

		Iterator nodeItr = nodeList.iterator();
		while(nodeItr.hasNext()) {
			EObject nodeObj = (EObject) nodeItr.next();
			String nodeName = (String) EcoreUtils.getValue(nodeObj, "name");
			PreferenceNode node = new PreferenceNode(nodeName, 
					new GenericFormPage(nodeName, nodeObj));
			EOListNode.add(node);
			PreferenceUtils.createChildTree(node, nodeObj);
		}
	}
	/**
     *
     */
    private void readEcoreFiles()
    {
        try {
            URL idlURL = DataPlugin.getDefault().find(new Path("model"
                    + File.separator + ICWProject.IDL_ECORE_FILENAME));
            File ecoreFile = new Path(Platform.resolve(idlURL).getPath())
                    .toFile();
            EPackage rmdPackage = EcoreModels.get(ecoreFile.getAbsolutePath());
            _idlSpecsClass = (EClass) rmdPackage.
                getEClassifier(IDL_SPECS_CLASS_NAME);
        } catch (IOException ex) {
            LOG.error("IDL Specs Ecore File cannot be read", ex);
        }
    }
    /**
     *
     * @param project - IProject
     */
    private void readResource(IProject project)
    {
        try {
//            Now get the xmi file from the project which may or
            //may not have data
            String dataFilePath = project.getLocation().toOSString()
            + File.separator + ICWProject.CW_PROJECT_IDL_DIR_NAME
            + File.separator + ICWProject.IDL_XML_DATA_FILENAME;
            URI uri = URI.createFileURI(dataFilePath);
            File xmiFile = new File(dataFilePath);

            _idlResource = xmiFile.exists()
                ? EcoreModels.getUpdatedResource(uri) : EcoreModels.create(uri);
            EList idlList = _idlResource.getContents();
            if (idlList.isEmpty()) {
               _idlObj = EcoreUtils.createEObject(_idlSpecsClass, true);
               idlList.add(_idlObj);
            } else {
                _idlObj = (EObject) idlList.get(0);
            }
            ClovisUtils.initializeDependency(idlList);
            _model = new Model(_idlResource, (NotifyingList) idlList, _idlObj.eClass().getEPackage());
            _viewModel = _model.getViewModel();
            EcoreUtils.addListener(_model.getEList(), ProjectDataModel.getProjectDataModel(_project).getModelTrackListener(), -1);
            }  catch (Exception e) {
                LOG.error("Error reading RMD XMI File", e);
            }
    }
    /**
     * @param parent Composite
     * @return Control
     */
    protected Control createContents(Composite parent)
    {
    	Control control = (Composite) super.createContents(parent); 
       /*EObject eoObj = _viewModel.getEObject();
        String name = EcoreUtils.getName(eoObj);
        if (!name.matches("^[a-zA-Z][a-zA-Z0-9_]*$")) {
            getButton(IDialogConstants.OK_ID).setEnabled(false);
            setMessage("EO name is not valid", IMessageProvider.ERROR);
        }*/
    	control.addHelpListener(new HelpListener(){
			public void helpRequested(HelpEvent e) {
				PlatformUI.getWorkbench().getHelpSystem().displayHelp("com.clovis.cw.help.idl");
			}});
        PreferenceUtils.addListenerForTree(this, getTreeViewer());
        return control;
    }
    
    /**
     * 
     * @returns the EONames to user
     */
    public List getEOs() {
		List eoList = new Vector();
		ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
		List compList = pdm.getComponentModel().getEList();
		EObject rootObject = (EObject) compList.get(0);
		List compEditorObjects = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ComponentEditorConstants.SAFCOMPONENT_REF_NAME));
		for (int i = 0; i < compEditorObjects.size(); i++) {
			EObject eobj = (EObject) compEditorObjects.get(i);
			EObject eoObj = (EObject) EcoreUtils.getValue(eobj,
					ComponentEditorConstants.EO_PROPERTIES_NAME);
			if (eoObj != null) {
				String eoName = (String) EcoreUtils.getValue(eoObj,
						ComponentEditorConstants.EO_NAME);
				eoList.add(eoName);
			}
		}
		return eoList;
	}
    /**
	 * Save the Resource on OK pressed
	 */
    protected void okPressed()
    {
        if (_viewModel != null) {
            try {
                _viewModel.save(true);
            } catch (Exception e) {
                LOG.error("Save Error.", e);
            }
        }
        /*try {
            EcoreModels.save(_idlResource);
        } catch (IOException e) {
            LOG.error("IDL Resource cannot be saved" + e);
        }*/
        super.okPressed();
    }
    /**
     *
     * @return IDLSpecsClass
     */
    public EClass getIDLSpecsClass()
    {
        return _idlSpecsClass;
    }
    /**
     *
     * @return IDLSpecsObject
     */
    public EObject getIDLSpecsObject()
    {
        return _idlObj;
    }
    /* (non-Javadoc)
     * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
     */
    protected void configureShell(Shell shell) {
    	super.configureShell(shell);
    	shell.setText(_project.getName() + " - " + DIALOG_TITLE);
    }
    
    /***** This code needs to be removed in future ******/
    protected Control createHelpControl(Composite parent) {
		Image helpImage = JFaceResources.getImage(DLG_IMG_HELP);
		if (helpImage != null) {
			return createHelpImageButton(parent, helpImage);
		}
		return createHelpLink(parent);
    }
	private ToolBar createHelpImageButton(Composite parent, Image image) {
        ToolBar toolBar = new ToolBar(parent, SWT.FLAT | SWT.NO_FOCUS);
        ((GridLayout) parent.getLayout()).numColumns++;
		toolBar.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_CENTER));
		final Cursor cursor = new Cursor(parent.getDisplay(), SWT.CURSOR_HAND);
		toolBar.setCursor(cursor);
		toolBar.addDisposeListener(new DisposeListener() {
			public void widgetDisposed(DisposeEvent e) {
				cursor.dispose();
			}
		});		
        ToolItem item = new ToolItem(toolBar, SWT.NONE);
		item.setImage(image);
		item.setToolTipText(JFaceResources.getString("helpToolTip")); //$NON-NLS-1$
		item.addSelectionListener(new SelectionAdapter() {
            public void widgetSelected(SelectionEvent e) {
            	PlatformUI.getWorkbench().getHelpSystem().displayHelp("com.clovis.cw.help.idl");
            }
        });
		return toolBar;
	}
	private Link createHelpLink(Composite parent) {
		Link link = new Link(parent, SWT.WRAP | SWT.NO_FOCUS);
        ((GridLayout) parent.getLayout()).numColumns++;
		link.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_CENTER));
		link.setText("<a>"+IDialogConstants.HELP_LABEL+"</a>"); //$NON-NLS-1$ //$NON-NLS-2$
		link.setToolTipText(IDialogConstants.HELP_LABEL);
		link.addSelectionListener(new SelectionAdapter() {
            public void widgetSelected(SelectionEvent e) {
            	PlatformUI.getWorkbench().getHelpSystem().displayHelp("com.clovis.cw.help.idl");
            }
        });
		return link;
	}
	/***** This code needs to be removed in future ******/
}

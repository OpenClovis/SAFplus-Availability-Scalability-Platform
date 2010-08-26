/**
 * 
 */
package com.clovis.cw.editor.ca.dialog;

import java.util.List;

import org.eclipse.core.resources.IContainer;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.preference.IPreferencePage;
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

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.cw.project.data.DependencyListener;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * Memory Configuration Dialog.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class MemoryConfigurationDialog extends GenericPreferenceDialog {

	private static MemoryConfigurationDialog _instance;

	private Model _eoDefinitions;

	private DependencyListener _dependencyListener;
	
	private String 			  _contextHelpId = "com.clovis.cw.help.memory";

	/**
	 * Constructor.
	 * 
	 * @param parentShell
	 *            the parent shell
	 * @param manager
	 *            the preference manager
	 * @param resource
	 *            the project resource
	 */
	public MemoryConfigurationDialog(Shell parentShell,
			PreferenceManager manager, IResource resource) {

		super(parentShell, manager, (IProject) resource);

		ProjectDataModel pdm = ProjectDataModel
				.getProjectDataModel((IContainer) _project);
		_eoDefinitions = pdm.getEODefinitions();
		_viewModel = _eoDefinitions.getViewModel();

		_dependencyListener = new DependencyListener(pdm, DependencyListener.VIEWMODEL_OBJECT);
		EcoreUtils.addListener(_viewModel.getEList(), _dependencyListener, -1);

		addPreferenceNodes();
	}

	/**
	 * Adds the nodes to the tree of the dialog.
	 */
	protected void addPreferenceNodes() {
		EObject eoDefinitionsObj = _viewModel.getEObject();

		EObject memoryConfigurationObj = (EObject) EcoreUtils.getValue(
					eoDefinitionsObj, "memoryConfiguration");
		PreferenceNode memoryConfigurationNode = new PreferenceNode(
				"Memory Configuration", new GenericFormPage(
						"Memory Configuration", memoryConfigurationObj));
		_preferenceManager.addToRoot(memoryConfigurationNode);
		PreferenceUtils.createChildTree(memoryConfigurationNode, memoryConfigurationObj);
/*
		EObject iocConfigurationObj = (EObject) EcoreUtils.getValue(
				eoDefinitionsObj, "iocConfiguration");
		PreferenceNode iocConfigurationNode = new PreferenceNode(
				"IOC Configuration", new GenericFormPage(
						"IOC Configuration", iocConfigurationObj));
		_preferenceManager.addToRoot(iocConfigurationNode);
		PreferenceUtils.createChildTree(iocConfigurationNode, iocConfigurationObj);
*/	}

	/**
	 * Returns the instance of the dialog.
	 * 
	 * @return the instance of the dialog
	 */
	public static MemoryConfigurationDialog getInstance() {
		return _instance;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.window.Window#open()
	 */
	public int open() {
		_instance = this;
		return super.open();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.preference.PreferenceDialog#close()
	 */
	@Override
	public boolean close() {
		if (_viewModel != null) {
			EcoreUtils.removeListener(_viewModel.getEList(),
					_dependencyListener, -1);
			_viewModel.dispose();
			_viewModel = null;
		}
		_instance = null;
		return super.close();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.preference.PreferenceDialog#okPressed()
	 */
	@Override
	protected void okPressed() {
		_viewModel.save(true);
		super.okPressed();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.preference.PreferenceDialog#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell shell) {
		super.configureShell(shell);
		shell.setText("EO Definitions");
	}
	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.preference.PreferenceDialog#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent) {
		Control control = super.createContents(parent);
		
		List prefNodes = getPreferenceManager().getElements(
        		PreferenceManager.PRE_ORDER);
        for (int i = 0; i < prefNodes.size(); i++)
        {
        	
        	PreferenceNode prefNode = (PreferenceNode) prefNodes.get(i);
        	
        		IPreferencePage prefPage =  prefNode.getPage();
        		if (prefPage instanceof GenericFormPage) {
	        		GenericFormPage page = (GenericFormPage) prefPage;
	        		page.getValidator().setOKButton(
	        				getButton(IDialogConstants.OK_ID));
	        		page.getValidator().setApplyButton(getButton(3457));
        		}
        }
        control.addHelpListener(new HelpListener(){
			public void helpRequested(HelpEvent e) {
				PlatformUI.getWorkbench().getHelpSystem().displayHelp(_contextHelpId);
			}});
		PreferenceUtils.addListenerForTree(this, getTreeViewer());
		return control;
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
            	PlatformUI.getWorkbench().getHelpSystem().displayHelp(_contextHelpId);
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
            	PlatformUI.getWorkbench().getHelpSystem().displayHelp(_contextHelpId);
            }
        });
		return link;
	}
	/***** This code needs to be removed in future ******/
}

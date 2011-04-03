/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/ProblemsView.java $
 * $Author: srajyaguru $
 * $Date: 2007/04/30 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.List;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.draw2d.ColorConstants;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableItem;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;

import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.ui.table.TableUI;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.action.AutoCorrectAction;
import com.clovis.cw.workspace.action.ProblemDetailsAction;
import com.clovis.cw.workspace.action.ProblemsViewRefreshAction;
import com.clovis.cw.workspace.action.TableDoubleClickListener;
import com.clovis.cw.workspace.migration.MigrationUtils;
import com.clovis.cw.workspace.natures.SystemProjectNature;
import com.clovis.cw.workspace.project.IProjectValidator;
import com.clovis.cw.workspace.project.ProblemEcoreReader;
import com.clovis.cw.workspace.project.ProjectValidator;
import com.clovis.cw.workspace.project.ProjectValidatorThread;
/**
 * 
 * @author shubhada
 *
 * This class functions to view the Problem objects of validation
 * in the format of a table. 
 */
public class ProblemsView extends ViewPart implements ISelectionChangedListener
{
    private TableViewer _tableViewer = null;
    private Label _label = null;
    private IProject _project = null;
    private TableDoubleClickListener _clickListener = null;
    private IProjectValidator _validator = new ProjectValidator();
    private List _problemsList = new ClovisNotifyingListImpl();
    public final static String DETAILS_MENU_TEXT = "Details...";
    public final static String AUTOCORRECT_MENU_TEXT = "Auto Correction Options...";
    private boolean _isDirty;
    
    private static ProblemsView instance    = null;
    /**
     * Gets current instance of Dialog
     * @return current instance of Dialog
     */
    public static ProblemsView getInstance()
    {
        return instance;
    }
    
    public void dispose()
    {
        instance = null;
        super.dispose();
    }
    
    /**
     * creates the content of the view.
     * @param parent - Parent Composite
     */
    public void createPartControl(Composite parent)
    {
    	initProject();
        Composite container = new Composite(parent, SWT.NONE);
        GridLayout containerLayout = new GridLayout();
        container.setLayout(containerLayout);
        GridData containerData = new GridData();
        container.setLayoutData(containerData);
        _label = new Label(container, SWT.COLOR_CYAN);
        _label.setLayoutData(new GridData(GridData.BEGINNING
        		| GridData.FILL_HORIZONTAL));
        if (_project != null) {
			if(MigrationUtils.isMigrationRequired(_project)) {
				setLabel("Project is not in the latest version. Migrate the Project.");
			} else {
	            _label.setText(_problemsList.size()
	                    + " problems encountered in project ["
	                    + _project.getName() + "]");
			}
        } else {
            _label.setText("Problems not available");
        }
        
        int style = SWT.BORDER | SWT.H_SCROLL
        | SWT.V_SCROLL | SWT.FULL_SELECTION | SWT.HIDE_SELECTION | SWT.SINGLE
        | SWT.READ_ONLY;
        Table table = new Table (container, style);
        ClassLoader loader = getClass().getClassLoader();
        _tableViewer = new TableUI(table,
                ProblemEcoreReader.getInstance().getProblemClass(), loader, true);
        _tableViewer.setSorter(new ProblemsViewSorter());
        GridData gridData1 = new GridData();
        gridData1.horizontalAlignment = GridData.FILL;
        gridData1.grabExcessHorizontalSpace = true;
        gridData1.grabExcessVerticalSpace = true;
        gridData1.verticalAlignment = GridData.FILL;

        table.setLayoutData(gridData1);
        table.setLinesVisible(true);
        table.setHeaderVisible(true);
        _tableViewer.setInput(_problemsList);
        if (_clickListener == null) {
	        _clickListener = new TableDoubleClickListener();
        }
        _clickListener.setProject(_project);
        _tableViewer.addDoubleClickListener(_clickListener);
        initContextMenu();
        AutoCorrectAction.setProject(_project);
        contributeToToolBars();
        refreshProblemsList();
    }
    /**
     * Initializes the context menu.
     * 
     */
    protected void initContextMenu()
    {
        MenuManager menuMgr = new MenuManager("#PopupMenu");
        Menu menu = menuMgr.createContextMenu(_tableViewer.getTable());
        _tableViewer.getTable().setMenu(menu);
        AutoCorrectAction.setMenuManager(menuMgr);
        fillContextMenu(menuMgr);
        
    }
    /**
     * Fills the menu manager with the menu actions
     * @param manager - IMenuManager
     */
    private void fillContextMenu(IMenuManager manager)
    {
    	IWorkbenchPage page = WorkspacePlugin.getDefault().getWorkbench()
		.getActiveWorkbenchWindow().getActivePage();
    	Action detailsAction = new ProblemDetailsAction(_tableViewer);
    	detailsAction.setText(DETAILS_MENU_TEXT);
    	detailsAction.setToolTipText("Details of the problem");
    	//_tableViewer.addSelectionChangedListener(
        //		(ISelectionChangedListener) detailsAction);
    	
    	manager.add(detailsAction);
    	manager.add(new Separator());
    	
    	MenuManager subMenuMgr = new MenuManager(
    			AUTOCORRECT_MENU_TEXT,AUTOCORRECT_MENU_TEXT); 
    	Action autoAction = new AutoCorrectAction((EObject) 
                ((IStructuredSelection) _tableViewer.getSelection()).
                getFirstElement());
    	autoAction.setText("Default Action");
    	autoAction.setToolTipText(
    			"Automatically corrects the problems encountered in model");
    	_tableViewer.addSelectionChangedListener(
        		(ISelectionChangedListener) autoAction);
    	subMenuMgr.add(autoAction);
    	manager.add(subMenuMgr);
	}
    /**
     * Adds icons and actions to the toolbar
     *
     */
    private void contributeToToolBars()
    {
    	IWorkbenchPage page = WorkspacePlugin.getDefault().getWorkbench()
		.getActiveWorkbenchWindow().getActivePage();
		IActionBars bars = getViewSite().getActionBars();
		ProblemsViewRefreshAction refreshAction = 
			new ProblemsViewRefreshAction(this);
		refreshAction.setText("Refresh");
		refreshAction.setToolTipText(
				"Refreshes the problems of selected projects");
		URL url = WorkspacePlugin.getDefault().find(new Path("icons"
                    + File.separator + "refresh.gif"));
		try {
			refreshAction.setImageDescriptor(ImageDescriptor.createFromURL(
					Platform.resolve(url)));
		} catch (IOException e) {
			
			WorkspacePlugin.LOG.warn("Unable to set the toolbar for ProblemsView", e);
		}
		bars.getToolBarManager().add(refreshAction);
	}
    /**
     * initializes the view
     *
     */
    private void initProject()
    {
    	IWorkbenchPage page = WorkspacePlugin.getDefault().getWorkbench()
    		.getActiveWorkbenchWindow().getActivePage();
    	if (page != null) {
	        ClovisNavigator navigator = ((ClovisNavigator) page
	               .findView("com.clovis.cw.workspace.clovisWorkspaceView"));
	        if(navigator == null) {
	        	try {
	        		navigator = (ClovisNavigator)page.showView("com.clovis.cw.workspace.clovisWorkspaceView");
				} catch (PartInitException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
	        }
	        navigator.getTreeViewer().addSelectionChangedListener(this);
            //navigator.addPropertyListener(this);
	        ISelection selection = navigator.getTreeViewer().getSelection();
	        IProject project = null;
	        if (selection instanceof IStructuredSelection) {
	            IStructuredSelection sel = (IStructuredSelection) selection;
	            if (sel.getFirstElement() instanceof IResource) {
	                IResource resource = (IResource) sel.getFirstElement();
	                project = resource.getProject();
	            }
	        }
	        try {
	            if (project != null
	                    && project.exists()
	                    && project.isOpen()
	                    && project
	                            .hasNature(SystemProjectNature.CLOVIS_SYSTEM_PROJECT_NATURE)) {
	               _project = project;
	               AutoCorrectAction.setProject(project);
	               if (_clickListener == null) {
	            	   _clickListener = new TableDoubleClickListener();
	               } 
	               _clickListener.setProject(project);
	               refreshProblemsList();
            } 
        } catch (Exception e) {
            WorkspacePlugin.LOG.warn("Problems in initializing the view", e);
        }
    
        
	}
    }
    /**
     * initializes the view
     * @param site - IViewSite
     */
    public void init(IViewSite site) throws PartInitException
    {
        instance = this;
        AutoCorrectAction.setShell(site.getShell());
    	initProject();
    	/*if (_project == null) {
    		_project = ResourcesPlugin.getWorkspace().getRoot().getProjects() [0];
    	}*/
        super.init(site);
       
    }
    /**
     * Setting diff Color for diff severity
     * NOTE : This method should be removed. this needs to be handled 
     *        at the time of adding problems. 
     */
    public void updateColorsForProblems() {
		// This code needs to be cleaned. This needs to be handled at the
		// time of adding problems in view.
		if (_tableViewer != null && _tableViewer.getTable() != null) {
			TableItem[] items = _tableViewer.getTable().getItems();
			for (int i = 0; i < items.length; i++) {
				TableItem item = items[i];
				if (item.getText().equals("WARNING")) {
					item.setForeground(ColorConstants.blue);
				} else if (item.getText().equals("ERROR")) {
					item.setForeground(ColorConstants.red);
				}
			}
		}
	}
    /**
	 * Add New Problems in View
	 * 
	 * @param problems
	 *            Problems List
	 */
    public void addProblems(List problems) {
    	_problemsList.clear();
    	_problemsList.addAll(problems);
    	if (_label != null && getProject() != null) {
            _label.setText(_problemsList.size()
                    + " problems encountered in project ["
                    + getProject().getName() + "]");
        }
    }
    /**
     * Returns Selected Project
     * @return IProject
     */
    public IProject getProject() {
    	if(_project == null) {
    		findAndUpdateSelectedProject();
    	}
    	return _project;
    }
    /**
     * Set the Selected Project
     *
     */
    public IProject findAndUpdateSelectedProject() {
    	IWorkbenchPage page = WorkspacePlugin.getDefault().getWorkbench()
		.getActiveWorkbenchWindow().getActivePage();
    	if (page != null) {
	        ClovisNavigator navigator = ((ClovisNavigator) page
	               .findView("com.clovis.cw.workspace.clovisWorkspaceView"));
	        if(navigator == null) {
	        	try {
	        		navigator = (ClovisNavigator)page.showView("com.clovis.cw.workspace.clovisWorkspaceView");
				} catch (PartInitException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
	        }
            navigator.getTreeViewer().addSelectionChangedListener(this);
	        ISelection selection = navigator.getTreeViewer().getSelection();
	        IProject project = null;
	        if (selection instanceof IStructuredSelection) {
	            IStructuredSelection sel = (IStructuredSelection) selection;
	            if (sel.getFirstElement() instanceof IResource) {
	                IResource resource = (IResource) sel.getFirstElement();
	                project = resource.getProject();
	            }
	        }
	        try {
	            if (project != null
	                    && project.exists()
	                    && project.isOpen()
	                    && project
	                            .hasNature(SystemProjectNature.CLOVIS_SYSTEM_PROJECT_NATURE)) {
	               _project = project;
	               AutoCorrectAction.setProject(project);
	               if (_clickListener == null) {
	            	   _clickListener = new TableDoubleClickListener();
	               } 
	               _clickListener.setProject(project);
	            } 
	        } catch (Exception e) {
	        	WorkspacePlugin.LOG.warn("Could not initialize the selected project", e);
	        }
    	}
    	return _project;
    }
    /**
     * refreshes the problems list
     *
     */
    public void refreshProblemsList()
    {
    	if (_project == null) {
    		findAndUpdateSelectedProject();
    	}
    	if (_project != null) {
	    	ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
	    	GenericEditorInput caInput = (GenericEditorInput)pdm.getCAEditorInput();
			GenericEditorInput compInput = (GenericEditorInput)pdm.getComponentEditorInput();
			if((caInput != null && caInput.isDirty()) || compInput != null && compInput.isDirty()) {
				if(MessageDialog.openQuestion(getViewSite().getShell(), "Save Information Model",
				"Information Model has been modified. Save changes?")) {
					if(caInput != null)
						caInput.getEditor().doSave(null);
					if(compInput != null)
						compInput.getEditor().doSave(null);
				}
			}
			AutoCorrectAction.setProject(_project);
			_clickListener.setProject(_project);
			List problems = pdm.getModelProblems();
			if(problems == null || pdm.isModified()) {
				ProjectValidatorThread validatorThread = new ProjectValidatorThread(_project);
				Display.getDefault().syncExec(validatorThread);
				problems = validatorThread.getProblems();
				pdm.setModelProblems(problems);
				pdm.setModified(false);
			}
			addProblems(problems);
			updateColorsForProblems();
            if (_label != null) {
    			if(MigrationUtils.isMigrationRequired(_project)) {
    				setLabel("Project is not in the latest version. Migrate the Project.");
    			} else {
                    _label.setText(_problemsList.size()
                            + " problems encountered in project ["
                            + _project.getName() + "]");
    			}
            }
    	} 
    }
    /**
     * Set Focus method.
     */
    public void setFocus()
    {
    	if (_isDirty
				|| (_project != null && ProjectDataModel.getProjectDataModel(
						_project).isModified())) {
			refreshProblemsList();
			_isDirty = false;
		} else if(_project == null) {
			_problemsList.clear();
			_label.setText("Problems not available");
		}
        _tableViewer.getTable().setFocus();
        
    }
    /**
     *  clears the problems from the view
     *
     */
    public void clearModelProblems()
    {
    	_problemsList.clear();
    }
    /**
     * @param event - SelectionChangedEvent
     */
    public void selectionChanged(SelectionChangedEvent event)
    {
		ISelection selection = event.getSelection();
		IProject project = null;
		if (selection instanceof IStructuredSelection) {
			IStructuredSelection sel = (IStructuredSelection) selection;
			if (sel.getFirstElement() instanceof IResource) {
				IResource resource = (IResource) sel.getFirstElement();
				project = resource.getProject();
			}
		}

		try {
			if (project != null
					&& project.exists()
					&& project.isOpen()
					&& project
							.hasNature(SystemProjectNature.CLOVIS_SYSTEM_PROJECT_NATURE)) {

				if (_project != null && project.getName().equals(_project.getName()))
					return;
				_project = project;

				if (isVisible()) {

					_isDirty = false;
					refreshProblemsList();
				} else {
					_isDirty = true;
				}

			} else {
				_project = null;
				if (isVisible()) {
					_problemsList.clear();
					_label
							.setText("Problems not available");
				}
			}

		} catch (Exception e) {
			WorkspacePlugin.LOG.warn("Problems in initializing the view", e);
		}
    }

    /**
     * Returns whether problems view is visible or not.
     * @return
     */
    public boolean isVisible() {
		IWorkbenchPage page = WorkspacePlugin.getDefault().getWorkbench()
				.getActiveWorkbenchWindow().getActivePage();

		if (page != null) {
			IViewPart part = page
					.findView("com.clovis.cw.workspace.problemsView");

			if (part != null && page.isPartVisible(part)) {
				return true;
			}
		}

		return false;
	}
    /**
     * Returns TableViewer
     * @return TableViewer
     */
    public TableViewer getTableViewer() {
    	return _tableViewer;
    }
    /**
	 * Sets the Label for the problems view.
	 * 
	 * @param label
	 */
	public void setLabel(String label) {
		if (_label != null) {
			_label.setText(label);
		}
	}
}

package com.clovis.cw.editor.ca.manageability.ui;

import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.StringTokenizer;
import java.util.Vector;

import org.eclipse.core.resources.IContainer;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.draw2d.ColorConstants;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.notify.impl.NotifyingListImpl;
import org.eclipse.emf.common.util.BasicEList;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.BusyIndicator;
import org.eclipse.swt.custom.StackLayout;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ControlListener;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.FocusEvent;
import org.eclipse.swt.events.FocusListener;
import org.eclipse.swt.events.TreeEvent;
import org.eclipse.swt.events.TreeListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Tree;
import org.eclipse.swt.widgets.TreeItem;
import org.eclipse.ui.IEditorInput;
import org.eclipse.ui.IEditorPart;
import org.eclipse.ui.IEditorSite;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.EditorPart;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.cw.editor.ca.ResourceDataUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.editor.ca.constants.SafConstants;
import com.clovis.cw.editor.ca.dialog.NodeProfileDialog;
import com.clovis.cw.editor.ca.manageability.common.LoadedMibUtils;
import com.clovis.cw.editor.ca.manageability.common.ResourceTreeNode;
import com.clovis.cw.editor.ca.manageability.common.ResourceTypeBrowserUI;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.project.data.DependencyListener;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.project.data.SubModelMapReader;
import com.clovis.cw.project.utils.FormatConversionUtils;

/**
 * Editor for Manageability UI
 * @author Pushparaj
 *
 */
public class ManageabilityEditor extends EditorPart implements PropertyChangeListener{

	private ManageabilityEditorInput _editorInput;
	private Model _viewModel;
	private List<EObject> _nodeList;
	private ProjectDataModel _pdm;
	private ResourceTypeBrowserUI _availableResViewer;
	private Tree _createdResTree;
	private RessourceEditingComposite _editComp;
	private List<EObject> _resourceList;
	private Map<String, EObject> _resNameObjMap = new HashMap<String, EObject>();
	private Map<String, EObject> _compNameObjMap = new HashMap<String, EObject>();
	private List<EObject> _componentList;
	private boolean _isDirty = false;
	private GenericEditorInput _caInput;
	private GenericEditorInput _compInput;
	private Model _rtModel;
    private Model _resourceModel;
    private Model _componentModel;
    private Map<EObject, List<EObject>> _compResourcesMap = new HashMap<EObject, List<EObject>>();
    private Map<EObject, Resource> _nodeResourceMap = new HashMap<EObject, Resource>();
    private List<String> _resNames;
    private InstancesTreeComposite _componentsTreeComposite;
    private String _chassisName;  
    private Model _mapViewModel = null;
    private Model _alarmRuleViewModel = null;
    private Model _resourceAssociationModel;
    private ManageabilityEditor _editor;
    
	@Override
	public void doSave(IProgressMonitor monitor) {
		updateAssociatedResources();
		_resourceAssociationModel.save(false);
		_rtModel.save(true);
    	writeCompToResMap();
    	_isDirty = false;
    	firePropertyChange(IEditorPart.PROP_DIRTY);
    	if(_caInput != null && _caInput.getEditor() != null && _caInput.getEditor().isDirty()) {
    		_caInput.getEditor().doSave(null);
    	} else {
    		FormatConversionUtils.convertToResourceFormat((EObject) _resourceModel.getEList().
        			get(0), "Resource Editor");
    		_resourceModel.save(true);
    	}
    	_mapViewModel.save(true);
        _alarmRuleViewModel.save(true);
	}
	
	@Override
	public void doSaveAs() {
		
	}

	@Override
	public void init(IEditorSite site, IEditorInput input)
			throws PartInitException {
		setSite(site);
		setInput(input);
		String title = ((ManageabilityEditorInput)input).getProjectDataModel().getProject().getName() + " - " + getPartName();
        setPartName(title);
        ((ManageabilityEditorInput)input).setToolTipText(title);
	}

	@Override
	public boolean isDirty() {
		return _isDirty;
	}

	@Override
	public boolean isSaveAsAllowed() {
		// TODO Auto-generated method stub
		return false;
	}

	@Override
	public void createPartControl(Composite parent) {
		final Composite container = new Composite(parent, SWT.NONE);
		container.setBackground(ColorConstants.white);
        GridLayout containerLayout = new GridLayout();
        containerLayout.numColumns = 2;
        containerLayout.verticalSpacing = 10;
        container.setLayout(containerLayout);
        GridData containerData = new GridData(GridData.FILL, GridData.FILL, true, true);
        container.setLayoutData(containerData);
        parent.addDisposeListener(new DisposeListener(){
			public void widgetDisposed(DisposeEvent e) {
				_editorInput.setEditor(null);
			}});
        
        ManageabilityLoadMibComposite mibComposite = new ManageabilityLoadMibComposite(container, SWT.NONE, _pdm.getLoadedMibs(), _pdm, this);
        mibComposite.setBackground(ColorConstants.white);
        mibComposite.setLayout(containerLayout);
        GridData gridData = new GridData(GridData.FILL_HORIZONTAL);
        gridData.horizontalSpan =  2;
        mibComposite.setLayoutData(gridData);
        
        /************** Container for Available and Created resources Tree **************/
        final Composite comp1 = new Composite(container, SWT.NONE);
        GridLayout compLayout = new GridLayout();
        compLayout.numColumns = 2;
        comp1.setLayout(compLayout);
        comp1.setLayoutData(new GridData(GridData.FILL, GridData.FILL, true, true));
        comp1.setBackground(ColorConstants.white);
        parent.addControlListener(new ControlListener(){
			public void controlMoved(ControlEvent e) {}
			public void controlResized(ControlEvent e) {
				updateTreeSize();
			}
		});
        
        int style = SWT.MULTI | SWT.BORDER | SWT.CHECK | SWT.H_SCROLL | SWT.V_SCROLL
		| SWT.FULL_SELECTION | SWT.HIDE_SELECTION;
        ClassLoader loader = getClass().getClassLoader();
        _availableResViewer = new ResourceTypeBrowserUI(comp1, style, loader);
        _availableResViewer.setContentProvider(new ResourceTreeContentProvider());
        _availableResViewer.addCheckStateListener(new TreeSelectionHandler());
        mibComposite.setResourceTypeBrowser(_availableResViewer);
        final Tree availableResTree = _availableResViewer.getTree();
        GridData gridData0 = new GridData();
        gridData0.horizontalSpan = 2;
        gridData0.horizontalAlignment = GridData.FILL;
        gridData0.grabExcessHorizontalSpace = true;
        gridData0.grabExcessVerticalSpace = true;
        gridData0.verticalAlignment = GridData.FILL;
        gridData0.widthHint = getSite().getShell().getDisplay().getClientArea().height / 2;
        gridData0.heightHint = getSite().getShell().getDisplay().getClientArea().height / 2;
        availableResTree.setLayoutData(gridData0);
        availableResTree.setBackground(ColorConstants.white);
        Vector<ResourceTreeNode> elements = new Vector<ResourceTreeNode>();
		_availableResViewer.setInput(elements);
        _availableResViewer.expandAll();
        LoadedMibUtils.loadExistingMibs(_pdm, _pdm.getLoadedMibs(), _availableResViewer);
        
        style = SWT.MULTI | SWT.BORDER | SWT.H_SCROLL | SWT.V_SCROLL
		| SWT.FULL_SELECTION | SWT.HIDE_SELECTION;
        _createdResTree = new Tree(comp1, style);
        GridData gridData1 = new GridData();
        gridData1.horizontalSpan = 2;
        gridData1.horizontalAlignment = GridData.FILL;
        gridData1.grabExcessHorizontalSpace = true;
        gridData1.grabExcessVerticalSpace = true;
        gridData1.verticalAlignment = GridData.FILL;
        gridData1.widthHint = getSite().getShell().getDisplay().getClientArea().height / 2;
        gridData1.heightHint = getSite().getShell().getDisplay().getClientArea().height / 2;
        _createdResTree.setLayoutData(gridData1);
        _createdResTree.setBackground(ColorConstants.white);
        buildCreatedResourcesTreeItem(_createdResTree);
        
        /************** Container for Instance Tree **************/
        final Composite comp2 = new Composite(container, SWT.NONE);
        GridData data = new GridData(GridData.FILL, GridData.FILL, true, false);
        comp2.setLayoutData(data);
        final StackLayout stackLayout = new StackLayout();
        comp2.setLayout(stackLayout);
        
        /************** Instance Tree ***************/
        EObject assObj = (EObject)_resourceAssociationModel.getEList().get(0);
        final InstancesTreeComposite insComp = new InstancesTreeComposite(this,
				comp2, _availableResViewer, _createdResTree, _componentList,
				_nodeList, _pdm, _compResourcesMap, assObj);
		setComponentsTreeComposite(insComp);
        /************** Resource Editing Control ***************/
        _editComp = new RessourceEditingComposite(comp2, this, (EClass) _pdm.getNodeProfiles()
				.getEPackage().getEClassifier("resourceType"));
        
        _availableResViewer
				.addPostSelectionChangedListener(new ISelectionChangedListener() {
					public void selectionChanged(SelectionChangedEvent event) {
						TreeItem root = availableResTree.getItem(0);
						if (root.getItemCount() == 0) {
							setMinimumTreeSize(comp1, availableResTree);
							comp1.layout();
						}
					}
				});
		availableResTree.addTreeListener(new TreeListener() {
			public void treeCollapsed(TreeEvent e) {
				TreeItem item = (TreeItem) e.item;
				if (item.getText().equals("Resource Type Browser")) {
					setMinimumTreeSize(comp1, availableResTree);
					// setMaximumTreeSize(comp1, _createdResTree);
					comp1.layout();
				}
			}

			public void treeExpanded(TreeEvent e) {
				TreeItem item = (TreeItem) e.item;
				if (item.getText().equals("Resource Type Browser")) {
					setMaximumTreeSize(comp1, availableResTree);
					//setMinimumTreeSize(comp1, _createdResTree);
					comp1.layout();
				}
			}
		});;
        _createdResTree.addTreeListener(new TreeListener() {
			public void treeCollapsed(TreeEvent e) {
				TreeItem item = (TreeItem) e.item;
				if (item.getText().equals("Created Resources")) {
					//setMaximumTreeSize(comp1, availableResTree);
					setMinimumTreeSize(comp1, _createdResTree);
					comp1.layout();
				}
			}

			public void treeExpanded(TreeEvent e) {
				TreeItem item = (TreeItem) e.item;
				if (item.getText().equals("Created Resources")) {
					//setMinimumTreeSize(comp1, availableResTree);
					setMaximumTreeSize(comp1, _createdResTree);
					comp1.layout();
				}
			}
		});
		availableResTree.addFocusListener(new FocusListener() {
			public void focusGained(FocusEvent e) {
				stackLayout.topControl = insComp;
				comp2.layout();
			}

			public void focusLost(FocusEvent e) {
			}
		});
		_createdResTree.addFocusListener(new FocusListener() {
			public void focusGained(FocusEvent e) {
				stackLayout.topControl = _editComp;
				comp2.layout();
			}

			public void focusLost(FocusEvent e) {
			}
		});
		_createdResTree.addListener(SWT.MouseDoubleClick, new Listener() {
			public void handleEvent(Event event) {
				Point point = new Point(event.x, event.y);
				TreeItem item = _createdResTree.getItem(point);
				if (item != null) {
					PreferenceManager pmanager = new PreferenceManager();
					EObject resObj = (EObject) getResEObject(item.getText());
					if (resObj != null) {
						ResourcePropertiesDialog pDialog = new ResourcePropertiesDialog(
								getSite().getShell(), pmanager, resObj,
								_resourceList, _pdm.getProject(), _editor);
						Model viewModel = pDialog.getViewModel();
						DependencyListener listener = new DependencyListener(
								DependencyListener.VIEWMODEL_OBJECT);
						EcoreUtils.addListener(viewModel.getEObject(),
								listener, -1);
						int ok = pDialog.open();
						EcoreUtils.removeListener(viewModel.getEObject(),
								listener, -1);
						if(ok == Dialog.OK)
							propertyChange(null);
					}
				}
			}
		});
        _createdResTree.addListener(SWT.MouseDown, new Listener() {
			public void handleEvent(Event event) {
				Point point = new Point(event.x, event.y);
				TreeItem item = _createdResTree.getItem(point);
				if (item != null) {
					if (item.getData() instanceof List) {
						List list = (List) item.getData();
						_editComp.setSelectedResource(item.getText());
						_editComp.setTableInput(list);
					} else {
						_editComp.setSelectedResource(null);
						_editComp.setTableInput(null);
					}
				}
			}
		});
        comp1.layout();
        stackLayout.topControl = insComp;
        hookContextMenuForResourcesTree(_createdResTree);
        hookContextMenuForComponentsTree(insComp.getComponentsTree());
	}
	private void updateTreeSize() {
		Tree resTypeBrowser = _availableResViewer.getTree();
		Composite comp = resTypeBrowser.getParent();
		TreeItem item1 = resTypeBrowser.getTopItem();
		TreeItem item2 = _createdResTree.getTopItem();
		if (item1.getExpanded() && item2.getExpanded()) {
			setMaximumTreeSize(comp, resTypeBrowser);
			setMaximumTreeSize(comp, _createdResTree);
		} else {
			if (item1.getExpanded()) {
				setMaximumTreeSize(comp, resTypeBrowser);
			} else {
				setMinimumTreeSize(comp, resTypeBrowser);
			}
			if (item2.getExpanded()) {
				setMaximumTreeSize(comp, _createdResTree);
			} else {
				setMinimumTreeSize(comp, _createdResTree);
			}
		}
		comp.layout();
	}
	private void setMinimumTreeSize(Composite comp, Tree tree) {
		GridData gridData3 = new GridData();
		gridData3.horizontalSpan = 2;
		gridData3.horizontalAlignment = GridData.FILL;
		gridData3.grabExcessHorizontalSpace = true;
		//gridData3.grabExcessVerticalSpace = true;
		//gridData3.verticalAlignment = GridData.FILL;
		gridData3.widthHint = comp.getDisplay().getClientArea().height / 2;
		gridData3.heightHint = 25;//comp.getDisplay().getClientArea().height / 15;
		tree.setLayoutData(gridData3);
	}
	private void setMaximumTreeSize(Composite comp, Tree tree) {
		GridData gridData2 = new GridData();
		gridData2.horizontalSpan = 2;
		gridData2.horizontalAlignment = GridData.FILL;
		gridData2.grabExcessHorizontalSpace = true;
		gridData2.grabExcessVerticalSpace = true;
		gridData2.verticalAlignment = GridData.FILL;
		gridData2.widthHint = comp.getDisplay().getClientArea().height / 2;
		gridData2.heightHint = comp.getDisplay().getClientArea().height / 2;
		tree.setLayoutData(gridData2);
	}
	private void hookContextMenuForComponentsTree(final Tree tree) {
		MenuManager menuMgr = new MenuManager("#PopupMenu");
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager manager) {
				ManageabilityEditor.this.fillContextMenu(manager, tree);
			}
		});
		Menu menu = menuMgr.createContextMenu(tree);
		tree.setMenu(menu);
	}
	
	private void hookContextMenuForResourcesTree(final Tree tree) {
		MenuManager menuMgr = new MenuManager("#PopupMenu");
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager manager) {
				if (tree.getSelection().length > 0) {
					if (tree.getSelection()[0].getData() != null) {
						if (tree.getSelection()[0].getData() instanceof List) {
							ManageabilityEditor.this
									.fillContextMenuForResource(manager, tree);
						} else if (tree.getSelection()[0].getData() instanceof EObject
								&& ((EObject) tree.getSelection()[0].getData())
										.eClass().getName().equals(
												"ComponentInstance")) {
							ManageabilityEditor.this
									.fillContextMenuForComponent(manager, tree);
						} else {
							ManageabilityEditor.this.fillContextMenu(manager, tree);
						}
					}else {
						ManageabilityEditor.this.fillContextMenu(manager, tree);
					}
				} 
			}
		});
		Menu menu = menuMgr.createContextMenu(tree);
		tree.setMenu(menu);
	}

	Action deleteAction;
	Action propertiesAction;
	Action deleteAllAction;
	Action refreshAction;
	private void fillContextMenuForResource(IMenuManager manager, Tree tree) {
		makeActionsForResource(tree);
		manager.add(propertiesAction);
		manager.add(deleteAction);
		manager.add(refreshAction);
		manager.add(new Separator());
		manager.add(new Separator(IWorkbenchActionConstants.MB_ADDITIONS));
	}
	private void fillContextMenuForComponent(IMenuManager manager, Tree tree) {
		makeActionsForResource(tree);
		manager.add(deleteAllAction);
		manager.add(refreshAction);
		manager.add(new Separator());
		manager.add(new Separator(IWorkbenchActionConstants.MB_ADDITIONS));
	}
	private void fillContextMenu(IMenuManager manager, Tree tree) {
		makeActionsForResource(tree);
		manager.add(refreshAction);
		manager.add(new Separator());
		manager.add(new Separator(IWorkbenchActionConstants.MB_ADDITIONS));
	}
	private void makeActionsForResource(final Tree tree) {
		deleteAction = new Action() {
			public void run() {
				if (MessageDialog.openConfirm(getEditorSite().getShell(),
						"Delete All Resources",
						"This will delete all associated resources of selected resource types"
								+ ". Do you still want to continue?")) {
					BusyIndicator.showWhile(Display.getCurrent(),
							new Runnable() {
								public void run() {
									TreeItem items[] = tree.getSelection();
									for (int j = 0; j < items.length; j++) {
										TreeItem item = items[j];
										String resName = item.getText();
										if (item.getData() instanceof List) {
											List<EObject> resList = (List) item
													.getData();
											List<EObject> removeList = new ArrayList<EObject>();
											for (int i = 0; i < resList.size(); i++) {
												EObject resObj = resList.get(i);
												if (getResourceTypeForInstanceID(
														(String) EcoreUtils
																.getValue(
																		resObj,
																		"moID"))
														.equals(resName)) {
													removeList.add(resObj);
												}
											}
											resList.removeAll(removeList);
											propertyChange(null);
											item.dispose();
											tree.setSelection(tree
															.getTopItem());
											_editComp.setSelectedResource(null);
											_editComp.setTableInput(null);
										}
									}
								}
							});
				}
			}
		};
		deleteAction.setText("Delete");
		
		deleteAllAction = new Action() {
			public void run() {
				final TreeItem compItem = tree.getSelection()[0];
				if (compItem.getData() instanceof EObject
						&& ((EObject) compItem.getData()).eClass().getName()
								.equals("ComponentInstance")) {
					if (MessageDialog.openConfirm(getEditorSite().getShell(),
							"Delete All Resources",
							"This will delete all resources associated with "
									+ compItem.getText()
									+ ". Do you still want to continue?")) {
						BusyIndicator.showWhile(Display.getCurrent(),
								new Runnable() {
									public void run() {
										TreeItem items[] = compItem.getItems();
										for (int j = 0; j < items.length; j++) {
											TreeItem item = items[j];
											String resName = item.getText();
											if (item.getData() instanceof List) {
												List<EObject> resList = (List) item
														.getData();
												List<EObject> removeList = new ArrayList<EObject>();
												for (int i = 0; i < resList
														.size(); i++) {
													EObject resObj = resList
															.get(i);
													if (getResourceTypeForInstanceID(
															(String) EcoreUtils
																	.getValue(
																			resObj,
																			"moID"))
															.equals(resName)) {
														removeList.add(resObj);
													}
												}
												resList.removeAll(removeList);
												propertyChange(null);
												item.dispose();
												_editComp
														.setSelectedResource(null);
												_editComp.setTableInput(null);
											}
										}
									}
								});
					}
				}
			}
		};
		deleteAllAction.setText("Delete	");
		
		propertiesAction = new Action() {
			public void run() {
				TreeItem item = tree.getSelection()[0];
				if (item.getData() instanceof List) {
					EObject resObj = (EObject) getResEObject(item.getText());
					if (resObj != null) {
						PreferenceManager pmanager = new PreferenceManager();
						ResourcePropertiesDialog pDialog = new ResourcePropertiesDialog(
								getSite().getShell(), pmanager, resObj,
								_resourceList, _pdm.getProject(), _editor);
						 
						Model viewModel = pDialog.getViewModel();
						DependencyListener listener = new DependencyListener(
								DependencyListener.VIEWMODEL_OBJECT);
						EcoreUtils.addListener(viewModel.getEObject(),
								listener, -1);
						int ok = pDialog.open();
						EcoreUtils.removeListener(viewModel.getEObject(),
								listener, -1);
						if (ok == Dialog.OK)
							propertyChange(null);
					}
				}
			}
		};
		propertiesAction.setText("Properties...");
		
		refreshAction = new Action() {
			public void run() {
				BusyIndicator.showWhile(Display.getCurrent(), new Runnable() {
					public void run() {
						refresh();
					}});
			}
		};
		refreshAction.setText("Refresh");
	}
	
	private void setComponentsTreeComposite(InstancesTreeComposite composite) {
		_componentsTreeComposite = composite;
	}
	public InstancesTreeComposite getComponentsTreeComposite() {
		return _componentsTreeComposite;
	}
	@Override
	public void setFocus() {

	}
	
	/**
	 * This will update the Manageability Editor with updated data.
	 */
	public void refresh() {
		_compResourcesMap.clear();
		_createdResTree.removeAll();
		populateResObjectMap();
		populateCompObjectMap();
		_componentsTreeComposite.populateCompTypeMap();
		_componentsTreeComposite.createTreeItems();
		buildCreatedResourcesTreeItem(_createdResTree);
		_chassisName = getChassisName();
		if(_createdResTree.getItem(0).getItemCount() == 0) {
			setMinimumTreeSize(_createdResTree.getParent(), _createdResTree);
			_createdResTree.getParent().layout();
		}
	}
	
	@Override
	public void setInput(IEditorInput input)
    {
		super.setInput(input);
		_editorInput = (ManageabilityEditorInput) input;
		_editorInput.setEditor(this);
		_editor = this;
		_pdm = _editorInput.getProjectDataModel();
		_caInput = _editorInput.getCAInput();
		_compInput = _editorInput.getCompInput();
		_viewModel = _pdm.getNodeProfiles();
		_resourceModel = _pdm.getCAModel();
		_resourceList = _resourceModel.getEList();
		_chassisName = getChassisName();
		_componentModel = _pdm.getComponentModel();
		_componentList = _componentModel.getEList();
		
		Model mapModel = _pdm.getResourceAlarmMapModel();
        _mapViewModel = mapModel.getViewModel();
        Model alarmRuleModel = _pdm.getAlarmRules();
        _alarmRuleViewModel	= alarmRuleModel.getViewModel();
        
		EObject mapObj =  _pdm.getComponentResourceMapModel().getEObject();
        EObject linkObj = SubModelMapReader.getLinkObject(mapObj, ComponentEditorConstants.
        		ASSOCIATE_RESOURCES_NAME);
        _rtModel = new Model(_pdm.getComponentResourceMapModel().getResource(), linkObj).getViewModel();
        _resourceAssociationModel = _pdm.getResourceAssociationModel();
		populateResObjectMap();
		populateCompObjectMap();
		EObject amfObj = _viewModel.getEObject();
        EClass amfClass = (EClass) _viewModel.getEPackage().getEClassifier("amfConfig");
        EReference nodeInstsRef = (EReference) amfClass.getEStructuralFeature(SafConstants.NODE_INSTANCES_NAME);
        EObject nodeInstancesObj = (EObject) amfObj.eGet(nodeInstsRef);
        
        EStructuralFeature nodeFeature = (EStructuralFeature) nodeInstancesObj
		.eClass().getEStructuralFeature(SafConstants.NODE_INSTANCELIST_NAME);
        _nodeList = (EList) nodeInstancesObj.eGet(nodeFeature);
        EObject rootObj = (EObject) _componentModel.getEList().get(0);
        NotifyingList safComponentList = (NotifyingList) EcoreUtils.getValue(rootObj,
        		ComponentEditorConstants.SAFCOMPONENT_REF_NAME);
        SubModelMapReader reader = SubModelMapReader.getSubModelMappingReader(
        		_pdm.getProject(), "component", "resource");
        reader.initializeLinkRDN(safComponentList);
    }
	/**
	 * Returns Component List
	 * @return
	 */
	public List getComponentList() {
		return _componentList;
	}
	/**
	 * Returns Resource List
	 * @return
	 */
	public List getResourceList() {
		return _resourceList;
	}
	/**
	 * Returns Resource model
	 * @return
	 */
	public Model getResourceModel() {
		return _resourceModel;
	}
	/**
	 * Creates Tree Items for the Created Resource
	 * 
	 * @param tree
	 */
	public void buildCreatedResourcesTreeItem(Tree tree) {
		TreeItem root = new TreeItem(tree, SWT.NONE);
		root.setText("Created Resources");
		for (int i = 0; i < _nodeList.size(); i++) {
			EObject nodeObj = _nodeList.get(i);
			TreeItem item = new TreeItem(root, SWT.NONE);
			item.setText(EcoreUtils.getName(nodeObj));
			item.setData(nodeObj);
			updateCompResValues(nodeObj);
			EObject serviceUnitInstsObj = (EObject) EcoreUtils.getValue(
					nodeObj, SafConstants.SERVICEUNIT_INSTANCES_NAME);
			if (serviceUnitInstsObj != null) {
				List suInstList = (List) EcoreUtils.getValue(serviceUnitInstsObj,
						SafConstants.SERVICEUNIT_INSTANCELIST_NAME);
				for (int j = 0; j < suInstList.size(); j++) {
					EObject suInstObj = (EObject) suInstList.get(j);
					EObject compInstsObj = (EObject) EcoreUtils.getValue(suInstObj,
							SafConstants.COMPONENT_INSTANCES_NAME);
					if (compInstsObj != null) {
						List compInstList = (List) EcoreUtils.getValue(
								compInstsObj,
								SafConstants.COMPONENT_INSTANCELIST_NAME);
						for (int k = 0; k < compInstList.size(); k++) {
							EObject compInstObj = (EObject) compInstList.get(k);
							TreeItem item1 = new TreeItem(item, SWT.NONE);
							item1.setText(EcoreUtils.getName(compInstObj));
							item1.setData(compInstObj);
							Map<String, List> typeInstListMap = new HashMap<String, List>();
							List<EObject> resourceList = (EList) EcoreUtils.getValue(
									compInstObj, "resources");
							
							for (int l = 0; l < resourceList.size(); l++) {
								EObject resObj = resourceList.get(l);
								String moID = (String)EcoreUtils.getValue(resObj, "moID");
								String resName = getResourceTypeForInstanceID(moID);
								if (_resNames.contains(resName)) {
									List resList = typeInstListMap.get(resName);
									if (resList == null) {
										resList = new NotifyingListImpl();
										typeInstListMap.put(resName, resList);
									}
									resList.add(resObj);
								}
							}
							
							Iterator<String> iterator = typeInstListMap.keySet().iterator();
							while(iterator.hasNext()) {
								TreeItem item2 = new TreeItem(item1, SWT.NONE);
								String name = iterator.next();
								item2.setText(name);
								item2.setData(resourceList);
							}
						}
					}
				}
			}
		}
	}
	/**
	 * Update the Created Resource Tree. and expand/select the items if required
	 * @param tree
	 * @param resNames
	 * @param compNames
	 * @param needsToBeExpand
	 */
	public void updateCreatedResourcesTreeItem(Tree tree,
			List<String> resNames, List<String> compNames,
			boolean needsToBeExpand) {
		populateResObjectMap();
		TreeItem root = new TreeItem(tree, SWT.NONE);
		List<TreeItem> needSelectionList = new ArrayList<TreeItem>(resNames
				.size());
		root.setText("Created Resources");
		for (int i = 0; i < _nodeList.size(); i++) {
			EObject nodeObj = _nodeList.get(i);
			TreeItem item = new TreeItem(root, SWT.NONE);
			item.setText(EcoreUtils.getName(nodeObj));
			item.setData(nodeObj);
			EObject serviceUnitInstsObj = (EObject) EcoreUtils.getValue(
					nodeObj, SafConstants.SERVICEUNIT_INSTANCES_NAME);
			if (serviceUnitInstsObj != null) {
				List suInstList = (List) EcoreUtils.getValue(
						serviceUnitInstsObj,
						SafConstants.SERVICEUNIT_INSTANCELIST_NAME);
				for (int j = 0; j < suInstList.size(); j++) {
					EObject suInstObj = (EObject) suInstList.get(j);
					EObject compInstsObj = (EObject) EcoreUtils.getValue(
							suInstObj, SafConstants.COMPONENT_INSTANCES_NAME);
					if (compInstsObj != null) {
						List compInstList = (List) EcoreUtils.getValue(
								compInstsObj,
								SafConstants.COMPONENT_INSTANCELIST_NAME);
						for (int k = 0; k < compInstList.size(); k++) {
							EObject compInstObj = (EObject) compInstList.get(k);
							String compType = (String) EcoreUtils.getValue(
									compInstObj, "type");
							TreeItem item1 = new TreeItem(item, SWT.NONE);
							item1.setText(EcoreUtils.getName(compInstObj));
							item1.setData(compInstObj);

							Map<String, List> typeInstListMap = new HashMap<String, List>();
							List<EObject> resourceList = (EList) EcoreUtils
									.getValue(compInstObj, "resources");

							for (int l = 0; l < resourceList.size(); l++) {
								EObject resObj = resourceList.get(l);
								String moID = (String) EcoreUtils.getValue(
										resObj, "moID");
								String resName = getResourceTypeForInstanceID(moID);
								if (_resNames.contains(resName)) {
									List resList = typeInstListMap.get(resName);
									if (resList == null) {
										resList = new NotifyingListImpl();
										typeInstListMap.put(resName, resList);
									}
									resList.add(resObj);
								}
							}

							Iterator<String> iterator = typeInstListMap
									.keySet().iterator();
							if (compNames.contains(compType) && needsToBeExpand) {
								while (iterator.hasNext()) {
									TreeItem item2 = new TreeItem(item1,
											SWT.NONE);
									String name = iterator.next();
									item2.setText(name);
									item2.setData(resourceList);
									if (resNames.contains(name)) {
										item2.setExpanded(true);
										needSelectionList.add(item2);
									}
								}
							} else {
								while (iterator.hasNext()) {
									TreeItem item2 = new TreeItem(item1,
											SWT.NONE);
									String name = iterator.next();
									item2.setText(name);
									item2.setData(resourceList);
								}
							}
						}
					}
				}
			}
		}
		TreeItem[] needsToBeSelected = new TreeItem[needSelectionList.size()];
		for (int i = 0; i < needSelectionList.size(); i++) {
			needsToBeSelected[i] = needSelectionList.get(i);
		}
		tree.setSelection(needsToBeSelected);
	}
	/**
	 * Update the Created Resource Tree. and expand/select the items if required
	 * @param tree
	 * @param compNames
	 * @param needsToBeExpand
	 */
	public void updateCreatedResourcesTreeItem(Tree tree,
			List<String> compNames,
			boolean needsToBeExpand) {
		populateResObjectMap();
		TreeItem root = new TreeItem(tree, SWT.NONE);
		List<TreeItem> needSelectionList = new ArrayList<TreeItem>();
		root.setText("Created Resources");
		for (int i = 0; i < _nodeList.size(); i++) {
			EObject nodeObj = _nodeList.get(i);
			TreeItem item = new TreeItem(root, SWT.NONE);
			item.setText(EcoreUtils.getName(nodeObj));
			item.setData(nodeObj);
			EObject serviceUnitInstsObj = (EObject) EcoreUtils.getValue(
					nodeObj, SafConstants.SERVICEUNIT_INSTANCES_NAME);
			if (serviceUnitInstsObj != null) {
				List suInstList = (List) EcoreUtils.getValue(
						serviceUnitInstsObj,
						SafConstants.SERVICEUNIT_INSTANCELIST_NAME);
				for (int j = 0; j < suInstList.size(); j++) {
					EObject suInstObj = (EObject) suInstList.get(j);
					EObject compInstsObj = (EObject) EcoreUtils.getValue(
							suInstObj, SafConstants.COMPONENT_INSTANCES_NAME);
					if (compInstsObj != null) {
						List compInstList = (List) EcoreUtils.getValue(
								compInstsObj,
								SafConstants.COMPONENT_INSTANCELIST_NAME);
						for (int k = 0; k < compInstList.size(); k++) {
							EObject compInstObj = (EObject) compInstList.get(k);
							String compName = EcoreUtils.getName(compInstObj);
							TreeItem item1 = new TreeItem(item, SWT.NONE);
							item1.setText(EcoreUtils.getName(compInstObj));
							item1.setData(compInstObj);

							Map<String, List> typeInstListMap = new HashMap<String, List>();
							List<EObject> resourceList = (EList) EcoreUtils
									.getValue(compInstObj, "resources");

							for (int l = 0; l < resourceList.size(); l++) {
								EObject resObj = resourceList.get(l);
								String moID = (String) EcoreUtils.getValue(
										resObj, "moID");
								String resName = getResourceTypeForInstanceID(moID);
								if (_resNames.contains(resName)) {
									List resList = typeInstListMap.get(resName);
									if (resList == null) {
										resList = new NotifyingListImpl();
										typeInstListMap.put(resName, resList);
									}
									resList.add(resObj);
								}
							}

							Iterator<String> iterator = typeInstListMap
									.keySet().iterator();
							if (compNames.contains(compName) && needsToBeExpand) {
								while (iterator.hasNext()) {
									TreeItem item2 = new TreeItem(item1,
											SWT.NONE);
									String name = iterator.next();
									item2.setText(name);
									item2.setData(resourceList);
								}
								item1.setExpanded(true);
								needSelectionList.add(item1);
							} else {
								while (iterator.hasNext()) {
									TreeItem item2 = new TreeItem(item1,
											SWT.NONE);
									String name = iterator.next();
									item2.setText(name);
									item2.setData(resourceList);
								}
							}
						}
					}
				}
			}
		}
		TreeItem[] needsToBeSelected = new TreeItem[needSelectionList.size()];
		for (int i = 0; i < needSelectionList.size(); i++) {
			needsToBeSelected[i] = needSelectionList.get(i);
		}
		tree.setSelection(needsToBeSelected);
	}
	/**
     * Updates the Component Instance to Associated Resource 
     * Values after reading from the Resource. 
     * @param nodeInstObj - Node Instances Object
     */
	private void updateCompResValues(EObject nodeInstObj) {
		HashMap<String, EObject> compInstMap = new HashMap<String, EObject>();
		EObject serviceUnitInstsObj = (EObject) EcoreUtils.getValue(
				nodeInstObj, SafConstants.SERVICEUNIT_INSTANCES_NAME);
		if (serviceUnitInstsObj != null) {
			List suInstList = (List) EcoreUtils.getValue(serviceUnitInstsObj,
					SafConstants.SERVICEUNIT_INSTANCELIST_NAME);
			for (int j = 0; j < suInstList.size(); j++) {
				EObject suInstObj = (EObject) suInstList.get(j);
				EObject compInstsObj = (EObject) EcoreUtils.getValue(suInstObj,
						SafConstants.COMPONENT_INSTANCES_NAME);
				if (compInstsObj != null) {
					List compInstList = (List) EcoreUtils.getValue(
							compInstsObj,
							SafConstants.COMPONENT_INSTANCELIST_NAME);
					for (int k = 0; k < compInstList.size(); k++) {
						EObject compInstObj = (EObject) compInstList.get(k);
						compInstMap.put(EcoreUtils.getName(compInstObj),
								compInstObj);
					}
				}
			}
		}
		Resource rtResource = NodeProfileDialog.getCompResResource(EcoreUtils
				.getName(nodeInstObj), false, ProjectDataModel
				.getProjectDataModel((IContainer) _pdm.getProject()));
		if (rtResource != null && rtResource.getContents().size() > 0) {
			_nodeResourceMap.put(nodeInstObj, rtResource);
			EObject compInstancesObj = (EObject) rtResource.getContents()
					.get(0);
			List compInstList = (List) EcoreUtils.getValue(compInstancesObj,
					"compInst");
			for (int j = 0; compInstList != null && j < compInstList.size(); j++) {
				EObject compObj = (EObject) compInstList.get(j);
				String compInstName = EcoreUtils.getName(compObj);
				EObject compInstObj = (EObject) compInstMap.get(compInstName);
				if (compInstObj != null) {
					BasicEList compResList = (BasicEList) EcoreUtils.getValue(compInstObj,
							SafConstants.RESOURCES_NAME);
					List resList = (List) EcoreUtils.getValue(compObj,
							SafConstants.RESOURCELIST_NAME);
					_compResourcesMap.put(compInstObj, resList);
					compResList.clear();
					compResList.grow(resList.size());
					compResList.addAllUnique(resList);
					//resList.clear();
				}
			}
		}
	}

	/**
	 * Return ResourceType for the moID
	 * @param id resource moID
	 * @return String Resource type
	 */
	private String getResourceTypeForInstanceID(String id) {
    	if (id.contains(":") && id.contains("\\")) {
			String paths[] = id.split(":");
			StringTokenizer tokenizer = new StringTokenizer(
					paths[paths.length - 2], "\\");
			tokenizer.nextToken();
			String resName = tokenizer.nextToken();
			return resName;
		}
		return id;
    }
	/**
	 * Return instance from the moID
	 * @param id resource moID
	 * @return int instance number
	 */
	private int getInstanceNumberFromMoID(String id) {
    	if (id.contains(":") && id.contains("\\") && !id.endsWith("*")) {
			String paths[] = id.split(":");
			return Integer.parseInt(paths[paths.length - 1]);
		}
		return 0;
    }
	/**
	 * Creates name-Object mapping for all the resources.
	 */
	public void populateResObjectMap(){
		EObject rootObject = (EObject) _resourceList.get(0);
		List<EObject> mibList = (EList)rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.MIB_RESOURCE_REF_NAME));
		_resNames = new ArrayList<String>(mibList.size());
		for (int i = 0; i < mibList.size(); i++) {
			EObject resObj = mibList.get(i);
			String name = EcoreUtils.getName(resObj);
			_resNames.add(name);
		}
		_resNameObjMap.clear();
		List<EObject> resourcesList = ResourceDataUtils.getMoList(_resourceList);
		for (int i = 0; i < resourcesList.size(); i++) {
			EObject resObj = resourcesList.get(i);
			_resNameObjMap.put(EcoreUtils.getName(resObj), resObj);
		}
	}
	/**
	 * Creates name-Object mapping for all the components.
	 */
	public void populateCompObjectMap(){
		_compNameObjMap.clear();
		EObject rootObject = (EObject) _componentList.get(0);
		EList<EObject> compList = (EList)rootObject.eGet(rootObject.eClass().getEStructuralFeature("safComponent"));
		for (int i = 0; i < compList.size(); i++) {
			EObject resObj = compList.get(i);
			_compNameObjMap.put(EcoreUtils.getName(resObj), resObj);
		}
	}
	/**
	 * Returns the EObject for the resource
	 * @param name Resource name
	 * @return Resource Object
	 */
	private EObject getResEObject(String name){
		return _resNameObjMap.get(name);
	}
	/**
	 * Editor PropertyChange 
	 */
	public void propertyChange(PropertyChangeEvent evt) {
		_isDirty = true;
		firePropertyChange(IEditorPart.PROP_DIRTY);
	}
	@Override
	public void dispose() {
		_rtModel.dispose();
				 
		if(_mapViewModel != null) {
        	_mapViewModel.dispose();
        	_mapViewModel = null;
        }
        if(_alarmRuleViewModel != null) {
        	_alarmRuleViewModel.dispose();
        	_alarmRuleViewModel = null;
        }
		super.dispose();
	}
	 /**
     * 
     * @return the link view model
     */
    public Model getLinkViewModel()
    {
    	return _mapViewModel;
    }
    
    /**
     * 
     * @return the alarm rule view model
     */
    public Model getAlarmRuleViewModel()
    {
    	return _alarmRuleViewModel;
    }
	/**
	 * update the max instances for Resources and associate resources to the
	 * components
	 */
	private void updateAssociatedResources(){
		Map<String, NotifyingList<String>> compResMap = new HashMap<String, NotifyingList<String>>();
		Map<String, EObject> createdMoIDResourceMap = new HashMap<String, EObject>();
		for (int i = 0; i < _nodeList.size(); i++) {
			EObject nodeObj = _nodeList.get(i);
			EObject serviceUnitInstsObj = (EObject) EcoreUtils.getValue(
					nodeObj, SafConstants.SERVICEUNIT_INSTANCES_NAME);
			if (serviceUnitInstsObj != null) {
				List suInstList = (List) EcoreUtils.getValue(serviceUnitInstsObj,
						SafConstants.SERVICEUNIT_INSTANCELIST_NAME);
				for (int j = 0; j < suInstList.size(); j++) {
					EObject suInstObj = (EObject) suInstList.get(j);
					EObject compInstsObj = (EObject) EcoreUtils.getValue(suInstObj,
							SafConstants.COMPONENT_INSTANCES_NAME);
					if (compInstsObj != null) {
						List compInstList = (List) EcoreUtils.getValue(
								compInstsObj,
								SafConstants.COMPONENT_INSTANCELIST_NAME);
						for (int k = 0; k < compInstList.size(); k++) {
							EObject compInstObj = (EObject) compInstList.get(k);
							String compType = (String) EcoreUtils.getValue(
									compInstObj, "type");
							Map<String, List> typeInstListMap = new HashMap<String, List>();
							List<EObject> resourceList = (EList) EcoreUtils.getValue(
									compInstObj, "resources");
														
							NotifyingList<String> associatedResourcesList = null;
							
							associatedResourcesList = compResMap.get(compType);
							if (associatedResourcesList == null) {
								String rdn = (String) EcoreUtils.getValue(
 										_compNameObjMap.get(compType),
										ModelConstants.RDN_FEATURE_NAME);
								associatedResourcesList = (NotifyingList) SubModelMapReader
										.getLinkTargetObjects(
												_rtModel
														.getEObject(), rdn);
								if (associatedResourcesList == null) {
									associatedResourcesList = (NotifyingList) SubModelMapReader
											.createLinkTargets(
													_rtModel
															.getEObject(),
													EcoreUtils
															.getName(_compNameObjMap
																	.get(compType)),
													rdn);
								}
								associatedResourcesList.removeAll(_resNames);
								compResMap.put(compType,
										associatedResourcesList);
							}
							for (int l = 0; l < resourceList.size(); l++) {
								EObject resObj = resourceList.get(l);
								String moID = (String)EcoreUtils.getValue(resObj, "moID");
								String resName = getResourceTypeForInstanceID(moID);
								
								if(!associatedResourcesList.contains(resName)) {
									associatedResourcesList.add(resName);
								}
								EObject obj = createdMoIDResourceMap.get(moID);
								if(obj != null) {
									EcoreUtils.setValue(obj,"primaryOI", "false");
									EcoreUtils.setValue(resObj,"primaryOI", "false");
								} else {
									EcoreUtils.setValue(resObj,"primaryOI", "true");
									createdMoIDResourceMap.put(moID, resObj);
								}
							}
						}
					}
				}
			}
		}
	}
	/**
	 * Saving the resources in rt.xml
	 */
	public void writeCompToResMap() {
		EObject amfObj = (EObject) _viewModel.getEList().get(0);
		EReference nodeInstsRef = (EReference) amfObj.eClass()
				.getEStructuralFeature(SafConstants.NODE_INSTANCES_NAME);
		EObject nodeInstsObj = (EObject) amfObj.eGet(nodeInstsRef);
		List nodeList = null;
		if (nodeInstsObj != null) {
			EReference nodeInstRef = (EReference) nodeInstsObj.eClass()
					.getEStructuralFeature(SafConstants.NODE_INSTANCELIST_NAME);
			nodeList = (List) nodeInstsObj.eGet(nodeInstRef);
		}

		EClass mapClass = (EClass) _pdm.getNodeProfiles().getEPackage()
				.getEClassifier("compInstances");
		EReference compInstRef = (EReference) mapClass
				.getEStructuralFeature("compInst");
		EObject mapCompInstsObj = EcoreUtils.createEObject(mapClass, true);

		Iterator iterator = nodeList.iterator();
		while (iterator.hasNext()) {
			EObject nodeInstObj = (EObject) iterator.next();
			EObject suInstsObj = (EObject) EcoreUtils.getValue(nodeInstObj,
					SafConstants.SERVICEUNIT_INSTANCES_NAME);
			List mapCompInstList = (List) mapCompInstsObj.eGet(compInstRef);
			mapCompInstList.clear();
			if (suInstsObj != null) {
				List suInstList = (List) EcoreUtils.getValue(suInstsObj,
						SafConstants.SERVICEUNIT_INSTANCELIST_NAME);
				for (int j = 0; j < suInstList.size(); j++) {
					EObject suInstObj = (EObject) suInstList.get(j);
					EObject compInstsObj = (EObject) EcoreUtils.getValue(
							suInstObj, SafConstants.COMPONENT_INSTANCES_NAME);
					if (compInstsObj != null) {
						List compInstList = (List) EcoreUtils.getValue(
								compInstsObj,
								SafConstants.COMPONENT_INSTANCELIST_NAME);
						for (int k = 0; k < compInstList.size(); k++) {
							EObject compInstObj = (EObject) compInstList.get(k);
							EObject mapCompObj = EcoreUtils.createEObject(
									compInstRef.getEReferenceType(), true);
							mapCompInstList.add(mapCompObj);
							EcoreUtils.setValue(mapCompObj, "compName",
									(String) EcoreUtils.getValue(compInstObj,
											"name"));
							List compResList = (List) EcoreUtils.getValue(
									compInstObj, SafConstants.RESOURCES_NAME);
							BasicEList mapResList = (BasicEList) EcoreUtils.getValue(
									mapCompObj, SafConstants.RESOURCELIST_NAME);
							mapResList.clear();
							mapResList.grow(compResList.size());
							mapResList.addAllUnique(compResList);
							//compResList.clear();
						}
					}
				}
				Resource rtResource = NodeProfileDialog.getCompResResource(EcoreUtils
						.getName(nodeInstObj), true, _pdm);
				rtResource.getContents().clear();
				rtResource.getContents().add(mapCompInstsObj);
				try {
					EcoreModels.saveResource(rtResource);
				} catch (Exception e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
		}
	}
	
	/**
	 * Returns Chassis Name
	 * @return
	 */
	public String getChassisName() {
		if(_chassisName != null)
			return _chassisName;
		List<EObject> resObjects = _resourceModel.getEList();
    	EObject rootObject = (EObject) resObjects.get(0);
		EReference ref = (EReference) rootObject.eClass()
						.getEStructuralFeature(ClassEditorConstants.CHASSIS_RESOURCE_REF_NAME);
		EList<EObject> chassisList = (EList) rootObject.eGet(ref);

		if (chassisList.size() != 1) return "Chassis";

		EObject eobj = (EObject) chassisList.get(0);
		String chassisName = EcoreUtils.getName(eobj);
		_chassisName = chassisName;
		return _chassisName;
	}
}

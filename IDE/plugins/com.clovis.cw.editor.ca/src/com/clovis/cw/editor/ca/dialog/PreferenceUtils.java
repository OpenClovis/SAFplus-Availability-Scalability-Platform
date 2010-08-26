package com.clovis.cw.editor.ca.dialog;

import java.util.Iterator;
import java.util.List;

import org.eclipse.emf.common.notify.Adapter;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.impl.NotificationImpl;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EAnnotation;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.ENamedElement;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Tree;
import org.eclipse.swt.widgets.TreeItem;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.menus.IMenuService;

import com.clovis.common.utils.constants.AnnotationConstants;
import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.DialogValidator;
/**
 * This class contains the helper methods to manipulate the tree
 * for the Preference Dialog.
 * @author Suraj Rajyaguru
 *
 */
public class PreferenceUtils {

	/**
	 * Adds a listener to the tree shown on the left side of
	 * the dialog.
     * @param dialog PreferenceDialog
     * @param viewer TreeViewer
     */
	public static void addListenerForTree(final PreferenceDialog dialog,
    		final TreeViewer viewer) {
    	Tree tree = (Tree) viewer.getControl();
    	tree.setData("dialog", dialog);
    	final IMenuService menuService
    	   = (IMenuService)PlatformUI.getWorkbench()
    	     .getService(IMenuService.class);
    	final MenuManager manager = new MenuManager();
    	manager.setRemoveAllWhenShown(true);
    	Control control = viewer.getControl();
    	final Menu menu = manager.createContextMenu(control);
    	
    	manager.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr) {
				if (PreferenceUtils.isSelectionAvailable(viewer)) {
					PreferenceSelectionData psd = PreferenceUtils
					.getCurrentSelectionData(viewer);
					PreferencePage page = psd.getPrefPage();
					boolean menuNeeded = true;
							
					if(!(page instanceof GenericFormPage)) {
			        	String selectedItemText = psd.getTreeItem().getText();
						if (!selectedItemText.endsWith("List")
								|| ((PreferencePage) ((PreferenceDialog) dialog)
										.getSelectedPage()).isValid() == false) {
							menuNeeded = false;
						}
			        } else {
			        	if (PreferenceUtils.isTreeNode(psd.getEObject()
								.eContainingFeature())) {
			        		menuNeeded = false;
						}
			        	if(PreferenceUtils.isTreeDumbNode(psd.getEObject())) {
			        		menuNeeded = false;
			        	}
					}
					if(menuNeeded){
						menuService.populateContributionManager(manager,
							"popup:#PreferenceDialogContext?after=additions");
					}
				}
			}
		});
    	tree.setMenu(menu);
    	
    	control.addDisposeListener(new DisposeListener() {
    	public void widgetDisposed(DisposeEvent e) {
    	menuService.releaseContributions(manager);
    	} });
    	    	
        viewer.addSelectionChangedListener(new PreferenceSelectionChangedListener());
	}

    /**
     * Creates the child tree for the given node by recursively
     * calling itself.
     * @param parentNode PreferenceNode
     * @param eObj EObject
     */
    public static void createChildTree(PreferenceNode parentNode, EObject eObj) {
    	List featureList = eObj.eClass().getEAllStructuralFeatures();
    	for(int i=0; i<featureList.size() ; i++) {
    		EStructuralFeature feature = (EStructuralFeature) featureList.get(i);

    		if(isTreeListNode(feature)) {
    			Object obj = eObj.eGet(feature);
    			EList objList;
    			if(obj instanceof List) {
    				objList = (EList) obj;
    			} else {
        			EObject eObject = (EObject) obj;
        			EStructuralFeature listFeature = (EStructuralFeature) eObject
        				.eClass().getEAllReferences().get(0);
        			objList = (EList) eObject.eGet(listFeature);
    			}
        		String label = EcoreUtils.getAnnotationVal(feature, null,
						AnnotationConstants.TREE_LABEL);
    			PreferenceNode listNode = new PreferenceNode(label,
    				new BlankPreferencePage(label));
    			parentNode.add(listNode);
    			for(int j=0 ; j<objList.size() ; j++) {
    				EObject childObj = (EObject) objList.get(j);
    				String childName = (String) EcoreUtils.getName(childObj);
    				PreferenceNode node = new PreferenceNode(childName,
    						new GenericFormPage(childName, childObj));
    				listNode.add(node);
    				createChildTree(node, childObj);
    			}
    		} else if(PreferenceUtils.isTreeNode(feature)) {
				EObject childObj = (EObject) eObj.eGet(feature);
				if(childObj == null) {
					childObj = EcoreUtils.createEObject((EClass) feature.getEType(), true);
				}
				String childName = EcoreUtils.getAnnotationVal(feature, null,
						AnnotationConstants.TREE_LABEL);
				PreferenceNode node = new PreferenceNode(childName,
						new GenericFormPage(childName, childObj));
				parentNode.add(node);
				createChildTree(node, childObj);
			}
    	}
    }
    
    /**
     * Initializes the Eobject and puts it into the List.
     * @param nodeObj EObject
     * @param nodeList EList
     * @param dialog Dialog 
     * @return true if successful.
     */
	public static boolean initializeEObject(EObject nodeObj, List nodeList, Dialog dialog) {
		if(! initializeNameField(nodeObj, nodeList, dialog)) {
			return false;
		}
		EcoreUtils.setValue(nodeObj, ModelConstants.RDN_FEATURE_NAME,
				nodeObj.toString());

		// handle specific field initialization in generic manner
		String initializationInfo = EcoreUtils.getAnnotationVal(nodeObj
				.eClass(), null, "initializationFields");
		if (initializationInfo != null) {
			EcoreUtils.initializeFields(nodeObj, nodeList, initializationInfo);
		}
		return true;
	}
    
    /**
	 * Initializes the Name Field for the EObject.
	 * 
	 * @param nodeObj
	 *            EObject
	 * @param nodeList
	 *            Elist
     * @param dialog Dialog
	 * @return true if successful.
	 */
	private static boolean initializeNameField(EObject nodeObj, List nodeList, Dialog dialog) {
		String nameField = EcoreUtils.getNameField(nodeObj.eClass());
		if (nameField != null) {
			String name;
			EAnnotation ann = nodeObj.eClass().getEAnnotation("CWAnnotation");
			String isNameComboMsg = (String) (ann != null ? ann.getDetails()
					.get(AnnotationConstants.IS_NAME_COMBO) : null);
			if (isNameComboMsg != null) {
				name = getNextComboValue(nameField, nodeObj, nodeList, dialog);
			} else {
				name = EcoreUtils.getNextValue(nodeObj.eClass().getName(),
						nodeList, nameField);
			}
			if (name == null) {
				MessageDialog.openError(null, "Validations", isNameComboMsg);
				return false;
			}
			EcoreUtils.setValue(nodeObj, nameField, name);
		}
		return true;
	}

	/**
	 * Gets Next value for the feature from the combo box editor for the
	 * feature.
	 * 
	 * @param nameField
	 *            String
	 * @param nodeObj
	 *            EObject
	 * @param nodeList
	 *            EList
	 * @param dialog Dialog
	 * @return true if successful.
	 */
	@SuppressWarnings("unchecked")
	private static String getNextComboValue(String nameField, EObject nodeObj,
			List nodeList, Dialog dialog) {

		List<String> comboValues = null;
		if(dialog instanceof RMDDialog) {
			comboValues = RMDDialog.getInstance().getEOs();
		}

		if (comboValues.size() == nodeList.size()) {
			return null;
		}

		String values[] = (String[]) comboValues.toArray(new String[] {});
		for (int i = 0; i < values.length; i++) {
			String value = values[i];
			boolean existFlag = false;

			Iterator itr = nodeList.iterator();
			while (itr.hasNext()) {
				if (value.equalsIgnoreCase(EcoreUtils.getName((EObject) itr
						.next()))) {
					existFlag = true;
					break;
				}
			}
			if (!existFlag) {
				return value;
			}
		}
		return null;
	}

	/**
	 * Creates the preference node and corresponding preference page from the
	 * EObject and EList and adds it to the current node.
	 * 
	 * @param currentPrefNode
	 *            PreferenceNode
	 * @param nodeObj
	 *            EObject
	 * @param nodeList
	 *            EList
	 * @return PreferenceNode
	 */
    public static PreferenceNode createAndAddPerferenceNode(PreferenceNode parentPrefNode,
    	EObject nodeObj) {
		PreferencePage nodePrefPage = new GenericFormPage(EcoreUtils.
				getName(nodeObj), nodeObj);
		PreferenceNode nodePrefNode = new PreferenceNode(EcoreUtils.
				getName(nodeObj), nodePrefPage);
		parentPrefNode.add(nodePrefNode);
		return nodePrefNode;
    }

    /**
	 * Sets the selection in the tree viewer of the dialog.
	 * @param viewer TreeViewer
	 * @param node PreferenceNode
	 */
	public static void setTreeViewerSelection(TreeViewer viewer, PreferenceNode node) {
		viewer.refresh();
		viewer.setSelection(new StructuredSelection(node), true);
	}

	/**
	 * Sets the selection in the tree viewer of the dialog.
	 * @param viewer TreeViewer
	 * @param currentPrefNode PreferenceNode
	 * @param nodePrefNode PreferenceNode
	 */
	public static void setTreeViewerSelection(TreeViewer viewer, PreferenceNode currentPrefNode,
			PreferenceNode nodePrefNode) {
		viewer.refresh();
		viewer.expandToLevel(currentPrefNode, 1);
		viewer.setSelection(new StructuredSelection(nodePrefNode), true);
	}

	/**
	 * Checks wether selection is made for the Tree Viewer or not.
	 * @param treeViewer TreeViewer
	 * @return boolean
	 */
	public static boolean isSelectionAvailable(TreeViewer treeViewer) {
		if(treeViewer.getTree().getSelection().length  == 0) {
			return false;
		} else {
			return true;
		}
	}

	/**
	 * Returns the EList which contains the eObject.
	 * @param eObject EObject
	 * @return EList
	 */
	public static EList getContainerEList(EObject eObject) {
		EStructuralFeature containerFeature = eObject.eContainingFeature();
		EObject containerObj = eObject.eContainer();
		EList eList = (EList) containerObj.eGet(containerFeature);
		return eList;
	}
	
	/**
	 * Notifies the Dialog Validators which have been registered
	 * with this eObj.
	 * @param eObj
	 */
	public static void performNotification(final EObject eObj) {
		NotificationImpl notification = new NotificationImpl(
				Notification.SET, null, eObj, true) {
			public boolean isTouch() {
				return false;
			}
			public Object getNotifier() {
				return eObj;
			}
		};
		Iterator itr = eObj.eAdapters().iterator();
		while(itr.hasNext()) {
			Adapter adapter = (Adapter) itr.next();
			if(adapter instanceof DialogValidator) {
				adapter.notifyChanged(notification);
			}
		}
	}

	/**
	 * Returns the current selection for the tree viewer.
	 * @param treeViewer TreeViewer
	 * @return PreferenceSelectionData
	 */
	public static PreferenceSelectionData getCurrentSelectionData(TreeViewer treeViewer) {
		TreeItem treeItem = treeViewer.getTree().getSelection()[0];
		PreferenceNode prefNode = (PreferenceNode)treeItem.getData();
		PreferencePage prefPage = (PreferencePage) prefNode.getPage();
		EObject eObject = null;
		EObject containerEObject = null;
		TreeItem parentTreeItem = treeItem.getParentItem();
		PreferenceNode parentPrefNode = null;
		if(prefPage instanceof GenericFormPage) {
			eObject = ((GenericFormPage) prefPage).getEObject();
			containerEObject = eObject.eContainer();
			parentPrefNode = (parentTreeItem != null) 
				? (PreferenceNode) parentTreeItem.getData() : null;
		} else {
			if(parentTreeItem != null) {
				parentPrefNode = (PreferenceNode) parentTreeItem.getData();
				PreferencePage page = (PreferencePage) parentPrefNode.getPage();
				if(page instanceof GenericFormPage) {
					eObject = ((GenericFormPage) page).getEObject();
					containerEObject = eObject.eContainer();
				}
			}
		}

		return new PreferenceSelectionData(treeItem, prefNode, prefPage,
			eObject, containerEObject, parentTreeItem, parentPrefNode);
	}
	
	/**
	 * Class that holds the current selection details of the tree viewer.
	 * @author Suraj Rajyaguru
	 *
	 */
	public static class PreferenceSelectionData {
		private TreeItem _treeItem;
		private PreferenceNode _prefNode;
		private PreferencePage _prefPage;
		private EObject _eObject;
		private EObject _containerEObject;
		private TreeItem _parentTreeItem;
		private PreferenceNode _parentPrefNode;

		/**
		 * Constructor.
		 * @param treeItem TreeItem
		 * @param prefNode PreferenceNode
		 * @param prefPage PreferencePage
		 * @param eObject EObject
		 * @param containerEObject EObject
		 * @param parentTreeItem TreeItem
		 * @param parentPrefNode PreferenceNode
		 */
		PreferenceSelectionData(TreeItem treeItem, PreferenceNode prefNode,
			PreferencePage prefPage, EObject eObject, EObject containerEObject,
			TreeItem parentTreeItem, PreferenceNode parentPrefNode) {

			_treeItem = treeItem;
			_prefNode = prefNode;
			_prefPage = prefPage;
			_eObject = eObject;
			_containerEObject = containerEObject;
			_parentTreeItem = parentTreeItem;
			_parentPrefNode = parentPrefNode;
		}

		/**
		 * @return
		 */
		public PreferenceNode getPrefNode() {
			return _prefNode;
		}

		/**
		 * @return
		 */
		public PreferencePage getPrefPage() {
			return _prefPage;
		}

		/**
		 * @return
		 */
		public TreeItem getTreeItem() {
			return _treeItem;
		}

		/**
		 * @return
		 */
		public PreferenceNode getParentPrefNode() {
			return _parentPrefNode;
		}

		/**
		 * @return
		 */
		public TreeItem getParentTreeItem() {
			return _parentTreeItem;
		}

		/**
		 * @return
		 */
		public EObject getContainerEObject() {
			return _containerEObject;
		}

		/**
		 * @return
		 */
		public EObject getEObject() {
			return _eObject;
		}
	}

	/**
     * Whether this named element should appear as a node in tree
     * for the preference dialog. Uses isTreeNode key in 
     * CwAnnotation to find this.
     * @param element Element
     * @return boolean
     */
    public static boolean isTreeNode(ENamedElement element)
    {
        String nodeStr = EcoreUtils.getAnnotationVal(element, null,
				AnnotationConstants.IS_TREE_NODE);
		return nodeStr != null ? Boolean.parseBoolean(nodeStr) : false;
    }

    /**
     * Whether this named element should appear as a node in tree
     * for the preference dialog. Uses isTreeListNode key in 
     * CwAnnotation to find this.
     * @param element Element
     * @return boolean
     */
    public static boolean isTreeListNode(ENamedElement element)
    {
        String nodeStr = EcoreUtils.getAnnotationVal(element, null,
				AnnotationConstants.IS_TREE_LISTNODE);
		return nodeStr != null ? Boolean.parseBoolean(nodeStr) : false;
    }

    /**
	 * Whether this named element in tree for the preference dialog should have
	 * the popup behaviour or not. Uses isTreeDumbNode key in CwAnnotation to
	 * find this.
	 * 
	 * @param element
	 *            Element
	 * @return boolean
	 */
    public static boolean isTreeDumbNode(EObject eObj)
    {
        String dumbFeature = EcoreUtils.getAnnotationVal(eObj.eClass(), null,
				AnnotationConstants.IS_TREE_DUMBNODE);

        if(dumbFeature != null) {
        	if(dumbFeature.equals("true")) {
        		return true;
        	}

        	List disable = (List) EcoreUtils.getValue(eObj, "Disable");
    		if(disable.contains(dumbFeature)) {
    			return true;
    		}
        }
		return false;
    }
}

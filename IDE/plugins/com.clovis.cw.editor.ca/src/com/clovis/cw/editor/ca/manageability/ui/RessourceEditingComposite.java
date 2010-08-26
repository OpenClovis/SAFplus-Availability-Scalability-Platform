package com.clovis.cw.editor.ca.manageability.ui;

import java.io.InputStreamReader;
import java.util.List;
import java.util.StringTokenizer;

import org.eclipse.draw2d.ColorConstants;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.notify.impl.AdapterImpl;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Table;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.MenuBuilder;
import com.clovis.common.utils.ui.table.TableUI;
import com.clovis.cw.editor.ca.dialog.NodeProfileDialog;
import com.clovis.cw.editor.ca.manageability.common.StatusLineManagerUtils;

/**
 * 
 * @author Pushparaj
 *
 */
public class RessourceEditingComposite extends Composite {

	private Label _titleLabel;
	private TableUI _tableViewer;
	private EClass _eClass;
	private String _selectedResName;
	private EAdapter _listener;
    private ManageabilityEditor _editor;
    RessourceEditingComposite(Composite parent, ManageabilityEditor editor, EClass eClass) {
		super(parent, SWT.NONE);
		_eClass = eClass;
		_listener = new EAdapter();
        _editor = editor;
        setControls();
	}
	
	private void setControls() {
		setBackground(ColorConstants.white);
		GridLayout gridLayout = new GridLayout();
		gridLayout.numColumns = 2;
		setLayout(gridLayout);
		GridData d1 = new GridData(GridData.FILL_HORIZONTAL);
		setLayoutData(d1);
		/*Label empty = new Label(this, SWT.NONE);
		empty.setBackground(ColorConstants.white);
		d1 = new GridData(GridData.FILL_HORIZONTAL);
		d1.horizontalSpan = 2;
		empty.setLayoutData(d1);*/
		_titleLabel = new Label(this, SWT.NONE);
		d1 = new GridData(GridData.FILL_HORIZONTAL);
		d1.horizontalSpan = 2;
		_titleLabel.setText("Resource instances for the selected resource type");
		_titleLabel.setLayoutData(d1);
		_titleLabel.setBackground(ColorConstants.white);
		int style = SWT.MULTI | SWT.BORDER | SWT.H_SCROLL | SWT.V_SCROLL
				| SWT.FULL_SELECTION | SWT.HIDE_SELECTION;
		Table table = new Table(this, style);
		GridData gridData1 = new GridData();
		gridData1.horizontalAlignment = GridData.FILL;
		gridData1.grabExcessHorizontalSpace = true;
		gridData1.grabExcessVerticalSpace = true;
		gridData1.verticalAlignment = GridData.FILL;
		gridData1.heightHint = getDisplay().getClientArea().height / 15;
		ClassLoader loader = getClass().getClassLoader();
		_tableViewer = new ResourceTableViewer(table, _eClass, loader, false, null);
		table.setLayoutData(gridData1);
		//table.setBackground(ColorConstants.white);
		ViewerFilter filter = new ResourceFilter();
		ViewerFilter filters[] = new ViewerFilter[1];
		filters[0] = filter;
		_tableViewer.setFilters(filters);
		table.setLinesVisible(true);
		table.setHeaderVisible(true);
		try {
            InputStreamReader reader = new InputStreamReader(
                getClass().getResourceAsStream("tabletoolbar.xml"));
            Composite toolbar = new MenuBuilder(reader, _tableViewer).getToolbar(this, 0);
            toolbar.setBackground(ColorConstants.white);
        } catch (Exception e) { e.printStackTrace(); }
        setEnabled(false);
    }
	/**
	 * Change the input
	 */
	public void setTableInput(List<EObject> input){
		if(input == null) {
			_tableViewer.refresh();
			setEnabled(false);
		} else {
			if(_tableViewer.getInput() != null) {
				EcoreUtils.removeListener((NotifyingList) _tableViewer.getInput(), _listener, -1);
			}
			_tableViewer.setInput(input);
			EcoreUtils.addListener((NotifyingList) input, _listener, -1);
			_tableViewer.refresh();
			setEnabled(true);
		}
	}
		
	@Override
	public void dispose () {
		super.dispose();
		if(_tableViewer.getInput() != null) {
			EcoreUtils.removeListener((NotifyingList) _tableViewer.getInput(), _listener, -1);
		}
	}
	/**
	 * Set selected resource
	 * @param resName
	 */
	public void setSelectedResource(String resName) {
		_selectedResName = resName;
		if(resName == null) {
			_titleLabel.setText("Resource instances for the selected resource type");
			setEnabled(false);
		} else {
			_titleLabel.setText("Instances of " + resName + " resource");
			setEnabled(true);
		}
	}
	
	class ResourceFilter extends ViewerFilter {

		@Override
		public boolean select(Viewer viewer, Object parentElement,
				Object element) {
			if(_selectedResName == null){
				return false;
			}
			if (element instanceof EObject) {
				EObject resObj = (EObject) element;
				String moID = (String) EcoreUtils.getValue(resObj, "moID");
				
				if(moID.trim().equals("") && !_selectedResName.equals("")) {
					moID = "\\" + _editor.getChassisName() + ":0" + "\\" +_selectedResName + ":" + 0;
					EcoreUtils.setValue(resObj, "moID", moID);
					if(AssociateResourceUtils.getInitializedArrayAttrResList(_editor.getResourceList()).contains(_selectedResName)) { 	 
                        EcoreUtils.setValue(resObj, "autoCreate", "false"); 	 
					}
				}
				return (getResourceTypeForInstanceID(moID).equals(_selectedResName));
			}
			return false;
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
	}
	
	public class ResourceTableViewer extends TableUI {

		public ResourceTableViewer(Table table, EClass class1, ClassLoader loader,
				boolean isReadOnly, Environment parentEnv) {
			super(table, class1, loader, isReadOnly, parentEnv);
		}
		
		public ManageabilityEditor getManageabilityEditor() {
			return _editor;
		}
	}
	public class EAdapter extends AdapterImpl
    {
		public void notifyChanged(Notification notification)
        {
        	if (notification.isTouch() || NodeProfileDialog.getInstance() != null) {
        		return;
        	}
            switch (notification.getEventType()) {
            case Notification.SET:
            	Object object = notification.getNotifier();
	            if (object instanceof EObject) {
	            	if(((EStructuralFeature)notification.getFeature()).getName().equals("moID")) {
	            		String oldValue = notification.getOldStringValue();
	            		String newValue = notification.getNewStringValue();
	            		if (!newValue.equals(oldValue)) {
	            			if (newValue.contains("*")
									|| AssociateResourceUtils
											.getInitializedArrayAttrResList(
													_editor.getResourceList())
											.contains(
													AssociateResourceUtils
															.getResourceTypeFromInstanceID(newValue))) {
								EcoreUtils.setValue((EObject) object,
										"autoCreate", "false");
							}
						}
	            	} else if(((EStructuralFeature)notification.getFeature()).getName().equals("autoCreate")) {
	            		EObject modObject = (EObject) object;
	            		final String moID = (String) EcoreUtils.getValue(modObject, "moID");
	            		 if (moID.contains("*")
								|| AssociateResourceUtils
										.getInitializedArrayAttrResList(
												_editor.getResourceList())
										.contains(
												AssociateResourceUtils
														.getResourceTypeFromInstanceID(moID))) {
							EcoreUtils.setValue(modObject, "autoCreate",
									"false");
							if (moID.contains("*")) {
								StatusLineManagerUtils
										.setErrorMessage("'Auto Create' can not be true if wildcard exists in the instance ID");
							} else {
								StatusLineManagerUtils
										.setErrorMessage("'Auto Create' can not be true for the resource which have intialized array attribute");
							}
						}
	            	}
	            }
            	_editor.propertyChange(null);	
                break;
            case Notification.ADD:
            	Object newVal = notification.getNewValue();
 	            if (newVal instanceof EObject) {
 	            	EcoreUtils.addListener(newVal, this, 1);
 	            }
 	           _editor.propertyChange(null);	
               break;
            case Notification.REMOVE:
            	Object oldVal = notification.getOldValue();
 	            if (oldVal instanceof EObject) {
 	            	EcoreUtils.removeListener(oldVal, this, 1);
 	            }
 	           _editor.propertyChange(null);	
               break;
            case Notification.ADD_MANY:
            case Notification.REMOVE_MANY:
            	_editor.propertyChange(null);	
                break;
            }
        }
	}
}

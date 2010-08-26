/*******************************************************************************
 * ModuleName  : com
 * $File:  $
 * $Author: matt $
 * $Date: 2007/10/15 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.dialog;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.StringTokenizer;
import java.util.regex.Pattern;

import org.eclipse.core.resources.IContainer;
import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IFolder;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IPath;
import org.eclipse.core.runtime.Path;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ComboBoxCellEditor;
import org.eclipse.jface.viewers.ICellModifier;
import org.eclipse.jface.viewers.IStructuredContentProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.TextCellEditor;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.events.HelpEvent;
import org.eclipse.swt.events.HelpListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.TabFolder;
import org.eclipse.swt.widgets.TabItem;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.TableItem;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.PlatformUI;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.builders.ClovisConfigurator;
import com.clovis.cw.workspace.builders.MakeImages;
import com.clovis.cw.workspace.project.wizard.NodeSlotInstanceInfo;

/**
 * 
 * @author matt
 * Class to capture target.conf settings
 */
public class MakeImagesDialog extends TitleAreaDialog
{
	public static final String NODE_INSTANCE_COL = "Node Instance";
	public static final String SLOT_NUM_COL = "Slot Number";
	public static final String NET_INTERFACE_COL = "Network Interface";
	
	private Text _trapIPText;
    private Text _cmmIPText;
    private CCombo _cmmAuthText;
    private Text _cmmUsernameText;
    private Text _cmmPasswordText;
    private Text _tipcNetIDText;
    private Button _createTarballs;
    private Button _instantiateImages;
    private org.eclipse.swt.widgets.List _binList;
    private org.eclipse.swt.widgets.List _libList;
    
    private IResource _project;
    private HashMap _settings;
	private Table _table;
	private TableViewer _tableViewer;
	private String[] _columnNames = new String[] {NODE_INSTANCE_COL, SLOT_NUM_COL, NET_INTERFACE_COL};
	private String[] _slotNumbers;
	private String[] _authTypes = new String[] {"", "none", "md2", "md5", "straight"};
	private NodeSlotInstanceInfo[] _nodeInstanceList;
	private boolean _isCMMEnabled = false;
	private boolean _having3rdpartyComponents = false;
    
    /**
     * Constructor for class that displays a dialog to collect target.conf settings.
     * @param shell
     * @param resource
     */
	public MakeImagesDialog(Shell shell, IResource resource, HashMap settings)
	{
		super(shell);
		setShellStyle(SWT.RESIZE); //allow the dialog to be resized
		_project = resource;
		_having3rdpartyComponents = isHaving3rdpartyComponents();
		_settings = settings;
		int maxSlots = 0;
		
        // pull slot and link maps out of settings
		HashMap slots = (HashMap)_settings.get("SLOTS");
        HashMap links = (HashMap)_settings.get("LINKS");
		
        // get information needed from the model
        ProjectDataModel pdm = ProjectDataModel.getProjectDataModel((IContainer) _project);
        List _nodeProfileObjects = pdm.getNodeProfiles().getEList();
        List nodeObjs = ProjectDataModel.getNodeInstListFrmNodeProfile(_nodeProfileObjects);

        List resList = pdm.getCAModel().getEList();
		EObject rootObject = (EObject) resList.get(0);
		List list = (EList) rootObject.eGet(rootObject.eClass().getEStructuralFeature(ClassEditorConstants.CHASSIS_RESOURCE_REF_NAME));
		EObject resObj = (EObject) list.get(0);
		maxSlots = ((Integer) EcoreUtils.getValue(resObj,ClassEditorConstants.CHASSIS_NUM_SLOTS)).intValue();

		// get the node instances, match them with their slot numbers in the
		//  configuration settings and then add them to the node instance array
		//  ...do the same for the link interfaces
		_nodeInstanceList = new NodeSlotInstanceInfo[nodeObjs.size()];

		for (int i=0; i<nodeObjs.size(); i++)
        {
        	EObject eobj = (EObject)nodeObjs.get(i);
            String nodeType = EcoreUtils.getName(eobj);
            
            // find the associated slot number
            Integer slotNum = null;
            if (slots != null && slots.get("SLOT_" + nodeType) != null)
            {
            	slotNum = Integer.valueOf((String)slots.get("SLOT_" + nodeType));
            	if (slotNum > maxSlots) slotNum = null;
            }
            
            // find the associated link interface
            String linkInterface = null;
            if (links != null && links.get("LINK_" + nodeType) != null)
            {
            	linkInterface = (String)links.get("LINK_" + nodeType);
            }
            
    		_nodeInstanceList[i] = new NodeSlotInstanceInfo(nodeType, slotNum, linkInterface);
        }
		
		// add the valid slot numbers to the list of slot numbers
		_slotNumbers = new String[maxSlots+1];
		_slotNumbers[0] = "";
		for (int i=1; i<=maxSlots; i++)
		{
			_slotNumbers[i] = String.valueOf(i);
		}
		
    	/* See if the project has been configured for chassis management */
    	_isCMMEnabled = new Boolean(ClovisConfigurator.getCMBuildMode(_project)).booleanValue();
	}
	/**
	 *@see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	protected Control createDialogArea(Composite composite)
    {
		/* Create container for dialog controls */		
		Composite container = new Composite(composite, org.eclipse.swt.SWT.NONE);
        GridLayout glayout = new GridLayout();
        glayout.numColumns = 2;
        container.setLayout(glayout);
        container.setLayoutData(new GridData(GridData.FILL_BOTH));
        
        TabFolder tabFolder = new TabFolder(container, SWT.BORDER);
        glayout = new GridLayout();
        glayout.numColumns = 2;
        tabFolder.setLayout(glayout);
        tabFolder.setLayoutData(new GridData(GridData.FILL_BOTH));
        
        TabItem generalTab = new TabItem(tabFolder, SWT.NULL);
        generalTab.setText("General");
        TabItem cmTab = new TabItem(tabFolder, SWT.NULL);
        cmTab.setText("Chassis Management");
        TabItem thirdPartyTab = new TabItem(tabFolder, SWT.NULL);
        thirdPartyTab.setText("3rdParty");
        
        /* Create group to hold tarp ip, tipc net id controls*/
    	Composite generalGroup = new Composite(tabFolder, SWT.BORDER);
    	GridData data = new GridData(GridData.FILL_HORIZONTAL);
    	data.horizontalSpan = 2;
    	generalGroup.setLayoutData(data);
    	GridLayout layout = new GridLayout();
    	layout.numColumns = 2;
    	generalGroup.setLayout(layout);
    	generalTab.setControl(generalGroup);
    	
        /* Create group to hold chassis management module controls */
        Composite chassisGroup = new Composite(tabFolder, SWT.BORDER);
    	data = new GridData(GridData.FILL_HORIZONTAL);
    	data.horizontalSpan = 2;
    	chassisGroup.setLayoutData(data);
    	layout = new GridLayout();
    	layout.numColumns = 2;
    	chassisGroup.setLayout(layout);
    	cmTab.setControl(chassisGroup);
    	
    	/* Create group to hold 3rd party controls */
    	Composite thirdPartyGroup = new Composite(tabFolder, SWT.BORDER);
    	data = new GridData(GridData.FILL_HORIZONTAL);
    	data.horizontalSpan = 2;
    	thirdPartyGroup.setLayoutData(data);
    	layout = new GridLayout();
    	layout.numColumns = 2;
    	thirdPartyGroup.setLayout(layout);
    	thirdPartyTab.setControl(thirdPartyGroup);
    	
    	/******************************************************/
    	/* Create trap IP controls                            */
    	/******************************************************/
    	Label trapIPLabel = new Label(generalGroup, SWT.NONE);
    	trapIPLabel.setText("Trap IP: ");
    	_trapIPText = new Text(generalGroup, SWT.BORDER);
    	_trapIPText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
    	_trapIPText.setText(_settings.get(MakeImages.TRAP_IP_KEY) != null ? (String)_settings.get(MakeImages.TRAP_IP_KEY) : "" );
    	_trapIPText.setEnabled(true);

    	/******************************************************/
    	/* Create tipc net id controls                        */
    	/******************************************************/
    	Label tipcNetIDLabel = new Label(generalGroup, SWT.NONE);
    	tipcNetIDLabel.setText("TIPC Net ID: ");
    	_tipcNetIDText = new Text(generalGroup, SWT.BORDER);
    	_tipcNetIDText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
    	_tipcNetIDText.setText(_settings.get(MakeImages.TIPC_NETID_KEY) != null ? (String)_settings.get(MakeImages.TIPC_NETID_KEY) : "" );
    	_tipcNetIDText.setEnabled(true);
    	
    	/******************************************************/
    	/* Instntiate images controls                       */
    	/******************************************************/
    	_instantiateImages = new Button(generalGroup, SWT.CHECK);
    	_instantiateImages.setText("Create Node Specific Images");
    	_instantiateImages.setAlignment(SWT.LEFT);
    	GridData data2 = new GridData(GridData.FILL_HORIZONTAL);
    	data2.horizontalSpan = 2;
    	_instantiateImages.setLayoutData(data2);

    	boolean instantiateImageSetting = false;
    	if (_settings.get(MakeImages.INSTANTIATE_IMAGES_KEY) != null)
    	{
    		if (((String)_settings.get(MakeImages.INSTANTIATE_IMAGES_KEY)).toLowerCase().equals("yes"))
    		{
    			instantiateImageSetting = true;
    		}
    	}
    	_instantiateImages.setSelection(instantiateImageSetting);
    	
    	/******************************************************/
    	/* Create make tarball controls                       */
    	/******************************************************/
    	_createTarballs = new Button(generalGroup, SWT.CHECK);
    	_createTarballs.setText("Package Images into Tarballs");
    	_createTarballs.setAlignment(SWT.LEFT);
    	data2 = new GridData(GridData.FILL_HORIZONTAL);
    	data2.horizontalSpan = 2;
    	_createTarballs.setLayoutData(data2);

    	boolean tarballSetting = true;
    	if (_settings.get(MakeImages.CREATE_TARBALLS_KEY) != null)
    	{
    		if (((String)_settings.get(MakeImages.CREATE_TARBALLS_KEY)).toLowerCase().equals("no"))
    		{
    	    	tarballSetting = false;
    		}
    	}
		_createTarballs.setSelection(tarballSetting);
		
		/******************************************************/
    	/* Create Chassis Management instruction label        */
    	/******************************************************/
    	Label cmm_instruct = new Label(chassisGroup, SWT.WRAP | SWT.NONE);
    	GridData two_col = new GridData(GridData.FILL_HORIZONTAL);
    	two_col.horizontalSpan = 2;
    	cmm_instruct.setLayoutData(two_col);
    	cmm_instruct.setText("The Chassis Management Settings are only enabled if you are using an ATCA chassis. The Authentication Type, Username, and Password fields must be set as a group.");
    			
    	/******************************************************/
    	/* Create CMM IP controls                             */
    	/******************************************************/
    	Label cmmIPLabel = new Label(chassisGroup, SWT.NONE);
    	cmmIPLabel.setText("Module IP: ");
    	_cmmIPText = new Text(chassisGroup, SWT.BORDER);
    	_cmmIPText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
    	if (_isCMMEnabled)
    	{
	    	_cmmIPText.setText(_settings.get(MakeImages.CMM_IP_KEY) != null ? (String)_settings.get(MakeImages.CMM_IP_KEY) : "" );
	    	_cmmIPText.setEnabled(true);
    	} else {
	    	_cmmIPText.setEnabled(false);
    	}

    	/******************************************************/
    	/* Create CMM authorization type controls             */
    	/******************************************************/
    	Label cmmAuthLabel = new Label(chassisGroup, SWT.NONE);
    	cmmAuthLabel.setText("Authentication Type: ");
    	_cmmAuthText = new CCombo(chassisGroup, SWT.DROP_DOWN | SWT.BORDER);
    	_cmmAuthText.setItems(_authTypes);
    	_cmmAuthText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
    	if (_isCMMEnabled)
    	{
	    	_cmmAuthText.setText(_settings.get(MakeImages.CMM_AUTH_TYPE_KEY) != null ? (String)_settings.get(MakeImages.CMM_AUTH_TYPE_KEY) : "" );
	    	_cmmAuthText.setEnabled(true);
    	} else {
    		_cmmAuthText.setEnabled(false);
    	}

    	/******************************************************/
    	/* Create CMM username controls                       */
    	/******************************************************/
    	Label cmmUsernameLabel = new Label(chassisGroup, SWT.NONE);
    	cmmUsernameLabel.setText("Username: ");
    	_cmmUsernameText = new Text(chassisGroup, SWT.BORDER);
    	_cmmUsernameText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
    	if (_isCMMEnabled)
    	{
	    	_cmmUsernameText.setText(_settings.get(MakeImages.CMM_USERNAME_KEY) != null ? (String)_settings.get(MakeImages.CMM_USERNAME_KEY) : "" );
	    	_cmmUsernameText.setEnabled(true);
    	} else {
    		_cmmUsernameText.setEnabled(false);
    	}

    	/******************************************************/
    	/* Create CMM password controls                       */
    	/******************************************************/
    	Label cmmPasswordLabel = new Label(chassisGroup, SWT.NONE);
    	cmmPasswordLabel.setText("Password: ");
    	_cmmPasswordText = new Text(chassisGroup, SWT.BORDER);
    	_cmmPasswordText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
    	if (_isCMMEnabled)
    	{
	    	_cmmPasswordText.setText(_settings.get(MakeImages.CMM_PASSWORD_KEY) != null ? (String)_settings.get(MakeImages.CMM_PASSWORD_KEY) : "" );
	    	_cmmPasswordText.setEnabled(true);
    	} else {
    		_cmmPasswordText.setEnabled(false);
    	}
    	
    	createTable(generalGroup);
    	createTableViewer();
    	
    	/* Create label for 3rd party */
    	Label thirdPartyLabel = new Label(thirdPartyGroup, SWT.WRAP | SWT.NONE);
    	two_col = new GridData(GridData.FILL_HORIZONTAL);
    	two_col.horizontalSpan = 2;
    	thirdPartyLabel.setLayoutData(two_col);
    	String label = "Add your 3rd party component's libraries and binaries which will be copied to model bin/lib directories.";
    	if (!_having3rdpartyComponents)
    	{
    		label = label + " These controls will only be enabled if a component in the model has been configured as a third party component.";
    	}
    	thirdPartyLabel.setText(label);
    	
    	/* Create list controls for 3rd party */
    	Composite binContainer = new Composite(thirdPartyGroup, SWT.NONE);
    	GridData one_col = new GridData(GridData.FILL_BOTH);
    	binContainer.setLayoutData(one_col);
    	layout = new GridLayout();
    	layout.numColumns = 2;
    	binContainer.setLayout(layout);
    	
    	Button addBinBtn = new Button(binContainer, SWT.BORDER);
    	addBinBtn.setText("Add Bins...");
    	Button deleteBinBtn = new Button(binContainer, SWT.BORDER);
    	deleteBinBtn.setText("Delete Bins");
    	_binList = new org.eclipse.swt.widgets.List(
				binContainer, SWT.V_SCROLL | SWT.H_SCROLL | SWT.BORDER | SWT.MULTI);
    	two_col = new GridData(SWT.FILL, SWT.FILL, true, true);
 		two_col.horizontalSpan = 2;
    	_binList.setLayoutData(two_col);
    	addBinBtn.addSelectionListener(new SelectionListener(){
			public void widgetSelected(SelectionEvent e) {
				FileDialog dialog = new FileDialog(getShell(), SWT.MULTI);
				dialog.setFilterPath(System.getProperty("user.home"));
				if(dialog.open() != null) {
					String dir = dialog.getFilterPath();
					String files[] = dialog.getFileNames();
					for (int i = 0; i < files.length; i++) {
						_binList.add(dir + File.separator + files[i]);
					}
				}
			}
			public void widgetDefaultSelected(SelectionEvent e) {}
			}
    	);
    	deleteBinBtn.addSelectionListener(new SelectionListener() {
			public void widgetDefaultSelected(SelectionEvent e) {}
			public void widgetSelected(SelectionEvent e) {
				_binList.remove(_binList.getSelectionIndices());
			}
		}
    	);
    	Composite libContainer = new Composite(thirdPartyGroup, SWT.NONE);
    	one_col = new GridData(GridData.FILL_BOTH);
    	libContainer.setLayoutData(one_col);
    	layout = new GridLayout();
    	layout.numColumns = 2;
    	libContainer.setLayout(layout);
    	
    	Button addLibBtn = new Button(libContainer, SWT.BORDER);
    	addLibBtn.setText("Add Libs...");
    	Button deleteLibBtn = new Button(libContainer, SWT.BORDER);
    	deleteLibBtn.setText("Delete Libs");
    	_libList = new org.eclipse.swt.widgets.List(
				libContainer, SWT.V_SCROLL | SWT.H_SCROLL | SWT.BORDER | SWT.MULTI);
    	two_col = new GridData(SWT.FILL, SWT.FILL, true, true);
 		two_col.horizontalSpan = 2;
    	_libList.setLayoutData(two_col);
    	addLibBtn.addSelectionListener(new SelectionListener(){
			public void widgetSelected(SelectionEvent e) {
				FileDialog dialog = new FileDialog(getShell(), SWT.MULTI);
				dialog.setFilterPath(System.getProperty("user.home"));
				if(dialog.open() != null) {
					String dir = dialog.getFilterPath();
					String files[] = dialog.getFileNames();
					for (int i = 0; i < files.length; i++) {
						_libList.add(dir + File.separator + files[i]);
					}
				}
			}
			public void widgetDefaultSelected(SelectionEvent e) {}
			}
    	);
    	deleteLibBtn.addSelectionListener(new SelectionListener() {
			public void widgetDefaultSelected(SelectionEvent e) {}
			public void widgetSelected(SelectionEvent e) {
				_libList.remove(_libList.getSelectionIndices());
			}
		}
    	);
    	
    	setTitle("Make Images Configuration");
    	setMessage("Configure make images settings", IMessageProvider.INFORMATION);
    	composite.addHelpListener(new HelpListener() {
			public void helpRequested(HelpEvent e) {
				PlatformUI.getWorkbench().getHelpSystem()
						.displayHelp("com.clovis.cw.help.configure");
			}
		});
    	
    	if(!_having3rdpartyComponents) {
    		thirdPartyGroup.setEnabled(false);
    		addBinBtn.setEnabled(false);
    		deleteBinBtn.setEnabled(false);
    		addLibBtn.setEnabled(false);
    		deleteLibBtn.setEnabled(false);
    	} else {
    		populate3rdPartyLists();
    		addBinBtn.setEnabled(true);
    		deleteBinBtn.setEnabled(true);
    		addLibBtn.setEnabled(true);
    		deleteLibBtn.setEnabled(true);
    	}
        return container;
    }

	/**
	 * Create the Table
	 */
	private void createTable(Composite parent) {
		int style = SWT.SINGLE | SWT.BORDER | SWT.H_SCROLL | SWT.V_SCROLL | 
					SWT.FULL_SELECTION | SWT.HIDE_SELECTION;

		_table = new Table(parent, style);

		GridData gridData = new GridData(GridData.FILL_BOTH);
		gridData.grabExcessVerticalSpace = true;
		gridData.horizontalSpan = 2;
		_table.setLayoutData(gridData);		

		_table.setLinesVisible(true);
		_table.setHeaderVisible(true);

		// 1st column
		TableColumn column = new TableColumn(_table, SWT.CENTER, 0);		
		column.setText(NODE_INSTANCE_COL);
		column.setWidth(140);
		
		// 2nd column
		column = new TableColumn(_table, SWT.LEFT, 1);
		column.setText(SLOT_NUM_COL);
		column.setWidth(100);
		
		// 3rd column
		column = new TableColumn(_table, SWT.LEFT, 2);
		column.setText(NET_INTERFACE_COL);
		column.setWidth(170);
	}

	/**
	 * Create the TableViewer 
	 */
	private void createTableViewer() {

		_tableViewer = new TableViewer(_table);
		_tableViewer.setUseHashlookup(true);
		
		_tableViewer.setColumnProperties(_columnNames);

		// Create the cell editors
		CellEditor[] editors = new CellEditor[_columnNames.length];

		// Column 2 :
		editors[1] = new ComboBoxCellEditor(_table, _slotNumbers, SWT.READ_ONLY);
		editors[1].setValue(new Integer(1));

		// Column 3 :
		editors[2] = new TextCellEditor(_table);
		
		// Assign the cell editors to the viewer 
		_tableViewer.setCellEditors(editors);
		_tableViewer.setContentProvider(new SlotContentProvider());
		_tableViewer.setLabelProvider(new SlotLabelProvider());
		_tableViewer.setInput(_nodeInstanceList);

		// Set the cell modifier for the viewer
		_tableViewer.setCellModifier(new SlotCellModifier(_columnNames, _slotNumbers));
	}

	/**
	 * 
	 * @author matt
	 * Label provider for Table
	 */
	class SlotLabelProvider extends LabelProvider implements ITableLabelProvider {
		
		/**
		 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
		 */
		public Image getColumnImage(Object element, int columnIndex) {
			// TODO Auto-generated method stub
			return null;
		}
		/**
		 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
		 */
		public String getColumnText(Object element, int columnIndex) {
			String result = "";
			NodeSlotInstanceInfo slotInst = (NodeSlotInstanceInfo) element;
			switch (columnIndex) {
				case 0:
					result = slotInst.getInstanceName();
					break;
				case 1 :
					result = slotInst.getSlotNum() != null ? String.valueOf(slotInst.getSlotNum()) : "";
					break;
				case 2 :
					result = slotInst.getNetInterface() != null ? slotInst.getNetInterface() : "";
					break;
				default :
					break; 	
			}
			return result;
		}
	}

	/**
	 * 
	 * @author matt
	 * Content Provider for Table
	 */
	class SlotContentProvider implements IStructuredContentProvider {

	    public SlotContentProvider() { }
		
		/**
		 * @see org.eclipse.jface.viewers.IContentProvider#inputChanged(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
		 */
		public void inputChanged(Viewer v, Object oldInput, Object newInput) {
		}
		/**
		 *  @see org.eclipse.jface.viewers.IContentProvider#dispose()
		 */
		public void dispose() {
		}
		/**
		 * @see org.eclipse.jface.viewers.IStructuredContentProvider#getElements(java.lang.Object)
		 */
		public Object[] getElements(Object parent) {
			return _nodeInstanceList;

		}
	}

	/**
	 * 
	 * @author matt
	 * CellModifier for Table
	 */
	public class SlotCellModifier implements ICellModifier {

		private List _columnNames;
		private List _slotNums;
		/**
		 * Constructor 
		 * @param TableViewerExample an instance of a TableViewerExample 
		 */
		public SlotCellModifier(String[] names, String[] slots) {
			super();
			_columnNames = Arrays.asList(names);
			_slotNums = Arrays.asList(slots);
		}

		/**
		 * @see org.eclipse.jface.viewers.ICellModifier#canModify(java.lang.Object, java.lang.String)
		 */
		public boolean canModify(Object element, String property) {
			return true;
		}

		/**
		 * @see org.eclipse.jface.viewers.ICellModifier#getValue(java.lang.Object, java.lang.String)
		 */
		public Object getValue(Object element, String property) {

			// Find the index of the column
			int columnIndex = _columnNames.indexOf(property);

			Object result = null;
			NodeSlotInstanceInfo node = (NodeSlotInstanceInfo) element;
			switch (columnIndex) {
			case 1:
				if (node == null || node.getSlotNum() == null)
				{
					result = new Integer(0);
				} else {
					result = new Integer(_slotNums.indexOf(node.getSlotNum().toString()));
				}
				break;
			case 2:
				if (node == null || node.getNetInterface() == null)
				{
					result = "";
				} else {
					result = String.valueOf(node.getNetInterface());
				}
				break;
			default:
				result = "";
			}
			return result;
		}

		/**
		 * @see org.eclipse.jface.viewers.ICellModifier#modify(java.lang.Object, java.lang.String, java.lang.Object)
		 */
		public void modify(Object element, String property, Object value)
		{
			if(value == null || value.equals("")) return;

			// Find the index of the column 
			int columnIndex = _columnNames.indexOf(property);

			TableItem item = (TableItem) element;
			NodeSlotInstanceInfo node = (NodeSlotInstanceInfo) item.getData();
			String valueString;
			
			switch (columnIndex)
			{
				case 1: 
					valueString = _slotNumbers[((Integer)value).intValue()];
					if (valueString != null && valueString.length() > 0)
					{
						node.setSlotNum(Integer.valueOf(valueString));
					} else {
						node.setSlotNum(null);
					}
					break;
				case 2:
					valueString = ((String) value).trim();
					if (valueString != null && valueString.length() > 0)
					{
						node.setNetInterface(valueString);
					} else {
						node.setNetInterface(null);
					}
					break;
				default:
					break;
			}
			_tableViewer.refresh();
		}
	}

	/**
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	protected void configureShell(Shell shell)
	{
		shell.setText("Make Images");
		super.configureShell(shell);
		shell.setBounds(50,50,475,500);
	}
	/**
	 * Process the values that have been entered by validating them, putting them
	 * in a HashMap, and passing that HashMap to the MakeImages class for writing
	 * to the target.conf file.
	 *@see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	protected void okPressed()
	{
		if (!validateDialog()) return;
		
		HashMap<String, Object> settings = new HashMap<String, Object>();
		
		String fieldVal;
		
		fieldVal = _trapIPText.getText().trim();
		if (fieldVal != null && fieldVal.length() > 0) settings.put(MakeImages.TRAP_IP_KEY, fieldVal);

	    fieldVal = _cmmIPText.getText().trim();
		if (fieldVal != null && fieldVal.length() > 0) settings.put(MakeImages.CMM_IP_KEY, fieldVal);
		
		fieldVal = _cmmAuthText.getText().trim();
		if (fieldVal != null && fieldVal.length() > 0) settings.put(MakeImages.CMM_AUTH_TYPE_KEY, fieldVal);
		
		fieldVal = _cmmUsernameText.getText().trim();
		if (fieldVal != null && fieldVal.length() > 0) settings.put(MakeImages.CMM_USERNAME_KEY, fieldVal);
		
		fieldVal = _cmmPasswordText.getText().trim();
		if (fieldVal != null && fieldVal.length() > 0) settings.put(MakeImages.CMM_PASSWORD_KEY, fieldVal);
		
		fieldVal = _tipcNetIDText.getText().trim();
		if (fieldVal != null && fieldVal.length() > 0) settings.put(MakeImages.TIPC_NETID_KEY, fieldVal);
		
		if (_createTarballs.getSelection())
		{
			settings.put(MakeImages.CREATE_TARBALLS_KEY, "YES");
		} else {
			settings.put(MakeImages.CREATE_TARBALLS_KEY, "NO");
		}
		
		if (_instantiateImages.getSelection())
		{
			settings.put(MakeImages.INSTANTIATE_IMAGES_KEY, "YES");
		} else {
			settings.put(MakeImages.INSTANTIATE_IMAGES_KEY, "NO");
		}
		
		HashMap<String, String> slots = new HashMap<String, String>();
		HashMap<String, String> links = new HashMap<String, String>();
		String instanceName;
		Integer slotNum;
		String netInterface;
		for (int i=0; i<_nodeInstanceList.length; i++)
		{
			instanceName = _nodeInstanceList[i].getInstanceName();
			slotNum = _nodeInstanceList[i].getSlotNum();
			netInterface = _nodeInstanceList[i].getNetInterface();
			
			if (slotNum != null)
			{
				slots.put(instanceName, slotNum.toString());
			}
			
			if (netInterface != null && netInterface.length() > 0)
			{
				links.put(instanceName, netInterface);
			}
		}
		settings.put(MakeImages.SLOT_TABLE_KEY, slots);
		settings.put(MakeImages.LINK_TABLE_KEY, links);
		try {
			if (_having3rdpartyComponents) {
				copyBaseImagesScript();
				write3rdPartyFilesList();
			} else {
				remove3rdPartyRelatedFiles();
			}
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		} catch (CoreException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
		
		MakeImages.writeTargetConf((IProject)_project, settings);
		super.okPressed();
	}
	/**
	 * Read copy_list file and populate all file names in UI List.
	 * @throws FileNotFoundException 
	 */
	private void populate3rdPartyLists(){
		IPath buildPath = new Path("src").append("build");
		IFolder buildFolder = ((IProject)_project).getFolder(buildPath);
		if(!buildFolder.exists()) {
			return;
		}
		IFolder baseImagesFolder = buildFolder.getFolder("base-images");
		if(!baseImagesFolder.exists()) {
			return;
		}
		IFolder scriptsFolder = baseImagesFolder.getFolder("scripts");
		if(!scriptsFolder.exists()) {
			return;
		}
		IFile copyFile = scriptsFolder.getFile("copy_list");
		if(!copyFile.exists()) {
			return;
		}
		try {
			BufferedReader in = new BufferedReader(new FileReader(copyFile.getLocation().toFile()));
			String readLine = null;
			while((readLine = in.readLine())!= null) {
				StringTokenizer tokenizer = new StringTokenizer(readLine,":");
				String type = tokenizer.nextToken();
				String path = tokenizer.nextToken();
				if(type.equals("bin")) {
					_binList.add(path);
				} else if(type.equals("lib")) {
					_libList.add(path);
				}
			}
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	/** Copy base-images.sh file to src/build/base-images/scripts *
	 * @throws CoreException 
	 * @throws FileNotFoundException */
	private void copyBaseImagesScript() throws CoreException, FileNotFoundException {
		IPath buildPath = new Path("src").append("build");
		IFolder buildFolder = ((IProject)_project).getFolder(buildPath);
		if(!buildFolder.exists()) {
			buildFolder.create(true, true, null);
		}
		IFolder baseImagesFolder = buildFolder.getFolder("base-images");
		if(!baseImagesFolder.exists()) {
			baseImagesFolder.create(true, true, null);
		}
		IFolder scriptsFolder = baseImagesFolder.getFolder("scripts");
		if(!scriptsFolder.exists()) {
			scriptsFolder.create(true, true, null);
		}
		IFile destFile = scriptsFolder.getFile("base-images.sh");
		File srcFile = ((IProject)_project).getFile(new Path("scripts").append("base-images.sh")).getLocation().toFile();
		if(!destFile.exists()) {
			destFile.create(new FileInputStream(srcFile), true, null);
		}
	}
	/**
	 * Writing copy_list file with 3rd party file names which will be used to copy
	 * 3rd party files into model directory
	 * @throws CoreException 
	 * @throws IOException 
	 */
	private void write3rdPartyFilesList() throws CoreException, IOException {
		IPath copyFilePath = new Path("src").append("build").append("base-images").append("scripts").append("copy_list");
		IFile copyFile = ((IProject)_project).getFile(copyFilePath);
		if(copyFile.exists()) {
			copyFile.delete(true, null);
		}
		copyFile.create(null, true, null);
		FileOutputStream out = new FileOutputStream(copyFile.getLocation()
				.toFile());
		String binaries[] = _binList.getItems();
		for (int i = 0; i < binaries.length; i++) {
			out.write(new String("bin:" + binaries[i] + "\n").getBytes());
		}
		String libraries[] = _libList.getItems();
		for (int i = 0; i < libraries.length; i++) {
			out.write(new String("lib:" + libraries[i] + "\n").getBytes());
		}
		out.flush();
		out.close();
	}
	/**
	 * Remove files which are not required if there is no third party components.
	 * @throws CoreException 
	 */
	private void remove3rdPartyRelatedFiles() throws CoreException {
		IPath copyFilePath = new Path("src").append("build").append("base-images").append("scripts").append("copy_list");
		IFile copyFile = ((IProject)_project).getFile(copyFilePath);
		if(copyFile.exists()) {
			copyFile.delete(true, null);
		}
		IPath scriptFilePath = new Path("src").append("build").append("base-images").append("scripts").append("base-images.sh");
		IFile scriptFile = ((IProject)_project).getFile(scriptFilePath);
		if(scriptFile.exists()) {
			scriptFile.delete(true, null);
		}
	}
    /**
     * Validate the fields on the dialog
     */
	private boolean validateDialog()
    {
    	String message = "";
    	
		ArrayList<Integer> usedSlots = new ArrayList<Integer>();
    	
		// check for slots assigned to more than one node instance
		for (int i=0; i<_nodeInstanceList.length; i++)
		{
			if (_nodeInstanceList[i].getSlotNum() != null)
			{
				if (usedSlots.contains(_nodeInstanceList[i].getSlotNum()))
				{
					message = "The same slot number is used for more than one node instance. Please correct and try again.\n\n";
				} else {
					usedSlots.add(_nodeInstanceList[i].getSlotNum());
				}
			}
		}
		
		// check trap ip address
		String trapIP = _trapIPText.getText().trim();
		if (trapIP != null && trapIP.length() > 0)
		{	
			 if (!Pattern.compile("[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}").matcher(trapIP).matches())
			{
				message = message + "The Trap IP entered does not match the standard IP format. Please reenter and try again.\n\n";
	        }
			
		} else {
			message = message + "The Trap IP is a required field. Please enter a value and try again.\n\n";
		} 
		
		// check tipc net id
		String tipcNetID = _tipcNetIDText.getText().trim();
		if (tipcNetID != null && tipcNetID.length() > 0)
		{
			int testInt;
			try
			{
				testInt = Integer.parseInt(tipcNetID);
			} catch (NumberFormatException nfe) {
				testInt = -1;
			}
			
			if (testInt < 1 || testInt > 9999)
			{
				message = message + "The TIPC Net ID must be a valid number between 1 and 9999. Please reenter and try again.\n\n";
			}
		} else {
			message = message + "The TIPC Net ID is a required field. Please enter a value and try again.\n\n";
		}
    	
		//check the CMM settings
		if (_isCMMEnabled)
		{
			String cmmIP = _cmmIPText.getText().trim();
			if (cmmIP.length() == 0)
			{
				message = message + "The Module IP is required because the project is configured to use chassis management. Please enter a value and try again.\n\n";
			}
			
			if (cmmIP.length() > 0) {
				if (!Pattern.compile("[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}").matcher(cmmIP).matches())
				{
					message = message + "The Module IP entered for Chassis Management does not match the standard IP format. Please reenter and try again.\n\n";
		        }
			}

			String cmmAuth = _cmmAuthText.getText().trim();
			String cmmUser = _cmmUsernameText.getText().trim();
			String cmmPass = _cmmPasswordText.getText().trim();
			boolean allFilled = (cmmAuth.length() > 0 && cmmUser.length() > 0 && cmmPass.length() > 0);
			boolean noneFilled = (cmmAuth.length() == 0 && cmmUser.length() == 0 && cmmPass.length() == 0);
	
			if (!allFilled && !noneFilled)
			{
				message = message + "The Chassis Manager settings for Authentication, Username, and Password must either all be empty or all have values. Please correct and try again.\n\n";
			}
		}
		
		// if any of the validations have set a message then diaplay it
		if (message.length() > 0)
		{
			MessageDialog.openError(getShell(),
					"Make Images Error for " + _project.getName(),
					message);
			return false;
		}
		
		return true;
    }
	/**
	 * Check whether the project is having 3rd party component
	 * @return boolean
	 */
	private boolean isHaving3rdpartyComponents() {
		ProjectDataModel pdm = ProjectDataModel
				.getProjectDataModel((IContainer) _project);
		EList rootlist = pdm.getComponentModel().getEList();
		EObject rootObject = (EObject) rootlist.get(0);
		EReference ref = (EReference) rootObject.eClass()
				.getEStructuralFeature(
						ComponentEditorConstants.SAFCOMPONENT_REF_NAME);
		EList list = (EList) rootObject.eGet(ref);
		for (int j = 0; j < list.size(); j++) {
			EObject eobj = (EObject) list.get(j);
			boolean is3rdparty = Boolean.parseBoolean(EcoreUtils.getValue(eobj,
					"is3rdpartyComponent").toString());
			if(is3rdparty)
				return true;
		}
		return false;
	}
}

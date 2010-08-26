/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/project/wizard/AddNodeWizardPage.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.workspace.project.wizard;

import java.io.File;
import java.net.URL;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.ecore.EEnum;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ComboBoxCellEditor;
import org.eclipse.jface.viewers.IStructuredContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.TextCellEditor;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.wizard.IWizard;
import org.eclipse.jface.wizard.IWizardPage;
import org.eclipse.jface.wizard.WizardPage;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;

import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.workspace.WorkspacePlugin;

/**
 * 
 * @author pushparaj
 * Wizard for capturing Nodes
 */
public class AddNodeWizardPage extends WizardPage{
    
	private Table _table;
	private NodeInfoList _taskList;
	private TableViewer _tableViewer;
	private final String  NODE_NAME 		  = "Node Name";
	private final String NODE_CLASS 		  = "Node Class";
	private static final Log LOG = Log.getLog(WorkspacePlugin.getDefault());
	private String[] columnNames = new String[] { 
			NODE_NAME, 
			NODE_CLASS
			};
	public static String UI_CLASS_TYPES[];
	public static List CLASS_TYPES;

	/**
	 * Constructor
	 * @param pageName Name
	 * @param tasks List of blades
	 */
	public AddNodeWizardPage(String pageName, NodeInfoList tasks) {
		super(pageName);
		_taskList = tasks;
		addClassTypes();
	}
	/**
	 * add class types
	 *
	 */
	private void addClassTypes()
	{
		EPackage ePackage = null;
		URL caURL = DataPlugin.getDefault().find(
                new Path("model" + File.separator
                         + ICWProject.COMPONENT_ECORE_FILENAME));
        try {
            File ecoreFile = new Path(Platform.resolve(caURL).getPath())
                    .toFile();
            ePackage = EcoreModels.get(ecoreFile.getAbsolutePath());
        } catch (Exception e) {
            LOG.warn("Component Editor ecore file cannot be read", e);
        }
        EEnum uiNodeClassEnum = (EEnum) ePackage.getEClassifier("UIAMSNodeClassType");
		List uiliterals = uiNodeClassEnum.getELiterals();
		
		// only make Class B and Class C available in the wizard
		List availUILiterals = new ArrayList();
		for (Iterator iter=uiliterals.iterator(); iter.hasNext();)
		{
			EEnumLiteral literal = (EEnumLiteral) iter.next();
			if (literal.getValue() == 1 || literal.getValue() == 2)
			{
				availUILiterals.add(literal);
			}
		}
		UI_CLASS_TYPES = new String [availUILiterals.size()];
		for (int i = 0; i < availUILiterals.size(); i++) {
			EEnumLiteral uiLiteral = (EEnumLiteral) availUILiterals.get(i);
        	UI_CLASS_TYPES[i] = uiLiteral.getName();
        }

		EEnum nodeClassEnum = (EEnum) ePackage.getEClassifier("AmsNodeClass");
		List literals = nodeClassEnum.getELiterals();
		
		// only make Class B and Class C available in the wizard
		List availLiterals = new ArrayList();
		for (Iterator iter=literals.iterator(); iter.hasNext();)
		{
			EEnumLiteral literal = (EEnumLiteral) iter.next();
			if (literal.getValue() == 1 || literal.getValue() == 2)
			{
				availLiterals.add(literal);
			}
		}
		CLASS_TYPES = new ArrayList();
		for (int i = 0; i < availLiterals.size(); i++) {
        	EEnumLiteral literal = (EEnumLiteral) availLiterals.get(i);
        	CLASS_TYPES.add(literal.getName());
        }
	}
	/**
	 * @see org.eclipse.jface.dialogs.IDialogPage#createControl(org.eclipse.swt.widgets.Composite)
	 */
	public void createControl(Composite parent) {
		Composite composite = new Composite(parent, SWT.NULL);
        composite.setFont(parent.getFont());

        initializeDialogUnits(parent);

        // Create a composite to hold the children
		GridData gridData = new GridData (GridData.HORIZONTAL_ALIGN_FILL | GridData.FILL_BOTH);
		composite.setLayoutData (gridData);

		// Set numColumns to 3 for the buttons 
		GridLayout layout = new GridLayout(3, false);
		layout.numColumns = 4;
		composite.setLayout (layout);
        
        //Create the table 
		createTable(composite);
		
		// Create and setup the TableViewer
		createTableViewer();
		_tableViewer.setContentProvider(new BladeContentProvider(getWizard()));
		_tableViewer.setLabelProvider(new BladeLabelProvider());
		_tableViewer.setInput(_taskList);
		createButtons(composite);
        setPageComplete(true);
        // Show description on opening
        setErrorMessage(null);
        setMessage(null);
        setControl(composite);		

		Composite bottomControl = new Composite(composite, SWT.NONE);
		GridData data = new GridData();
		bottomControl.setLayoutData(data);
		RowLayout rowlayout = new RowLayout();
		rowlayout.marginTop = 15;
		rowlayout.marginBottom = 0;
		rowlayout.pack = true;
		bottomControl.setLayout(rowlayout);
		Group descGroup = new Group(bottomControl, SWT.BORDER);
		descGroup.setText("Note:");
		RowLayout descLayout = new RowLayout(SWT.VERTICAL);
		descLayout.marginHeight = 4;
		descLayout.marginWidth = 14;
		descGroup.setLayout(descLayout);
		Label descLabel = new Label(descGroup, SWT.WRAP);
		descLabel.setText("A SAF Node is conceptually all the software running on a blade."
						+ " Create multiple node 'types' if you have different software to run on each"
						+ " node, for example, 'controller' and 'worker' nodes.  Use the Add button to"
						+ " create SAF Node Types. You can always add more nodes once this"
						+ " wizard is complete.");
		RowData rowData = new RowData();
		rowData.width = 525;
		descLabel.setLayoutData(rowData);
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
		gridData.horizontalSpan = 3;
		_table.setLayoutData(gridData);		
					
		_table.setLinesVisible(true);
		_table.setHeaderVisible(true);

		// 1st column
		TableColumn column = new TableColumn(_table, SWT.CENTER, 0);		
		column.setText(NODE_NAME);
		column.setWidth(100);
		
		// 2nd column
		column = new TableColumn(_table, SWT.LEFT, 1);
		column.setText(NODE_CLASS);
		column.setWidth(170);
	}
	/**
	 * Create the TableViewer 
	 */
	private void createTableViewer() {

		_tableViewer = new TableViewer(_table);
		_tableViewer.setUseHashlookup(true);
		
		_tableViewer.setColumnProperties(columnNames);

		// Create the cell editors
		CellEditor[] editors = new CellEditor[columnNames.length];

		// Column 1 :
		editors[0] = new TextCellEditor(_table);

		// Column 2 :
		editors[1] = new ComboBoxCellEditor(_table, UI_CLASS_TYPES, SWT.READ_ONLY);
		editors[1].setValue(new Integer(0));
		
		// Assign the cell editors to the viewer 
		_tableViewer.setCellEditors(editors);
		// Set the cell modifier for the viewer
		_tableViewer.setCellModifier(new NodeCellModifier(columnNames, CLASS_TYPES, _taskList));
	}
	/**
	 * Add the "Add", "Delete" and "Close" buttons
	 * @param parent the parent composite
	 */
	private void createButtons(Composite parent) {
		Composite composite = new Composite(parent, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.numColumns = 1;
		composite.setLayout(layout);
		GridData gridData = new GridData(GridData.GRAB_HORIZONTAL | GridData.GRAB_VERTICAL);
		gridData.horizontalSpan = 1;
		// Create and configure the "Add" button
		Button add = new Button(composite, SWT.PUSH | SWT.CENTER);
		add.setText("Add");
		gridData = new GridData (GridData.HORIZONTAL_ALIGN_BEGINNING);
		gridData.widthHint = 80;
		add.setLayoutData(gridData);
		add.addSelectionListener(new SelectionAdapter() {
       	
       		// Add a task to the TaskList and refresh the view
			public void widgetSelected(SelectionEvent e) {
				_taskList.addTask();
			}
		});

		//	Create and configure the "Delete" button
		Button delete = new Button(composite, SWT.PUSH | SWT.CENTER);
		delete.setText("Delete");
		gridData = new GridData (GridData.HORIZONTAL_ALIGN_BEGINNING);
		gridData.widthHint = 80; 
		delete.setLayoutData(gridData); 

		delete.addSelectionListener(new SelectionAdapter() {
       	
			//	Remove the selection and refresh the view
			public void widgetSelected(SelectionEvent e) {
				NodeInfo task = (NodeInfo) ((IStructuredSelection) 
						_tableViewer.getSelection()).getFirstElement();
				if (task != null) {
					_taskList.removeTask(task);
				} 
			}
		});
	}

	/**
	 * Return the node types
	 */
	protected NodeInfoList getNodeTypeList()
	{
		return _taskList;
	}

	/**
	 * Override this method so that if the controller count is invalid the
	 * user cannot get to the next page.
	 */
	public boolean canFlipToNextPage()
	{
		if (! isControllerCountValid()) return false;
		
		return super.canFlipToNextPage();
	}

	/**
	 * Override this method so that if the controller count is invalid the
	 * user cannot get back to the previous page.
	 */
	public IWizardPage getPreviousPage()
	{
		if (! isControllerCountValid()) return null;
		return super.getPreviousPage();
	}
	
	/**
	 * Checks if the controller count is valid. If not it displays an error
	 * message and marks the page as not complete so that the finish button
	 * cannot be clicked.
	 * 
	 */
	public void checkControllerCount()
	{
		if (! isControllerCountValid())
		{
			setErrorMessage("You can have maximum one System Controller node type in the model.");
			setPageComplete(false);
		} else {
			setErrorMessage(null);
			setPageComplete(true);
		}
	}
	
	/**
	 * There can be a maximum of two system controllers in the model. This method checks that
	 * the number of system controllers is valid.
	 * 
	 * @return True if the controller count is valid, false otherwise
	 */
	public boolean isControllerCountValid()
	{
		Vector list = _taskList.getTasks();
		Iterator iter = list.iterator();
		int controllerCount = 0;
		while (iter.hasNext())
		{
			NodeInfo info = (NodeInfo) iter.next();
			if (info.getNodeClass().equals("CL_AMS_NODE_CLASS_B")) controllerCount++;
		}
		
		if (controllerCount > 1) return false;

		return true;
	}

	/**
	 * 
	 * @author pushparaj
	 * Label provider for Table
	 */
	class BladeLabelProvider 
	extends LabelProvider
	implements ITableLabelProvider {
		
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
			NodeInfo task = (NodeInfo) element;
			switch (columnIndex) {
				case 0:
					result = task.getNodeName();
					break;
				case 1 :
					result = String.valueOf(UI_CLASS_TYPES[CLASS_TYPES.indexOf(task.getNodeClass())]);
					break;
				default :
					break; 	
			}
			return result;
		}
	}
	/**
	 * 
	 * @author pushparaj
	 * Content Provider for Table
	 */
	class BladeContentProvider implements IStructuredContentProvider, ITaskListViewer {

		private IWizard _wizard;
		
	    public BladeContentProvider(IWizard wizard)
	    {
	    	_wizard = wizard;
	    }
		
		/**
		 * @see org.eclipse.jface.viewers.IContentProvider#inputChanged(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
		 */
		public void inputChanged(Viewer v, Object oldInput, Object newInput) {
			if (newInput != null)
				((NodeInfoList) newInput).addChangeListener(this);
			if (oldInput != null)
				((NodeInfoList) oldInput).removeChangeListener(this);
		}
		/**
		 *  @see org.eclipse.jface.viewers.IContentProvider#dispose()
		 */
		public void dispose() {
			_taskList.removeChangeListener(this);
		}
		/**
		 * @see org.eclipse.jface.viewers.IStructuredContentProvider#getElements(java.lang.Object)
		 */
		public Object[] getElements(Object parent) {
			return _taskList.getTasks().toArray();
		}
		/**
		 * @see ITaskListViewer#addTask(Object)
		 */
		public void addTask(Object task) {
			_tableViewer.add(task);
			checkControllerCount();
			_wizard.getContainer().updateButtons();
		}

		/**
		 * @see ITaskListViewer#removeTask(Object)
		 */
		public void removeTask(Object task) {
			_tableViewer.remove(task);			
			checkControllerCount();
			_wizard.getContainer().updateButtons();
		}

		/**
		 * @see ITaskListViewer#updateTask(Object)
		 */
		public void updateTask(Object task, String oldValue) {
			_tableViewer.update(task, null);	
			checkControllerCount();
			_wizard.getContainer().updateButtons();
		}
	}
}

/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/dialog/AssociateResourceWizardPage.java $
 * $Author: matt $
 * $Date: 2007/08/20 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.project.wizard;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;

import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ComboBoxCellEditor;
import org.eclipse.jface.viewers.IStructuredContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.TextCellEditor;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerSorter;
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

/**
 * 
 * @author matt
 *
 * Wizard page to capture the resources associated with node types
 * entered in the new project wizard
 */
public class ProgramNameWizardPage extends WizardPage  
{
	private Table _table;
	private TableViewer _tableViewer;
	private ProgramNameInfoList _programNames;
	private String[] _columnNames = new String[] { COL_NODE_TYPE, COL_PROGRAM_NAME };
	public static final String COL_NODE_TYPE = "Node Type";
	public static final String COL_PROGRAM_NAME = "Program Name";
	private List _nodeTypeList = new ArrayList();

	Button _addButton;
	Button _deleteButton;
	
	/**
     * Constructor
     * @param pageName - Page Name
     */
	public ProgramNameWizardPage(String pageName, ProgramNameInfoList programNames)
	{
		super(pageName);
		_programNames = programNames;
	}

	/**
     * Creates the page controls
     * @param parent - Parent Composite 
	 */
	public void createControl(Composite parent) {
		Composite composite = new Composite(parent, SWT.NULL);
        composite.setFont(parent.getFont());

        initializeDialogUnits(parent);

        // Create a composite to hold the children
		GridData gridData = new GridData (GridData.HORIZONTAL_ALIGN_FILL | GridData.FILL_BOTH);
		composite.setLayoutData (gridData);

		GridLayout layout = new GridLayout(3, false);
		layout.numColumns = 4;
		composite.setLayout (layout);
        
        //Create the table 
		createTable(composite);
		
        // Get a reference to the previous page
		AddNodeWizardPage prevPage = (AddNodeWizardPage) getPreviousPage();

        // Create and setup the TableViewer
		createTableViewer();
		NodeTypeContentProvider ntcp = new NodeTypeContentProvider();
        prevPage.getNodeTypeList().addChangeListener(ntcp);
		_tableViewer.setContentProvider(ntcp);
		_tableViewer.setLabelProvider(new NodeTypeLabelProvider());
		_tableViewer.setInput(_programNames);
		createButtons(composite);
		
		// program names need to sort grouped by the node type
//		_tableViewer.setSorter(new ProgramNameViewerSorter());
		
        setPageComplete(true);
        setControl(composite);		

		Composite bottomControl = new Composite(composite, SWT.NONE);
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
		descLabel.setText("Highly available programs (SAF Service Types) are composed of a variety of"
						+ " SAF entities (Service Group, Service Unit, etc). This wizard will autogenerate"
						+ " the basic SAF component hierarchy for each program you specify. Use the Add"
						+ " button to create and name each program (we will use this as a prefix for all"
						+ " SAF components associated with that program, and for the program executable"
						+ " name). You can always add more programs once this wizard is complete.");
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
		column.setText(COL_NODE_TYPE);
		column.setWidth(100);

		// 2nd column
		column = new TableColumn(_table, SWT.CENTER, 1);		
		column.setText(COL_PROGRAM_NAME);
		column.setWidth(100);
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

		// Column 1 :
		editors[0] = new ComboBoxCellEditor(_table, listToStringArray(_nodeTypeList), SWT.READ_ONLY);

		// Column 2 :
		editors[1] = new TextCellEditor(_table);

		// Assign the cell editors to the viewer 
		_tableViewer.setCellEditors(editors);
		// Set the cell modifier for the viewer
		_tableViewer.setCellModifier(new ProgramNameCellModifier(_columnNames, _programNames, _nodeTypeList));
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
		_addButton = new Button(composite, SWT.PUSH | SWT.CENTER);
		_addButton.setText("Add");
		gridData = new GridData (GridData.HORIZONTAL_ALIGN_BEGINNING);
		gridData.widthHint = 80;
		_addButton.setLayoutData(gridData);
		_addButton.addSelectionListener(new SelectionAdapter() {
       	
       		// Add a task to the TaskList and refresh the view
			public void widgetSelected(SelectionEvent e)
			{
				AddNodeWizardPage prevPage = (AddNodeWizardPage) getPreviousPage();
				NodeInfo nodeInfo = (NodeInfo)prevPage.getNodeTypeList().getTasks().get(0);
				_programNames.addTask(new ProgramNameInfo(nodeInfo.getNodeName(), getNewProgramName()));
			}
		});
		_addButton.setEnabled(false);

		//	Create and configure the "Delete" button
		_deleteButton = new Button(composite, SWT.PUSH | SWT.CENTER);
		_deleteButton.setText("Delete");
		gridData = new GridData (GridData.HORIZONTAL_ALIGN_BEGINNING);
		gridData.widthHint = 80; 
		_deleteButton.setLayoutData(gridData); 

		_deleteButton.addSelectionListener(new SelectionAdapter() {
       	
			//	Remove the selection and refresh the view
			public void widgetSelected(SelectionEvent e) {
				ProgramNameInfo progName = (ProgramNameInfo) ((IStructuredSelection) 
						_tableViewer.getSelection()).getFirstElement();
				if (progName != null) {
					_programNames.removeTask(progName);
				} 
			}
		});
		_deleteButton.setEnabled(false);
	}

	/**
	 * Returns unique Program Name
	 * @return v
	 */
	private String getNewProgramName()
	{
		HashSet set = new HashSet();
		for(int i = 0; i < _programNames.getTasks().size(); i++)
		{
			ProgramNameInfo programName = (ProgramNameInfo) _programNames.getTasks().get(i);
			set.add(programName.getProgramName());
		}
		int i = 0;
		String newName = null;
		do {
			newName = "SAFComponent" + i;
			i++;
		} while (set.contains(newName));
		return newName;
	}
	
	private String[] listToStringArray(List list)
	{
		String[] array = new String[list.size()];
		
		for (int i=0; i<list.size(); i++)
		{
			array[i] = (String) list.get(i);
		}
		
		return array;
	}

	/************************************************************/
	/* Inner classes begin here                                 */
	/************************************************************/

	/**
	 * 
	 * @author matt
	 * Label provider for Table
	 */
	class NodeTypeLabelProvider extends LabelProvider implements ITableLabelProvider {

		/**
		 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
		 */
		public Image getColumnImage(Object element, int columnIndex) {
			return null;
		}
		
		/**
		 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
		 */
		public String getColumnText(Object element, int columnIndex) {
			String result = "";
			ProgramNameInfo progName = (ProgramNameInfo) element;

			switch (columnIndex) {
				case 0: //node type
					result = progName.getNodeTypeName();
					break;
				case 1 : //program name
					result = progName.getProgramName();
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
	class NodeTypeContentProvider implements IStructuredContentProvider, ITaskListViewer {

		/**
		 * @see org.eclipse.jface.viewers.IContentProvider#inputChanged(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
		 */
		public void inputChanged(Viewer v, Object oldInput, Object newInput) {
			if (newInput != null)
				((ProgramNameInfoList) newInput).addChangeListener(this);
			if (oldInput != null)
				((ProgramNameInfoList) oldInput).removeChangeListener(this);
		}
		
		/**
		 *  @see org.eclipse.jface.viewers.IContentProvider#dispose()
		 */
		public void dispose() {
			_programNames.removeChangeListener(this);
		}
		
		/**
		 * @see org.eclipse.jface.viewers.IStructuredContentProvider#getElements(java.lang.Object)
		 */
		public Object[] getElements(Object parent) {
			return _programNames.getTasks().toArray();
		}
		
		/**
		 * @see ITaskListViewer#addTask(Object)
		 */
		public void addTask(Object task)
		{
			//We are listening to two different lists here. The Program Name list from this page
			// and the Node list from the previous page. In this case we don't care about additions
			// of ProgramNameInfo items since you can't add items on this page...but we still
			// need to avoid them.
			if (task instanceof NodeInfo)
			{
				ComboBoxCellEditor cellEditor = (ComboBoxCellEditor) _tableViewer.getCellEditors()[0];
				_nodeTypeList.add(((NodeInfo)task).getNodeName());
				cellEditor.setItems(listToStringArray(_nodeTypeList));
				_addButton.setEnabled(true);
			}
			else
			{
				_tableViewer.add( (ProgramNameInfo) task );
				_deleteButton.setEnabled(true);
			}
		}

		/**
		 * @see ITaskListViewer#removeTask(Object)
		 */
		public void removeTask(Object task) 
		{
			//We are listening to two different lists here. The Program Name list from this page
			// and the Node list from the previous page. So we must handle the two cases.
			if (task instanceof ProgramNameInfo)
			{
				_tableViewer.remove( (ProgramNameInfo) task);
				if (_programNames.getTasks().size() == 0) _deleteButton.setEnabled(false);
			} else {
				NodeInfo node = ((NodeInfo)task);
				ArrayList progNames = new ArrayList();
				Iterator iter = _programNames.getTasks().iterator();
				while (iter.hasNext())
				{
					ProgramNameInfo progName = (ProgramNameInfo)iter.next();
					if (progName.getNodeTypeName().equals(node.getNodeName()))
					{
						progNames.add(progName);
					}
				}
				_programNames.removeTasks(progNames);

				ComboBoxCellEditor cellEditor = (ComboBoxCellEditor) _tableViewer.getCellEditors()[0];
				_nodeTypeList.remove(node.getNodeName());
				cellEditor.setItems(listToStringArray(_nodeTypeList));
				if (_nodeTypeList.size() == 0) _addButton.setEnabled(false);
			}
		}

		/**
		 * @see ITaskListViewer#updateTask(Object)
		 */
		public void updateTask(Object task, String oldValue)
		{
			//We are listening to two different lists here. The Program Name list from this page
			// and the Node list from the previous page. So we must handle the two cases.
			if (task instanceof ProgramNameInfo)
			{
				_tableViewer.update(task, null);
			} else {

				NodeInfo nodeInfo = (NodeInfo)task;
				
				// update the combo box contents
				ComboBoxCellEditor cellEditor = (ComboBoxCellEditor) _tableViewer.getCellEditors()[0];
				for (int i=0; i<_nodeTypeList.size(); i++)
				{
					if (_nodeTypeList.get(i).equals(oldValue)) _nodeTypeList.set(i, nodeInfo.getNodeName());
				}
				
				cellEditor.setItems(listToStringArray(_nodeTypeList));


				// update any program name items that were using the old node type name
				Iterator iter = _programNames.getTasks().iterator();
				while (iter.hasNext())
				{
					ProgramNameInfo progName = (ProgramNameInfo)iter.next();
					if (progName.getNodeTypeName().equals(oldValue))
					{
						progName.setNodeTypeName(nodeInfo.getNodeName());
	
						_tableViewer.update(progName, null);
					}
				}
			}
		}
	}
}

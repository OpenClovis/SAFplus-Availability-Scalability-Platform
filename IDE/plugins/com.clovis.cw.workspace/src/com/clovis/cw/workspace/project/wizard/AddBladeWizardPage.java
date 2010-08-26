/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/project/wizard/AddBladeWizardPage.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.workspace.project.wizard;


import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.StringTokenizer;

import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ComboBoxCellEditor;
import org.eclipse.jface.viewers.IStructuredContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.TextCellEditor;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.wizard.WizardPage;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.VerifyEvent;
import org.eclipse.swt.events.VerifyListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.Text;

import com.clovis.cw.data.ICWProject;
import com.clovis.cw.workspace.WorkspacePlugin;
/**
 * @author pushparaj
 * Wizard for capturing Blades
 */
public class AddBladeWizardPage extends WizardPage
{
	private Table _table;
	private BladeInfoList _taskList;
	private TableViewer _tableViewer;
	private final String  BLADE_TYPE 		= "Blade Type";
	private final String  BLADE_NAME 		= "Blade Name";
	private final String NUMBER_BLADES 		= "Number of blades";
	
	private final String CUSTOM_TYPE		= "Default";
	private String BLADE_TYPES [] = {CUSTOM_TYPE};
	private String[] columnNames = new String[] { 
			BLADE_TYPE,
			BLADE_NAME,
			NUMBER_BLADES
			};
	/**
	 * Constructor
	 * @param pageName Name
	 * @param tasks List of Blades
	 */
	public AddBladeWizardPage(String pageName, BladeInfoList tasks) {
		super(pageName);
		_taskList = tasks;
		addBladeTypes();
	}
	/**
	 * Add blade types
	 *
	 */
	private void addBladeTypes() {
		URL url = WorkspacePlugin.getDefault().find(new Path(ICWProject.RESOURCE_TEMPLATE_FOLDER));
		if(url == null)
			return;
		try {
			File templateFolder = new Path(Platform.resolve(url).getPath())
					.toFile();
			String[] templates = templateFolder.list();
			BLADE_TYPES = new String[templates.length + 1];
			BLADE_TYPES[0] = CUSTOM_TYPE;
			for (int i = 0; i < templates.length; i++) {
                          String filePath = templateFolder + File.separator + templates[i];
                          if (new File(filePath).isDirectory()) {
                            String templateDirMarker = filePath + File.separator + ICWProject.CW_PROJECT_TEMPLATE_GROUP_MARKER;
                            if (new File(templateDirMarker).isFile())
				BLADE_TYPES[i+1] = new StringTokenizer(templates[i], ".").nextToken();
                          }
			}
		} catch (IOException e) {
			e.printStackTrace();
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

		GridLayout layout = new GridLayout(4, false);
		layout.numColumns = 5;
		composite.setLayout (layout);
        
        //Create the table 
		createTable(composite);
		
		// Create and setup the TableViewer
		createTableViewer();
		_tableViewer.setContentProvider(new BladeContentProvider());
		_tableViewer.setLabelProvider(new BladeLabelProvider());
		_tableViewer.setInput(_taskList);
		createButtons(composite);
        setPageComplete(true);
        // Show description on opening
        setErrorMessage(null);
        setMessage(null);
        setControl(composite);		
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
		column.setText(BLADE_TYPE);
		column.setWidth(100);
		
		// 2nd column
		column = new TableColumn(_table, SWT.LEFT, 1);
		column.setText(BLADE_NAME);
		column.setWidth(100);
		
		// 3rd column
		column = new TableColumn(_table, SWT.LEFT, 2);
		column.setText(NUMBER_BLADES);
		column.setWidth(100);
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
		editors[0] = new ComboBoxCellEditor(_table, BLADE_TYPES, SWT.READ_ONLY);

		// Column 2 :
		editors[1] = new TextCellEditor(_table);
		
		// Column 3 :
		TextCellEditor textEditor = new TextCellEditor(_table);
		((Text) textEditor.getControl()).addVerifyListener(
				
				new VerifyListener() {
					public void verifyText(VerifyEvent e) {
						// Here, we could use a RegExp such as the following 
						// if using JRE1.4 such as  e.doit = e.text.matches("[\\-0-9]*");
						e.doit = "0123456789".indexOf(e.text) >= 0 ;
					}
				});
		editors[2] = textEditor;

		// Assign the cell editors to the viewer 
		_tableViewer.setCellEditors(editors);
		// Set the cell modifier for the viewer
		_tableViewer.setCellModifier(new BladeCellModifier(columnNames, BLADE_TYPES, _taskList));
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
				BladeInfo task = (BladeInfo) ((IStructuredSelection) 
						_tableViewer.getSelection()).getFirstElement();
				if (task != null) {
					_taskList.removeTask(task);
				} 
			}
		});
	}
	/**
	 * Label provider for Table
	 * @author pushparaj
	 *
	 */
	class BladeLabelProvider extends LabelProvider
	implements ITableLabelProvider {
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
			BladeInfo task = (BladeInfo) element;
			switch (columnIndex) {
				case 0:
					result = task.getBladeType();
					break;
				case 1 :
					result = task.getBladeName();
					break;	
				case 2 :
					result = String.valueOf(task.getNumberOfBlades());
					break;
				default :
					break; 	
			}
			return result;
		}
	}
	/**
	 * Content Provider for Table
	 * @author pushparaj
	 *
	 */
	class BladeContentProvider implements IStructuredContentProvider, ITaskListViewer {
		public void inputChanged(Viewer v, Object oldInput, Object newInput) {
			if (newInput != null)
				((BladeInfoList) newInput).addChangeListener(this);
			if (oldInput != null)
				((BladeInfoList) oldInput).removeChangeListener(this);
		}
		/**
		 * @see org.eclipse.jface.viewers.IContentProvider#dispose()
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
		}

		/**
		 * @see ITaskListViewer#removeTask(Object)
		 */
		public void removeTask(Object task) {
			_tableViewer.remove(task);			
		}

		/* (non-Javadoc)
		 * @see ITaskListViewer#updateTask(Object)
		 */
		public void updateTask(Object task, String oldValue) {
			_tableViewer.update(task, null);	
		}
	}
	

}


